// Copyright Epic Games, Inc. All Rights Reserved.

#include "SRaceHUDWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

void SRaceHUDWidget::Construct(const FArguments& InArgs)
{
    PlayerHealthFraction = InArgs._PlayerHealthFraction;
    NPCHealthFraction    = InArgs._NPCHealthFraction;
    TimerText            = InArgs._TimerText;

    ChildSlot
    [
        SNew(SOverlay)

        + SOverlay::Slot()
        .VAlign(VAlign_Top)
        .HAlign(HAlign_Fill)
        .Padding(FMargin(0.f, 12.f, 0.f, 0.f))
        [
            SNew(SHorizontalBox)

            // ---------------------------------------------------------------
            // Left side — Player
            // ---------------------------------------------------------------
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Top)
            .Padding(FMargin(20.f, 0.f, 0.f, 0.f))
            [
                MakeRacerPanel(InArgs._PlayerName,
                               PlayerHealthFraction,
                               FLinearColor(0.1f, 0.7f, 0.2f),
                               /*bRightAlign=*/false)
            ]

            // ---------------------------------------------------------------
            // Centre — Timer
            // ---------------------------------------------------------------
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Top)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("Border"))
                .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.72f))
                .Padding(FMargin(18.f, 6.f))
                [
                    SNew(STextBlock)
                    .Text(TimerText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                    .ColorAndOpacity(FLinearColor::White)
                    .Justification(ETextJustify::Center)
                ]
            ]

            // ---------------------------------------------------------------
            // Right side — NPC
            // ---------------------------------------------------------------
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Top)
            .Padding(FMargin(0.f, 0.f, 20.f, 0.f))
            [
                MakeRacerPanel(InArgs._NPCName,
                               NPCHealthFraction,
                               FLinearColor(0.85f, 0.15f, 0.1f),
                               /*bRightAlign=*/true)
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SRaceHUDWidget::MakeRacerPanel(const FText&        Name,
                                                    TAttribute<float>   HealthFrac,
                                                    const FLinearColor& BarColour,
                                                    bool                bRightAlign) const
{
    const ETextJustify::Type Justify = bRightAlign
        ? ETextJustify::Right
        : ETextJustify::Left;

    // Capture HealthFrac by value for use inside the lambdas below.
    TAttribute<float> CapturedFrac = HealthFrac;

    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("Border"))
        .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.72f))
        .Padding(FMargin(14.f, 8.f))
        [
            SNew(SBox)
            .WidthOverride(220.f)
            [
                SNew(SVerticalBox)

                // Name label
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 0.f, 0.f, 6.f))
                [
                    SNew(STextBlock)
                    .Text(Name)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                    .ColorAndOpacity(FLinearColor::White)
                    .Justification(Justify)
                ]

                // Health bar background
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .HeightOverride(18.f)
                    [
                        SNew(SOverlay)

                        // Dark trough
                        + SOverlay::Slot()
                        [
                            SNew(SColorBlock)
                            .Color(FLinearColor(0.08f, 0.08f, 0.08f, 1.f))
                        ]

                        // Coloured fill — width driven by health fraction
                        + SOverlay::Slot()
                        .HAlign(bRightAlign ? HAlign_Right : HAlign_Left)
                        [
                            SNew(SBox)
                            .WidthOverride_Lambda([CapturedFrac]() -> FOptionalSize
                            {
                                return FOptionalSize(FMath::Clamp(CapturedFrac.Get(), 0.f, 1.f) * 220.f);
                            })
                            [
                                SNew(SColorBlock)
                                .Color_Lambda([CapturedFrac, BarColour]() -> FLinearColor
                                {
                                    const float F = FMath::Clamp(CapturedFrac.Get(), 0.f, 1.f);
                                    // Interpolate from the base colour toward red as HP drops
                                    return FLinearColor::LerpUsingHSV(
                                        FLinearColor(0.85f, 0.1f, 0.1f),
                                        BarColour,
                                        F);
                                })
                            ]
                        ]
                    ]
                ]

                // HP fraction text (e.g. "75%")
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 3.f, 0.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text_Lambda([CapturedFrac]() -> FText
                    {
                        const int32 Pct = FMath::RoundToInt(
                            FMath::Clamp(CapturedFrac.Get(), 0.f, 1.f) * 100.f);
                        return FText::Format(
                            NSLOCTEXT("RaceHUD", "HPPct", "{0}%"),
                            FText::AsNumber(Pct));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
                    .ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f))
                    .Justification(Justify)
                ]
            ]
        ];
}
