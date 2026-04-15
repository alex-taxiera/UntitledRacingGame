// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "HubTypes.h"

class URacingGameInstance;
class UTextureRenderTarget2D;
class AHubVehicleDisplayActor;
class UVehicleDefinition;

/**
 * SHubRootWidget
 *
 * Top-level Slate widget for the hub level.  Owns and manages:
 *   - SDealershipPanel
 *   - SGaragePanel
 *   - SHubMenuOverlay (modal, shown on demand)
 *
 * Panel switching:
 *   Call ShowPanel(EHubPanel) to swap the visible panel.
 *   The menu overlay sits in an SOverlay slot above both panels at all times
 *   and is shown/hidden independently.
 *
 * New-game enforcement:
 *   If VehicleInventory has no vehicles, only the dealership is shown
 *   and panel switching (including the menu overlay) is locked until
 *   the player purchases their first car.  On purchase, the widget
 *   automatically transitions to the garage.
 */
class VEHICLEEXAMPLE_API SHubRootWidget : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SHubRootWidget)
        : _GameInstance(nullptr)
        , _DisplayActor(nullptr)
    {}
        SLATE_ARGUMENT(URacingGameInstance*,       GameInstance)
        SLATE_ARGUMENT(AHubVehicleDisplayActor*,   DisplayActor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /**
     * Switches to the given panel.
     * No-op for panels that are not yet implemented (Perks, Tuning, Collection).
     * Ignored while the new-game dealership lock is active.
     */
    void ShowPanel(EHubPanel Panel);

private:

    TWeakObjectPtr<URacingGameInstance>  GameInstance;
    AHubVehicleDisplayActor*             DisplayActor = nullptr;

    EHubPanel ActivePanel     = EHubPanel::Dealership;
    bool bMenuVisible          = false;
    bool bSystemMenuVisible    = false;

    /** True while the player has no vehicles (forces dealership). */
    bool bDealershipLocked = false;

    // -----------------------------------------------------------------------
    // Child widget refs (for swapping visibility)
    // -----------------------------------------------------------------------

    TSharedPtr<SWidget> DealershipSlot;
    TSharedPtr<SWidget> GarageSlot;
    TSharedPtr<SWidget> MenuOverlaySlot;
    TSharedPtr<SWidget> SystemOverlaySlot;

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    void OpenMenu();
    void CloseMenu();
    void OnMenuPanelSelected(EHubPanel Panel);

    void OpenSystemMenu();
    void CloseSystemMenu();

    /** Called by SDealershipPanel when a vehicle is purchased. */
    void OnVehiclePurchased(UVehicleDefinition* Definition);

    /** Asks the display actor to show the given definition. */
    void OnPreviewRequested(UVehicleDefinition* Definition);

    /** Called by SGaragePanel's Enter Race button. */
    void OnEnterRaceRequested();

    void ApplyPanelVisibility();
};
