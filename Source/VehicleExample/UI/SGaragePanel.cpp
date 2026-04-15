// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGaragePanel.h"
#include "RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleInventory.h"
#include "RacingVehicleSystem/OwnedVehicle.h"
#include "RacingVehicleSystem/VehicleDefinition.h"
#include "RacingCharacterSystem/PlayerPerkManager.h"
#include "RacingCharacterSystem/CharacterPerkState.h"
#include "RacingCharacterSystem/PerkData.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

namespace GarageLayout
{
    static constexpr float FontSize   = 16.f;
    static constexpr float TitleSize  = 22.f;
    static constexpr float SlotHeight = 44.f;
}

// ---------------------------------------------------------------------------
// Construct
// ---------------------------------------------------------------------------

void SGaragePanel::Construct(const FArguments& InArgs)
{
    GameInstance          = InArgs._GameInstance;
    RenderTarget          = InArgs._RenderTarget;
    OnEnterRaceRequested  = InArgs._OnEnterRaceRequested;
    OnMenuRequested       = InArgs._OnMenuRequested;

    RefreshPreviewBrush();

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
        .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f))
        [
            SNew(SVerticalBox)

            // ?? Top: vehicle 3D preview + name ????????????????????????
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                MakeVehiclePreview()
            ]

            // ?? Middle: stats + skill slots ???????????????????????????
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .Padding(FMargin(24.f, 16.f))
                [
                    MakePlayerStats()
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .Padding(FMargin(24.f, 16.f))
                [
                    MakeSkillSlotGrid()
                ]
            ]

            // ?? Bottom bar ????????????????????????????????????????????
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeBottomBar()
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// Sub-widget builders
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SGaragePanel::MakeVehiclePreview()
{
    return SNew(SOverlay)

        + SOverlay::Slot()
        [
            SNew(SImage)
            .Image_Lambda([this]() -> const FSlateBrush*
            {
                return &PreviewBrush;
            })
        ]

        + SOverlay::Slot()
        .VAlign(VAlign_Bottom)
        .Padding(FMargin(24.f, 0.f, 24.f, 16.f))
        [
            SNew(STextBlock)
            .Text(this, &SGaragePanel::GetVehicleName)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                (int32)GarageLayout::TitleSize))
            .ColorAndOpacity(FLinearColor::White)
        ];
}

TSharedRef<SWidget> SGaragePanel::MakePlayerStats()
{
    return SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(0.f, 4.f))
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("Garage", "StatsHeader", "PLAYER"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                (int32)GarageLayout::FontSize))
            .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(0.f, 8.f, 0.f, 4.f))
        [
            SNew(STextBlock)
            .Text(this, &SGaragePanel::GetCurrencyText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                (int32)GarageLayout::FontSize))
            .ColorAndOpacity(FLinearColor(0.9f, 0.85f, 0.2f))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(0.f, 4.f))
        [
            SNew(STextBlock)
            .Text(this, &SGaragePanel::GetPointsText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                (int32)GarageLayout::FontSize))
            .ColorAndOpacity(FLinearColor(0.6f, 0.9f, 0.6f))
        ];
}

TSharedRef<SWidget> SGaragePanel::MakeSkillSlotGrid()
{
    TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

    Box->AddSlot()
    .AutoHeight()
    .Padding(FMargin(0.f, 0.f, 0.f, 8.f))
    [
        SNew(STextBlock)
        .Text_Lambda([this]()
        {
            return FText::Format(
                NSLOCTEXT("Garage", "SlotsHeader", "SKILL SLOTS  {0} / {1}"),
                FText::AsNumber(
                    GameInstance.IsValid() && GameInstance->GetPerkManager()->PerkState
                    ? GameInstance->GetPerkManager()->PerkState->EquippedSkillPerkIDs.Num()
                    : 0),
                FText::AsNumber(GetTotalSlotCount()));
        })
        .Font(FCoreStyle::GetDefaultFontStyle("Bold",
            (int32)GarageLayout::FontSize))
        .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
    ];

    const int32 TotalSlots = GetTotalSlotCount();
    for (int32 i = 0; i < TotalSlots; ++i)
    {
        Box->AddSlot()
        .AutoHeight()
        .Padding(FMargin(0.f, 4.f))
        [
            MakeSkillSlotRow(i)
        ];
    }

    return Box;
}

TSharedRef<SWidget> SGaragePanel::MakeSkillSlotRow(int32 SlotIndex)
{
    return SNew(SBox)
    .HeightOverride(GarageLayout::SlotHeight)
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("Border"))
        .BorderBackgroundColor_Lambda([this, SlotIndex]()
        {
            return IsSlotOccupied(SlotIndex)
                ? FLinearColor(0.1f, 0.15f, 0.25f)
                : FLinearColor(0.08f, 0.08f, 0.08f);
        })
        .Padding(FMargin(12.f, 4.f))
        [
            SNew(SHorizontalBox)

            // Slot label
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(this, &SGaragePanel::GetSlotLabel, SlotIndex)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                    (int32)GarageLayout::FontSize - 1))
                .ColorAndOpacity_Lambda([this, SlotIndex]()
                {
                    return IsSlotOccupied(SlotIndex)
                        ? FLinearColor::White
                        : FLinearColor(0.4f, 0.4f, 0.4f);
                })
            ]

            // Unequip button (visible only when occupied)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .Visibility_Lambda([this, SlotIndex]()
                {
                    return IsSlotOccupied(SlotIndex)
                        ? EVisibility::Visible
                        : EVisibility::Collapsed;
                })
                [
                    SNew(SButton)
                    .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                    .OnClicked(this, &SGaragePanel::OnUnequipSlot, SlotIndex)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("?")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                        .ColorAndOpacity(FLinearColor(0.7f, 0.3f, 0.3f))
                    ]
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SGaragePanel::MakeBottomBar()
{
    return SNew(SBorder)
    .BorderImage(FCoreStyle::Get().GetBrush("Border"))
    .BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f))
    .Padding(FMargin(24.f, 12.f))
    [
        SNew(SHorizontalBox)

        // Menu button
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(140.f)
            .HeightOverride(48.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() -> FReply
                {
                    OnMenuRequested.ExecuteIfBound();
                    return FReply::Handled();
                })
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                [
                    SNew(STextBlock)
                    .Text(NSLOCTEXT("Garage", "Menu", "MENU"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                        (int32)GarageLayout::FontSize))
                    .ColorAndOpacity(FLinearColor::White)
                ]
            ]
        ]

        + SHorizontalBox::Slot()
        .FillWidth(1.f)

        // Enter Race button
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(200.f)
            .HeightOverride(48.f)
            [
                SNew(SButton)
                .OnClicked_Lambda([this]() -> FReply
                {
                    OnEnterRaceRequested.ExecuteIfBound();
                    return FReply::Handled();
                })
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                [
                    SNew(STextBlock)
                    .Text(NSLOCTEXT("Garage", "EnterRace", "ENTER RACE"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                        (int32)GarageLayout::FontSize))
                    .ColorAndOpacity(FLinearColor(0.2f, 0.9f, 0.4f))
                ]
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// Slot callbacks
// ---------------------------------------------------------------------------

FReply SGaragePanel::OnUnequipSlot(int32 SlotIndex)
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetPerkManager()->PerkState) { return FReply::Handled(); }

    TArray<FName>& Equipped = GI->GetPerkManager()->PerkState->EquippedSkillPerkIDs;
    if (Equipped.IsValidIndex(SlotIndex))
    {
        Equipped.RemoveAt(SlotIndex);
        GI->GetPerkManager()->OnEquippedSkillsChanged.Broadcast();
    }
    return FReply::Handled();
}

FReply SGaragePanel::OnEquipPerk(FName PerkID, int32 SlotIndex)
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetPerkManager()->PerkState) { return FReply::Handled(); }

    TArray<FName>& Equipped = GI->GetPerkManager()->PerkState->EquippedSkillPerkIDs;

    // Expand to slot if needed
    while (Equipped.Num() <= SlotIndex)
    {
        Equipped.Add(NAME_None);
    }
    Equipped[SlotIndex] = PerkID;
    GI->GetPerkManager()->OnEquippedSkillsChanged.Broadcast();
    return FReply::Handled();
}

// ---------------------------------------------------------------------------
// Attribute bindings
// ---------------------------------------------------------------------------

FText SGaragePanel::GetVehicleName() const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI) { return FText::GetEmpty(); }
    UOwnedVehicle* Current = GI->GetVehicleInventory()->GetCurrentVehicle();
    if (!Current || !Current->Definition) { return FText::GetEmpty(); }
    return Current->Nickname.IsEmpty()
        ? Current->Definition->DisplayName
        : FText::FromString(Current->Nickname);
}

FText SGaragePanel::GetCurrencyText() const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI) { return FText::GetEmpty(); }
    return FText::Format(
        NSLOCTEXT("Garage", "Currency", "Currency:  ¥ {0}"),
        FText::AsNumber(GI->GetVehicleInventory()->PlayerCurrency));
}

FText SGaragePanel::GetPointsText() const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI) { return FText::GetEmpty(); }
    return FText::Format(
        NSLOCTEXT("Garage", "Points", "Skill Points:  {0}"),
        FText::AsNumber(GI->GetPerkManager()->SkillPointBank));
}

FText SGaragePanel::GetSlotLabel(int32 SlotIndex) const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetPerkManager()->PerkState) { return FText::GetEmpty(); }

    const TArray<FName>& Equipped = GI->GetPerkManager()->PerkState->EquippedSkillPerkIDs;
    if (!Equipped.IsValidIndex(SlotIndex) || Equipped[SlotIndex].IsNone())
    {
        return NSLOCTEXT("Garage", "EmptySlot", "— empty —");
    }

    const FName PerkID = Equipped[SlotIndex];
    for (UPerkData* Perk : GI->GetPerkManager()->AllPerks)
    {
        if (Perk && Perk->PerkID == PerkID)
        {
            return Perk->DisplayName;
        }
    }
    return FText::FromName(PerkID);
}

bool SGaragePanel::IsSlotOccupied(int32 SlotIndex) const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetPerkManager()->PerkState) { return false; }

    const TArray<FName>& Equipped = GI->GetPerkManager()->PerkState->EquippedSkillPerkIDs;
    return Equipped.IsValidIndex(SlotIndex) && !Equipped[SlotIndex].IsNone();
}

int32 SGaragePanel::GetTotalSlotCount() const
{
    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetPerkManager()->PerkState) { return 3; }

    return GI->GetPerkManager()->PerkState->GetSkillSlotCount(
        GI->GetPerkManager()->AllPerks,
        GI->GetPerkManager()->BaseSkillSlots);
}

void SGaragePanel::RefreshPreviewBrush()
{
    UTextureRenderTarget2D* RT = RenderTarget.Get();
    if (RT)
    {
        PreviewBrush.SetResourceObject(RT);
        PreviewBrush.ImageSize = FVector2D(1024.f, 512.f);
    }
    else
    {
        PreviewBrush = FSlateBrush();
    }
}
