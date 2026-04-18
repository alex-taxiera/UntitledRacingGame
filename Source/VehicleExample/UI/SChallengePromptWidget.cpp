// Copyright Epic Games, Inc. All Rights Reserved.

#include "SChallengePromptWidget.h"
#include "NPCRacerData.h"
#include "Engine/Texture2D.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

void SChallengePromptWidget::Construct(const FArguments& InArgs)
{
    RacerData  = InArgs._RacerData;
    OnResponse = InArgs._OnResponse;
    OverrideName = InArgs._ChallengerName;

    // Build portrait brush if a texture is available
    UNPCRacerData* Data = RacerData.Get();
    if (Data && Data->Portrait.IsValid())
    {
        UTexture2D* Tex = Data->Portrait.LoadSynchronous();
        if (Tex)
        {
            PortraitBrush.SetResourceObject(Tex);
            PortraitBrush.ImageSize  = FVector2D(128.f, 128.f);
            PortraitBrush.DrawAs     = ESlateBrushDrawType::Image;
            PortraitBrush.Tiling     = ESlateBrushTileType::NoTile;
            PortraitBrush.ImageType  = ESlateBrushImageType::FullColor;
        }
    }

    FText NPCName;
    if (!OverrideName.IsEmpty())
    {
        NPCName = OverrideName;
    }
    else if (Data)
    {
        NPCName = Data->RacerName;
    }
    else
    {
        NPCName = NSLOCTEXT("Challenge", "UnknownRacer", "Unknown Racer");
    }

    ChildSlot
    [
        SNew(SOverlay)

        // Dim overlay � does not block input so the player can still drive
        + SOverlay::Slot()
        .VAlign(VAlign_Bottom)
        .HAlign(HAlign_Center)
        .Padding(FMargin(0.f, 0.f, 0.f, 80.f))
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("Border"))
            .BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.88f))
            .Padding(FMargin(32.f, 24.f))
            [
                SNew(SHorizontalBox)

                // Portrait (optional)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0.f, 0.f, 24.f, 0.f))
                [
                    SNew(SBox)
                    .WidthOverride(128.f)
                    .HeightOverride(128.f)
                    [
                        SNew(SImage)
                        .Image(&PortraitBrush)
                        .Visibility(PortraitBrush.GetResourceObject()
                            ? EVisibility::Visible
                            : EVisibility::Collapsed)
                    ]
                ]

                // Text + buttons
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)

                    // Challenge prompt text
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.f, 0.f, 0.f, 8.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::Format(
                            NSLOCTEXT("Challenge", "Prompt", "{0} wants to race!"),
                            NPCName))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                        .ColorAndOpacity(FLinearColor::White)
                    ]

                    // NPC description (optional)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(FMargin(0.f, 0.f, 0.f, 20.f))
                    [
                        SNew(STextBlock)
                        .Text(Data ? Data->Description : FText::GetEmpty())
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
                        .ColorAndOpacity(FLinearColor(0.65f, 0.65f, 0.65f))
                        .AutoWrapText(true)
                        .WrapTextAt(400.f)
                        .Visibility(Data && !Data->Description.IsEmpty()
                            ? EVisibility::Visible
                            : EVisibility::Collapsed)
                    ]

                    // Buttons
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(FMargin(0.f, 0.f, 12.f, 0.f))
                        [
                            SNew(SBox)
                            .WidthOverride(130.f)
                            .HeightOverride(48.f)
                            [
                                SNew(SButton)
                                .OnClicked(this, &SChallengePromptWidget::OnAccept)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                .ButtonStyle(&FCoreStyle::Get()
                                    .GetWidgetStyle<FButtonStyle>("Button"))
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("Challenge", "Race", "RACE"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                                    .ColorAndOpacity(FLinearColor(0.2f, 0.9f, 0.3f))
                                ]
                            ]
                        ]

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SBox)
                            .WidthOverride(130.f)
                            .HeightOverride(48.f)
                            [
                                SNew(SButton)
                                .OnClicked(this, &SChallengePromptWidget::OnDecline)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                .ButtonStyle(&FCoreStyle::Get()
                                    .GetWidgetStyle<FButtonStyle>("Button"))
                                [
                                    SNew(STextBlock)
                                    .Text(NSLOCTEXT("Challenge", "Ignore", "IGNORE"))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                                    .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
}

FReply SChallengePromptWidget::OnAccept()
{
    OnResponse.ExecuteIfBound(true);
    return FReply::Handled();
}

FReply SChallengePromptWidget::OnDecline()
{
    OnResponse.ExecuteIfBound(false);
    return FReply::Handled();
}
