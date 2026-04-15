// Copyright Epic Games, Inc. All Rights Reserved.

#include "SHubRootWidget.h"
#include "SDealershipPanel.h"
#include "SGaragePanel.h"
#include "SHubMenuOverlay.h"
#include "HubVehicleDisplayActor.h"
#include "RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleInventory.h"
#include "RacingVehicleSystem/VehicleDefinition.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Construct
// ---------------------------------------------------------------------------

void SHubRootWidget::Construct(const FArguments& InArgs)
{
    GameInstance  = InArgs._GameInstance;
    DisplayActor  = InArgs._DisplayActor;

    URacingGameInstance* GI = GameInstance.Get();
    bDealershipLocked = !GI || !GI->GetVehicleInventory()->HasAnyVehicle();
    ActivePanel = bDealershipLocked ? EHubPanel::Dealership : EHubPanel::Garage;

    UTextureRenderTarget2D* RT = DisplayActor ? DisplayActor->GetRenderTarget() : nullptr;

    // Pre-show the current vehicle if one exists
    if (GI && DisplayActor)
    {
        UOwnedVehicle* Current = GI->GetVehicleInventory()->GetCurrentVehicle();
        if (Current && Current->Definition)
        {
            DisplayActor->SetVehicle(Current->Definition);
        }
    }

    // Build child panels
    TSharedRef<SDealershipPanel> Dealership =
        SNew(SDealershipPanel)
        .GameInstance(GI)
        .RenderTarget(RT)
        .OnVehiclePurchased(this, &SHubRootWidget::OnVehiclePurchased)
        .OnPreviewRequested(this, &SHubRootWidget::OnPreviewRequested);

    TSharedRef<SGaragePanel> Garage =
        SNew(SGaragePanel)
        .GameInstance(GI)
        .RenderTarget(RT)
        .OnEnterRaceRequested(this, &SHubRootWidget::OnEnterRaceRequested)
        .OnMenuRequested(this, &SHubRootWidget::OpenMenu);

    TSharedRef<SHubMenuOverlay> MenuOverlay =
        SNew(SHubMenuOverlay)
        .CurrentPanel(ActivePanel)
        .OnPanelSelected(this, &SHubRootWidget::OnMenuPanelSelected)
        .OnClosed(this, &SHubRootWidget::CloseMenu);

    DealershipSlot  = Dealership;
    GarageSlot      = Garage;
    MenuOverlaySlot = MenuOverlay;

    ChildSlot
    [
        SNew(SOverlay)

        // Dealership panel
        + SOverlay::Slot()
        [
            Dealership
        ]

        // Garage panel
        + SOverlay::Slot()
        [
            Garage
        ]

        // Menu overlay (always in the tree, visibility-toggled)
        + SOverlay::Slot()
        [
            MenuOverlay
        ]
    ];

    ApplyPanelVisibility();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void SHubRootWidget::ShowPanel(EHubPanel Panel)
{
    // Ignore unimplemented panels
    if (Panel == EHubPanel::Perks ||
        Panel == EHubPanel::Tuning ||
        Panel == EHubPanel::Collection)
    {
        return;
    }

    // While locked, only the dealership is allowed
    if (bDealershipLocked && Panel != EHubPanel::Dealership)
    {
        return;
    }

    ActivePanel = Panel;
    ApplyPanelVisibility();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void SHubRootWidget::OpenMenu()
{
    if (bDealershipLocked) { return; }
    bMenuVisible = true;
    ApplyPanelVisibility();
}

void SHubRootWidget::CloseMenu()
{
    bMenuVisible = false;
    ApplyPanelVisibility();
}

void SHubRootWidget::OnMenuPanelSelected(EHubPanel Panel)
{
    CloseMenu();
    ShowPanel(Panel);
}

void SHubRootWidget::OnVehiclePurchased(UVehicleDefinition* /*Definition*/)
{
    // First purchase — lift the lock and move to garage
    bDealershipLocked = false;
    ShowPanel(EHubPanel::Garage);
}

void SHubRootWidget::OnPreviewRequested(UVehicleDefinition* Definition)
{
    if (DisplayActor)
    {
        DisplayActor->SetVehicle(Definition);
    }
}

void SHubRootWidget::OnEnterRaceRequested()
{
    URacingGameInstance* GI = GameInstance.Get();
    if (GI)
    {
        GI->SaveGame();
        UGameplayStatics::OpenLevel(GI, TEXT("RaceLevel"));
    }
}

void SHubRootWidget::ApplyPanelVisibility()
{
    if (DealershipSlot.IsValid())
    {
        DealershipSlot->SetVisibility(
            ActivePanel == EHubPanel::Dealership
            ? EVisibility::Visible
            : EVisibility::Collapsed);
    }

    if (GarageSlot.IsValid())
    {
        GarageSlot->SetVisibility(
            ActivePanel == EHubPanel::Garage
            ? EVisibility::Visible
            : EVisibility::Collapsed);
    }

    if (MenuOverlaySlot.IsValid())
    {
        MenuOverlaySlot->SetVisibility(
            bMenuVisible
            ? EVisibility::Visible
            : EVisibility::Collapsed);
    }
}
