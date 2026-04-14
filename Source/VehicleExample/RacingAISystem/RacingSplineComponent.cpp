// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingSplineComponent.h"
#include "EngineUtils.h"

URacingSplineComponent::URacingSplineComponent()
{
    // Closed loop is common for circuit tracks; designers can disable it in Blueprint
    SetClosedLoop(false);
}

// ---------------------------------------------------------------------------
// Racing-line queries
// ---------------------------------------------------------------------------

float URacingSplineComponent::GetNearestSplineDistance(const FVector& WorldLocation,
                                                        int32 NumIterations) const
{
    const float TotalLength = GetSplineLength();
    if (TotalLength <= 0.0f) { return 0.0f; }

    // Coarse pass: sample every 1000 cm and find the closest segment start
    const float CoarseStep = 1000.0f;
    float BestDist = 0.0f;
    float BestDistSq = MAX_FLT;

    for (float D = 0.0f; D <= TotalLength; D += CoarseStep)
    {
        const FVector P = GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
        const float DSq = FVector::DistSquared(P, WorldLocation);
        if (DSq < BestDistSq)
        {
            BestDistSq = DSq;
            BestDist = D;
        }
    }

    // Refinement: binary-search between BestDist ± CoarseStep
    float Low  = FMath::Max(0.0f, BestDist - CoarseStep);
    float High = FMath::Min(TotalLength, BestDist + CoarseStep);

    for (int32 i = 0; i < NumIterations; ++i)
    {
        const float MidA = Low  + (High - Low) / 3.0f;
        const float MidB = High - (High - Low) / 3.0f;

        const FVector PA = GetLocationAtDistanceAlongSpline(MidA, ESplineCoordinateSpace::World);
        const FVector PB = GetLocationAtDistanceAlongSpline(MidB, ESplineCoordinateSpace::World);

        if (FVector::DistSquared(PA, WorldLocation) < FVector::DistSquared(PB, WorldLocation))
        {
            High = MidB;
        }
        else
        {
            Low = MidA;
        }
    }

    return (Low + High) * 0.5f;
}

float URacingSplineComponent::GetLookaheadCurvature(float DistanceAlongSpline,
                                                      float LookaheadDist) const
{
    const float TotalLength = GetSplineLength();
    if (TotalLength <= 0.0f || LookaheadDist <= 0.0f) { return 0.0f; }

    // Sample tangent at the current point and at the lookahead point
    const float D0 = DistanceAlongSpline;
    const float D1 = IsClosedLoop()
                   ? FMath::Fmod(D0 + LookaheadDist, TotalLength)
                   : FMath::Min(D0 + LookaheadDist, TotalLength);

    const FVector T0 = GetTangentAtDistanceAlongSpline(D0, ESplineCoordinateSpace::World).GetSafeNormal();
    const FVector T1 = GetTangentAtDistanceAlongSpline(D1, ESplineCoordinateSpace::World).GetSafeNormal();

    // Curvature ? angle between tangents / arc length
    const float DotProduct  = FMath::Clamp(FVector::DotProduct(T0, T1), -1.0f, 1.0f);
    const float AngleRad    = FMath::Acos(DotProduct);
    const float ArcLength   = FMath::Abs(D1 - D0);

    return (ArcLength > SMALL_NUMBER) ? (AngleRad / ArcLength) : 0.0f;
}

FVector URacingSplineComponent::GetDirectionAtDistance(float Distance) const
{
    return GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
}

FVector URacingSplineComponent::GetLocationAtDistance(float Distance) const
{
    return GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

// ---------------------------------------------------------------------------
// World search helper
// ---------------------------------------------------------------------------

URacingSplineComponent* URacingSplineComponent::FindInWorld(UWorld* World)
{
    if (!World) { return nullptr; }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        URacingSplineComponent* Spline =
            (*It)->FindComponentByClass<URacingSplineComponent>();
        if (Spline) { return Spline; }
    }

    return nullptr;
}
