// Copyright NoxxProjects and Primrose Taylor. All rights reserved.

#pragma once

#include "NAI/NAIUtils/Public/NAICalculator.h"

float FNAICalculator::Distance(const FVector& A, const FVector& B)
{
	const float Result = (
		(A.X * A.X) - (B.X * B.X) +
		(A.Y * A.Y) - (B.Y * B.Y) +
		(A.Z * A.X) - (B.Z * B.Z)
	);
	
	return SqrRoot(Result);
}

float FNAICalculator::SqrRoot(const float InNumber)
{
	const float Half = InNumber * 0.5f;
	const float ThreeHalves = 1.5f;

	long i = *(long*)&InNumber;	// evil floating point bit level hacking
	i  = 0x5f3759df - (i >> 1);		// what the fuck? 
	float N  = *(float*)&i;
	N  = N * (ThreeHalves - (Half * N * N));	// 1st iteration
	// N  = N * (ThreeHalves - (Half * N * N));	// 2nd iteration, this can be removed .. kept it tho coz accuracy
	
	return N * InNumber;
}
