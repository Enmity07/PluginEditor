// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkingTool.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BenchmarkSettings)
	int LineTracesPerTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BenchmarkSettings)
	int LineTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BenchmarkSettings)
	int ObjectSweepsPerTick;

private:
	UPROPERTY()
	class UWorld *WorldRef;

	FTraceDelegate LineTraceCompleteDelegate;
};
