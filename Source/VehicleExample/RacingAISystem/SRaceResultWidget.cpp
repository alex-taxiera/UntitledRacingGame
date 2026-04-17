// Copyright Epic Games, Inc. All Rights Reserved.

#include "SRaceResultWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

void SRaceResultWidget::Construct(const FArguments& InArgs)
{
    OnContinue = InArgs._OnContinue;

    const bool  bWon           = InArgs._bPlayerWon;
    const float ElapsedSeconds = InArgs._ElapsedSeconds;

    // Format elapsed time as M:SS
    const int32 TotalSecs  = FMath::FloorToInt(ElapsedSeconds);
    const int32 Minutes    = TotalSecs / 60;
    const int32 Secs       = TotalSecs % 60;
    const FText TimeText   = FText::Format(
        NSLOCTEXT("RaceResult", "TimeFmt", "Battle time: {0}:{1}"),
        FText::AsNumber(Minutes),
        FText::Format(NSLOCTEXT("RaceResult", "SecFmt", "{0}"),
            FText::AsNumber(Secs < 10 ? Secs : Secs)));  // zero-pad handled below

    // Rebuild as proper zero-padded string
    const FString TimeString = FString::Printf(TEXT("Battle time: %d:%02d"), Minutes, Secs);

    const FText ResultHeading = bWon
        ? NSLOCTEXT("RaceResult", "Win",  "YOU WIN!")
        : NSLOCTEXT("RaceResult", "Lose", "YOU LOSE!");

    const FLinearColor HeadingColour = bWon
        ? FLinearColor(0.1f, 0.9f, 0.2f)
        : FLinearColor(0.9f, 0.15f, 0.1f);

    ChildSlot
    [
        SNew(SOverlay)

        // Full-screen dim
        + SOverlay::Slot()
        [
            SNew(SColorBlock)
            .Color(FLinearColor(0.f, 0.f, 0.f, 0.55f))
        ]

        // Centred card
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("Border"))
            .BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.95f))
            .Padding(FMargin(60.f, 40.f))
            [
                SNew(SVerticalBox)

                // Result heading ("YOU WIN!" / "YOU LOSE!")
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.f, 0.f, 0.f, 16.f))
                [
                    SNew(STextBlock)
                    .Text(ResultHeading)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 36))
                    .ColorAndOpacity(HeadingColour)
                    .Justification(ETextJustify::Center)
                ]

                // Battle duration
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(0.f, 0.f, 0.f, 32.f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TimeString))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                    .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
                    .Justification(ETextJustify::Center)
                ]

                // CONTINUE button
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(SButton)
                    .OnClicked(this, &SRaceResultWidget::HandleContinueClicked)
                    .ContentPadding(FMargin(40.f, 12.f))
                    [
                        SNew(STextBlock)
                        .Text(NSLOCTEXT("RaceResult", "Continue", "CONTINUE"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        .ColorAndOpacity(FLinearColor::White)
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
        ]
    ];
}

FReply SRaceResultWidget::HandleContinueClicked()
{
    OnContinue.ExecuteIfBound();
    return FReply::Handled();
}
