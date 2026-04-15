// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleInventory.h"
#include "VehiclePartData.h"

// ---------------------------------------------------------------------------
// Currency
// ---------------------------------------------------------------------------

void UVehicleInventory::AddCurrency(int32 Amount)
{
    PlayerCurrency = FMath::Max(0, PlayerCurrency + Amount);
}

// ---------------------------------------------------------------------------
// Points
// ---------------------------------------------------------------------------

void UVehicleInventory::AddPoints(int32 Amount)
{
    PlayerPoints = FMath::Max(0, PlayerPoints + Amount);
    OnPlayerPointsChanged.Broadcast(PlayerPoints);
}

// ---------------------------------------------------------------------------
// Vehicle Purchase
// ---------------------------------------------------------------------------

bool UVehicleInventory::CanPurchaseVehicle(UVehicleDefinition* Definition) const
{
    if (!Definition) { return false; }
    return PlayerCurrency >= Definition->PurchasePrice;
}

UOwnedVehicle* UVehicleInventory::PurchaseVehicle(UVehicleDefinition* Definition)
{
    if (!CanPurchaseVehicle(Definition)) { return nullptr; }

    AddCurrency(-Definition->PurchasePrice);

    UOwnedVehicle* NewVehicle = UOwnedVehicle::CreateFromDefinition(this, Definition);
    OwnedVehicles.Add(NewVehicle);

    OnVehiclePurchased.Broadcast(NewVehicle);
    return NewVehicle;
}

// ---------------------------------------------------------------------------
// Part Installation
// ---------------------------------------------------------------------------

bool UVehicleInventory::CanInstallPart(UOwnedVehicle* Vehicle, EPartSlot Slot, int32 Level) const
{
    if (!Vehicle || !Vehicle->Definition) { return false; }

    UVehiclePartData* PartData = Vehicle->Definition->GetPartData(Slot);
    if (!PartData || !PartData->IsValidLevel(Level)) { return false; }

    // Only forward upgrades
    if (Level <= Vehicle->GetInstalledLevel(Slot)) { return false; }

    const FPartLevelData* LevelData = PartData->GetLevelData(Level);
    if (!LevelData) { return false; }

    if (PlayerCurrency < LevelData->PurchasePrice) { return false; }
    if (PlayerPoints < LevelData->UnlockPointsRequired) { return false; }

    return true;
}

bool UVehicleInventory::InstallPart(UOwnedVehicle* Vehicle, EPartSlot Slot, int32 Level)
{
    if (!CanInstallPart(Vehicle, Slot, Level)) { return false; }

    UVehiclePartData* PartData = Vehicle->Definition->GetPartData(Slot);
    const FPartLevelData* LevelData = PartData->GetLevelData(Level);

    AddCurrency(-LevelData->PurchasePrice);
    Vehicle->SetPartLevel(Slot, Level);

    OnPartInstalled.Broadcast(Vehicle, Slot);
    return true;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

UOwnedVehicle* UVehicleInventory::FindOwnedVehicleByID(FGuid InstanceID) const
{
    for (UOwnedVehicle* Vehicle : OwnedVehicles)
    {
        if (Vehicle && Vehicle->InstanceID == InstanceID) { return Vehicle; }
    }
    return nullptr;
}

bool UVehicleInventory::OwnsVehicleModel(UVehicleDefinition* Definition) const
{
    if (!Definition) { return false; }
    for (const UOwnedVehicle* Vehicle : OwnedVehicles)
    {
        if (Vehicle && Vehicle->Definition == Definition) { return true; }
    }
    return false;
}

void UVehicleInventory::SetCurrentVehicle(UOwnedVehicle* Vehicle)
{
    CurrentVehicleID = Vehicle ? Vehicle->InstanceID : FGuid();
}

UOwnedVehicle* UVehicleInventory::GetCurrentVehicle() const
{
    if (!CurrentVehicleID.IsValid()) { return nullptr; }
    return FindOwnedVehicleByID(CurrentVehicleID);
}
