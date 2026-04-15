// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString URacingSaveGame::SlotName = TEXT("RacingSave");

bool URacingSaveGame::DoesSaveExist()
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}
