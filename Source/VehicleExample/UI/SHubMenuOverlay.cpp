// Copyright Epic Games, Inc. All Rights Reserved.

#include "SHubMenuOverlay.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

namespace HubMenuLayout
{
    static constexpr float ButtonWidth  = 260.0f;
    static constexpr float ButtonHeight = 52.0f;
    static constexpr float ButtonSpacing = 10.0f;
    static constexpr float FontSize      = 18.0f;
}

void SHubMenuOverlay::Construct(const FArguments& InArgs)
{
    CurrentPanel    = InArgs._CurrentPanel;
    OnPanelSelected = InArgs._OnPanelSelected;
    OnClosed        = InArgs._OnClosed;

    // Helper: one nav button
    auto MakeNavButton = [this](const FText& Label, EHubPanel Panel, bool bEnabled)
    {
        return SNew(SBox)
            .WidthOverride(HubMenuLayout::ButtonWidth)
            .HeightOverride(HubMenuLayout::ButtonHeight)
            .Padding(FMargin(0.f, HubMenuLayout::ButtonSpacing * 0.5f))
        [
            SNew(SButton)
            .IsEnabled(bEnabled)
            .OnClicked(this, &SHubMenuOverlay::OnNavButtonClicked, Panel)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle(bEnabled ? "Bold" : "Regular",
                    (int32)HubMenuLayout::FontSize))
                .ColorAndOpacity(bEnabled
                    ? FLinearColor::White
                    : FLinearColor(0.4f, 0.4f, 0.4f))
            ]
        ];
    };

    ChildSlot
    [
        SNew(SOverlay)

        // ?? Dim background (clickable to close) ??????????????????????????
        + SOverlay::Slot()
        [
            SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .OnClicked(this, &SHubMenuOverlay::OnDimBackgroundClicked)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
                .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.72f))
            ]
        ]

        // ?? Menu card ????????????????????????????????????????????????????
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("Border"))
            .BorderBackgroundColor(FLinearColor(0.07f, 0.07f, 0.07f, 0.97f))
            .Padding(FMargin(48.f, 40.f))
            [
                SNew(SVerticalBox)

                // Header row: title + close button
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Fill)
                .Padding(FMargin(0.f, 0.f, 0.f, 28.f))
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("HubMenu", "Title", "MENU"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                        .ColorAndOpacity(FLinearColor::White)
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked(this, &SHubMenuOverlay::OnCloseClicked)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("?")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                            .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
                        ]
                    ]
                ]

                // Nav buttons
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeNavButton(
                        NSLOCTEXT("HubMenu", "Dealership", "DEALERSHIP"),
                        EHubPanel::Dealership, true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeNavButton(
                        NSLOCTEXT("HubMenu", "Garage", "GARAGE"),
                        EHubPanel::Garage, true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeNavButton(
                        NSLOCTEXT("HubMenu", "Perks", "PERKS  (coming soon)"),
                        EHubPanel::Perks, false)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeNavButton(
                        NSLOCTEXT("HubMenu", "Tuning", "TUNING  (coming soon)"),
                        EHubPanel::Tuning, false)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeNavButton(
                        NSLOCTEXT("HubMenu", "Collection", "COLLECTION  (coming soon)"),
                        EHubPanel::Collection, false)
                ]
            ]
        ]
    ];
}

FReply SHubMenuOverlay::OnNavButtonClicked(EHubPanel Panel)
{
    OnPanelSelected.ExecuteIfBound(Panel);
    return FReply::Handled();
}

FReply SHubMenuOverlay::OnCloseClicked()
{
    OnClosed.ExecuteIfBound();
    return FReply::Handled();
}

FReply SHubMenuOverlay::OnDimBackgroundClicked()
{
    OnClosed.ExecuteIfBound();
    return FReply::Handled();
}
