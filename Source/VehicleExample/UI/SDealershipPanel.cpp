// Copyright Epic Games, Inc. All Rights Reserved.

#include "SDealershipPanel.h"
#include "RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleDefinition.h"
#include "RacingVehicleSystem/VehicleInventory.h"
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
#include "Styling/SlateBrush.h"

namespace DealershipLayout
{
    static constexpr float CardWidth    = 240.f;
    static constexpr float CardHeight   = 72.f;
    static constexpr float FontSize     = 16.f;
    static constexpr float TitleSize    = 22.f;
}

// ---------------------------------------------------------------------------
// Construct
// ---------------------------------------------------------------------------

void SDealershipPanel::Construct(const FArguments& InArgs)
{
    GameInstance       = InArgs._GameInstance;
    RenderTarget       = InArgs._RenderTarget;
    OnVehiclePurchased = InArgs._OnVehiclePurchased;
    OnPreviewRequested = InArgs._OnPreviewRequested;
    OnMenuRequested    = InArgs._OnMenuRequested;

    RefreshPreviewBrush();

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
        .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f))
        [
            SNew(SVerticalBox)

            // ?? Main content row ???????????????????????????????????????
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SNew(SHorizontalBox)

                // ?? Left: vehicle list ?????????????????????????????????????
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                    .WidthOverride(DealershipLayout::CardWidth + 24.f)
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("Border"))
                        .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f))
                        .Padding(FMargin(12.f))
                        [
                            MakeVehicleList()
                        ]
                    ]
                ]

                // ?? Right: preview + details ???????????????????????????????
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                [
                    MakePreviewPanel()
                ]
            ]

            // ?? Bottom bar ?????????????????????????????????????????????
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("Border"))
                .BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f))
                .Padding(FMargin(24.f, 12.f))
                [
                    SNew(SHorizontalBox)

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
                                .Text(NSLOCTEXT("Dealership", "Menu", "MENU"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                                    (int32)DealershipLayout::FontSize))
                                .ColorAndOpacity(FLinearColor::White)
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// Sub-widget builders
// ---------------------------------------------------------------------------

TSharedRef<SWidget> SDealershipPanel::MakeVehicleList()
{
    TSharedRef<SScrollBox> List = SNew(SScrollBox);

    URacingGameInstance* GI = GameInstance.Get();
    if (!GI || !GI->GetVehicleInventory()) { return List; }

    for (UVehicleDefinition* Def : GI->GetVehicleInventory()->AllVehicles)
    {
        if (!Def) { continue; }
        List->AddSlot()
        .Padding(FMargin(0.f, 4.f))
        [
            MakeVehicleCard(Def)
        ];
    }

    return List;
}

TSharedRef<SWidget> SDealershipPanel::MakeVehicleCard(UVehicleDefinition* Def)
{
    return SNew(SBox)
        .WidthOverride(DealershipLayout::CardWidth)
        .HeightOverride(DealershipLayout::CardHeight)
    [
        SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .OnClicked(this, &SDealershipPanel::OnVehicleCardClicked, Def)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("Border"))
            .BorderBackgroundColor_Lambda([this, Def]() { return GetCardColor(Def); })
            .Padding(FMargin(12.f, 8.f))
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(Def->DisplayName)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                        (int32)DealershipLayout::FontSize))
                    .ColorAndOpacity(FLinearColor::White)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 2.f, 0.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text_Lambda([Def]()
                    {
                        return FText::Format(
                            NSLOCTEXT("Dealership", "Price", "¥ {0}"),
                            FText::AsNumber(Def->PurchasePrice));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                        (int32)DealershipLayout::FontSize - 2))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.f))
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SDealershipPanel::MakePreviewPanel()
{
    return SNew(SVerticalBox)

        // 3D render target image
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
            .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f))
            .Padding(FMargin(0.f))
            [
                SNew(SImage)
                .Image_Lambda([this]() -> const FSlateBrush*
                {
                    return &PreviewBrush;
                })
            ]
        ]

        // Vehicle name
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(24.f, 16.f, 24.f, 4.f))
        [
            SNew(STextBlock)
            .Text(this, &SDealershipPanel::GetSelectedName)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                (int32)DealershipLayout::TitleSize))
            .ColorAndOpacity(FLinearColor::White)
        ]

        // Price
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(24.f, 0.f, 24.f, 4.f))
        [
            SNew(STextBlock)
            .Text(this, &SDealershipPanel::GetSelectedPrice)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                (int32)DealershipLayout::FontSize))
            .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.f))
        ]

        // Description
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(24.f, 0.f, 24.f, 16.f))
        [
            SNew(STextBlock)
            .Text(this, &SDealershipPanel::GetSelectedDescription)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",
                (int32)DealershipLayout::FontSize - 2))
            .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
            .AutoWrapText(true)
        ]

        // Buy button
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Left)
        .Padding(FMargin(24.f, 0.f, 24.f, 24.f))
        [
            SNew(SBox)
            .WidthOverride(180.f)
            .HeightOverride(48.f)
            [
                SNew(SButton)
                .IsEnabled(this, &SDealershipPanel::IsBuyEnabled)
                .OnClicked(this, &SDealershipPanel::OnBuyClicked)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
                [
                    SNew(STextBlock)
                    .Text(this, &SDealershipPanel::GetBuyButtonText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",
                        (int32)DealershipLayout::FontSize))
                    .ColorAndOpacity(FLinearColor::White)
                ]
            ]
        ];
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

FReply SDealershipPanel::OnVehicleCardClicked(UVehicleDefinition* Def)
{
    SelectedDefinition = Def;
    OnPreviewRequested.ExecuteIfBound(Def);
    RefreshPreviewBrush();
    return FReply::Handled();
}

FReply SDealershipPanel::OnBuyClicked()
{
    URacingGameInstance* GI = GameInstance.Get();
    UVehicleDefinition*  Def = SelectedDefinition.Get();
    if (!GI || !Def) { return FReply::Handled(); }

    UOwnedVehicle* Purchased = GI->GetVehicleInventory()->PurchaseVehicle(Def);
    if (Purchased)
    {
        // Auto-select the purchased vehicle as current
        GI->GetVehicleInventory()->SetCurrentVehicle(Purchased);
        OnVehiclePurchased.ExecuteIfBound(Def);
    }

    return FReply::Handled();
}

// ---------------------------------------------------------------------------
// Attribute bindings
// ---------------------------------------------------------------------------

bool SDealershipPanel::IsBuyEnabled() const
{
    URacingGameInstance* GI  = GameInstance.Get();
    UVehicleDefinition*  Def = SelectedDefinition.Get();
    if (!GI || !Def) { return false; }

    UVehicleInventory* Inv = GI->GetVehicleInventory();
    return !Inv->OwnsVehicleModel(Def) && Inv->CanPurchaseVehicle(Def);
}

FText SDealershipPanel::GetBuyButtonText() const
{
    URacingGameInstance* GI  = GameInstance.Get();
    UVehicleDefinition*  Def = SelectedDefinition.Get();
    if (!GI || !Def)
    {
        return NSLOCTEXT("Dealership", "BuySelect", "SELECT A VEHICLE");
    }
    if (GI->GetVehicleInventory()->OwnsVehicleModel(Def))
    {
        return NSLOCTEXT("Dealership", "BuyOwned", "ALREADY OWNED");
    }
    if (!GI->GetVehicleInventory()->CanPurchaseVehicle(Def))
    {
        return NSLOCTEXT("Dealership", "BuyFunds", "INSUFFICIENT FUNDS");
    }
    return NSLOCTEXT("Dealership", "Buy", "BUY");
}

FText SDealershipPanel::GetSelectedName() const
{
    UVehicleDefinition* Def = SelectedDefinition.Get();
    return Def ? Def->DisplayName : NSLOCTEXT("Dealership", "NoSel", "Select a vehicle");
}

FText SDealershipPanel::GetSelectedPrice() const
{
    UVehicleDefinition* Def = SelectedDefinition.Get();
    if (!Def) { return FText::GetEmpty(); }
    return FText::Format(
        NSLOCTEXT("Dealership", "PriceFmt", "¥ {0}"),
        FText::AsNumber(Def->PurchasePrice));
}

FText SDealershipPanel::GetSelectedDescription() const
{
    UVehicleDefinition* Def = SelectedDefinition.Get();
    return Def ? Def->Description : FText::GetEmpty();
}

FLinearColor SDealershipPanel::GetCardColor(UVehicleDefinition* Def) const
{
    if (SelectedDefinition.Get() == Def)
    {
        return FLinearColor(0.15f, 0.15f, 0.35f, 1.f);
    }
    URacingGameInstance* GI = GameInstance.Get();
    if (GI && GI->GetVehicleInventory()->OwnsVehicleModel(Def))
    {
        return FLinearColor(0.1f, 0.18f, 0.1f, 1.f);
    }
    return FLinearColor(0.1f, 0.1f, 0.1f, 1.f);
}

void SDealershipPanel::RefreshPreviewBrush()
{
    UTextureRenderTarget2D* RT = RenderTarget.Get();
    if (RT)
    {
        PreviewBrush.SetResourceObject(RT);
        PreviewBrush.ImageSize = FVector2D(512.f, 512.f);
    }
    else
    {
        PreviewBrush = FSlateBrush();
    }
}
