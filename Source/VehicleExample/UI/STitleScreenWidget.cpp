// Copyright Epic Games, Inc. All Rights Reserved.

#include "STitleScreenWidget.h"
#include "RacingGameInstance.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------

namespace TitleScreenLayout
{
    static constexpr float ButtonWidth      = 280.0f;
    static constexpr float ButtonHeight     = 52.0f;
    static constexpr float ButtonSpacing    = 12.0f;
    static constexpr float TitleFontSize    = 48.0f;
    static constexpr float ButtonFontSize   = 20.0f;
    static constexpr float SmallFontSize    = 16.0f;
}

// ---------------------------------------------------------------------------
// Construct
// ---------------------------------------------------------------------------

void STitleScreenWidget::Construct(const FArguments& InArgs)
{
    GameInstance = InArgs._GameInstance;

    // Helper lambda: builds one standard menu button
    auto MakeButton = [this](const FText& Label, FOnClicked OnClicked, TAttribute<bool> bEnabled)
    {
        return SNew(SBox)
            .WidthOverride(TitleScreenLayout::ButtonWidth)
            .HeightOverride(TitleScreenLayout::ButtonHeight)
            .Padding(FMargin(0.0f, TitleScreenLayout::ButtonSpacing * 0.5f))
        [
            SNew(SButton)
            .ButtonStyle(&GetMenuButtonStyle())
            .OnClicked(OnClicked)
            .IsEnabled(bEnabled)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                    (int32)TitleScreenLayout::ButtonFontSize))
                .ColorAndOpacity(FLinearColor::White)
            ]
        ];
    };

    ChildSlot
    [
        // Full-screen black background
        SNew(SOverlay)

        // ?? Background fill ??????????????????????????????????????????????
        + SOverlay::Slot()
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
            .BorderBackgroundColor(FLinearColor::Black)
        ]

        // ?? Main layout ??????????????????????????????????????????????????
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .Visibility(TAttribute<EVisibility>(this, &STitleScreenWidget::GetMainButtonsVisibility))
            [
                SNew(SVerticalBox)

                // Title
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 32.0f))
                [
                    SNew(STextBlock)
                    .Text(NSLOCTEXT("TitleScreen", "Title", "UNTITLED RACING GAME"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                        (int32)TitleScreenLayout::TitleFontSize))
                    .ColorAndOpacity(FLinearColor::White)
                ]

                // Profile selector
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("TitleScreen", "ProfileLabel", "PROFILE"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                            (int32)TitleScreenLayout::SmallFontSize))
                        .ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
                    ]

                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4.f, 0.f))
                        [
                            SNew(SBox).WidthOverride(84.f).HeightOverride(36.f)
                            [
                                SNew(SButton)
                                .ButtonStyle(&GetMenuButtonStyle())
                                .OnClicked(this, &STitleScreenWidget::OnSlotClicked, FString(TEXT("RacingSave")))
                                .HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("TitleScreen", "Slot1", "SLOT 1"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                                    .ColorAndOpacity(TAttribute<FSlateColor>(this,
                                        &STitleScreenWidget::GetSlot1Color))
                                ]
                            ]
                        ]

                        + SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4.f, 0.f))
                        [
                            SNew(SBox).WidthOverride(84.f).HeightOverride(36.f)
                            [
                                SNew(SButton)
                                .ButtonStyle(&GetMenuButtonStyle())
                                .OnClicked(this, &STitleScreenWidget::OnSlotClicked, FString(TEXT("RacingSave2")))
                                .HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("TitleScreen", "Slot2", "SLOT 2"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                                    .ColorAndOpacity(TAttribute<FSlateColor>(this,
                                        &STitleScreenWidget::GetSlot2Color))
                                ]
                            ]
                        ]

                        + SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4.f, 0.f))
                        [
                            SNew(SBox).WidthOverride(84.f).HeightOverride(36.f)
                            [
                                SNew(SButton)
                                .ButtonStyle(&GetMenuButtonStyle())
                                .OnClicked(this, &STitleScreenWidget::OnSlotClicked, FString(TEXT("RacingSave3")))
                                .HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("TitleScreen", "Slot3", "SLOT 3"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                                    .ColorAndOpacity(TAttribute<FSlateColor>(this,
                                        &STitleScreenWidget::GetSlot3Color))
                                ]
                            ]
                        ]
                    ]
                ]

                // New Game
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("TitleScreen", "NewGame", "NEW GAME"),
                        FOnClicked::CreateSP(this, &STitleScreenWidget::OnNewGameClicked),
                        true)
                ]

                // Continue
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("TitleScreen", "Continue", "CONTINUE"),
                        FOnClicked::CreateSP(this, &STitleScreenWidget::OnContinueClicked),
                        TAttribute<bool>(this, &STitleScreenWidget::IsContinueEnabled))
                ]

                // Delete Save  (hidden when no save)
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(SBox)
                    .Visibility(TAttribute<EVisibility>(this,
                        &STitleScreenWidget::GetDeleteSaveVisibility))
                    [
                        MakeButton(
                            NSLOCTEXT("TitleScreen", "DeleteSave", "DELETE SAVE"),
                            FOnClicked::CreateSP(this, &STitleScreenWidget::OnDeleteSaveClicked),
                            true)
                    ]
                ]

                // Exit
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    MakeButton(
                        NSLOCTEXT("TitleScreen", "Exit", "EXIT"),
                        FOnClicked::CreateSP(this, &STitleScreenWidget::OnExitClicked),
                        true)
                ]
            ]
        ]

        // ?? Delete confirmation overlay ???????????????????????????????????
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .Visibility(TAttribute<EVisibility>(this,
                &STitleScreenWidget::GetConfirmOverlayVisibility))
            .WidthOverride(400.0f)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("Border"))
                .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.97f))
                .Padding(FMargin(32.0f))
                [
                    SNew(SVerticalBox)

                    // Warning headline
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("TitleScreen", "ConfirmTitle", "DELETE SAVE DATA?"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                            (int32)TitleScreenLayout::ButtonFontSize))
                        .ColorAndOpacity(FLinearColor(1.0f, 0.35f, 0.35f))
                    ]

                    // Warning sub-text
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(0.0f, 0.0f, 0.0f, 28.0f))
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("TitleScreen", "ConfirmBody",
                            "All progress will be permanently lost.\nThis cannot be undone."))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                            (int32)TitleScreenLayout::SmallFontSize))
                        .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                        .Justification(ETextJustify::Center)
                    ]

                    // Confirm / Cancel row
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
                        [
                            SNew(SBox)
                            .WidthOverride(128.0f)
                            .HeightOverride(44.0f)
                            [
                                SNew(SButton)
                                .ButtonStyle(&GetMenuButtonStyle())
                                .OnClicked(this, &STitleScreenWidget::OnConfirmDeleteClicked)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("TitleScreen", "ConfirmYes", "CONFIRM"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                                        (int32)TitleScreenLayout::SmallFontSize))
                                    .ColorAndOpacity(FLinearColor(1.0f, 0.35f, 0.35f))
                                ]
                            ]
                        ]

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(SBox)
                            .WidthOverride(128.0f)
                            .HeightOverride(44.0f)
                            [
                                SNew(SButton)
                                .ButtonStyle(&GetMenuButtonStyle())
                                .OnClicked(this, &STitleScreenWidget::OnCancelDeleteClicked)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("TitleScreen", "ConfirmNo", "CANCEL"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                                        (int32)TitleScreenLayout::SmallFontSize))
                                    .ColorAndOpacity(FLinearColor::White)
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// Button callbacks
// ---------------------------------------------------------------------------

FReply STitleScreenWidget::OnNewGameClicked()
{
    if (URacingGameInstance* GI = GameInstance.Get())
    {
        GI->StartNewGame();
    }
    return FReply::Handled();
}

FReply STitleScreenWidget::OnContinueClicked()
{
    if (URacingGameInstance* GI = GameInstance.Get())
    {
        GI->ContinueGame();
    }
    return FReply::Handled();
}

FReply STitleScreenWidget::OnDeleteSaveClicked()
{
    bConfirmDeleteVisible = true;
    return FReply::Handled();
}

FReply STitleScreenWidget::OnConfirmDeleteClicked()
{
    bConfirmDeleteVisible = false;
    if (URacingGameInstance* GI = GameInstance.Get())
    {
        GI->DeleteSave();
    }
    return FReply::Handled();
}

FReply STitleScreenWidget::OnCancelDeleteClicked()
{
    bConfirmDeleteVisible = false;
    return FReply::Handled();
}

FReply STitleScreenWidget::OnExitClicked()
{
    if (GEngine)
    {
        GEngine->DeferredCommands.Add(TEXT("quit"));
    }
    return FReply::Handled();
}

FReply STitleScreenWidget::OnSlotClicked(FString SlotName)
{
    if (URacingGameInstance* GI = GameInstance.Get())
    {
        GI->SetActiveSaveSlot(SlotName);
    }
    return FReply::Handled();
}

// ---------------------------------------------------------------------------
// Attribute bindings
// ---------------------------------------------------------------------------

bool STitleScreenWidget::IsContinueEnabled() const
{
    const URacingGameInstance* GI = GameInstance.Get();
    return GI && GI->HasSaveGame();
}

EVisibility STitleScreenWidget::GetDeleteSaveVisibility() const
{
    const URacingGameInstance* GI = GameInstance.Get();
    return (GI && GI->HasSaveGame()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility STitleScreenWidget::GetConfirmOverlayVisibility() const
{
    return bConfirmDeleteVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility STitleScreenWidget::GetMainButtonsVisibility() const
{
    return bConfirmDeleteVisible ? EVisibility::Hidden : EVisibility::Visible;
}

// ---------------------------------------------------------------------------
// Style helpers
// ---------------------------------------------------------------------------

FSlateColor STitleScreenWidget::GetSlotButtonColor(const FString& SlotName) const
{
    const URacingGameInstance* GI = GameInstance.Get();
    if (GI && GI->GetActiveSaveSlot() == SlotName)
    {
        return FSlateColor(FLinearColor(0.2f, 0.7f, 1.0f)); // active: bright blue
    }
    return FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f));     // inactive: grey
}

const FButtonStyle& STitleScreenWidget::GetMenuButtonStyle()
{
    return FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
}
