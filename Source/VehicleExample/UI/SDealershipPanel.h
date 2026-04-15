// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class URacingGameInstance;
class UVehicleDefinition;
class UTextureRenderTarget2D;

DECLARE_DELEGATE_OneParam(FOnDealershipVehiclePurchased, UVehicleDefinition*);
DECLARE_DELEGATE_OneParam(FOnDealershipPreviewRequested, UVehicleDefinition*);
DECLARE_DELEGATE(FOnDealershipMenuRequested);
DECLARE_DELEGATE(FOnDealershipSystemMenuRequested);

/**
 * SDealershipPanel
 *
 * The dealership hub panel.  Displays available vehicles for purchase.
 *
 * Layout:
 *   ????????????????????????????????????????????????????
 *   ?  [Vehicle Card 1]  ?  [3D Render Target Image]   ?
 *   ?  [Vehicle Card 2]  ?  Name / Price / Stats        ?
 *   ?  [Vehicle Card 3]  ?  [BUY]                       ?
 *   ?  ...               ?                              ?
 *   ????????????????????????????????????????????????????
 *
 * - Vehicle cards on the left are drawn from
 *   URacingGameInstance::GetVehicleInventory()->AllVehicles.
 * - Selecting a card updates the right-hand preview via the shared
 *   render target (written to by AHubVehicleDisplayActor).
 * - The Buy button is disabled when the player cannot afford the car
 *   or already owns an instance of it.
 * - On successful purchase, OnVehiclePurchased is fired so the parent
 *   widget (SHubRootWidget) can react (e.g. auto-navigate to garage).
 */
class VEHICLEEXAMPLE_API SDealershipPanel : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SDealershipPanel)
        : _GameInstance(nullptr)
        , _RenderTarget(nullptr)
    {}
        SLATE_ARGUMENT(URacingGameInstance*,       GameInstance)
        SLATE_ARGUMENT(UTextureRenderTarget2D*,    RenderTarget)
        SLATE_EVENT(FOnDealershipVehiclePurchased,  OnVehiclePurchased)
        SLATE_EVENT(FOnDealershipPreviewRequested,   OnPreviewRequested)
        SLATE_EVENT(FOnDealershipMenuRequested,      OnMenuRequested)
        SLATE_EVENT(FOnDealershipSystemMenuRequested, OnSystemMenuRequested)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:

    TWeakObjectPtr<URacingGameInstance>    GameInstance;
    TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
    FOnDealershipVehiclePurchased           OnVehiclePurchased;
    FOnDealershipPreviewRequested           OnPreviewRequested;
    FOnDealershipMenuRequested              OnMenuRequested;
    FOnDealershipSystemMenuRequested        OnSystemMenuRequested;

    /** Currently highlighted definition in the list. */
    TWeakObjectPtr<UVehicleDefinition> SelectedDefinition;

    // -----------------------------------------------------------------------
    // Sub-widget construction
    // -----------------------------------------------------------------------

    TSharedRef<SWidget> MakeVehicleList();
    TSharedRef<SWidget> MakeVehicleCard(UVehicleDefinition* Def);
    TSharedRef<SWidget> MakePreviewPanel();

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    FReply OnVehicleCardClicked(UVehicleDefinition* Def);
    FReply OnBuyClicked();

    // -----------------------------------------------------------------------
    // Attribute bindings
    // -----------------------------------------------------------------------

    bool   IsBuyEnabled() const;
    FText  GetBuyButtonText() const;
    FText  GetSelectedName() const;
    FText  GetSelectedPrice() const;
    FText  GetSelectedDescription() const;
    FLinearColor GetCardColor(UVehicleDefinition* Def) const;

    // -----------------------------------------------------------------------
    // Render target brush
    // -----------------------------------------------------------------------

    /** Updated whenever the selection changes. */
    FSlateBrush PreviewBrush;
    void RefreshPreviewBrush();
};
