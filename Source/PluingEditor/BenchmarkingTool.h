// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <chrono>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Thread.h"
#include "BenchmarkingTool.generated.h"

#define Chrono std::chrono

#define STEADY_CLOCK std::chrono::time_point<std::chrono::steady_clock>
#define GET_HIGH_RES_TIME std::chrono::high_resolution_clock::now()

#define CHR_DURATION(_OUT_TYPE_, _TIME_SCALE_) std::chrono::duration<_OUT_TYPE_, std::ratio<1, _TIME_SCALE_>>

struct PLUINGEDITOR_API FTimerTimes
{
	double Seconds;
	double MilliSeconds;
	long long MicroSeconds;
	long long NanoSeconds;

	FTimerTimes(const double InSec, const double InMilli, const long long InMicro, const long long InNano)
		: Seconds(InSec), MilliSeconds(InMilli), MicroSeconds(InMicro), NanoSeconds(InNano)
	{ }

	FTimerTimes() : Seconds(0.00f), MilliSeconds(0.00f), MicroSeconds(0), NanoSeconds(0)
	{ }
};

struct PLUINGEDITOR_API FHighResTimer
{
	FORCEINLINE void StartTimer()
	{
		StartTime = GET_HIGH_RES_TIME;
	}

	FORCEINLINE FTimerTimes StopTimer()
	{
		EndTime = GET_HIGH_RES_TIME;

		const auto DeltaTime = EndTime - StartTime;
		
		const auto NanosecondsRaw =
			Chrono::duration_cast<Chrono::nanoseconds>(DeltaTime);

		const auto MicrosecondsRaw =
			Chrono::duration_cast<Chrono::microseconds>(DeltaTime);

		const long long NanoSeconds = NanosecondsRaw.count();
		const long long MicroSeconds = MicrosecondsRaw.count();
		const double MilliSeconds = MicroSeconds * 0.001f;
		const double Seconds = MilliSeconds * 0.001f;
		
		return FTimerTimes(Seconds, MilliSeconds, MicroSeconds, NanoSeconds);
	}
private:
	STEADY_CLOCK StartTime;
	STEADY_CLOCK EndTime;
};

UCLASS()
class PLUINGEDITOR_API ABenchmarkingTool : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABenchmarkingTool();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called when the game end or when destroyed. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void OnLineTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data);
	void OnObjectSweepTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BenchmarkSettings)
	uint8 bDoLineTraceBenchmarks : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BenchmarkSettings)
	uint8 bDoObjectSweepTraceBenchmarks : 1;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|LineTraces")
	int LineTracesPerTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|LineTraces")
	float LineTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|LineTraces")
	uint8 bMultiLineTraces : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|LineTraces")
	uint8 bLineTraceDelegateOutput : 1;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	int ObjectSweepsPerTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	float ObjectSweepsTraceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	float ObjectSweepsTraceHalfHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	float ObjectSweepsTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	uint8 bMultiObjectSweepTraces : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BenchmarkSettings|ObjectSweeps")
	uint8 bObjectSweepTraceDelegateOutput : 1;

private:
	UPROPERTY()
	class UWorld *WorldRef;
	
	FTraceDelegate LineTraceCompleteDelegate;
	FTraceDelegate ObjectSweepsTraceCompleteDelegate;

	EAsyncTraceType	AsyncLineTraceType;
	EAsyncTraceType	AsyncObjectSweepsTraceType;

	FCollisionShape VirtualCapsule;

	uint8 bShouldPrintThisFrame : 1;
	float TimeSinceLastUpdate;
};
