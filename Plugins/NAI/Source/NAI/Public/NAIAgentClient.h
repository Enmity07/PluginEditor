// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NAIAgentSettingsGlobals.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NAIAgentClient.generated.h"

UCLASS(BlueprintType)
class NAI_API ANAIAgentClient : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANAIAgentClient();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agent")
	EAgentType AgentType;
	
	UPROPERTY(EditAnywhere)
	class ANAIAgentManager *AgentManagerVariable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = "0", ClampMax = 1000, UIMin = 0, UIMax = 1000))
	float MoveSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = "0", ClampMax = 1, UIMin = 0, UIMax = 1))
	float LookAtRotationRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
			(ClampMin = "0", ClampMax = 144, UIMin = 0, UIMax = 144))
	float MoveTickInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
			(ClampMin = "0", ClampMax = 144, UIMin = 0, UIMax = 144))
	float PathfindingTickInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = "0", ClampMax = 144, UIMin = 0, UIMax = 144))
	float AvoidanceTickInterval;
	
	void PathCompleteDelegate(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer);

	void OnFrontTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	void OnRightTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	void OnLeftTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent")
	float Speed;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent")
	FVector Velocity;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent *CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent *SkeletalMeshComponent;
	
private:
	UPROPERTY()
	FGuid Guid;
	
	UPROPERTY()
	class ANAIAgentManager *AgentManager;
	
	UPROPERTY()
	class UWorld *WorldRef;

	FORCEINLINE bool CheckIfBlockedByAgent(const FTraceDatum Actors) const
	{
		for(int i = 0; i < Actors.OutHits.Num(); i++)
		{
			if(Actors.OutHits[i].Actor.Get()->IsValidLowLevelFast())
			{
				if(Actors.OutHits[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
				{
					return true;
				}
			}
		}
		return false;
	}
};
