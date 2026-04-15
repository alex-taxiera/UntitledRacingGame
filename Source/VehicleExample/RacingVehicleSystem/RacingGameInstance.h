// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VehicleInventory.h"
#include "PlayerPerkManager.h"
#include "PerkData.h"
#include "RacingSaveGame.h"
#include "RacingGameInstance.generated.h"

/**
 * URacingGameInstance
 *
 * The game instance subclass for the racing game.
 * Set this as the Game Instance Class in Project Settings ? Maps & Modes.
 *
 * Owns the UVehicleInventory which persists across level loads.
 * The AllVehicles catalogue is set via the Blueprint defaults of the
 * GameInstance asset — simply add UVehicleDefinition Data Assets to the
 * array exposed on the VehicleInventory property.
 *
 * Save/Load stubs are provided here; wire them into your save system when ready.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API URacingGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    URacingGameInstance();

    // -----------------------------------------------------------------------
    // Designer-populated catalogues
    // Set these in Project Settings once RacingGameInstance is the Game
    // Instance Class — they appear under "Racing" in the class defaults.
    // -----------------------------------------------------------------------

    /**
     * All vehicle definitions available in the dealership.
     * Add your UVehicleDefinition (or subclass) Data Assets here.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing|Catalogue")
    TArray<TObjectPtr<UVehicleDefinition>> AllVehicles;

    /**
     * Complete perk catalogue.  Add all UPerkData assets here.
     * The perk manager reads this array at runtime.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing|Catalogue")
    TArray<TObjectPtr<UPerkData>> AllPerks;

    /**
     * The player's vehicle inventory.  Always valid after Init().
     * Accessible from Blueprint for UI binding and event subscription.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Racing")
    TObjectPtr<UVehicleInventory> VehicleInventory;

    /** Convenience getter callable from anywhere that has a world context. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Racing",
        meta = (WorldContext = "WorldContextObject"))
    static URacingGameInstance* Get(const UObject* WorldContextObject);

    /** Returns the VehicleInventory. Never null after game start. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Racing")
    UVehicleInventory* GetVehicleInventory() const { return VehicleInventory; }

    /**
     * The player's perk manager.  Always valid after Init().
     * Holds the perk catalogue, skill point bank, and unlocked perk state.
     * Populate AllPerks from the Blueprint defaults of this GameInstance asset.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Racing")
    TObjectPtr<UPlayerPerkManager> PerkManager;

    /** Returns the PerkManager. Never null after game start. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Racing")
    UPlayerPerkManager* GetPerkManager() const { return PerkManager; }

    // -----------------------------------------------------------------------
    // UGameInstance interface
    // -----------------------------------------------------------------------

    virtual void Init() override;

    // -----------------------------------------------------------------------
    // Save / Load / Delete
    // -----------------------------------------------------------------------

    /** Returns true if a save file exists on disk. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Racing|Save")
    bool HasSaveGame() const;

    /**
     * Serialises the current game state (inventory + perk manager) to disk.
     * Call this whenever the player makes progress that should persist.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing|Save")
    void SaveGame();

    /**
     * Deserialises the save file from disk and restores all state.
     * Safe to call even if no save exists (does nothing in that case).
     */
    UFUNCTION(BlueprintCallable, Category = "Racing|Save")
    void LoadGame();

    /**
     * Permanently deletes the save file from disk and resets in-memory state
     * to defaults.  The title screen calls this after the player confirms.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing|Save")
    void DeleteSave();

    // -----------------------------------------------------------------------
    // Game Flow
    // -----------------------------------------------------------------------

    /**
     * Starts a fresh game: resets all state and opens the game level.
     * Does NOT delete any existing save — call DeleteSave() first if needed.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing")
    void StartNewGame();

    /**
     * Loads the existing save then opens the game level.
     * No-op if no save exists — the title screen should hide this button in that case.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing")
    void ContinueGame();

private:

    /** The level to open when starting or continuing a game. */
    static const FName GameLevelName;

    /** Currency granted to the player at the start of a new game. */
    static const int32 StartingCurrency;
};
