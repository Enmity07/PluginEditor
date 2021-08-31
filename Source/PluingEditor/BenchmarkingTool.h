// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <atomic>
#include <mutex>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkingTool.generated.h"

struct PLUINGEDITOR_API FThreadedTimer
{
	double Time;

	FThreadedTimer() : Time(0.0f)
	{ }
	FThreadedTimer(const double InTime) : Time(InTime)
	{ }
	~FThreadedTimer()
	{ }
	
	FORCEINLINE void GetTime(const bool bHighPriority, double& Out)
	{
		if(bHighPriority)
		{
			HighPriorityLock();
			Out = Time;
			HighPriorityUnlock();
			return;
		}

		LowPriorityLock();
		Out = Time;
		LowPriorityUnlock();
	}

	FORCEINLINE void SetTime(const bool bHighPriority, const double InValue)
	{
		if(bHighPriority)
		{
			HighPriorityLock();
			Time = InValue;
			HighPriorityUnlock();
			return;
		}

		LowPriorityLock();
		Time = InValue;
		LowPriorityUnlock();
	}
	
	FORCEINLINE void GetLowPriority(double& Out)
	{
		LowPriorityLock();
		Out = Time;
		LowPriorityUnlock();
	}

	FORCEINLINE void SetLowPriority(const double& InValue)
	{
		LowPriorityLock();
		Time = InValue;
		LowPriorityUnlock();
	}

	FORCEINLINE void GetHighPriority(double& Out)
	{
		HighPriorityLock();
		Out = Time;
		HighPriorityUnlock();
	}

	FORCEINLINE void SetHighPriority(const double& InValue)
	{
		HighPriorityLock();
		Time = InValue;
		HighPriorityUnlock();
	}

private:
	FORCEINLINE void LowPriorityLock()
	{
		LowPriorityAccessLock.lock();
        NextToAccessLock.lock();
        DataAccessLock.lock();

        NextToAccessLock.unlock();
	}

	FORCEINLINE void LowPriorityUnlock()
	{
		DataAccessLock.unlock();
		LowPriorityAccessLock.unlock();
	}
	
	FORCEINLINE void HighPriorityLock()
	{
		NextToAccessLock.lock();
		DataAccessLock.lock();

		NextToAccessLock.unlock();
	}

	FORCEINLINE void HighPriorityUnlock()
	{
		DataAccessLock.unlock();
	}
	
	std::mutex DataAccessLock;
	std::mutex NextToAccessLock;
	std::mutex LowPriorityAccessLock;
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

private: // multithreading stuff

	std::mutex TimerThreadLock;
	std::atomic<bool> bShouldTimerThreadRun;

	void TimerThread();

	TStaticArray<FThreadedTimer, 1024, 0> Timers;
};
