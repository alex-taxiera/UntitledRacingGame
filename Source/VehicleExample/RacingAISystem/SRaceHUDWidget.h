// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SRaceHUDWidget
 *
 * Heads-up display shown during a race battle.
 * Displays both racers' names and live health bars at the top of the screen,
 * with a running battle timer in the centre.
 *
 * Health fractions and the timer text are fed as TAttributes so the widget
 * re-reads them every frame without any manual update calls.
 *
 * Added/removed from the viewport by ACourseGameMode.
 */
class VEHICLEEXAMPLE_API SRaceHUDWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SRaceHUDWidget)
        : _PlayerName(NSLOCTEXT("RaceHUD", "DefaultPlayer", "PLAYER"))
        , _NPCName(NSLOCTEXT("RaceHUD", "DefaultNPC", "OPPONENT"))
        , _PlayerHealthFraction(1.f)
        , _NPCHealthFraction(1.f)
        , _TimerText(NSLOCTEXT("RaceHUD", "DefaultTimer", "0:00"))
    {}
        /** Display name shown on the player's side (top-left). */
        SLATE_ARGUMENT(FText, PlayerName)

        /** Display name shown on the NPC's side (top-right). */
        SLATE_ARGUMENT(FText, NPCName)

        /** Player health as a fraction in [0,1]. Updated each frame via attribute. */
        SLATE_ATTRIBUTE(float, PlayerHealthFraction)

        /** NPC health as a fraction in [0,1]. Updated each frame via attribute. */
        SLATE_ATTRIBUTE(float, NPCHealthFraction)

        /** Formatted timer string (e.g. "1:23"). Updated each frame via attribute. */
        SLATE_ATTRIBUTE(FText, TimerText)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    TAttribute<float> PlayerHealthFraction;
    TAttribute<float> NPCHealthFraction;
    TAttribute<FText> TimerText;

    /**
     * Builds one racer panel: name label above a coloured health bar.
     * @param Name        Racer display name.
     * @param HealthFrac  Live fraction attribute [0,1].
     * @param BarColour   Fill colour for full health.
     * @param bRightAlign If true the name is right-aligned (NPC side).
     */
    TSharedRef<SWidget> MakeRacerPanel(const FText&          Name,
                                       TAttribute<float>     HealthFrac,
                                       const FLinearColor&   BarColour,
                                       bool                  bRightAlign) const;
};
