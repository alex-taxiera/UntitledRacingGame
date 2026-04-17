// Copyright Epic Games, Inc. All Rights Reserved.

#include "SInputDebugWidget.h"
#include "VehicleExamplePawn.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Styling/CoreStyle.h"

void SInputDebugWidget::Construct(const FArguments& InArgs)
{
    PlayerPawn = InArgs._PlayerPawn;

    ChildSlot
    [
        SNew(SBox)
        .Padding(FMargin(16.f))
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Bottom)
        [
            SNew(SVerticalBox)

            // Throttle
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 4.f)
            [
                MakeBar(
                    FText::FromString(TEXT("THROTTLE")),
                    TAttribute<float>::CreateSP(this, &SInputDebugWidget::GetThrottle),
                    FLinearColor(0.1f, 0.8f, 0.1f))
            ]

            // Brake
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 4.f)
            [
                MakeBar(
                    FText::FromString(TEXT("BRAKE")),
                    TAttribute<float>::CreateSP(this, &SInputDebugWidget::GetBrake),
                    FLinearColor(0.9f, 0.15f, 0.1f))
            ]

            // Steering  (centred, shows left/right deflection)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 4.f)
            [
                MakeBar(
                    FText::FromString(TEXT("STEERING")),
                    TAttribute<float>::CreateSP(this, &SInputDebugWidget::GetSteering),
                    FLinearColor(0.3f, 0.6f, 1.0f),
                    /*bCentred=*/true)
            ]

            // Clutch placeholder
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.f, 4.f)
            [
                MakeBar(
                    FText::FromString(TEXT("CLUTCH")),
                    TAttribute<float>::Create([] { return 0.f; }),
                    FLinearColor(0.8f, 0.7f, 0.1f))
            ]
        ]
    ];
}

float SInputDebugWidget::GetThrottle() const
{
    return PlayerPawn.IsValid() ? PlayerPawn->GetThrottleInput() : 0.f;
}

float SInputDebugWidget::GetBrake() const
{
    return PlayerPawn.IsValid() ? PlayerPawn->GetBrakeInput() : 0.f;
}

float SInputDebugWidget::GetSteering() const
{
    // Remap -1..1 to 0..1 so the bar fills from centre
    return PlayerPawn.IsValid()
        ? (PlayerPawn->GetSteeringInput() * 0.5f + 0.5f)
        : 0.5f;
}

TSharedRef<SWidget> SInputDebugWidget::MakeBar(const FText&       Label,
                                                TAttribute<float>  Fill,
                                                FLinearColor       BarColour,
                                                bool               bCentred) const
{
    const float BarWidth  = 220.f;
    const float BarHeight = 22.f;
    const FLinearColor BackColour(0.05f, 0.05f, 0.05f, 0.75f);

    return SNew(SHorizontalBox)

        // Label
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.f, 0.f, 8.f, 0.f)
        [
            SNew(SBox).WidthOverride(80.f)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FLinearColor::White)
            ]
        ]

        // Bar background + fill overlay
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(BarWidth)
            .HeightOverride(BarHeight)
            [
                SNew(SOverlay)

                // Background
                + SOverlay::Slot()
                [
                    SNew(SColorBlock)
                    .Color(BackColour)
                ]

                // Fill (uses SBox width driven by Fill attribute)
                + SOverlay::Slot()
                .HAlign(bCentred ? HAlign_Center : HAlign_Left)
                [
                    SNew(SBox)
                    .WidthOverride_Lambda([Fill, BarWidth, bCentred]()
                    {
                        const float F = FMath::Clamp(Fill.Get(), 0.f, 1.f);
                        if (bCentred)
                        {
                            // Width = deviation from 0.5 * 2 * half-bar
                            return FOptionalSize(FMath::Abs(F - 0.5f) * 2.f * BarWidth);
                        }
                        return FOptionalSize(F * BarWidth);
                    })
                    .HAlign(HAlign_Fill)
                    [
                        SNew(SColorBlock)
                        .Color(BarColour)
                    ]
                ]
            ]
        ]

        // Numeric value
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(6.f, 0.f, 0.f, 0.f)
        [
            SNew(STextBlock)
            .Text_Lambda([Fill, bCentred]()
            {
                const float F = Fill.Get();
                const float Display = bCentred ? (F * 2.f - 1.f) : F;
                return FText::FromString(FString::Printf(TEXT("%.2f"), Display));
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            .ColorAndOpacity(FLinearColor::White)
        ];
}
