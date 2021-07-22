// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NAI/NAIUtils/Public/NAICalculator.h"
#include "NavigationSystem.h"

#include "CoreMinimal.h"

#include "NAIAgentClient.h"
#include "NAIAgentSettingsGlobals.h"

#include "GameFramework/Actor.h"
#include "NAIAgentManager.generated.h"

UENUM()
enum class EAgentTaskType : uint8
{
	Pathfinding UMETA(DisplayName = "Pathfinding"),
	TracingFront UMETA(DisplayName = "TracingFront"),
	TracingLeft UMETA(DisplayName = "TracingLeft"),
	TracingRight UMETA(DisplayName = "TracingRight"),
	Default UMETA(DisplayName = "Default"),
};

USTRUCT()
struct NAI_API FAgentTimers
{
	GENERATED_BODY()

	float MoveTime;
	float PathTime;
	float TraceTime;

	uint8 bIsMoveReady : 1;
	uint8 bIsPathReady : 1;
	uint8 bIsTraceReady : 1;
};

USTRUCT()
struct NAI_API FAgentProperties
{
	GENERATED_BODY()

	EAgentType AgentType;
	
	float MoveSpeed;
	float LookAtRotationRate;
	
	float MoveTickRate;
	float PathfindingTickRate;
	float TracingTickRate;
};

class AAgentManager;

USTRUCT()
struct NAI_API FAgent
{
	GENERATED_BODY()
	
	FGuid Guid;
	
	// Pointer to the Agent Object itself
	UPROPERTY()
	class ANAIAgentClient *AgentClient;
	// Pointer to the manager that created this for easy access
	UPROPERTY()
	class ANAIAgentManager *AgentManager;
	
	// Contains all the agents properties,
	// such as Move Speed, Tick rates etc...
	// All these vars are set on the AgentClient
	// object itself via EditAnywhere UPROPERTY()'s
	FAgentProperties AgentProperties;
	
	// Async Pathfinding
	FNavAgentProperties NavAgentProperties;
	// Local copy of the delegate which is called after an Async path task completes
	FNavPathQueryDelegate NavPathQueryDelegate;
	// Current navigation path as an array of vectors
	TArray<FNavPathPoint> CurrentPath;

	// Async Raytracing
	FTraceDelegate RaytraceFrontDelegate;
	FTraceDelegate RaytraceRightDelegate;
	FTraceDelegate RaytraceLeftDelegate;

	// Object that contains the timers
	FAgentTimers Timers;

	// Local copy of the agents current velocity Vector
	float Speed;
	FVector Velocity;
private:
	FVector PositionThisFrame;
	FVector PositionLastFrame;
	
public:

	// Whether or not this agent is halted
	uint8 bIsHalted : 1;

	FORCEINLINE void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
	FORCEINLINE void SetIsHalted(const uint8 IsHalted) { bIsHalted = IsHalted; }
	
	// Update the Agents PathPoints for their current navigation path
	FORCEINLINE void UpdatePathPoints(const TArray<FNavPathPoint>& Points)
	{
		CurrentPath = Points;
	}

	// Update the Timer settings for each agent using the passed in DeltaTime
	// Won't update a timer if it has already ticked fully but hasn't been used yet
	FORCEINLINE void UpdateTimers(const float DeltaTime)
	{
		if(!Timers.bIsMoveReady)
		{
			if(Timers.MoveTime >= AgentProperties.MoveTickRate)
			{
				Timers.bIsMoveReady = true;
				Timers.MoveTime = 0.0f;
			}
			else
			{
				Timers.MoveTime += DeltaTime;
			}
		}

		if(!Timers.bIsPathReady)
		{
			if(Timers.PathTime >= AgentProperties.PathfindingTickRate)
			{
				Timers.bIsPathReady = true;
				Timers.PathTime = 0.0f;
			}
			else
			{
				Timers.PathTime += DeltaTime;
			}
		}

		if(!Timers.bIsTraceReady)
		{
			if(Timers.TraceTime >= AgentProperties.TracingTickRate)
			{
				Timers.bIsTraceReady = true;
				Timers.TraceTime = 0.0f;
			}
			else
			{
				Timers.TraceTime += DeltaTime;
			}
		}
	}

	FORCEINLINE void CalculateAndUpdateSpeed(
		const FVector& Location,
		const float DeltaTime)
	{
		PositionLastFrame = PositionThisFrame;
		PositionThisFrame = Location;
		const float Distance = FNAICalculator::Distance(PositionThisFrame, PositionLastFrame);
		Speed = Distance / DeltaTime;
		AgentClient->Speed = Speed;
	}
};

UCLASS(BlueprintType)
class NAI_API ANAIAgentManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANAIAgentManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AgentManager|Agents|Limits", meta =
		(ClampMin = 0, ClampMax = 10000, UIMin = 0, UIMax = 10000))
	int MaxAgentCount;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
public:
	void AddAgent(const FAgent& Agent);
	void RemoveAgent(const FGuid& Guid);
	void UpdateAgent(const FAgent& Agent);
	void UpdateAgentPath(const FGuid& Guid, const TArray<FNavPathPoint>& PathPoints);
	void AgentPathTaskAsync(
		const FGuid& Guid,
		const FVector& Location,
		const FVector& Start
	);

private:
	FORCEINLINE FVector GetAgentGoalLocationFromType(
		const EAgentType& AgentType,
		const FVector& PlayerLocation) const
	{
		switch(AgentType)
        {
        case EAgentType::PathToPlayer:
        	return PlayerLocation;
        case EAgentType::PathToLocation:
        	break;
        case EAgentType::Static:
        	break;
        case EAgentType::Dynamic:
        	break;
        case EAgentType::FollowCustomPath:
        	break;
        default:
        	break;
        }
        
        return FVector();
	}
	
	void Initialize();
	
private:
	UPROPERTY()
	class UWorld *WorldRef;
	UPROPERTY()
	class UNavigationSystemV1 *NavSysRef;
	UPROPERTY()
	class ANavigationData *NavDataRef;
	
	FSharedConstNavQueryFilter NavQueryRef;
	
	/* Hash table containing all of the Agents.
	* Each Agent has a guid assigned to it for identification.
	* All Agent guids are stored in the AgentGuids array,
	* which can be used to iterate through the entire table.
	*/
	UPROPERTY()
	TMap<FGuid, FAgent> AgentMap;
	/* A list of all the guids belonging to each Agent currently in
	* the AgentMap hash table.
	*/
	UPROPERTY()
	TArray<FGuid> AgentGuids;
};
