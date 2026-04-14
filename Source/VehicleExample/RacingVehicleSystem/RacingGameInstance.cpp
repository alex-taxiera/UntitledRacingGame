// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingGameInstance.h"
#include "Kismet/GameplayStatics.h"

URacingGameInstance::URacingGameInstance()
{
}

void URacingGameInstance::Init()
{
    Super::Init();

    VehicleInventory = NewObject<UVehicleInventory>(this, TEXT("VehicleInventory"));
}

URacingGameInstance* URacingGameInstance::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject) { return nullptr; }
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    return World ? Cast<URacingGameInstance>(UGameplayStatics::GetGameInstance(World)) : nullptr;
}

void URacingGameInstance::SaveGame()
{
    // TODO: Serialise VehicleInventory (OwnedVehicles, PlayerCurrency, PlayerPoints)
    // using UGameplayStatics::SaveGameToSlot with a USaveGame subclass.
    UE_LOG(LogTemp, Warning, TEXT("URacingGameInstance::SaveGame — not yet implemented"));
}

void URacingGameInstance::LoadGame()
{
    // TODO: Deserialise from save slot and restore VehicleInventory state.
    UE_LOG(LogTemp, Warning, TEXT("URacingGameInstance::LoadGame — not yet implemented"));
}
