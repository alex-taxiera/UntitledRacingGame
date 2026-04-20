// Copyright Epic Games, Inc. All Rights Reserved.

#include "SHubSystemOverlay.h"
#include "RacingGameInstance.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

namespace SystemMenuLayout
{
    static constexpr float ButtonWidth  = 260.f;
    static constexpr float ButtonHeight = 52.f;
    static constexpr float ButtonSpacing = 10.f;
    static constexpr float FontSize      = 18.f;
}

void SHubSystemOverlay::Construct(const FArguments& InArgs)
{
    GameInstance = InArgs._GameInstance;
    OnClosed     = InArgs._OnClosed;

    auto MakeButton = [this](const FText& Label, bool bEnabled,
        FOnClicked::TMethodPtr<SHubSystemOverlay> Handler,
        FLinearColor TextColor = FLinearColor::White)
    {
        return SNew(SBox)
            .WidthOverride(SystemMenuLayout::ButtonWidth)
            .HeightOverride(SystemMenuLayout::ButtonHeight)
            .Padding(FMargin(0.f, SystemMenuLayout::ButtonSpacing * 0.5f))
        [
            SNew(SButton)
            .IsEnabled(bEnabled)
            .OnClicked(this, Handler)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle(
                    bEnabled ? "Bold" : "Regular",
                    (int32)SystemMenuLayout::FontSize))
                .ColorAndOpacity(bEnabled ? TextColor : FLinearColor(0.4f, 0.4f, 0.4f))
            ]
        ];
    };

    ChildSlot
    [
        SNew(SOverlay)

        // ?? Dim background ???????????????????????????????????????????????
        + SOverlay::Slot()
        [
            SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .OnClicked(this, &SHubSystemOverlay::OnDimBackgroundClicked)
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

                // Header
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
                        .Text(NSLOCTEXT("SystemMenu", "Title", "SYSTEM"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                        .ColorAndOpacity(FLinearColor::White)
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked(this, &SHubSystemOverlay::OnCloseClicked)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("?")))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                            .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
                        ]
                    ]
                ]

                // Save
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("SystemMenu", "Save", "SAVE"),
                        true, &SHubSystemOverlay::OnSaveClicked)
                ]

                // Settings (disabled)
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("SystemMenu", "Settings", "SETTINGS  (coming soon)"),
                        false, &SHubSystemOverlay::OnCloseClicked)
                ]

                // Return to Title
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("SystemMenu", "ReturnToTitle", "RETURN TO TITLE"),
                        true, &SHubSystemOverlay::OnReturnToTitleClicked,
                        FLinearColor(0.9f, 0.7f, 0.2f))
                ]

                // Exit Game
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("SystemMenu", "Exit", "EXIT GAME"),
                        true, &SHubSystemOverlay::OnExitGameClicked,
                        FLinearColor(0.9f, 0.3f, 0.3f))
                ]
            ]
        ]
    ];
}

FReply SHubSystemOverlay::OnSaveClicked()
{
    URacingGameInstance* GI = GameInstance.Get();
    if (GI) { GI->SaveGame(); }
    OnClosed.ExecuteIfBound();
    return FReply::Handled();
}

FReply SHubSystemOverlay::OnReturnToTitleClicked()
{
    URacingGameInstance* GI = GameInstance.Get();
    if (GI) { GI->ReturnToTitle(); }
    return FReply::Handled();
}

FReply SHubSystemOverlay::OnExitGameClicked()
{
    if (GEngine)
    {
        GEngine->DeferredCommands.Add(TEXT("quit"));
    }
    return FReply::Handled();
}

FReply SHubSystemOverlay::OnCloseClicked()
{
    OnClosed.ExecuteIfBound();
    return FReply::Handled();
}

FReply SHubSystemOverlay::OnDimBackgroundClicked()
{
    OnClosed.ExecuteIfBound();
    return FReply::Handled();
}
