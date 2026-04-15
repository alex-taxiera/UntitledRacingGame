// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerPerkManager.h"
#include "RacingSaveGame.h"

const FName URacingGameInstance::GameLevelName = TEXT("HubLevel");
const int32 URacingGameInstance::StartingCurrency = 10000000;

URacingGameInstance::URacingGameInstance()
{
}

void URacingGameInstance::Init()
{
    Super::Init();

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
    return URacingSaveGame::DoesSaveExist();
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

    for (UOwnedVehicle* Vehicle : VehicleInventory->OwnedVehicles)
    {
        if (!Vehicle || !Vehicle->Definition) { continue; }

        Save->OwnedVehicleIDs.Add(Vehicle->InstanceID);
        Save->VehicleDefinitionIDs.Add(Vehicle->InstanceID, Vehicle->Definition->VehicleID);
        Save->VehicleNicknames.Add(Vehicle->InstanceID, Vehicle->Nickname);
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

    UGameplayStatics::SaveGameToSlot(Save, URacingSaveGame::SlotName, URacingSaveGame::UserIndex);
}

void URacingGameInstance::LoadGame()
{
    if (!HasSaveGame()) { return; }

    URacingSaveGame* Save = Cast<URacingSaveGame>(
        UGameplayStatics::LoadGameFromSlot(URacingSaveGame::SlotName, URacingSaveGame::UserIndex));

    if (!Save) { return; }

    // --- Economy ---
    VehicleInventory->PlayerCurrency  = Save->PlayerCurrency;
    VehicleInventory->PlayerPoints    = Save->PlayerPoints;
    VehicleInventory->CurrentVehicleID = Save->CurrentVehicleID;

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

    // Note: OwnedVehicle reconstruction requires the full vehicle definition
    // catalogue to be loaded. Wire this up in a future pass once VehicleDefinition
    // assets are referenced from the GameInstance Blueprint defaults and
    // AllVehicles is populated before LoadGame() is called.
    UE_LOG(LogTemp, Log, TEXT("URacingGameInstance::LoadGame — save loaded (vehicle reconstruction pending)"));
}

void URacingGameInstance::DeleteSave()
{
    UGameplayStatics::DeleteGameInSlot(URacingSaveGame::SlotName, URacingSaveGame::UserIndex);

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

