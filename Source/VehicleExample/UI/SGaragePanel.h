// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class URacingGameInstance;
class UTextureRenderTarget2D;
class UPerkData;

DECLARE_DELEGATE(FOnEnterRaceRequested);
DECLARE_DELEGATE(FOnMenuRequested);
DECLARE_DELEGATE(FOnSystemMenuRequested);

/**
 * SGaragePanel
 *
 * The garage hub panel.  Home screen between races.
 *
 * Layout:
 *   ????????????????????????????????????????????????????
 *   ?  [3D Render Target — current vehicle]            ?
 *   ?  Vehicle Name                                    ?
 *   ????????????????????????????????????????????????????
 *   ?  Currency: ¥ xxx     ?  Skill Slots              ?
 *   ?  Points:   xxx       ?  [Slot 1: PerkName] [X]   ?
 *   ?                      ?  [Slot 2: PerkName] [X]   ?
 *   ?                      ?  [Slot 3: — empty —  ]    ?
 *   ????????????????????????????????????????????????????
 *   ?  [MENU]                          [ENTER RACE]    ?
 *   ????????????????????????????????????????????????????
 */
class VEHICLEEXAMPLE_API SGaragePanel : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SGaragePanel)
        : _GameInstance(nullptr)
        , _RenderTarget(nullptr)
    {}
        SLATE_ARGUMENT(URacingGameInstance*,    GameInstance)
        SLATE_ARGUMENT(UTextureRenderTarget2D*, RenderTarget)
        SLATE_EVENT(FOnEnterRaceRequested, OnEnterRaceRequested)
        SLATE_EVENT(FOnMenuRequested,       OnMenuRequested)
        SLATE_EVENT(FOnSystemMenuRequested, OnSystemMenuRequested)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Re-binds the preview brush to a newly available render target. */
    void SetRenderTarget(UTextureRenderTarget2D* RT);

private:

    TWeakObjectPtr<URacingGameInstance>    GameInstance;
    TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
    FOnEnterRaceRequested OnEnterRaceRequested;
    FOnMenuRequested      OnMenuRequested;
    FOnSystemMenuRequested OnSystemMenuRequested;

    FSlateBrush PreviewBrush;
    void RefreshPreviewBrush();

    // -----------------------------------------------------------------------
    // Sub-widget builders
    // -----------------------------------------------------------------------

    TSharedRef<SWidget> MakeVehiclePreview();
    TSharedRef<SWidget> MakePlayerStats();
    TSharedRef<SWidget> MakeSkillSlotGrid();
    TSharedRef<SWidget> MakeSkillSlotRow(int32 SlotIndex);
    TSharedRef<SWidget> MakeBottomBar();

    // -----------------------------------------------------------------------
    // Slot callbacks
    // -----------------------------------------------------------------------

    FReply OnUnequipSlot(int32 SlotIndex);
    FReply OnEquipPerk(FName PerkID, int32 SlotIndex);

    // -----------------------------------------------------------------------
    // Attribute bindings
    // -----------------------------------------------------------------------

    FText GetVehicleName() const;
    FText GetCurrencyText() const;
    FText GetPointsText() const;
    FText GetSlotLabel(int32 SlotIndex) const;
    bool  IsSlotOccupied(int32 SlotIndex) const;
    int32 GetTotalSlotCount() const;
};
