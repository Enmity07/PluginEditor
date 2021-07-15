// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class NAI_API FNAICalculator
{
public:
	static float Distance(const FVector& A, const FVector& B);
	static float SqrRoot(const float InNumber);
};
