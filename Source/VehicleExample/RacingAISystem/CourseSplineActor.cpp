// Copyright Epic Games, Inc. All Rights Reserved.

#include "CourseSplineActor.h"
#include "RacingSplineComponent.h"

ACourseSplineActor::ACourseSplineActor()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

URacingSplineComponent* ACourseSplineActor::GetSplineByName(FName Name) const
{
    TArray<URacingSplineComponent*> Found;
    GetComponents<URacingSplineComponent>(Found);
    for (URacingSplineComponent* SC : Found)
    {
        if (SC && SC->GetFName() == Name) { return SC; }
    }
    return nullptr;
}

TArray<FName> ACourseSplineActor::GetAllSplineNames() const
{
    TArray<URacingSplineComponent*> Found;
    GetComponents<URacingSplineComponent>(Found);
    TArray<FName> Names;
    for (URacingSplineComponent* SC : Found)
    {
        if (SC) { Names.Add(SC->GetFName()); }
    }
    return Names;
}

URacingSplineComponent* ACourseSplineActor::GetRandomSpline() const
{
    TArray<URacingSplineComponent*> Found;
    GetComponents<URacingSplineComponent>(Found);
    if (Found.IsEmpty()) { return nullptr; }
    return Found[FMath::RandRange(0, Found.Num() - 1)];
}
