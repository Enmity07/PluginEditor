// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <atomic>
#include <mutex>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkingTool.generated.h"

enum class PLUINGEDITOR_API EAccessType : uint8
{
	GET,
	SET
};

struct PLUINGEDITOR_API FThreadedTimer
{
	double Time;

	struct FAccessData
	{
		double& TimeOutput;
		double InTime; // doesn't need to be a ref/ptr
	};
	
	FORCEINLINE void LowPriorityAccessor()
	{
		LowPriorityAccessLock.lock();
		NextToAccessLock.lock();
		DataAccessLock.lock();

		NextToAccessLock.unlock();

		// do stuff

		DataAccessLock.unlock();
		LowPriorityAccessLock.unlock();
	}

	FORCEINLINE void HighPriorityAccessor(
		const EAccessType& AccessType, const FAccessData& AccessData)
	{
		NextToAccessLock.lock();
		DataAccessLock.lock();

		NextToAccessLock.unlock();

		if(AccessType == EAccessType::GET)
		{
			AccessData.TimeOutput = Time;
		}
		else // SET
		{
			Time = AccessData.InTime;
		}

		DataAccessLock.unlock();
	}

private:
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
};
