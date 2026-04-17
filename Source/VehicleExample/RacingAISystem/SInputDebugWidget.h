// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AVehicleExamplePawn;

/**
 * SInputDebugWidget
 *
 * Overlay widget showing live throttle, brake and steering inputs as
 * filled bars.  Add to the viewport from CourseGameMode; remove on EndPlay.
 */
class SInputDebugWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SInputDebugWidget) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AVehicleExamplePawn>, PlayerPawn)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<AVehicleExamplePawn> PlayerPawn;

    // Bar fill fractions (0-1) read each frame
    float GetThrottle()  const;
    float GetBrake()     const;
    float GetSteering()  const; // -1 to 1, displayed as 0-1 offset from centre

    // Slate geometry helpers
    TSharedRef<SWidget> MakeBar(const FText& Label,
                                TAttribute<float> Fill,
                                FLinearColor BarColour,
                                bool bCentred = false) const;
};
