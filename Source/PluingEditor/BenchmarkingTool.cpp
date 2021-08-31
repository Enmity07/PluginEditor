// Fill out your copyright notice in the Description page of Project Settings.

#include "BenchmarkingTool.h"

#include <thread>

#include <mutex>
#include <chrono>

#define GET load
#define SET store

// Sets default values
ABenchmarkingTool::ABenchmarkingTool()
{
	bShouldTimerThreadRun.SET(false);
	
	bDoLineTraceBenchmarks = false;
	bDoObjectSweepTraceBenchmarks = false;
	
	LineTracesPerTick = 100;
	LineTraceLength = 100.0f;
	bMultiLineTraces = false;
	bLineTraceDelegateOutput = false;
	AsyncLineTraceType = EAsyncTraceType::Single;
	
	ObjectSweepsPerTick = 100;
	ObjectSweepsTraceLength = 100.0f;
	ObjectSweepsTraceRadius = 50.0f;
	ObjectSweepsTraceHalfHeight = 96.0f;
	bMultiObjectSweepTraces = false;
	bObjectSweepTraceDelegateOutput = false;
	AsyncObjectSweepsTraceType = EAsyncTraceType::Single;

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
	ObjectSweepsTraceCompleteDelegate.BindUObject(this, &ABenchmarkingTool::OnObjectSweepTraceComplete);

	if(bMultiLineTraces)
	{
		AsyncLineTraceType = EAsyncTraceType::Multi;
	}
	
	if(bMultiObjectSweepTraces)
	{
		AsyncObjectSweepsTraceType = EAsyncTraceType::Multi;
	}

	VirtualCapsule = FCollisionShape::MakeCapsule(ObjectSweepsTraceRadius, ObjectSweepsTraceHalfHeight);
	double TempTime = 0.0f;

	Timers[0].GetTime(true, TempTime);
	Timers[0].GetTime(true, TempTime);
	
	std::thread([=]()
	{
		this->bShouldTimerThreadRun.SET(false);
		this->TimerThread();
	});
}

void ABenchmarkingTool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
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
	const FVector KindaSmallGap = FVector(KINDA_SMALL_NUMBER, 0.0f, 0.0f);

	
	
	if(bDoLineTraceBenchmarks)
	{
		for(int i = 0; i < LineTracesPerTick; i++)
		{
			const FVector StartPoint = InitialPoint + (KindaSmallGap * i);
			const FVector EndPoint = StartPoint + FVector(0.0f, 0.0f, LineTraceLength);

			WorldRef->AsyncLineTraceByObjectType(
				AsyncLineTraceType, StartPoint, EndPoint, ObjectQueryParams,
				FCollisionQueryParams::DefaultQueryParam,
				bLineTraceDelegateOutput ? &LineTraceCompleteDelegate : 0
			);
		}
		
	}

	if(bDoObjectSweepTraceBenchmarks)
	{
		for(int i = 0; i < ObjectSweepsPerTick; i++)
		{
			const FVector StartPoint = InitialPoint + (KindaSmallGap * i);
			const FVector EndPoint = StartPoint + FVector(0.0f, 0.0f, ObjectSweepsTraceLength);
		
			WorldRef->AsyncSweepByObjectType(
				AsyncObjectSweepsTraceType, StartPoint, EndPoint, FQuat(0.0f, 0.0f, 0.0f, 0.0f),
				ObjectQueryParams,
				VirtualCapsule,
				FCollisionQueryParams::DefaultQueryParam,
				bObjectSweepTraceDelegateOutput ? &ObjectSweepsTraceCompleteDelegate : 0
			);
		}
	}
} 

void ABenchmarkingTool::OnLineTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid())
		return;
}

void ABenchmarkingTool::OnObjectSweepTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid())
		return;
}

void ABenchmarkingTool::TimerThread()
{
	unsigned long long TickCount = 0;
	double LocalTicker = 0.00f;
	
	for(;;) //infinite loop
	{
		if(bShouldTimerThreadRun.GET() == false)
			return;
		
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		LocalTicker += 0.0001f;
		TickCount++;
	}
}
