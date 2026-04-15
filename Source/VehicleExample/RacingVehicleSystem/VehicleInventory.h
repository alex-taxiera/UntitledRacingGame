// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RacingVehicleTypes.h"
#include "VehicleDefinition.h"
#include "OwnedVehicle.h"
#include "VehicleInventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVehiclePurchased,  UOwnedVehicle*, NewVehicle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartInstalled,    UOwnedVehicle*, Vehicle, EPartSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerPointsChanged, int32, NewTotal);

/**
 * UVehicleInventory
 *
 * Central manager for the player's vehicle collection, currency, and part
 * upgrades.  Lives on the GameInstance for the lifetime of the session.
 *
 * All vehicles available for purchase are registered in AllVehicles by a
 * designer-edited array (set in the GameInstance Blueprint defaults or via
 * code).  Owned vehicles accumulate in OwnedVehicles at runtime.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UVehicleInventory : public UObject
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Shop Catalogue  (designer-populated)
    // -----------------------------------------------------------------------

    /**
     * The full list of vehicles that can appear in the shop.
     * Set this array in the GameInstance Blueprint defaults (or in code) by
     * pointing each entry at a UVehicleDefinition Data Asset.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    TArray<TObjectPtr<UVehicleDefinition>> AllVehicles;

    // -----------------------------------------------------------------------
    // Player State
    // -----------------------------------------------------------------------

    /** In-game currency balance */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 PlayerCurrency = 0;

    /**
     * Total points the player has spent (or earned) — used to gate part
     * unlock requirements without a separate levelling system.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 PlayerPoints = 0;

    /** All vehicles the player currently owns */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Garage")
    TArray<TObjectPtr<UOwnedVehicle>> OwnedVehicles;

    /**
     * The InstanceID of the vehicle the player currently has selected.
     * This is the car shown in the garage and used in races.
     * Invalid GUID = no vehicle selected (new game state).
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Garage")
    FGuid CurrentVehicleID;

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnVehiclePurchased OnVehiclePurchased;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPartInstalled OnPartInstalled;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerPointsChanged OnPlayerPointsChanged;

    // -----------------------------------------------------------------------
    // Currency
    // -----------------------------------------------------------------------

    /** Adds (or subtracts) currency.  Use negative values to deduct. */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    void AddCurrency(int32 Amount);

    // -----------------------------------------------------------------------
    // Points
    // -----------------------------------------------------------------------

    /** Awards points to the player (e.g. from completing races). */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    void AddPoints(int32 Amount);

    // -----------------------------------------------------------------------
    // Vehicle Purchase
    // -----------------------------------------------------------------------

    /**
     * Returns true if the player has enough currency to buy the given vehicle
     * and does not already own the maximum allowed duplicates (no cap by default).
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    bool CanPurchaseVehicle(UVehicleDefinition* Definition) const;

    /**
     * Purchases a vehicle, deducting its price and adding a new UOwnedVehicle
     * to OwnedVehicles.  Returns the new instance, or nullptr on failure.
     * Broadcasts OnVehiclePurchased on success.
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    UOwnedVehicle* PurchaseVehicle(UVehicleDefinition* Definition);

    // -----------------------------------------------------------------------
    // Part Installation
    // -----------------------------------------------------------------------

    /**
     * Returns true if the player meets all requirements to install the given
     * part level on the given owned vehicle:
     *   - Vehicle and slot must exist in the definition.
     *   - Level must be > currently installed level (forward upgrades only).
     *   - Player must have enough currency.
     *   - Player must meet the unlock points requirement for that level.
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    bool CanInstallPart(UOwnedVehicle* Vehicle, EPartSlot Slot, int32 Level) const;

    /**
     * Installs the part, deducts the price, and broadcasts OnPartInstalled.
     * Returns true on success.
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    bool InstallPart(UOwnedVehicle* Vehicle, EPartSlot Slot, int32 Level);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * Returns all owned vehicle instances in a Blueprint-friendly array.
     * (Raw UObject pointers are required for UFUNCTION signatures.)
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    TArray<UOwnedVehicle*> GetOwnedVehicles() const
    {
        TArray<UOwnedVehicle*> Out;
        Out.Reserve(OwnedVehicles.Num());
        for (const TObjectPtr<UOwnedVehicle>& Vehicle : OwnedVehicles)
        {
            if (Vehicle)
            {
                Out.Add(Vehicle.Get());
            }
        }
        return Out;
    }

    /** Internal C++ accessor that avoids array copy. */
    const TArray<TObjectPtr<UOwnedVehicle>>& GetOwnedVehiclesRef() const { return OwnedVehicles; }

    /** Finds an owned vehicle by its instance GUID, returns nullptr if not found. */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    UOwnedVehicle* FindOwnedVehicleByID(FGuid InstanceID) const;

    /** Returns true if the player owns at least one instance of the given definition. */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    bool OwnsVehicleModel(UVehicleDefinition* Definition) const;

    /**
     * Sets the current vehicle to the given owned instance.
     * Pass nullptr to clear the selection.
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleInventory")
    void SetCurrentVehicle(UOwnedVehicle* Vehicle);

    /**
     * Returns the currently selected vehicle, or nullptr if none is set
     * or the saved GUID no longer matches any owned vehicle.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VehicleInventory")
    UOwnedVehicle* GetCurrentVehicle() const;

    /** Returns true if the player has at least one owned vehicle. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VehicleInventory")
    bool HasAnyVehicle() const { return OwnedVehicles.Num() > 0; }
};
