// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "RacingVehicleSystem/RacingVehicleTypes.h"
#include "VehicleExamplePawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
struct FInputActionValue;
class SChallengePromptWidget;

/**
 *  Vehicle Pawn class
 *  Handles common functionality for all vehicle types,
 *  including input handling and camera management.
 *  
 *  Specific vehicle configurations are handled in subclasses.
 */
UCLASS(abstract)
class AVehicleExamplePawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	/** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** Cast pointer to the Chaos Vehicle movement component */
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:

	/** Steering Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SteeringAction;

	/** Throttle Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ThrottleAction;

	/** Brake Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BrakeAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleCameraAction;

	/** Reset Vehicle Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetVehicleAction;

	/** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

	/** Keeps track of whether the car is flipped. If this is true for two flip checks, resets the vehicle automatically */
	bool bPreviousFlipCheck = false;

	float CurrentThrottleInput = 0.f;
	float CurrentBrakeInput    = 0.f;
	float CurrentSteeringInput = 0.f;

	/** Time between automatic flip checks */
	UPROPERTY(EditAnywhere, Category="Flip Check", meta = (Units = "s"))
	float FlipCheckTime = 3.0f;

	/** Minimum dot product value for the vehicle's up direction that we still consider upright */
	UPROPERTY(EditAnywhere, Category="Flip Check")
	float FlipCheckMinDot = -0.2f;

	/** Flip check timer */
	FTimerHandle FlipCheckTimer;

public:
	AVehicleExamplePawn();

	// Begin Pawn interface

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	// End Pawn interface

	// Begin Actor interface

	/** Initialization */
	virtual void BeginPlay() override;

	/** Called on the owning client when this pawn is (re)started — ensures game-only input mode. */
	virtual void PawnClientRestart() override;

	/** Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Update */
	virtual void Tick(float Delta) override;

	// End Actor interface

protected:

	/** Handles steering input */
	void Steering(const FInputActionValue& Value);

	/** Handles throttle input */
	void Throttle(const FInputActionValue& Value);

	/** Handles brake input */
	void Brake(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	/** Handles handbrake start/stop inputs */
	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCamera(const FInputActionValue& Value);

	/** Handles reset vehicle input */
	void ResetVehicle(const FInputActionValue& Value);

public:

	/** Handle steering input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSteering(float SteeringValue);

	/** Handle throttle input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoThrottle(float ThrottleValue);

	/** Handle brake input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrake(float BrakeValue);

	// Debug accessors for UI
	float GetThrottleInput()  const { return CurrentThrottleInput; }
	float GetBrakeInput()     const { return CurrentBrakeInput; }
	float GetSteeringInput()  const { return CurrentSteeringInput; }

	/** Handle brake start input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStart();

	/** Handle brake stop input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStop();

	/** Handle handbrake start input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStart();

	/** Handle handbrake stop input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStop();

	/** Handle look input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoLookAround(float YawDelta);

	/** Handle toggle camera input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoToggleCamera();

	/** Handle reset vehicle input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoResetVehicle();

	/**
	 * Applies fully-resolved vehicle stats from a data asset to the Chaos physics
	 * simulation.  Call once after the pawn is possessed and the movement component
	 * is ready (i.e. from BeginPlay or a deferred next-tick).
	 *
	 * Sets: mass, engine MaxRPM & MaxTorque, transmission final drive ratio and
	 * per-gear ratios, and per-wheel friction multiplier (grip).
	 */
	UFUNCTION(BlueprintCallable, Category="Vehicle")
	void ApplyVehicleStats(const FEffectiveVehicleStats& Stats);

protected:

	/** Called when the brake lights are turned on or off */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

	/** Checks if the car is flipped upside down and automatically resets it */
	UFUNCTION()
	void FlippedCheck();

public:
	/** Returns the front spring arm subobject */
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	/** Returns the front camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	/** Returns the back spring arm subobject */
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	/** Returns the back camera subobject */
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	/** Returns the cast Chaos Vehicle Movement subobject */
	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }

	// -----------------------------------------------------------------------
	// Player-vs-Player challenge system
	// -----------------------------------------------------------------------

	/**
	 * Radius (cm) within which this pawn detects other player pawns for
	 * challenge prompts. Default 1500 cm = 15 m, same as NPC challenges.
	 */
	UPROPERTY(EditAnywhere, Category = "Challenge")
	float PlayerChallengeRadius = 1500.f;

	/**
	 * Send a challenge request to TargetPawn. Callable on the local client;
	 * routes through the server for validation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Challenge")
	void RequestChallengePlayer(AVehicleExamplePawn* TargetPawn);

	/**
	 * Accept or decline a pending challenge. Callable on the local client;
	 * routes through the server for authoritative resolution.
	 */
	UFUNCTION(BlueprintCallable, Category = "Challenge")
	void RespondToChallenge(bool bAccepted);

	/** The pawn that has sent us a pending challenge request (nullptr if none). */
	UFUNCTION(BlueprintPure, Category = "Challenge")
	AVehicleExamplePawn* GetPendingChallenger() const { return PendingChallenger.Get(); }

	/** True while this pawn is waiting for a response to an outgoing challenge. */
	UFUNCTION(BlueprintPure, Category = "Challenge")
	bool HasOutgoingChallenge() const { return bHasOutgoingChallenge; }

private:

	// --- Server RPCs ---

	UFUNCTION(Server, Reliable)
	void Server_RequestChallenge(AVehicleExamplePawn* TargetPawn);

	UFUNCTION(Server, Reliable)
	void Server_RespondToChallenge(AVehicleExamplePawn* ChallengerPawn, bool bAccepted);

	// --- Client RPCs ---

	/** Delivered to the challenged player so they can show the prompt. */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveChallengeRequest(AVehicleExamplePawn* ChallengerPawn);

	/** Delivered to both participants when the challenge is resolved. */
	UFUNCTION(Client, Reliable)
	void Client_OnChallengeAccepted(AVehicleExamplePawn* OpponentPawn);

	/** Delivered to the challenger if their target declines. */
	UFUNCTION(Client, Reliable)
	void Client_OnChallengeDeclined();

	// --- State ---

	/** The pawn that has challenged us (set on the challenged player's pawn). */
	UPROPERTY()
	TWeakObjectPtr<AVehicleExamplePawn> PendingChallenger;

	bool bHasOutgoingChallenge = false;

	/** Shows a player challenge prompt in the viewport on this client. */
	void ShowPlayerChallengePrompt(AVehicleExamplePawn* ChallengerPawn);
	void HidePlayerChallengePrompt();

	TSharedPtr<class SChallengePromptWidget> PlayerChallengeWidget;
};
