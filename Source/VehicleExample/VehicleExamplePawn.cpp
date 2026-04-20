// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleExamplePawn.h"
#include "VehicleExampleWheelFront.h"
#include "VehicleExampleWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "VehicleExample.h"
#include "TimerManager.h"
#include "RacingVehicleSystem/RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleInventory.h"
#include "RacingVehicleSystem/OwnedVehicle.h"
#include "SChallengePromptWidget.h"
#include "Engine/GameViewportClient.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

AVehicleExamplePawn::AVehicleExamplePawn()
{
	// Replicate this pawn to all clients so every player sees every vehicle.
	bReplicates = true;
	SetReplicatingMovement(true);

	// Send position updates at 60 Hz so simulated proxies see smooth movement.
	NetUpdateFrequency    = 60.f;
	MinNetUpdateFrequency = 30.f;

	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void AVehicleExamplePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AVehicleExamplePawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AVehicleExamplePawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AVehicleExamplePawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AVehicleExamplePawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AVehicleExamplePawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AVehicleExamplePawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &AVehicleExamplePawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogVehicleExample, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AVehicleExamplePawn::BeginPlay()
{
	Super::BeginPlay();

	// Simulated proxies that are NOT locally controlled (i.e. NPC pawns and other
	// players' cars seen from a remote client) must not run local Chaos physics.
	// Their position is driven by the replicated transform.  Without this, the
	// AI's constant steering input causes the NPC pawn's local simulation to
	// diverge from the server state and snap visibly.
	// The joining player's own pawn is ROLE_AutonomousProxy, not SimulatedProxy,
	// so this block does not affect it.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		GetMesh()->SetSimulatePhysics(false);
		return;
	}

	// set up the flipped check timer
	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &AVehicleExamplePawn::FlippedCheck, FlipCheckTime, true);

	// Defer one tick so the PlayerController has possessed this pawn and the
	// movement component is fully initialised before we apply data-asset stats.
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		// Only apply from the GameInstance for locally-controlled (player) pawns.
		// NPC pawns are handled explicitly in NPCPatrolActor::SpawnNPCPawn().
		if (!Cast<APlayerController>(GetController())) { return; }

		URacingGameInstance* GI = URacingGameInstance::Get(this);
		if (!GI) { return; }

		UOwnedVehicle* CurrentVehicle = GI->GetVehicleInventory()
			? GI->GetVehicleInventory()->GetCurrentVehicle()
			: nullptr;
		if (!CurrentVehicle) { return; }

		ApplyVehicleStats(CurrentVehicle->ComputeEffectiveStats());
	});
}

void AVehicleExamplePawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	// SetInputMode called in ACourseGameMode::PostLogin runs on the server's proxy
	// of the joining controller and never reaches the actual client.  Call it here
	// on the owning client so vehicle input works immediately after possession.
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->IsLocalController())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void AVehicleExamplePawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// clear the flipped check timer
	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	Super::EndPlay(EndPlayReason);
}

void AVehicleExamplePawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// SimulatedProxy pawns have no local physics or input — skip all gameplay logic.
	if (GetLocalRole() == ROLE_SimulatedProxy) { return; }

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);
	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	// PvP proximity challenge: scan for nearby player pawns and auto-request a
	// challenge when one enters the radius.  Only runs on the locally-controlled
	// pawn so each client handles their own side independently.
	if (IsLocallyControlled() && !bHasOutgoingChallenge && !PendingChallenger.IsValid())
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* OtherPC = It->Get();
			if (!OtherPC) { continue; }

			AVehicleExamplePawn* OtherPawn = Cast<AVehicleExamplePawn>(OtherPC->GetPawn());
			if (!OtherPawn || OtherPawn == this) { continue; }
			// Only challenge pawns that are also locally controlled from this client's
			// perspective (i.e. this is the server iterating its controllers).
			// On the listen-server host both controllers exist locally, so we use
			// HasAuthority() as the guard — only the server initiates the RPC.
			if (!HasAuthority()) { continue; }

			const float DistSq = FVector::DistSquared(GetActorLocation(), OtherPawn->GetActorLocation());
			if (DistSq <= PlayerChallengeRadius * PlayerChallengeRadius)
			{
				RequestChallengePlayer(OtherPawn);
				break;
			}
		}
	}
}

void AVehicleExamplePawn::Steering(const FInputActionValue& Value)
{
	// route the input
	DoSteering(Value.Get<float>());
}

void AVehicleExamplePawn::Throttle(const FInputActionValue& Value)
{
	// route the input
	DoThrottle(Value.Get<float>());
}

void AVehicleExamplePawn::Brake(const FInputActionValue& Value)
{
	// route the input
	DoBrake(Value.Get<float>());
}

void AVehicleExamplePawn::StartBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStart();
}

void AVehicleExamplePawn::StopBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStop();
}

void AVehicleExamplePawn::StartHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStart();
}

void AVehicleExamplePawn::StopHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStop();
}

void AVehicleExamplePawn::LookAround(const FInputActionValue& Value)
{
	// route the input
	DoLookAround(Value.Get<float>());
}

void AVehicleExamplePawn::ToggleCamera(const FInputActionValue& Value)
{
	// route the input
	DoToggleCamera();
}

void AVehicleExamplePawn::ResetVehicle(const FInputActionValue& Value)
{
	// route the input
	DoResetVehicle();
}

void AVehicleExamplePawn::DoSteering(float SteeringValue)
{
	CurrentSteeringInput = SteeringValue;
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void AVehicleExamplePawn::DoThrottle(float ThrottleValue)
{
	CurrentThrottleInput = ThrottleValue;
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
	// Do NOT reset brake here � the axis fires every frame even at zero,
	// which would cancel any brake input applied the same frame.
}

void AVehicleExamplePawn::DoBrake(float BrakeValue)
{
	CurrentBrakeInput = BrakeValue;
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);
	// Do NOT reset throttle here for the same reason.
}

void AVehicleExamplePawn::DoBrakeStart()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(true);
}

void AVehicleExamplePawn::DoBrakeStop()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AVehicleExamplePawn::DoHandbrakeStart()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void AVehicleExamplePawn::DoHandbrakeStop()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void AVehicleExamplePawn::DoLookAround(float YawDelta)
{
	// rotate the spring arm
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AVehicleExamplePawn::DoToggleCamera()
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void AVehicleExamplePawn::DoResetVehicle()
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

void AVehicleExamplePawn::ApplyVehicleStats(const FEffectiveVehicleStats& Stats)
{
	UChaosWheeledVehicleMovementComponent* MoveComp = ChaosVehicleMovement;
	if (!MoveComp) { return; }

	// --- Mass ---
	if (Stats.MassKg > 0.f)
	{
		GetMesh()->SetMassOverrideInKg(NAME_None, Stats.MassKg, true);
	}

	// --- Engine ---
	if (Stats.MaxRPM > 0.f || Stats.TorqueNm > 0.f)
	{
		FVehicleEngineConfig EngineCfg = MoveComp->EngineSetup;
		if (Stats.MaxRPM > 0.f)   { EngineCfg.MaxRPM    = Stats.MaxRPM; }
		if (Stats.TorqueNm > 0.f)  { EngineCfg.MaxTorque = Stats.TorqueNm; }
		MoveComp->EngineSetup = EngineCfg;
	}

	// --- Transmission ---
	{
		FVehicleTransmissionConfig TransCfg = MoveComp->TransmissionSetup;
		if (Stats.FinalDriveRatio > 0.f)
		{
			TransCfg.FinalRatio = Stats.FinalDriveRatio;
		}
		if (Stats.GearRatios.Num() > 0)
		{
			// Resize the gear array to match (preserving existing defaults for
			// any gears the data asset doesn't specify).
			TransCfg.ForwardGearRatios.SetNum(Stats.GearRatios.Num());
			for (int32 i = 0; i < Stats.GearRatios.Num(); ++i)
			{
				TransCfg.ForwardGearRatios[i] = Stats.GearRatios[i];
			}
		}
		MoveComp->TransmissionSetup = TransCfg;
	}

	// Reinitialise the Chaos vehicle simulation with the new setup values.
	// This must be called after modifying EngineSetup / TransmissionSetup.
	MoveComp->RecreatePhysicsState();

	// --- Wheel friction (grip) ---
	// Apply after RecreatePhysicsState so the Wheels array is fresh.
	if (Stats.GripMultiplier > 0.f)
	{
		for (UChaosVehicleWheel* Wheel : MoveComp->Wheels)
		{
			if (Wheel)
			{
				Wheel->FrictionForceMultiplier = Stats.GripMultiplier;
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ApplyVehicleStats: Mass=%.0fkg RPM=%.0f Torque=%.0fNm FinalDrive=%.3f Gears=%d Grip=%.2f"),
		Stats.MassKg, Stats.MaxRPM, Stats.TorqueNm,
		Stats.FinalDriveRatio, Stats.GearRatios.Num(), Stats.GripMultiplier);
}

void AVehicleExamplePawn::FlippedCheck()
{
	// check the difference in angle between the mesh's up vector and world up
	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());

	if (UpDot < FlipCheckMinDot)
	{
		// is this the second time we've checked that the vehicle is still flipped?
		if (bPreviousFlipCheck)
		{
			// reset the vehicle to upright
			DoResetVehicle();
		}
		
		// set the flipped check flag so the next check resets the car
		bPreviousFlipCheck = true;

	} else {

		// we're upright. reset the flipped check flag
		bPreviousFlipCheck = false;
	}
}

// ---------------------------------------------------------------------------
// Player-vs-Player challenge system
// ---------------------------------------------------------------------------

void AVehicleExamplePawn::RequestChallengePlayer(AVehicleExamplePawn* TargetPawn)
{
	if (!TargetPawn || TargetPawn == this) { return; }
	bHasOutgoingChallenge = true;
	Server_RequestChallenge(TargetPawn);
}

void AVehicleExamplePawn::Server_RequestChallenge_Implementation(AVehicleExamplePawn* TargetPawn)
{
	if (!TargetPawn || TargetPawn == this) { return; }

	// Validate proximity on the server
	const float Dist = FVector::Dist(GetActorLocation(), TargetPawn->GetActorLocation());
	if (Dist > PlayerChallengeRadius * 2.f) { return; }

	// Reject if either participant is already in a challenge
	if (TargetPawn->PendingChallenger.IsValid()) { return; }

	TargetPawn->PendingChallenger = this;
	TargetPawn->Client_ReceiveChallengeRequest(this);
}

void AVehicleExamplePawn::RespondToChallenge(bool bAccepted)
{
	AVehicleExamplePawn* Challenger = PendingChallenger.Get();
	if (!Challenger) { return; }
	Server_RespondToChallenge(Challenger, bAccepted);
}

void AVehicleExamplePawn::Server_RespondToChallenge_Implementation(AVehicleExamplePawn* ChallengerPawn,
                                                                    bool bAccepted)
{
	if (!ChallengerPawn) { return; }

	// Clear state on both sides regardless of outcome
	PendingChallenger = nullptr;
	ChallengerPawn->bHasOutgoingChallenge = false;

	if (bAccepted)
	{
		// Notify both participants so each client can start the race HUD
		Client_OnChallengeAccepted(ChallengerPawn);
		ChallengerPawn->Client_OnChallengeAccepted(this);
	}
	else
	{
		ChallengerPawn->Client_OnChallengeDeclined();
	}
}

void AVehicleExamplePawn::Client_ReceiveChallengeRequest_Implementation(AVehicleExamplePawn* ChallengerPawn)
{
	// Only do anything meaningful for the locally-controlled pawn
	if (!IsLocallyControlled()) { return; }
	ShowPlayerChallengePrompt(ChallengerPawn);
}

void AVehicleExamplePawn::Client_OnChallengeAccepted_Implementation(AVehicleExamplePawn* OpponentPawn)
{
	if (!IsLocallyControlled()) { return; }
	HidePlayerChallengePrompt();
	// The game mode handles the actual race start server-side; here we just
	// dismiss the prompt so the HUD can show the race state.
	UE_LOG(LogTemp, Log, TEXT("AVehicleExamplePawn: challenge accepted — opponent: %s"),
		*GetNameSafe(OpponentPawn));
}

void AVehicleExamplePawn::Client_OnChallengeDeclined_Implementation()
{
	if (!IsLocallyControlled()) { return; }
	bHasOutgoingChallenge = false;
	UE_LOG(LogTemp, Log, TEXT("AVehicleExamplePawn: challenge was declined"));
}

void AVehicleExamplePawn::ShowPlayerChallengePrompt(AVehicleExamplePawn* ChallengerPawn)
{
	HidePlayerChallengePrompt();

	const FText ChallengeName = FText::FromString(GetNameSafe(ChallengerPawn));

	PlayerChallengeWidget = SNew(SChallengePromptWidget)
		.ChallengerName(ChallengeName)
		.OnResponse_Lambda([this](bool bAccepted)
		{
			HidePlayerChallengePrompt();
			RespondToChallenge(bAccepted);
		});

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(
			PlayerChallengeWidget.ToSharedRef(), /*ZOrder=*/10);
	}
}

void AVehicleExamplePawn::HidePlayerChallengePrompt()
{
	if (PlayerChallengeWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PlayerChallengeWidget.ToSharedRef());
	}
	PlayerChallengeWidget.Reset();
}

#undef LOCTEXT_NAMESPACE
