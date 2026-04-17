// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE(FOnRaceResultContinue);

/**
 * SRaceResultWidget
 *
 * Modal popup shown when a race battle ends (either racer reaches 0 HP).
 * Displays win/loss text, the battle duration, and a CONTINUE button.
 *
 * The CONTINUE button fires OnContinue so ACourseGameMode can clean up the
 * battle state and restore normal gameplay.
 *
 * Added/removed from the viewport by ACourseGameMode.
 */
class VEHICLEEXAMPLE_API SRaceResultWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SRaceResultWidget)
        : _bPlayerWon(false)
        , _ElapsedSeconds(0.f)
    {}
        /** True when the player won; false when the NPC won. */
        SLATE_ARGUMENT(bool, bPlayerWon)

        /** Total battle duration in seconds (used to display time taken). */
        SLATE_ARGUMENT(float, ElapsedSeconds)

        /** Fired when the player clicks CONTINUE. */
        SLATE_EVENT(FOnRaceResultContinue, OnContinue)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    FOnRaceResultContinue OnContinue;

    FReply HandleContinueClicked();
};
