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
enum class EAgentRaytraceDirection : uint8
{
	TracingFront UMETA(DisplayName = "TracingFront"),
	TracingLeft UMETA(DisplayName = "TracingLeft"),
	TracingRight UMETA(DisplayName = "TracingRight"),
};

USTRUCT()
struct NAI_API FAgentTimedProperty
{
	GENERATED_BODY()

	float TickRate;
	float Time;
	uint8 bIsReady : 1;

	uint8 bIsPaused : 1;

	// Default initialization. Really important bIsReady == true fpr how
	// the plugin works
	FAgentTimedProperty() { TickRate = 1.0f; Time = 0.0f; bIsReady = true; bIsPaused = false;}

	FORCEINLINE void Pause() { bIsPaused = true; }
	FORCEINLINE void Unpause() { bIsPaused = false; }
	FORCEINLINE void Reset() { Time = 0.0f; bIsReady = false; }
	FORCEINLINE void AddTime(const float InTime)
	{
		if(bIsPaused)
			return;
		
		if(!bIsReady)
		{
			if(Time >= TickRate)
			{
				bIsReady = true;
				Time = 0.0f;
			}
			else
			{
				Time += InTime;
			}
		}
	}
	// Always add time
	FORCEINLINE void AddTimeRaw(const float InTime) { Time += InTime; }
};

USTRUCT()
struct NAI_API FAgentAvoidanceResultTimers
{
	GENERATED_BODY()
	
	FAgentTimedProperty Forward;
	FAgentTimedProperty Right;
	FAgentTimedProperty Left;
};

USTRUCT()
struct NAI_API FAgentTimers
{
	GENERATED_BODY()

	FAgentTimedProperty MoveTime;
	FAgentTimedProperty PathTime;
	FAgentTimedProperty TraceTime;

	FAgentTimedProperty FloorCheckTime;
	
	FAgentTimedProperty FloorCheckTimeResultAge;
	FAgentAvoidanceResultTimers AvoidanceResultAgeTimers;
};

// TODO: Get rid of this mess by making everything proportional
// to the agent's size
#define NORMAL_AVOIDANCE_COLUMNS		3
#define ADVANCED_AVOIDANCE_COLUMNS		5
#define AVOIDANCE_SIDE_COLUMNS			1
#define NORMAL_AVOIDANCE_ROWS			3
#define ADVANCED_AVOIDANCE_ROWS			5
#define AVOIDANCE_WIDTH_MULTIPLIER		1.5f
#define AVOIDANCE_HEIGHT_MULTIPLIER		0.8f

USTRUCT()
struct NAI_API FAgentAvoidanceTaskResults
{
	GENERATED_BODY()

	uint8 bForwardBlocked : 1;
	uint8 bRightBlocked : 1;
	uint8 bLeftBlocked : 1;
};

#define UP_VECTOR FVector(0.0f, 0.0f, 1.0f)

USTRUCT()
struct NAI_API FAgentAvoidanceProperties
{
	GENERATED_BODY()

	EAgentAvoidanceLevel AvoidanceLevel;
	float GridWidth;
	float GridHalfWidth;
	float GridHeight;
	float GridHalfHeight;

	uint8 GridColumns;
	uint8 GridRows;
	uint8 SideColumns;
	uint8 SideRows;

	float WidthIncrementSize;
	float SideWidthIncrementSize;
	float HeightIncrementSize;
	float StartOffsetWidth;
	float SideStartOffsetWidth;
	FVector StartOffsetHeightVector;
	FVector SideStartOffsetHeightVector;

	/**
	 * Initialize everything
	 * @param InAvoidanceLevel The avoidance level of the Agent
	 * @param InRadius The Radius of the Agent
	 * @param InHalfHeight The Half Height of the Agent
	*/
	FORCEINLINE void Initialize(
		const EAgentAvoidanceLevel& InAvoidanceLevel,
		const float InRadius,
		const float InHalfHeight)
	{
		AvoidanceLevel = InAvoidanceLevel;
		GridWidth = InRadius * AVOIDANCE_WIDTH_MULTIPLIER;
		GridHalfWidth = GridWidth * 0.5f;
		GridHeight = (InHalfHeight * 2.0f) * AVOIDANCE_HEIGHT_MULTIPLIER;
		GridHalfHeight = GridHeight * 0.5f;

		GridColumns = (AvoidanceLevel == EAgentAvoidanceLevel::Normal)
					? (NORMAL_AVOIDANCE_COLUMNS) : (ADVANCED_AVOIDANCE_COLUMNS);
		GridRows = (AvoidanceLevel == EAgentAvoidanceLevel::Normal)
					? (NORMAL_AVOIDANCE_ROWS) : (ADVANCED_AVOIDANCE_ROWS);

		SideColumns = AVOIDANCE_SIDE_COLUMNS;
		SideRows = GridRows;

		// TODO: Try get rid of division here
		WidthIncrementSize = GridWidth / GridColumns;
		SideWidthIncrementSize = (GridWidth * 0.5f) / SideColumns; // use half grid width for the sides
		HeightIncrementSize = GridHeight / GridRows;

		StartOffsetWidth = WidthIncrementSize * ((GridColumns - 1.0f) * 0.5f);
		SideStartOffsetWidth = WidthIncrementSize * ((SideColumns - 1.0f) * 0.5f);
		StartOffsetHeightVector = UP_VECTOR * (HeightIncrementSize * ((GridRows - 1.0f) * 0.5f));
		SideStartOffsetHeightVector = UP_VECTOR * (HeightIncrementSize * ((SideRows - 1.0f) * 0.5f));
	}
};

#undef UP_VECTOR

#undef NORMAL_AVOIDANCE_COLUMNS
#undef ADVANCED_AVOIDANCE_COLUMNS
#undef AVOIDANCE_SIDE_COLUMNS
#undef NORMAL_AVOIDANCE_ROWS
#undef ADVANCED_AVOIDANCE_ROWS
#undef AVOIDANCE_WIDTH_MULTIPLIER

USTRUCT()
struct NAI_API FAgentFloorCheckResult
{
	GENERATED_BODY()
	
	// Result may be invalid if the trace didn't hit anything
	uint8 bIsValidResult : 1;
	// Float that represents the Z point of the hit location
	float DetectedZPoint;
};

USTRUCT()
struct NAI_API FAgentNavigationProperties
{
	GENERATED_BODY()

	FNavAgentProperties NavAgentProperties;
	// Local copy of the delegate which is called after an Async path task completes
	FNavPathQueryDelegate NavPathQueryDelegate;

	FTraceDelegate FloorCheckTraceDelegate;
};

USTRUCT()
struct NAI_API FAgentProperties
{
	GENERATED_BODY()

	EAgentType AgentType;

	float CapsuleRadius;
	float CapsuleHalfHeight;
	
	float MoveSpeed;
	float LookAtRotationRate;
	
	// Async Raytracing
	FTraceDelegate RaytraceFrontDelegate;
	FTraceDelegate RaytraceRightDelegate;
	FTraceDelegate RaytraceLeftDelegate;

	FAgentNavigationProperties NavigationProperties;
	FAgentAvoidanceProperties AvoidanceProperties;
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
	// Current navigation path as an array of vectors
	TArray<FNavPathPoint> CurrentPath;

	// The latest results for Each Avoidance Trace direction
	// Timers for the age of each directions trace results are stored
	// in the Timers variable under the AvoidanceResults variable.
	// We don't want to use the data from a given direction if it's old..
	// ...this should only be relevant when the frame rate is very low
	FAgentAvoidanceTaskResults LatestAvoidanceTaskResults;

	FAgentFloorCheckResult LatestFloorCheckResult;
	
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

	FORCEINLINE void UpdateAvoidanceResult(const EAgentRaytraceDirection& InDirection, const bool bInResult)
	{
		switch(InDirection)
		{
			case EAgentRaytraceDirection::TracingFront:
				LatestAvoidanceTaskResults.bForwardBlocked = bInResult;
				Timers.AvoidanceResultAgeTimers.Forward.Reset();
				break;
			case EAgentRaytraceDirection::TracingLeft:
				LatestAvoidanceTaskResults.bLeftBlocked = bInResult;
				Timers.AvoidanceResultAgeTimers.Left.Reset();
				break;
			case EAgentRaytraceDirection::TracingRight:
				LatestAvoidanceTaskResults.bRightBlocked = bInResult;
				Timers.AvoidanceResultAgeTimers.Right.Reset();
				break;
		}
	}
	
	/**
	 * Update the LatestFloorCheckResult with new data
	 * @param HitZPoint The Z axis value for this result
	 * @param bSuccess Whether or not the Floor Check worked
	*/
	FORCEINLINE void UpdateFloorCheckResult(const float HitZPoint, const bool bSuccess)
	{
		LatestFloorCheckResult.bIsValidResult = bSuccess;
		// Make sure we set it to 0.0f if the trace failed in order to overwrite the old data
		LatestFloorCheckResult.DetectedZPoint = (bSuccess) ? (HitZPoint) : (0.0f);
		Timers.FloorCheckTimeResultAge.Reset();
	}
	
	// Update the Timer settings for each agent using the passed in DeltaTime
	// Won't update a timer if it has already ticked fully but hasn't been used yet
	FORCEINLINE void UpdateTimers(const float DeltaTime)
	{
		Timers.MoveTime.AddTime(DeltaTime);
		Timers.PathTime.AddTime(DeltaTime);
		Timers.TraceTime.AddTime(DeltaTime);
		Timers.FloorCheckTime.AddTime(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Forward.AddTimeRaw(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Right.AddTimeRaw(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Left.AddTimeRaw(DeltaTime);
		Timers.FloorCheckTimeResultAge.AddTimeRaw(DeltaTime);
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

DECLARE_DELEGATE(FAgentTaskFunctionRef);
template<typename TaskType>
struct NAI_API TAgentTask
{
	TaskType TypeVar;
	FAgentTaskFunctionRef FunctionRef;

	TAgentTask(const TaskType& InType, const TFunctionRef<void()> InFunctionRef)
	{
		TypeVar = InType;
		FunctionRef.BindLambda(InFunctionRef());
	}
	
	FORCEINLINE void RunTask() 
	{
		checkf(FunctionRef.IsBound(), TEXT("Attempting to call an unbound delegate!"));
		FunctionRef.Execute();
	}
};

UCLASS(BlueprintType)
class NAI_API ANAIAgentManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANAIAgentManager();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> PlayerClass;
	
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
	FORCEINLINE void UpdateAgentPath(
		const FGuid& Guid, const TArray<FNavPathPoint>& PathPoints)
	{
		AgentMap[Guid].UpdatePathPoints(PathPoints);
	}
	
	FORCEINLINE void UpdateAgentAvoidanceResult(
		const FGuid& Guid,
		const EAgentRaytraceDirection& TraceDirection, const bool bResult)
	{
		AgentMap[Guid].UpdateAvoidanceResult(TraceDirection, bResult);
	}

	/**
	* Update the LatestFloorCheckResult of a particular Agent
	* @param Guid The Guid of the Agent to update
	* @param HitZPoint The Z axis value for this result
	* @param bSuccess Whether or not the Floor Check worked
	*/
	FORCEINLINE void UpdateAgentFloorCheckResult(
		const FGuid& Guid, const float HitZPoint,
		const bool bSuccess)
	{
		AgentMap[Guid].UpdateFloorCheckResult(HitZPoint, bSuccess);
	}
	
private:
	void AgentPathTaskAsync(
		const FVector& Start,
		const FVector& Goal,
		const FAgentNavigationProperties& NavigationProperties
	) const;
	void AgentTraceTaskAsync( 
		const FVector& AgentLocation,
		const FVector& Forward,
		const FVector& Right,
		const FAgentProperties& AvoidanceProperties
	) const;
	
	// TODO: Not sure why i inlined this.. implement it properly
	FVector GetAgentGoalLocationFromType(
		const EAgentType& AgentType,
		const FVector& PlayerLocation) const;
	
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
