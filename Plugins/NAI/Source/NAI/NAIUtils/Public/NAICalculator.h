// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class NAI_API FNAICalculator
{
public:
	static FORCEINLINE float Distance(const FVector& A, const FVector& B)
	{
		const float Result = (
			(A.X * A.X) - (B.X * B.X) +
			(A.Y * A.Y) - (B.Y * B.Y) +
			(A.Z * A.X) - (B.Z * B.Z)
		);
	
		return SqrRoot(Result);
	}
	
	static FORCEINLINE float SqrRoot(const float InNumber)
	{
		const float Half = InNumber * 0.5f;
		const float ThreeHalves = 1.5f;

		long i = *(long*)&InNumber;	// evil floating point bit level hacking
		i  = 0x5f3759df - (i >> 1);		// what the fuck? 
		float N  = *(float*)&i;
		N  = N * (ThreeHalves - (Half * N * N));	// 1st iteration
		// N  = N * (ThreeHalves - (Half * N * N));	// 2nd iteration, this can be removed ..
	
		return N * InNumber;
		// ty id <3
	}

	static FORCEINLINE void GetHighestHitPoint(const TArray<FHitResult>& Hits, FVector& OutHighestHit)
	{
		for(int i = 0; i < Hits.Num(); i++)
		{
			if(Hits[i].ImpactPoint.Z > OutHighestHit.Z)
			{
				OutHighestHit = Hits[i].ImpactPoint;
			}
		}
	}
};
