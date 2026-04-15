// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TitleScreenGameMode.generated.h"

/**
 * ATitleScreenGameMode
 *
 * Game mode for the title screen level.
 * Creates and owns the STitleScreenWidget Slate widget, adds it to the
 * viewport on BeginPlay, and removes it on EndPlay.
 *
 * To use:
 *   1. Create (or configure) a level for the title screen.
 *   2. Set its World Settings ? Game Mode Override to ATitleScreenGameMode
 *      (or a Blueprint subclass of it).
 *   3. Set URacingGameInstance as the Game Instance Class in Project Settings.
 */
UCLASS()
class VEHICLEEXAMPLE_API ATitleScreenGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    ATitleScreenGameMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    /** The live Slate widget added to the viewport. */
    TSharedPtr<class STitleScreenWidget> TitleWidget;
};
