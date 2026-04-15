// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RacingSaveGame.generated.h"

/**
 * URacingSaveGame
 *
 * The single save slot object for the game.
 * Serialised to / from disk by URacingGameInstance::SaveGame() and LoadGame().
 *
 * Only primitive and plain-struct data is stored here.  UObject references
 * (like UOwnedVehicle) are reconstructed at load time using the saved data
 * as input rather than being serialised directly, which keeps the save file
 * robust against asset renames.
 *
 * Fields map 1-to-1 with the SaveGame-marked properties on UVehicleInventory
 * and UPlayerPerkManager.
 */
UCLASS()
class VEHICLEEXAMPLE_API URacingSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Save slot identity
    // -----------------------------------------------------------------------

    /** The slot name used for all save/load/delete operations. */
    static const FString SlotName;

    /** Always 0 — single-user, no profile index needed. */
    static constexpr int32 UserIndex = 0;

    // -----------------------------------------------------------------------
    // Economy
    // -----------------------------------------------------------------------

    UPROPERTY(SaveGame)
    int32 PlayerCurrency = 0;

    UPROPERTY(SaveGame)
    int32 PlayerPoints = 0;

    // -----------------------------------------------------------------------
    // Vehicle inventory
    // -----------------------------------------------------------------------

    /**
     * Each entry is the VehicleID of an owned vehicle instance paired with
     * its serialised state.  We avoid storing UObject pointers directly so
     * the save is not coupled to object lifetimes.
     */
    UPROPERTY(SaveGame)
    TArray<FGuid> OwnedVehicleIDs;

    /** Per-vehicle: which definition does this instance use (stable VehicleID FName). */
    UPROPERTY(SaveGame)
    TMap<FGuid, FName> VehicleDefinitionIDs;

    /** Per-vehicle: optional nickname string. */
    UPROPERTY(SaveGame)
    TMap<FGuid, FString> VehicleNicknames;

    /** Per-vehicle: installed part levels (EPartSlot int32 ? level index). */
    UPROPERTY(SaveGame)
    TMap<FGuid, FString> VehicleInstalledPartsJSON;

    /** The InstanceID of the currently selected vehicle. */
    UPROPERTY(SaveGame)
    FGuid CurrentVehicleID;

    // -----------------------------------------------------------------------
    // Perk manager
    // -----------------------------------------------------------------------

    UPROPERTY(SaveGame)
    int32 SkillPointBank = 0;

    UPROPERTY(SaveGame)
    TArray<FName> UnlockedPerkIDs;

    UPROPERTY(SaveGame)
    TArray<FName> EquippedSkillPerkIDs;

    UPROPERTY(SaveGame)
    int32 BaseSkillSlots = 3;

    UPROPERTY(SaveGame)
    TArray<FName> AchievedStoryFlags;

    // -----------------------------------------------------------------------
    // Static helpers
    // -----------------------------------------------------------------------

    /**
     * Returns true if a save file already exists in the default slot.
     * Does not load the save — just checks for its presence on disk.
     */
    static bool DoesSaveExist();
};
