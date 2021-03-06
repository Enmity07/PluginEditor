// Fill out your copyright notice in the Description page of Project Settings.

#include "BenchmarkingTool.h"

// Sets default values
ABenchmarkingTool::ABenchmarkingTool()
{
	LineTracesPerTick = 100;
	ObjectSweepsPerTick = 100;

	WorldRef = nullptr;
	
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABenchmarkingTool::BeginPlay()
{
	Super::BeginPlay();

	WorldRef = GetWorld();
	
	LineTraceCompleteDelegate.BindUObject(this, &ABenchmarkingTool::OnLineTraceComplete);
}

// Called every frame
void ABenchmarkingTool::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!WorldRef)
		return;
	
	const FCollisionObjectQueryParams ObjectQueryParams =
		(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));

	const FVector InitialPoint = FVector::ZeroVector;
	const FVector Gap = FVector(5.0f, 0.0f, 0.0f);
	
	for(int i = 0; i < LineTracesPerTick; i++)
	{
		const FVector StartPoint = InitialPoint + (Gap * i);
		const FVector EndPoint = StartPoint + FVector(0.0f, 0.0f, LineTraceLength);

		WorldRef->AsyncLineTraceByObjectType(
			EAsyncTraceType::Multi, StartPoint, EndPoint, ObjectQueryParams,
			FCollisionQueryParams::DefaultQueryParam, &LineTraceCompleteDelegate
		);
	}
} 

void ABenchmarkingTool::OnLineTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	//if(!Handle.IsValid())
	//	return;
}
