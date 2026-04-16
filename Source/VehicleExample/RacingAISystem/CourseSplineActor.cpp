// Copyright Epic Games, Inc. All Rights Reserved.

#include "CourseSplineActor.h"
#include "RacingSplineComponent.h"

ACourseSplineActor::ACourseSplineActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Minimal root so components can be attached
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void ACourseSplineActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // For each name in SplineNames, create a component if one doesn't exist yet.
    // Components are named after their SplineName so they're easy to find in
    // the Components panel. Existing components are left untouched so their
    // points aren't reset when you add a new name.
    for (const FName& SplineName : SplineNames)
    {
        if (SplineName.IsNone()) { continue; }

        // Check if a component with this name already exists on the actor
        URacingSplineComponent* Existing = Cast<URacingSplineComponent>(
            GetDefaultSubobjectByName(SplineName));

        if (!Existing)
        {
            // Also search the full component list (handles runtime-added ones)
            for (UActorComponent* Comp : GetComponents())
            {
                if (Comp && Comp->GetFName() == SplineName)
                {
                    Existing = Cast<URacingSplineComponent>(Comp);
                    break;
                }
            }
        }

        if (!Existing)
        {
            URacingSplineComponent* NewComp = NewObject<URacingSplineComponent>(
                this, SplineName);
            NewComp->SetupAttachment(GetRootComponent());
            NewComp->RegisterComponent();
            NewComp->SetClosedLoop(true);
            // Mark so it persists with the actor in the level
            NewComp->CreationMethod = EComponentCreationMethod::Instance;
        }
    }
}

void ACourseSplineActor::BeginPlay()
{
    Super::BeginPlay();

    // Populate the runtime name -> component map from all attached
    // URacingSplineComponents that were created in OnConstruction.
    for (const FName& SplineName : SplineNames)
    {
        if (SplineName.IsNone()) { continue; }
        if (Splines.Contains(SplineName)) { continue; }

        for (UActorComponent* Comp : GetComponents())
        {
            if (Comp && Comp->GetFName() == SplineName)
            {
                if (URacingSplineComponent* SC = Cast<URacingSplineComponent>(Comp))
                {
                    Splines.Add(SplineName, SC);
                    break;
                }
            }
        }
    }
}

URacingSplineComponent* ACourseSplineActor::GetSplineByName(FName Name) const
{
    const TObjectPtr<URacingSplineComponent>* Found = Splines.Find(Name);
    return Found ? Found->Get() : nullptr;
}

TArray<FName> ACourseSplineActor::GetAllSplineNames() const
{
    TArray<FName> Keys;
    Splines.GetKeys(Keys);
    return Keys;
}

URacingSplineComponent* ACourseSplineActor::GetRandomSpline() const
{
    if (Splines.IsEmpty()) { return nullptr; }

    TArray<FName> Keys;
    Splines.GetKeys(Keys);
    const int32 Idx = FMath::RandRange(0, Keys.Num() - 1);
    return Splines[Keys[Idx]].Get();
}

void ACourseSplineActor::AddSplineForName(FName Name, URacingSplineComponent* Spline)
{
    if (!Spline || Splines.Contains(Name)) { return; }
    Splines.Add(Name, Spline);
}
