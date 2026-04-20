// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPerkManager.h"
#include "RacingSaveGame.h"
#include "OwnedVehicle.h"
#include "VehicleDefinition.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Misc/CommandLine.h"

const int32 URacingGameInstance::StartingCurrency = 10000000;

URacingGameInstance::URacingGameInstance()
{
}

void URacingGameInstance::Init()
{
    Super::Init();

    // Apply -SaveSlot=<Name> command-line override if provided.
    FString CmdSlot;
    if (FParse::Value(FCommandLine::Get(), TEXT("SaveSlot="), CmdSlot) && !CmdSlot.IsEmpty())
    {
        ActiveSaveSlot = CmdSlot;
        UE_LOG(LogTemp, Log, TEXT("URacingGameInstance::Init — save slot set from command line: '%s'"), *ActiveSaveSlot);
    }

    VehicleInventory = NewObject<UVehicleInventory>(this, TEXT("VehicleInventory"));
    VehicleInventory->AllVehicles = AllVehicles;

    PerkManager = NewObject<UPlayerPerkManager>(this, TEXT("PerkManager"));
    PerkManager->AllPerks = AllPerks;
    PerkManager->Initialise();
}

URacingGameInstance* URacingGameInstance::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) { return nullptr; }
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    return World ? Cast<URacingGameInstance>(UGameplayStatics::GetGameInstance(World)) : nullptr;
}

// ---------------------------------------------------------------------------
// Save / Load / Delete
// ---------------------------------------------------------------------------

bool URacingGameInstance::HasSaveGame() const
{
    return UGameplayStatics::DoesSaveGameExist(ActiveSaveSlot, URacingSaveGame::UserIndex);
}

void URacingGameInstance::SaveGame()
{
    URacingSaveGame* Save = Cast<URacingSaveGame>(
        UGameplayStatics::CreateSaveGameObject(URacingSaveGame::StaticClass()));

    if (!Save) { return; }

    // --- Economy ---
    Save->PlayerCurrency = VehicleInventory->PlayerCurrency;
    Save->PlayerPoints   = VehicleInventory->PlayerPoints;
    Save->CurrentVehicleID = VehicleInventory->CurrentVehicleID;

    // --- Vehicle inventory ---
    Save->OwnedVehicleIDs.Empty();
    Save->VehicleDefinitionIDs.Empty();
    Save->VehicleNicknames.Empty();
    Save->VehiclePartLevels.Empty();

    for (UOwnedVehicle* Vehicle : VehicleInventory->OwnedVehicles)
    {
        if (!Vehicle || !Vehicle->Definition) { continue; }

        Save->OwnedVehicleIDs.Add(Vehicle->InstanceID);
        Save->VehicleDefinitionIDs.Add(Vehicle->InstanceID, Vehicle->Definition->VehicleID);
        Save->VehicleNicknames.Add(Vehicle->InstanceID, Vehicle->Nickname);

        FSavedVehiclePartLevels PartData;
        PartData.Levels = Vehicle->InstalledPartLevels;
        Save->VehiclePartLevels.Add(Vehicle->InstanceID, PartData);
    }

    // --- Perk manager ---
    if (PerkManager && PerkManager->PerkState)
    {
        Save->SkillPointBank       = PerkManager->SkillPointBank;
        Save->BaseSkillSlots       = PerkManager->BaseSkillSlots;
        Save->UnlockedPerkIDs      = PerkManager->PerkState->UnlockedPerkIDs;
        Save->EquippedSkillPerkIDs = PerkManager->PerkState->EquippedSkillPerkIDs;

        Save->AchievedStoryFlags.Empty();
        for (const FName& Flag : PerkManager->AchievedStoryFlags)
        {
            Save->AchievedStoryFlags.Add(Flag);
        }
    }

    UGameplayStatics::SaveGameToSlot(Save, ActiveSaveSlot, URacingSaveGame::UserIndex);
}

void URacingGameInstance::LoadGame()
{
    if (!HasSaveGame()) { return; }

    URacingSaveGame* Save = Cast<URacingSaveGame>(
        UGameplayStatics::LoadGameFromSlot(ActiveSaveSlot, URacingSaveGame::UserIndex));

    if (!Save) { return; }

    // --- Economy ---
    VehicleInventory->PlayerCurrency   = Save->PlayerCurrency;
    VehicleInventory->PlayerPoints     = Save->PlayerPoints;
    VehicleInventory->CurrentVehicleID = Save->CurrentVehicleID;

    // --- Vehicle inventory ---
    VehicleInventory->OwnedVehicles.Empty();

    // Build a lookup from VehicleID FName -> UVehicleDefinition* using AllVehicles
    TMap<FName, UVehicleDefinition*> DefinitionsByID;
    for (UVehicleDefinition* Def : AllVehicles)
    {
        if (Def) { DefinitionsByID.Add(Def->VehicleID, Def); }
    }

    for (const FGuid& InstanceID : Save->OwnedVehicleIDs)
    {
        const FName* DefID = Save->VehicleDefinitionIDs.Find(InstanceID);
        if (!DefID) { continue; }

        UVehicleDefinition** DefPtr = DefinitionsByID.Find(*DefID);
        if (!DefPtr || !*DefPtr) { continue; }

        // Reconstruct the OwnedVehicle from its definition
        UOwnedVehicle* Vehicle = UOwnedVehicle::CreateFromDefinition(VehicleInventory, *DefPtr);
        Vehicle->InstanceID = InstanceID; // restore the original GUID

        // Restore nickname
        if (const FString* Nick = Save->VehicleNicknames.Find(InstanceID))
        {
            Vehicle->Nickname = *Nick;
        }

        // Restore installed part levels
        if (const FSavedVehiclePartLevels* Parts = Save->VehiclePartLevels.Find(InstanceID))
        {
            Vehicle->InstalledPartLevels = Parts->Levels;
        }

        VehicleInventory->OwnedVehicles.Add(Vehicle);
    }

    UE_LOG(LogTemp, Log, TEXT("URacingGameInstance::LoadGame � loaded %d vehicle(s)"),
        VehicleInventory->OwnedVehicles.Num());

    // --- Perk manager ---
    if (PerkManager && PerkManager->PerkState)
    {
        PerkManager->SkillPointBank                      = Save->SkillPointBank;
        PerkManager->BaseSkillSlots                      = Save->BaseSkillSlots;
        PerkManager->PerkState->UnlockedPerkIDs          = Save->UnlockedPerkIDs;
        PerkManager->PerkState->EquippedSkillPerkIDs     = Save->EquippedSkillPerkIDs;

        PerkManager->AchievedStoryFlags.Empty();
        for (const FName& Flag : Save->AchievedStoryFlags)
        {
            PerkManager->AchievedStoryFlags.Add(Flag);
        }
    }
}

void URacingGameInstance::DeleteSave()
{
    UGameplayStatics::DeleteGameInSlot(ActiveSaveSlot, URacingSaveGame::UserIndex);

    // Reset in-memory state to defaults
    VehicleInventory->PlayerCurrency = 0;
    VehicleInventory->PlayerPoints   = 0;
    VehicleInventory->OwnedVehicles.Empty();
    VehicleInventory->CurrentVehicleID = FGuid();

    if (PerkManager)
    {
        PerkManager->SkillPointBank = 0;
        PerkManager->AchievedStoryFlags.Empty();

        if (PerkManager->PerkState)
        {
            PerkManager->PerkState->UnlockedPerkIDs.Empty();
            PerkManager->PerkState->EquippedSkillPerkIDs.Empty();
        }
    }
}

// ---------------------------------------------------------------------------
// Game Flow
// ---------------------------------------------------------------------------

void URacingGameInstance::StartNewGame()
{
    if (VehicleInventory)
    {
        VehicleInventory->PlayerCurrency = StartingCurrency;
        VehicleInventory->CurrentVehicleID = FGuid();
    }
    UGameplayStatics::OpenLevel(this, GameLevelName);
}

void URacingGameInstance::ContinueGame()
{
    LoadGame();
    UGameplayStatics::OpenLevel(this, GameLevelName);
}

void URacingGameInstance::StartCourse()
{
    SaveGame();
    // Open as a listen server so other players on the LAN can discover and join.
    UGameplayStatics::OpenLevel(this, CourseLevelName, true, TEXT("?listen"));
}

void URacingGameInstance::ReturnToTitle()
{
    if (VehicleInventory)
    {
        VehicleInventory->OwnedVehicles.Empty();
        VehicleInventory->PlayerCurrency   = 0;
        VehicleInventory->PlayerPoints     = 0;
        VehicleInventory->CurrentVehicleID = FGuid();
    }
    if (PerkManager)
    {
        PerkManager->SkillPointBank = 0;
        PerkManager->AchievedStoryFlags.Empty();
        if (PerkManager->PerkState)
        {
            PerkManager->PerkState->UnlockedPerkIDs.Empty();
            PerkManager->PerkState->EquippedSkillPerkIDs.Empty();
        }
    }
    UGameplayStatics::OpenLevel(this, TitleLevelName);
}

// ---------------------------------------------------------------------------
// Multiplayer / Session management
// ---------------------------------------------------------------------------

static const FName CourseSessionName = TEXT("CourseSession");

IOnlineSessionPtr URacingGameInstance::GetSessionInterface() const
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    return OSS ? OSS->GetSessionInterface() : nullptr;
}

void URacingGameInstance::HostCourseSession()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) { return; }

    // Destroy any leftover session from a previous run before creating a new one.
    if (Sessions->GetNamedSession(CourseSessionName))
    {
        Sessions->DestroySession(CourseSessionName);
    }

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch           = true;
    Settings.NumPublicConnections  = MaxPlayersPerSession;
    Settings.bShouldAdvertise      = true;
    Settings.bAllowJoinInProgress  = true;
    Settings.bUsesPresence         = false;
    Settings.bAllowInvites         = false;

    Sessions->OnCreateSessionCompleteDelegates.AddUObject(
        this, &URacingGameInstance::OnCreateSessionComplete);

    Sessions->CreateSession(0, CourseSessionName, Settings);
}

void URacingGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnCreateSessionCompleteDelegates(this);
    }
    UE_LOG(LogTemp, Log, TEXT("URacingGameInstance::OnCreateSessionComplete — %s: %s"),
        *SessionName.ToString(), bWasSuccessful ? TEXT("OK") : TEXT("FAILED"));
}

void URacingGameInstance::FindCourseSessions()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid() || bSearchingForSession) { return; }

    bSearchingForSession = true;

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery      = true;
    SessionSearch->MaxSearchResults = 16;

    Sessions->OnFindSessionsCompleteDelegates.AddUObject(
        this, &URacingGameInstance::OnFindSessionsComplete);

    Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void URacingGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegates(this);
    }

    bSearchingForSession = false;

    const int32 NumFound = (bWasSuccessful && SessionSearch.IsValid())
        ? SessionSearch->SearchResults.Num() : 0;

    UE_LOG(LogTemp, Log, TEXT("URacingGameInstance::OnFindSessionsComplete — found %d session(s)"),
        NumFound);

    OnSessionsFound.Broadcast(NumFound > 0);
}

void URacingGameInstance::JoinFirstFoundSession()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) { return; }
    if (!SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("URacingGameInstance::JoinFirstFoundSession — no results"));
        return;
    }

    Sessions->OnJoinSessionCompleteDelegates.AddUObject(
        this, &URacingGameInstance::OnJoinSessionComplete);

    Sessions->JoinSession(0, CourseSessionName, SessionSearch->SearchResults[0]);
}

void URacingGameInstance::OnJoinSessionComplete(FName SessionName,
                                                EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegates(this);
    }

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("URacingGameInstance::OnJoinSessionComplete — failed (%d)"),
            static_cast<int32>(Result));
        return;
    }

    FString TravelURL;
    if (Sessions->GetResolvedConnectString(SessionName, TravelURL))
    {
        APlayerController* PC = GetFirstLocalPlayerController();
        if (PC)
        {
            PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
        }
    }
}

