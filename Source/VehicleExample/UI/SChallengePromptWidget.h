// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UNPCRacerData;
class ANPCPatrolActor;

DECLARE_DELEGATE_OneParam(FOnChallengeResponse, bool /* bAccepted */);

/**
 * SChallengePromptWidget
 *
 * A Slate overlay shown when the player drives into an NPC's challenge radius.
 * Displays the NPC's portrait (if available), name, and two buttons:
 *   RACE   � accepts the challenge, fires OnResponse(true)
 *   IGNORE � declines,             fires OnResponse(false)
 *
 * Added/removed from the viewport by ACourseGameMode.
 * The game mode binds OnResponse and reacts accordingly.
 */
class VEHICLEEXAMPLE_API SChallengePromptWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SChallengePromptWidget)
        : _RacerData(nullptr)
    {}
        /** The NPC data asset � used for name and portrait. */
        SLATE_ARGUMENT(UNPCRacerData*, RacerData)
        /**
         * Optional override for the challenger name shown in the prompt.
         * When set (non-empty) this takes precedence over RacerData->RacerName.
         * Use this for player-vs-player challenges where there is no RacerData.
         */
        SLATE_ARGUMENT(FText, ChallengerName)
        /** Fired with true (race) or false (ignore) when the player clicks. */
        SLATE_EVENT(FOnChallengeResponse, OnResponse)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    TWeakObjectPtr<UNPCRacerData> RacerData;
    FText                         OverrideName;
    FOnChallengeResponse OnResponse;

    FSlateBrush PortraitBrush;

    FReply OnAccept();
    FReply OnDecline();
};
