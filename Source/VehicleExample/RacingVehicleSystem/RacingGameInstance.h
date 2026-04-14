// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VehicleInventory.h"
#include "PlayerPerkManager.h"
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
    // Inventory access
    // -----------------------------------------------------------------------

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
    // Save / Load  (stubs — wire to your save system)
    // -----------------------------------------------------------------------

    /**
     * Serialises the inventory state (owned vehicles, currency, points) to a
     * save slot.  Implementation left as a stub — fill in when your save
     * system is ready.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing|Save")
    void SaveGame();

    /**
     * Deserialises inventory state from the given save slot.
     * Implementation left as a stub.
     */
    UFUNCTION(BlueprintCallable, Category = "Racing|Save")
    void LoadGame();
};
