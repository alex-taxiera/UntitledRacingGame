// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class URacingGameInstance;

DECLARE_DELEGATE(FOnSystemMenuClosed);

/**
 * SHubSystemOverlay
 *
 * Full-screen modal system menu accessible from every hub panel.
 * Sits in the SHubRootWidget overlay stack above both SHubMenuOverlay
 * and the panel content.
 *
 * Buttons:
 *   Save              — calls URacingGameInstance::SaveGame()
 *   Settings          — disabled / coming soon
 *   Return to Title   — calls URacingGameInstance::ReturnToTitle()
 *   Exit Game         — calls FPlatformMisc::RequestExit()
 *
 * Dismissed by clicking the X button or the dim background.
 */
class VEHICLEEXAMPLE_API SHubSystemOverlay : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SHubSystemOverlay)
        : _GameInstance(nullptr)
    {}
        SLATE_ARGUMENT(URacingGameInstance*, GameInstance)
        SLATE_EVENT(FOnSystemMenuClosed, OnClosed)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    TWeakObjectPtr<URacingGameInstance> GameInstance;
    FOnSystemMenuClosed OnClosed;

    FReply OnSaveClicked();
    FReply OnReturnToTitleClicked();
    FReply OnExitGameClicked();
    FReply OnCloseClicked();
    FReply OnDimBackgroundClicked();
};
