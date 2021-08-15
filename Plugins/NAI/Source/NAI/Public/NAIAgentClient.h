// Copyright NoxxProjects and Primrose Taylor. All rights reserved.

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
	float DownwardOffsetForce;
	
	UPROPERTY(EditAnywhere)
	class ANAIAgentManager *AgentManagerVariable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = 0, ClampMax = 1000, UIMin = 0, UIMax = 1000))
	float MoveSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = 0, ClampMax = 1, UIMin = 0, UIMax = 1))
	float LookAtRotationRate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = 0, ClampMax = 1000, UIMin = 0, UIMax = 1000))
	float MaxStepHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
			(ClampMin = 0, ClampMax = 10, UIMin = 0, UIMax = 10))
	float MoveTickInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
			(ClampMin = 0, ClampMax = 10, UIMin = 0, UIMax = 10))
	float PathfindingTickInterval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta =
		(ClampMin = 0, ClampMax = 10, UIMin = 0, UIMax = 10))
	float AvoidanceTickInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	EAgentAvoidanceLevel AvoidanceLevel;
	
	void OnStepCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data);
	void OnAsyncPathComplete(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer);

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
	bool CheckIfBlockedByAgent(const TArray<FHitResult>& Objects);
	TArray<FVector> GetAllHitLocationsNotFromAgents(const TArray<FHitResult>& HitResults);
	
	UPROPERTY()
	FGuid Guid;
	
	UPROPERTY()
	class ANAIAgentManager *AgentManager;
	
	UPROPERTY()
	class UWorld *WorldRef;
};
