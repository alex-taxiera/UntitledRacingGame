// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "HubTypes.h"

DECLARE_DELEGATE_OneParam(FOnHubPanelSelected, EHubPanel);
DECLARE_DELEGATE(FOnHubMenuClosed);

/**
 * SHubMenuOverlay
 *
 * Full-screen modal navigation menu for the hub level.
 * Rendered on top of all panels with a dark semi-transparent background.
 *
 * Contains five navigation buttons:
 *   Dealership  — always enabled
 *   Garage      — always enabled
 *   Perks       — disabled (not yet implemented)
 *   Tuning      — disabled (not yet implemented)
 *   Collection  — disabled (not yet implemented)
 *
 * Dismissed by clicking the "X" close button or clicking the dim background.
 * Fires OnPanelSelected when an enabled button is pressed (and auto-closes).
 */
class VEHICLEEXAMPLE_API SHubMenuOverlay : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SHubMenuOverlay)
        : _CurrentPanel(EHubPanel::Garage)
    {}
        SLATE_ARGUMENT(EHubPanel, CurrentPanel)
        SLATE_EVENT(FOnHubPanelSelected, OnPanelSelected)
        SLATE_EVENT(FOnHubMenuClosed,    OnClosed)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    EHubPanel          CurrentPanel;
    FOnHubPanelSelected OnPanelSelected;
    FOnHubMenuClosed    OnClosed;

    FReply OnNavButtonClicked(EHubPanel Panel);
    FReply OnCloseClicked();
    FReply OnDimBackgroundClicked();
};
