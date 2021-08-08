// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NAI/NAIUtils/Public/NAICalculator.h"
#include "NavigationSystem.h"

#include "CoreMinimal.h"

#include "NAIAgentClient.h"
#include "NAIAgentSettingsGlobals.h"

#include "GameFramework/Actor.h"
#include "NAIAgentManager.generated.h"

/**
 * Simple enum used to categorize each set of Raytraces done
 * during each Agent's avoidance calculations.
 */
UENUM()
enum class EAgentRaytraceDirection : uint8
{
	TracingFront UMETA(DisplayName = "TracingFront"),
	TracingLeft UMETA(DisplayName = "TracingLeft"),
	TracingRight UMETA(DisplayName = "TracingRight"),
};

/**
 * Specialist timer which will automatically stop when a set
 * interval of time has passed, and become "ready".
 * The timer then has to be reset to be used again.
 * @biref Specialist timer for use by the Agents.
 */
USTRUCT()
struct NAI_API FAgentTimedProperty
{
	GENERATED_BODY()

	/** The tick/time interval this timer should become "ready" at. */
	float TickRate;
	/** Used to track time passed. */
	float Time;
	/** Has this timer completed it's tick interval? */
	uint8 bIsReady : 1;
	/** Used to pause/unpause the timer. */
	uint8 bIsPaused : 1;

	/** Create the timer, starting in the "ready" state. */
	FAgentTimedProperty() { TickRate = 1.0f; Time = 0.0f; bIsReady = true; bIsPaused = false;}

	/** Pause the timer. */
	FORCEINLINE void Pause() { bIsPaused = true; }

	/** Unpause the timer. */
	FORCEINLINE void Unpause() { bIsPaused = false; }

	/** Resets the timer. This will also unpause the timer. */
	FORCEINLINE void Reset() { Time = 0.0f; bIsReady = false; bIsPaused = false; }

	/**
	 * This will only add the InTime if the timer is not
	 * already in the "ready" state. It will also not add it
	 * if the timer is paused.
	 * @param InTime The amount of time to add.
	 */
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

	/** 
	 * Always add time regardless of the state of the timer.
	 * @param InTime The amount of time to add.
	 */
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
	FAgentTimedProperty StepCheckTime;
	
	FAgentTimedProperty FloorCheckTimeResultAge;
	FAgentTimedProperty StepCheckTimeResultAge;
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
	 * TODO: Document this
	 * @brief Calculates and sets up the Avoidance grid of traces.
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
struct NAI_API FAgentStepCheckProperties
{
	GENERATED_BODY()
	
	float ForwardOffset;
	float DownwardOffset;
	
	FORCEINLINE void Initialize(
		const float InRadius,
		const float InHalfHeight,
		const float InMaxStepHeight)
	{
		ForwardOffset = (InRadius + 10.0f);
		DownwardOffset = InHalfHeight - (InMaxStepHeight * 2.0f);
	}
};

USTRUCT()
struct NAI_API FAgentAvoidanceTaskResults
{
	GENERATED_BODY()

	uint8 bForwardBlocked : 1;
	uint8 bRightBlocked : 1;
	uint8 bLeftBlocked : 1;
};

/**
 * Simple struct used to hold the result of each Floor Check.
 * If the floor check failed, the DetectedZPoint is set to 0.0f.
 */
USTRUCT()
struct NAI_API FAgentFloorCheckResult
{
	GENERATED_BODY()
	
	/** Result may be invalid if the trace didn't hit anything */
	uint8 bIsValidResult : 1;
	/** Float that represents the Z point of the hit location if we hit one */
	float DetectedZPoint;
};

USTRUCT()
struct NAI_API FAgentStepCheckResult
{
	GENERATED_BODY()

	/** Did we detect a step on this check? */
	uint8 bStepWasDetected : 1;
	/** The height of the detected step, relative to the Agent. */
	float StepHeight;
};

USTRUCT()
struct NAI_API FAgentNavigationProperties
{
	GENERATED_BODY()

	FNavAgentProperties NavAgentProperties;
	/** Local copy of the delegate which is called after an Async path task completes */
	FNavPathQueryDelegate NavPathQueryDelegate;

	/** Sub-structure containing the Agent's Step Navigation properties. */
	FAgentStepCheckProperties StepProperties;
	
	FTraceDelegate FloorCheckTraceDelegate;
	FTraceDelegate StepCheckTraceDelegate;
};

USTRUCT()
struct NAI_API FAgentProperties
{
	GENERATED_BODY()

	/** The type of Agent. */
	EAgentType AgentType;
	/** The Radius of the Agent's Capsule collider. */
	float CapsuleRadius;
	/** The Half Height of the Agent's Capsule collider. */
	float CapsuleHalfHeight;
	/** The MoveSpeed of this Agent. */
	float MoveSpeed;
	/** The speed at which the Agent will rotate into the direction it's moving. */
	float LookAtRotationRate;

	/** The maximum step height for this Agent. */
	float MaxStepHeight;
	
	// Async Raytracing
	FTraceDelegate RaytraceFrontDelegate;
	FTraceDelegate RaytraceRightDelegate;
	FTraceDelegate RaytraceLeftDelegate;
	/** Sub-structure containing the Agent's Navigation properties. */
	FAgentNavigationProperties NavigationProperties;
	/** Sub-structure containing the Agent's Avoidance properties. */
	FAgentAvoidanceProperties AvoidanceProperties;
};

class AAgentManager;

/**
 * This struct serves as a container for all variables related to each Agent.
 * Things such as task variables and task results are stored in here.
 * The reason this struct was created initially was to make a thread-safe
 * wrapper type for each Agent. Instead now that we just used the Async systems
 * already present in UE, it has been modified to contain everything the GameThread
 * needs to work with those tasks.
 * @brief This is a container for everything each Agent needs/does.
 */
USTRUCT()
struct NAI_API FAgent
{
	GENERATED_BODY()

	/**
	 * A copy of the Guid for this Agent.
	 * This is used for easy identification, and to allow iteration
	 * in the agent map, in an ordered fashion. 
	 */
	FGuid Guid;
	/** Pointer to the Agent UObject itself. */
	UPROPERTY()
	class ANAIAgentClient *AgentClient;
	/** Pointer to the manager that created this, for easy access. */
	UPROPERTY()
	class ANAIAgentManager *AgentManager;
	
	/**
	 * Contains all the properties for this Agent.
	 * This includes the UPROPERTY(EditAnywhere) variables that
	 * are exposed to the Editors UI, along with things related to
	 * the Agent's tasks.
	 */
	FAgentProperties AgentProperties;
	
	/**
	 * When an Async Pathfinding job has completed, it calls
	 * a delegate on the AgentClient which will in turn update this
	 * with a new path for the Agent.
	 */
	TArray<FNavPathPoint> CurrentPath;

	/**
	 * The latest results for Each Avoidance Trace direction
	 * Timers for the age of each directions trace results are stored
	 * in the Timers variable under the AvoidanceResults variable.
	 * We don't want to use the data from a given direction if it's old..
	 * ...this should only be relevant when the frame rate is very low
	 */
	FAgentAvoidanceTaskResults LatestAvoidanceTaskResults;

	FAgentFloorCheckResult LatestFloorCheckResult;

	FAgentStepCheckResult LatestStepCheckResult;
	
	// Object that contains the timers
	FAgentTimers Timers;

	// Local copy of the agents current velocity Vector
	float Speed;
	FVector Velocity;
private:
	FVector PositionThisFrame;
	FVector PositionLastFrame;
	
public:
	/**
	 * Whether or not this agent is halted. If the Agent is halted,
	 * it will still have it's Timers and Velocity updated.
	 */
	uint8 bIsHalted : 1;

	/** Set the Velocity of this Agent. */
	FORCEINLINE void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }

	/** Set the Agent either be halted, or not. */
	FORCEINLINE void SetIsHalted(const uint8 IsHalted) { bIsHalted = IsHalted; }
	
	/** Update the Agents PathPoints for their current navigation path. */
	FORCEINLINE void UpdatePathPoints(const TArray<FNavPathPoint>& Points)
	{
		CurrentPath = Points;
	}

	/**
	 * Update a particular direction in the LatestAvoidanceResults
	 * @param InDirection The Direction of this trace result
	 * @param bInResult Whether or not the trace hit an Agent.
	 */
	FORCEINLINE void UpdateAvoidanceResult(
		const EAgentRaytraceDirection& InDirection,
		const bool bInResult)
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
			default:
				break;
		}
	}
	
	/**
	 * Update the LatestFloorCheckResult with new data.
	 * @param HitZPoint The Z axis value for this result.
	 * @param bSuccess Whether or not the Floor Check worked.
	*/
	FORCEINLINE void UpdateFloorCheckResult(
		const float HitZPoint, const bool bSuccess)
	{
		LatestFloorCheckResult.bIsValidResult = bSuccess;
		// Make sure we set it to 0.0f if the trace failed in order to overwrite the old data
		LatestFloorCheckResult.DetectedZPoint = (bSuccess) ?
			(HitZPoint) : (0.0f);
		Timers.FloorCheckTimeResultAge.Reset();
	}

	/**
	 * Update the LatestStepCheckResult with new data.
	 * @param RelativeStepHeight The height of the step relative to the Agent.
	 * @param bStepDetected Whether or not a step was detected.
	 */
	FORCEINLINE void UpdateStepCheckResult(
		const float RelativeStepHeight, const bool bStepDetected)
	{
		LatestStepCheckResult.bStepWasDetected = bStepDetected;
		// Make sure we set it to 0.0f if the trace failed in order to overwrite the old data
		LatestStepCheckResult.StepHeight = (bStepDetected) ?
			(RelativeStepHeight) : (0.0f);
		Timers.StepCheckTimeResultAge.Reset();
	}
	
	/**
	 * Update the Timer settings for each agent using the passed in DeltaTime.
	 * Won't update a timer if it has already ticked fully but hasn't been used yet,
	 * unless you use AddTimeRaw().
	 * @param DeltaTime The amount of time that has passed since the last update.
	*/
	FORCEINLINE void UpdateTimers(const float DeltaTime)
	{
		Timers.MoveTime.AddTime(DeltaTime);
		Timers.PathTime.AddTime(DeltaTime);
		Timers.TraceTime.AddTime(DeltaTime);
		Timers.FloorCheckTime.AddTime(DeltaTime);
		Timers.StepCheckTime.AddTime(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Forward.AddTimeRaw(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Right.AddTimeRaw(DeltaTime);
		Timers.AvoidanceResultAgeTimers.Left.AddTimeRaw(DeltaTime);
		Timers.FloorCheckTimeResultAge.AddTimeRaw(DeltaTime);
		Timers.StepCheckTimeResultAge.AddTimeRaw(DeltaTime);
	}

	/**
	 * TODO: Document this
	 * @param Location Location of the Agent.
	 * @param DeltaTime Time passed since last frame.
	 */
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

/**
 * TODO: Document this
 */
UCLASS(BlueprintType)
class NAI_API ANAIAgentManager : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this Agent's properties */
	ANAIAgentManager();

	/**
	 * If the Agent type is PathToPlayer, we need to know which class the player
	 * is so that we can then path to it.
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> PlayerClass;

	/** The maximum amount of Agents this manager should be allowed to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AgentManager|Agents|Limits", meta =
		(ClampMin = 0, ClampMax = 10000, UIMin = 0, UIMax = 10000))
	int MaxAgentCount;
	
protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

public:	
	/** Called every frame */
	virtual void Tick(float DeltaTime) override;
	
public:
	/**
	 * Add a new Agent to the map.
	 * We don't need to take the Guid because the Agent variable will
	 * contain a copy of it.
	 * @param Agent The new Agent to add to the map.
	 */
	void AddAgent(const FAgent& Agent);
	
	/**
	 * Remove an Agent from the map.
	 * @param Guid The Guid of the Agent to remove.
	 */
	void RemoveAgent(const FGuid& Guid);
	
	/**
	 * Overwrite the Agent in the map with a new one.
	 * We don't need to take the Guid because the Agent variable will
	 * contain a copy of it.
	 * @param Agent The new Agent were going to replace the old one with.
	 */
	void UpdateAgent(const FAgent& Agent);
	
	/**
	* Update the Agents current path with new PathPoints.
	* @param Guid The Guid of the Agent to update.
	* @param PathPoints The direction of this trace result.
	*/
	FORCEINLINE void UpdateAgentPath(
		const FGuid& Guid, const TArray<FNavPathPoint>& PathPoints)
	{
		AgentMap[Guid].UpdatePathPoints(PathPoints);
	}
	
	/**
	* Update the LatestAvoidanceResult for the given direction
	* on each Agent.
	* @param Guid The Guid of the Agent to update.
	* @param TraceDirection The direction of this trace result.
	* @param bResult Whether or not we are blocked in this direction.
	*/
	FORCEINLINE void UpdateAgentAvoidanceResult(
		const FGuid& Guid,
		const EAgentRaytraceDirection& TraceDirection, const bool bResult)
	{
		AgentMap[Guid].UpdateAvoidanceResult(TraceDirection, bResult);
	}
	
	/**
	* Update the LatestFloorCheckResult of a particular Agent.
	* @param Guid The Guid of the Agent to update.
	* @param HitZPoint The Z axis value for this result.
	* @param bSuccess Whether or not the Floor Check worked.
	*/
	FORCEINLINE void UpdateAgentFloorCheckResult(
		const FGuid& Guid, const float HitZPoint, const bool bSuccess)
	{
		AgentMap[Guid].UpdateFloorCheckResult(HitZPoint, bSuccess);
	}

	FORCEINLINE void UpdateAgentStepCheckResult(
		const FGuid& Guid, const float RelativeStepHeight, const bool bStepDetected)
	{
		AgentMap[Guid].UpdateStepCheckResult(RelativeStepHeight, bStepDetected);
	}
	
private:
	/**
	 * // TODO: Document this
	 * @param Start 
	 * @param Goal 
	 * @param NavigationProperties 
	 */
	void AgentPathTaskAsync(
		const FVector& Start,
		const FVector& Goal,
		const FAgentNavigationProperties& NavigationProperties
	) const;
	
	/**
	 * // TODO: Document this
	 * @param AgentLocation 
	 * @param Forward 
	 * @param Right 
	 * @param AvoidanceProperties 
	 */
	void AgentAvoidanceTraceTaskAsync( 
		const FVector& AgentLocation,
		const FVector& Forward,
		const FVector& Right,
		const FAgentProperties& AvoidanceProperties
	) const;
	
	// TODO: Not sure why i didn't inline this..
	/**
	 * // TODO: Document this
	 * @param AgentType 
	 * @param PlayerLocation 
	 * @return 
	 */
	FVector GetAgentGoalLocationFromType(
		const EAgentType& AgentType,
		const FVector& PlayerLocation) const;
	
	/**
	 * // TODO: Document this 
	 */
	void Initialize();
private:
	/** Used to hold a reference to the current World. */
	UPROPERTY()
	class UWorld *WorldRef;
	/** Used to hold a reference to the current Navigation System. */
	UPROPERTY()
	class UNavigationSystemV1 *NavSysRef;
	/** Used to hold a reference to the current Navigation Data. */
	UPROPERTY()
	class ANavigationData *NavDataRef;
	/** Used to hold a reference to the Shared Navigation Query. */
	FSharedConstNavQueryFilter NavQueryRef;
	
	/**
	 * Hash table containing all of the Agents.
	 * Each Agent has a guid assigned to it for identification.
	 * All Agent guids are stored in the AgentGuids array,
	 * which can be used to iterate through the entire table.
	 */
	UPROPERTY()
	TMap<FGuid, FAgent> AgentMap;
	/**
	 * A list of all the guids belonging to each Agent currently in
	 * the AgentMap hash table.
	 */
	UPROPERTY()
	TArray<FGuid> AgentGuids;
};
