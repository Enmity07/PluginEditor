// Copyright NoxxProjects and Primrose Taylor. All rights reserved.

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
enum class NAI_API EAgentAvoidanceTraceDirection : uint8
{
	TracingFront,
	TracingLeft,
	TracingRight,
};

/**
 * Specialist timer which will automatically stop when a set
 * interval of time has passed, and become "ready".
 * The timer then has to be reset to be used again.
 * @biref Specialist timer for use by the Agents.
 */
struct NAI_API FAgentTimedProperty
{
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
		if(bIsPaused || bIsReady)
			return;
		
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

	/** 
	 * Always add time regardless of the state of the timer.
	 * @param InTime The amount of time to add.
	 */
	FORCEINLINE void AddTimeRaw(const float InTime) { Time += InTime; }
};

/**
 * This serves as the base type for all the Agents tasks.
 * It's just simply a timer which need to be initialized and
 * incremented.
 */
struct NAI_API FAgentSimpleTask
{
private:
	FAgentTimedProperty TaskTimer;
public:
	/** Initialize the task with a tick rate. */
	virtual FORCEINLINE void InitializeTask(const float InTickRate)
	{
		TaskTimer.TickRate = InTickRate;
	}
	
	virtual FORCEINLINE void IncrementTimers(const float DeltaTime)
	{
		TaskTimer.AddTime(DeltaTime);
	}
	
	virtual FORCEINLINE void Reset() { TaskTimer.Reset(); }
	FORCEINLINE bool IsReady() const { return TaskTimer.bIsReady; }

	virtual ~FAgentSimpleTask() { } // Need this
};

/**
 * Simple container type for holding the Result type
 * for a given task, which has a result.
 */
template<typename TResultType>
struct NAI_API TAgentResultContainer
{
	/** How long before the Task becomes too old. */
	float Lifespan;
	/** Used to check whether or not the result has been used. */
	uint8 bIsDirty : 1;
	/** The result type which hold the actual result. */
	TResultType Result;
	/** How old the latest result is. We compare this against lifespan
	 * to figure out if the result is too old. */
	FAgentTimedProperty Age;

	TAgentResultContainer()	: Lifespan(0.0f),
		bIsDirty(false)
	{ }
};

/** Regular task type, holding a handle for access to result data. */
template<typename TResultType>
struct NAI_API TAgentTaskHandle : public FAgentSimpleTask
{
private:
	TAgentResultContainer<TResultType> ResultContainer;
	FTraceHandle TaskHandle;
	
public:
	/**
	* Initialize the task with a tick rate.
	* We also set the lifespan to be equal to the tick rate.
	* This is because we don't want to use a result that has lived
	* for longer than one whole tick.
	*/
	virtual FORCEINLINE void InitializeTask(const float InTickRate) override
	{
		FAgentSimpleTask::InitializeTask(InTickRate);
		ResultContainer.Lifespan = InTickRate;
	}
	
	virtual FORCEINLINE void IncrementTimers(const float DeltaTime) override
	{
		FAgentSimpleTask::IncrementTimers(DeltaTime);
		ResultContainer.Age.AddTimeRaw(DeltaTime);
	}

	virtual FORCEINLINE void Reset() override
	{
		FAgentSimpleTask::Reset();
		TaskHandle = FTraceHandle();
	}
	
	FORCEINLINE const TResultType& GetResult() const { return ResultContainer.Result; }
	FORCEINLINE void SetResult(const TResultType& InResult)
	{
		ResultContainer.Result = InResult;
		ResultContainer.Age.Reset();
	}
	
	FORCEINLINE const FTraceHandle& GetTraceHandle() const { return TaskHandle; }
	FORCEINLINE void SetTraceHandle(const FTraceHandle& InHandle) { TaskHandle = InHandle; }
};

/** Main task type, using a delegate for access to result data. */
template<typename TResultType, typename TDelegateType>
struct NAI_API TAgentTask : public TAgentTaskHandle<TResultType>
{
private:
	TAgentResultContainer<TResultType> ResultContainer;
	TDelegateType OnCompleteDelegate;
	
public:
	FORCEINLINE TDelegateType& GetOnCompleteDelegate() { return OnCompleteDelegate; }
};

/** Not really using this yet, might keep it.. might not.. */
template<typename TResultType, typename TDelegateType, uint8 TTaskCount = 1>
struct NAI_API TAgentMultiTask
{
	TArray<TAgentTask<TAgentResultContainer<TResultType>, TDelegateType>,
		TInlineAllocator<TTaskCount>> Tasks;

	TAgentMultiTask()
	{ }
	~TAgentMultiTask()
	{ }
	
	FORCEINLINE void InitializeTasks(
		const float InTickRates, const float InLifespans = InTickRates)
	{
		for(uint8 i = 0; i < TTaskCount; i++)
		{
			Tasks[i].InitializeTask(InTickRates, InLifespans);
		}
	}

	FORCEINLINE void IncrementTaskTimers(const float DeltaTime = 0.0f)
	{
		for(uint8 i = 0; i < TTaskCount; i++)
		{
			Tasks[i].IncrementTimers(DeltaTime);
		}
	}

	FORCEINLINE TAgentTask<TAgentResultContainer<TResultType>, TDelegateType>& GetTask(
		const uint8 Index = 0) const
	{
		/** Get the last task if the given Index is out of range */
		return !(Index > TTaskCount) ?
			Tasks[Index] : Tasks[TTaskCount];
	}

	FORCEINLINE TResultType& GetTaskResult(const uint8 Index = 0) const
	{
		/** Get the last task if the given Index is out of range */
		return !(Index > TTaskCount) ?
			Tasks[Index].GetResult() : Tasks[TTaskCount].GetResult();
	}

	FORCEINLINE TDelegateType& GetTaskOnCompleteDelegate(const uint8 Index = 0) const
	{
		/** Get the last task if the given Index is out of range */
		return !(Index > TTaskCount) ?
			Tasks[Index].GetOnCompleteDelegate() : Tasks[TTaskCount].GetOnCompleteDelegate();
	}
	
	FORCEINLINE void GetAllResultsFast(
		TArray<TResultType, TInlineAllocator<TTaskCount>>& OutResults) const
	{
		for(uint8 i = 0; i < TTaskCount; i++)
		{
			OutResults.Append(Tasks[i].GetResult());
		}
	}

	FORCEINLINE const uint8& GetTaskCount() const { return TTaskCount; }
};

struct NAI_API FAgentResultBase
{
	uint8 bIsValidResult : 1;

	/** Handle default initialization. */
	FAgentResultBase()
		: bIsValidResult(false)
	{ }
	
	FAgentResultBase(const uint8 InIsValid)
		: bIsValidResult(InIsValid)
	{ }
};

struct NAI_API FAgentPathResult : FAgentResultBase
{
	TArray<FNavPathPoint> Points;

	FAgentPathResult(const uint8 InIsValidPath, const TArray<FNavPathPoint>& InPathPoints)
		: FAgentResultBase(InIsValidPath), Points(InPathPoints)
	{ }

	FAgentPathResult() { } // Need this
};

struct NAI_API FAgentTraceResult : FAgentResultBase
{
	/** Vector that represents  the hit location if we hit one */
	FVector DetectedHitLocation;

	/** Handle default initialization. */
	FAgentTraceResult() : DetectedHitLocation(FVector())
	{ }
	
	FAgentTraceResult(const uint8 InIsValid, const FVector& InLocation) : FAgentResultBase(InIsValid),
		DetectedHitLocation(InLocation)
	{ }
	
	FORCEINLINE bool IsBlocked() const { return bIsValidResult; }
	FORCEINLINE const float& GetHitZValue() const { return DetectedHitLocation.Z; }
};

/**/
struct NAI_API FAgentLocalBoundsCheckResult : FAgentResultBase
{
private:
	FVector HighestHitPoint;

	uint8 bHasMultipleHits : 1;
	TArray<FVector> HitPoints;
public:
	/** Handle default initialization. */
	FAgentLocalBoundsCheckResult(
		const uint8 InIsValid = false,
		const FVector& InHighestHit = FVector::ZeroVector,
		const uint8 InHasMultipleHits = false,
		const TArray<FVector>& InHitPoints = TArray<FVector>())
	:
		FAgentResultBase(InIsValid),
		HighestHitPoint(InHighestHit),
		bHasMultipleHits(InHasMultipleHits),
		HitPoints(InHitPoints)
	{ }

	FORCEINLINE void Reset() { FAgentLocalBoundsCheckResult(); }
	
	FORCEINLINE const FVector& GetHighestHitPoint() const { return HighestHitPoint; }
	FORCEINLINE void SetHighestHitPoint(const FVector& InHighestHit) { HighestHitPoint = InHighestHit; }

	FORCEINLINE bool GetHasMultipleHits() const { return bHasMultipleHits; }
	FORCEINLINE void SetHasMultipleHits(const uint8& InHasMultipleHits) { bHasMultipleHits = InHasMultipleHits; }

	FORCEINLINE const TArray<FVector>& GetHitPoints() const { return HitPoints; }
	FORCEINLINE void SetHitPointByIndex(const uint32 Index = 0, const FVector& InHitPoint = FVector::ZeroVector) { HitPoints[Index] = InHitPoint; }
	FORCEINLINE void SetHitPoints(const TArray<FVector>& InHitPoints) { HitPoints = InHitPoints; }
};

// TODO: Get rid of this mess by making everything proportional
// TODO: Actually preform encapsulation here
// to the agent's size
#define NORMAL_AVOIDANCE_COLUMNS		3
#define ADVANCED_AVOIDANCE_COLUMNS		5
#define AVOIDANCE_SIDE_COLUMNS			1
#define NORMAL_AVOIDANCE_ROWS			3
#define ADVANCED_AVOIDANCE_ROWS			5
#define AVOIDANCE_WIDTH_MULTIPLIER		1.5f
#define AVOIDANCE_HEIGHT_MULTIPLIER		0.8f

struct NAI_API FAgentAvoidanceProperties
{
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

	/** Handle default initialization. */
	FAgentAvoidanceProperties() : AvoidanceLevel(EAgentAvoidanceLevel::Advanced),
		GridWidth(0.0f),
		GridHalfWidth(0.0f),
		GridHeight(0.0f),
		GridHalfHeight(0.0f),
		GridColumns(0),
		GridRows(0),
		SideColumns(0),
		SideRows(0),
		WidthIncrementSize(0.0f),
		SideWidthIncrementSize(0.0f),
		HeightIncrementSize(0.0f),
		StartOffsetWidth(0.0f),
		SideStartOffsetWidth(0.0f),
		StartOffsetHeightVector(FVector::ZeroVector),
		SideStartOffsetHeightVector(FVector::ZeroVector)
	{ }

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

		StartOffsetWidth = WidthIncrementSize * ((GridColumns - 1) * 0.5f);
		SideStartOffsetWidth = WidthIncrementSize * ((SideColumns - 1) * 0.5f);
		StartOffsetHeightVector = FVector::UpVector * (HeightIncrementSize * ((GridRows - 1) * 0.5f));
		SideStartOffsetHeightVector = FVector::UpVector * (HeightIncrementSize * ((SideRows - 1) * 0.5f));
	}
};

#undef NORMAL_AVOIDANCE_COLUMNS
#undef ADVANCED_AVOIDANCE_COLUMNS
#undef AVOIDANCE_SIDE_COLUMNS
#undef NORMAL_AVOIDANCE_ROWS
#undef ADVANCED_AVOIDANCE_ROWS
#undef AVOIDANCE_WIDTH_MULTIPLIER

struct NAI_API FAgentVirtualCapsuleSweepProperties
{
	float Radius;
	float HalfHeight;

	FCollisionShape VirtualCapsule;

	/** Handle default initialization. */
	FAgentVirtualCapsuleSweepProperties() : Radius(0.0f),
		HalfHeight(0.0f)
	{ }

	FORCEINLINE void InitializeOrUpdate(
		const float AgentCapsuleRadius, const float AgentCapsuleHalfHeight
	)
	{
		Radius = AgentCapsuleRadius + 5.0f;
		HalfHeight = AgentCapsuleHalfHeight + 5.0f;

		VirtualCapsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	}
};

struct NAI_API FAgentStepCheckProperties
{
	float ForwardOffset;
	float DownwardOffset;

	/** Handle default initialization. */
	FAgentStepCheckProperties() : ForwardOffset(0.0f),
		DownwardOffset(0.0f)
	{ }

	FORCEINLINE void Initialize(
		const float InRadius,
		const float InHalfHeight,
		const float InMaxStepHeight)
	{
		ForwardOffset = (InRadius + 1.0f);
		DownwardOffset = InHalfHeight - (InMaxStepHeight * 2.0f);
	}
};

struct NAI_API FAgentNavigationProperties
{
	FNavAgentProperties NavAgentProperties;

	FAgentVirtualCapsuleSweepProperties LocalBoundsCheckProperties;
	
	/** Sub-structure containing the Agent's Step Navigation properties. */
	FAgentStepCheckProperties StepProperties;

	/** Handle default initialization. */
	FAgentNavigationProperties() : NavAgentProperties(FNavAgentProperties()),
		LocalBoundsCheckProperties(FAgentVirtualCapsuleSweepProperties()),
		StepProperties(FAgentStepCheckProperties())
	{ }
};

struct NAI_API FAgentProperties
{
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
	
	/** Sub-structure containing the Agent's Navigation properties. */
	FAgentNavigationProperties NavigationProperties;
	/** Sub-structure containing the Agent's Avoidance properties. */
	FAgentAvoidanceProperties AvoidanceProperties;

	/** Handle default initialization. */
	FAgentProperties() : AgentType(EAgentType::PathToPlayer),
		CapsuleRadius(0.0f),
		CapsuleHalfHeight(0.0f),
		MoveSpeed(0.0f),
		LookAtRotationRate(0.0f),
		MaxStepHeight(0.0f),
		NavigationProperties(FAgentNavigationProperties()),
		AvoidanceProperties(FAgentAvoidanceProperties())
	{ }
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

	/** Agent Tasks */
	TAgentTask<FAgentPathResult, FNavPathQueryDelegate> PathTask;
	TAgentTask<FAgentTraceResult, FTraceDelegate> AvoidanceFrontTask;
	TAgentTask<FAgentTraceResult, FTraceDelegate> AvoidanceRightTask;
	TAgentTask<FAgentTraceResult, FTraceDelegate> AvoidanceLeftTask;
	TAgentTask<FAgentTraceResult, FTraceDelegate> FloorCheckTask;
	TAgentTask<FAgentTraceResult, FTraceDelegate> StepCheckTask;
	TAgentTask<FAgentLocalBoundsCheckResult, FTraceDelegate> LocalBoundsCheckTask;

	/** Simple Tasks. These don't have a result output. */
	FAgentSimpleTask MoveTask;

	// Local copy of the agents current velocity Vector
	float Speed;
	FVector Velocity;
private:
	FVector PositionThisFrame;
	FVector PositionLastFrame;
	
	/**
	* Whether or not this agent is halted. If the Agent is halted,
	* it will still have it's Timers and Velocity updated.
	*/
	uint8 bIsHalted : 1;
	
public:
	/** Handle default initialization. */
	FAgent() : AgentClient(nullptr),
		AgentManager(nullptr),
		AgentProperties(FAgentProperties()),
		Speed(0.0f),
		bIsHalted(false)
	{ }

	/** Set the Velocity of this Agent. */
	FORCEINLINE void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }

	/** Get whether or not the Agent is halted. */
	FORCEINLINE bool IsHalted() { return bIsHalted; }
	
	/** Set the Agent to either be halted, or not. */
	FORCEINLINE void SetIsHalted(const uint8 IsHalted) { bIsHalted = IsHalted; }
	
	/** Update the Agents PathPoints for their current navigation path. */
	FORCEINLINE void UpdatePathTaskResults(const FAgentPathResult& InResult)
	{
		PathTask.SetResult(InResult);
	}

	/**
	 * Update a particular direction in the LatestAvoidanceResults
	 * @param InDirection The Direction of this trace result
	 * @param bInResult Whether or not the trace hit an Agent.
	 */
	FORCEINLINE void UpdateAvoidanceResult(
		const EAgentAvoidanceTraceDirection& InDirection,
		const bool bInResult)
	{
		const FAgentTraceResult NewResult = FAgentTraceResult(
			bInResult, FVector::ZeroVector);
	
		switch(InDirection)
		{
			case EAgentAvoidanceTraceDirection::TracingFront:
				AvoidanceFrontTask.SetResult(NewResult);
				break;
			case EAgentAvoidanceTraceDirection::TracingRight:
				AvoidanceRightTask.SetResult(NewResult);
				break;
			case EAgentAvoidanceTraceDirection::TracingLeft:
				AvoidanceLeftTask.SetResult(NewResult);
				break;
			default:
				break;
		}
	}
	
	/**
	 * Update the LatestFloorCheckResult with new data.
	 * @param HitLocation The Z axis value for this result.
	 * @param bSuccess Whether or not the Floor Check worked.
	*/
	FORCEINLINE void UpdateFloorCheckResult(
		const FVector& HitLocation, const bool bSuccess)
	{
		// Make sure we set it to 0.0f if the trace failed in order to overwrite the old data
		const FAgentTraceResult NewResult = FAgentTraceResult(
			bSuccess,
			(bSuccess) ? (HitLocation) : (FVector::ZeroVector)
		);
		
		FloorCheckTask.SetResult(NewResult);
	}

	/**
	 * Update the LatestStepCheckResult with new data.
	 * @param HitLocation The height of the step relative to the Agent.
	 * @param bStepDetected Whether or not a step was detected.
	 */
	FORCEINLINE void UpdateStepCheckResult(
		const FVector& HitLocation, const bool bStepDetected)
	{
		// Make sure we set it to 0.0f if the trace failed in order to overwrite the old data
		const FAgentTraceResult NewResult = FAgentTraceResult(
			bStepDetected,
			(bStepDetected) ? (HitLocation) : (FVector::ZeroVector)
		);
		
		StepCheckTask.SetResult(NewResult);
	}

	FORCEINLINE void UpdateLocalBoundsCheckResult(
		const FAgentLocalBoundsCheckResult& InResult = FAgentLocalBoundsCheckResult())
	{
		LocalBoundsCheckTask.SetResult(InResult);
	}
	
	/**
	 * Update the Timer settings for each agent using the passed in DeltaTime.
	 * Won't update a timer if it has already ticked fully but hasn't been used yet,
	 * unless you use AddTimeRaw().
	 * @param DeltaTime The amount of time that has passed since the last update.
	*/
	FORCEINLINE void UpdateTimers(const float DeltaTime)
	{
		PathTask.IncrementTimers(DeltaTime);
		AvoidanceFrontTask.IncrementTimers(DeltaTime);
		AvoidanceRightTask.IncrementTimers(DeltaTime);
		AvoidanceLeftTask.IncrementTimers(DeltaTime);
		FloorCheckTask.IncrementTimers(DeltaTime);
		StepCheckTask.IncrementTimers(DeltaTime);

		LocalBoundsCheckTask.IncrementTimers(DeltaTime);
		
		MoveTask.IncrementTimers(DeltaTime);
	}

	/**
	 * Update the Speed of the AgentClient,
	 * this is so users can easily check the speed of the Agent.
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
 * This is the AgentManager. It serves as the "Brain"
 * behind each and every AI, or "Agent". All movement,
 * related tasks, like pathfinding, then following that path,
 * then local avoidance, stepping up/down ledges/stairs, and
 * anything else related to moving and Agent. This avoids the
 * need for a Controller, and it prevents having loads of AI's
 * which all each implement their own Tick() function. Having
 * everything be done in one object like this, significantly
 * improves performance.
 *
 * Everything in here is designed to be as tight as possible
 * when it comes to memory usage. We don't really care about
 * the amount of memory used since it's tiny anyway, but we do
 * care about avoiding references and the heap as much as possible
 * so we can abuse the stack for the speed benefits. 
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

	/** Called when the game end or when destroyed. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	FORCEINLINE void AddAgent(const FAgent& Agent)
	{
		AgentMap.Add(Agent.Guid, Agent);
		AgentGuids.AddUnique(Agent.Guid);
	}
	
	/**
	 * Remove an Agent from the map.
	 * @param Guid The Guid of the Agent to remove.
	 */
	FORCEINLINE void RemoveAgent(const FGuid& Guid)
	{
		AgentMap.Remove(Guid);
		const uint32 Index = AgentGuids.IndexOfByKey(Guid);
		AgentGuids.RemoveAtSwap(Index);
	}
	
	/**
	 * Overwrite the Agent in the map with a new one.
	 * We don't need to take the Guid because the Agent variable will
	 * contain a copy of it.
	 * @param Agent The new Agent were going to replace the old one with.
	 */
	FORCEINLINE void UpdateAgent(const FAgent& Agent)
	{
		// The Add() function for the TMap replaces copies
		// So if the Agent guid already exists in the map it will
		// get overwritten with this new one
		AgentMap.Add(Agent.Guid, Agent);
	}
	
	/**
	* Update the Agents current path with a new one.
	* @param Guid The Guid of the Agent to update.
	* @param InResult The new AgentPath result.
	*/
	FORCEINLINE void UpdateAgentPathResult(
		const FGuid& Guid, const FAgentPathResult& InResult)
	{
		AgentMap[Guid].UpdatePathTaskResults(InResult);
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
		const EAgentAvoidanceTraceDirection& TraceDirection, const bool bResult)
	{
		AgentMap[Guid].UpdateAvoidanceResult(TraceDirection, bResult);
	}
	
	/**
	* Update the LatestFloorCheckResult of a particular Agent.
	* @param Guid The Guid of the Agent to update.
	* @param HitLocation The location for this result.
	* @param bSuccess Whether or not the Floor Check worked.
	*/
	FORCEINLINE void UpdateAgentFloorCheckResult(
		const FGuid& Guid, const FVector& HitLocation, const bool bSuccess)
	{
		AgentMap[Guid].UpdateFloorCheckResult(HitLocation, bSuccess);
	}

	/**
	 * Update the result for the Step Check Task. 
	 * @param Guid The Guid of the Agent this update is for.
	 * @param HitLocation The location the trace hit.
	 * @param bStepDetected Whether or not a hit location was detected.
	 */
	FORCEINLINE void UpdateAgentStepCheckResult(
		const FGuid& Guid, const FVector& HitLocation, const bool bStepDetected)
	{
		AgentMap[Guid].UpdateStepCheckResult(HitLocation, bStepDetected);
	}
	
	/**
	 * @brief 
	 * @param PathId 
	 * @param ResultType 
	 * @param NavPointer 
	 * @param Guid 
	 */
	void OnAsyncPathComplete(
		uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer,
		FGuid Guid
	);
	
	/**
	 * @brief 
	 * @param Handle 
	 * @param Data
	 * @param Guid 
	 * @param Direction 
	 */
	void OnAvoidanceTraceComplete(
		const FTraceHandle& Handle, FTraceDatum& Data,
		FGuid Guid, EAgentAvoidanceTraceDirection Direction
	);

	/**
	 * @brief 
	 * @param Handle 
	 * @param Data 
	 * @param Guid 
	 */
	void OnFloorCheckTraceComplete(
		const FTraceHandle& Handle, FTraceDatum& Data,
		FGuid Guid
	);
	
	/**
	 * @brief 
	 * @param Handle 
	 * @param Data 
	 * @param Guid 
	 */
	void OnStepCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data, FGuid Guid);

	/**
	* @brief 
	* @param Handle 
	* @param Data 
	* @param Guid 
	*/
	void OnLocalBoundsCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data, FGuid Guid);

private:
	/**
	* @brief 
	* @param Objects 
	* @param Guid 
	* @return 
	*/
	bool CheckIfBlockedByAgent(const TArray<FHitResult>& Objects, const FGuid& Guid) const;
	
	/**
	 * @brief 
	 * @param HitResults 
	 * @return  
	 */
	TArray<FVector> GetAllHitLocationsNotFromAgents(const TArray<FHitResult>& HitResults) const;
	
private:
	/**
	 * // TODO: Document this
	 * @param Start The location this Pathfinding task starts from.
	 * @param Goal The location this Pathfinding task is to.
	 * @param NavAgentProperties The properties from this Agent related to the NavMesh settings.
	 * @param PathDelegate The delegate to be executed on completion of this task.
	 */
	void AgentPathTaskAsync(
		const FVector& Start,
		const FVector& Goal,
		const FNavAgentProperties& NavAgentProperties,
		const FNavPathQueryDelegate& PathDelegate
	) const;
	
	/**
	 * // TODO: Document this
	 * @param TraceDirection The Direction or Side of the agent this Trace is for.
	 * @param AgentLocation The location of the Agent.
	 * @param Forward The forward vector of the Agent.
	 * @param Right The right vector of the Agent.
	 * @param AgentProperties The properties we need from the Agent for this Task.
	 * @param TraceDirectionDelegate The delegate to be executed on completion of the Task.
	 */
	void AgentAvoidanceTraceTaskAsync(
		const EAgentAvoidanceTraceDirection& TraceDirection,
		const FVector& AgentLocation,
		const FVector& Forward,
		const FVector& Right,
		const FAgentProperties& AgentProperties,
		FTraceDelegate& TraceDirectionDelegate
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
	 * Initialize all the pointers we need.
	 * This includes things like the UWorld ref and
	 * the Nav System.. etc...
	 */
	void Initialize();
	
private:
	/** Used to hold a reference to the current World. */
	UPROPERTY()
	class UWorld *WorldRef;
	/** Used to hold a reference to the current Navigation System.  */
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

/**
 * Static utility class used to get a reference to the
 * active AgentManager, if one exists.
 * @note ONLY USE THIS AT RUNTIME
 * @param bManagerExists This will be true is a manager is active.
 * @param CurrentManager A pointer to the current manager.
 */
class UAgentManagerStatics
{
public:
	/** Variable used to check the if a manager reference exists. */
	static bool bManagerExists;

	/** Function used to set the CurrentManager pointer. */
	static void SetManagerReference(ANAIAgentManager* InManager);

	/** Return a reference to the current manager. */
	static ANAIAgentManager* GetManagerReference();

	/** Reset the class, remove the reference to the manager. */
	static void Reset();

private:
	/** Pointer to the current manager. */
	static ANAIAgentManager* CurrentManager;
};
