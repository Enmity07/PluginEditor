// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentManager.h"
#include "NAIAgentClient.h"
#include "Engine/CollisionProfile.h"

#include "Kismet/KismetMathLibrary.h"

#define NULL_VECTOR FVector(125.0f, 420.0f, 317.4f)
#define MAX_AGENT_PRE_ALLOC 1024

// Sets default values
ANAIAgentManager::ANAIAgentManager()
{
	MaxAgentCount = MAX_AGENT_PRE_ALLOC;
	
	WorldRef = nullptr;
	NavSysRef = nullptr;
	NavDataRef = nullptr;
	 
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANAIAgentManager::BeginPlay()
{
	Super::BeginPlay();

	AgentMap.Reserve(MAX_AGENT_PRE_ALLOC);
}

#undef MAX_AGENT_PRE_ALLOC

// Called every frame
void ANAIAgentManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//TArray<TSharedPtr<FName>> ProfileNames;
	//UCollisionProfile::GetProfileNames(ProfileNames);
	//if(ProfileNames.Num() > 0)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("---Start---"));
	//	for(int i = 0; i < ProfileNames.Num(); i++)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Name: %s"), *ProfileNames[i].Get()->ToString());
	//	}
	//	UE_LOG(LogTemp, Warning, TEXT("---End---"));
	//}

	
	if(WorldRef)
	{
		const int AgentCount = AgentMap.Num();
		if(AgentCount > 0)
		{
			for(int i = 0; i < AgentCount; i++)
			{
				// Get the Guid and use it to get a copy of the agent
				const FGuid Guid = AgentGuids[i];
				FAgent Agent = AgentMap[Guid];

				// Update all the agents timers for their tasks
				Agent.UpdateTimers(DeltaTime);

				// Calculate the Agents Speed and update the property on the AgentClient object
				const FVector AgentLocation = Agent.AgentClient->GetActorLocation();
				Agent.CalculateAndUpdateSpeed(AgentLocation, DeltaTime);
		
		        if(Agent.bIsHalted)
		        {
		            continue;
		        }

				// Kick of an Async Path Task if the agent is ready for one
				if(Agent.Timers.PathTime.bIsReady)
				{
					// Get the Goal Location from the agent type
					// the pathfinding can be radically different between types
					// so GetAgentGoalLocationFromType() determines what vector we need
					// for the async pathfinding task
					const FVector PlayerLocation = FVector(AgentLocation.X + 500.0f, AgentLocation.Y, AgentLocation.Z); // TODO: GET THE PLAYER!!!
					const EAgentType AgentType = Agent.AgentProperties.AgentType;
					const FVector GoalLocation = GetAgentGoalLocationFromType(AgentType, PlayerLocation);
					
					AgentPathTaskAsync(Guid, GoalLocation, AgentLocation); // start the task
					Agent.Timers.PathTime.Reset(); // Reset the timer
				}

				// Kick off an Async Trace Task if the agent is ready for one
				if(Agent.Timers.TraceTime.bIsReady)
				{
					AgentTraceTaskAsync(
						Guid,
						AgentLocation,
						Agent.AgentClient->GetActorForwardVector(),
						Agent.AgentClient->GetActorRightVector(),
						Agent.AgentProperties
					);
					
					Agent.Timers.TraceTime.Reset();
				}
				
				// Handle the movement/rotation of the agent for this frame
				if(Agent.Timers.MoveTime.bIsReady)
				{
					// No reason to move if we don't have a path
					if(Agent.CurrentPath.Num() > 1) // Need more than 1 path point to for it to be a path
					{
						const FVector Start = Agent.CurrentPath[0];
						const FVector End = Agent.CurrentPath[1];
						// Get the direction in a normalized format
						const FVector Direction = UKismetMathLibrary::GetDirectionUnitVector(Start, End);
						const FVector NewLoc = (AgentLocation + (Direction * (Agent.AgentProperties.MoveSpeed * DeltaTime)));
						// Set the new location and sweep for collision
						//Agent.AgentClient->SetActorLocation(NewLoc, true);

						// Turn the agent to face the direction it's moving in
						const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(AgentLocation, NewLoc);
						//Agent.AgentClient->SetActorRotation(
						//	FMath::Lerp(
						//		Agent.AgentClient->GetActorRotation(),
						//		LookAtRotation,
						//		Agent.AgentProperties.LookAtRotationRate
						//	)
						//);
					}
					
					Agent.Timers.MoveTime.Reset();
				}
			}
		}
	}
	else
	{
		Initialize();
	}
}

void ANAIAgentManager::AddAgent(const FAgent& Agent)
{	
	AgentMap.Add(Agent.Guid, Agent);
	AgentGuids.AddUnique(Agent.Guid);
}

void ANAIAgentManager::RemoveAgent(const FGuid& Guid)
{		
	AgentMap.Remove(Guid);
	AgentGuids.Remove(Guid);
}

void ANAIAgentManager::UpdateAgent(const FAgent& Agent)
{
	// The Add() function for the TMap replaces copies
	// So if the Agent guid already exists in the map it will
	// get overwritten with this new one
	AgentMap.Add(Agent.Guid, Agent);
}

void ANAIAgentManager::UpdateAgentPath(const FGuid& Guid, const TArray<FNavPathPoint>& PathPoints)
{
	AgentMap[Guid].UpdatePathPoints(PathPoints);
}

void ANAIAgentManager::AgentPathTaskAsync(const FGuid& Guid, const FVector& Location, const FVector& Start) const
{
	FPathFindingQuery PathfindingQuery;
	PathfindingQuery.EndLocation = Location;
	PathfindingQuery.StartLocation = Start;
	PathfindingQuery.QueryFilter = NavQueryRef;
	PathfindingQuery.NavData = NavDataRef;

	NavSysRef->FindPathAsync(
		AgentMap[Guid].NavAgentProperties,
		PathfindingQuery,
		AgentMap[Guid].NavPathQueryDelegate,
		EPathFindingMode::Regular
	);
}

void ANAIAgentManager::AgentTraceTaskAsync(
	const FGuid& Guid, const FVector& AgentLocation, const FVector& Forward, const FVector& Right,
	const FAgentProperties& AgentProperties) const
{
	// Lambda for each trace to avoid creating another function
	// And we can capture the variables passed to this function by reference
	// so no need to pass them trough to another function, which would be a mess
	auto DoTraceTask = [&](const EAgentRaytraceDirection& TraceDirectionType)
	{
		FTraceDelegate TraceDirectionDelegate;
		FVector RelativeForward;
		FVector RelativeRight;

		const uint8 Columns = (TraceDirectionType == EAgentRaytraceDirection::TracingFront) ?
			(AgentProperties.AvoidanceProperties.GridColumns) :
			(AgentProperties.AvoidanceProperties.SideColumns);

		const uint8 Rows = (TraceDirectionType == EAgentRaytraceDirection::TracingFront) ?
			(AgentProperties.AvoidanceProperties.GridRows) :
			(AgentProperties.AvoidanceProperties.SideRows)
		;
		
		const float WidthIncrementSize =
			(TraceDirectionType == EAgentRaytraceDirection::TracingFront) ?
				(AgentProperties.AvoidanceProperties.WidthIncrementSize) :
				(AgentProperties.AvoidanceProperties.SideWidthIncrementSize)
		;
		
		const float HeightIncrementSize = AgentProperties.AvoidanceProperties.HeightIncrementSize;
		
		const float StartOffsetWidth =
			(TraceDirectionType == EAgentRaytraceDirection::TracingFront) ?
				(AgentProperties.AvoidanceProperties.StartOffsetWidth) :
				(AgentProperties.AvoidanceProperties.SideStartOffsetWidth)
		;

		// We don't need to calculate the Height every time like the width,
		// since the "up" vector is always the same amount in the Z axis,
		// so the height is precalculated in the Initialize() function of the
		// FAgentAvoidanceProperties struct in NAIAgentManager.h
		const FVector StartOffsetHeightVector =
			(TraceDirectionType == EAgentRaytraceDirection::TracingFront) ?
				(AgentProperties.AvoidanceProperties.StartOffsetHeightVector) :
				(AgentProperties.AvoidanceProperties.SideStartOffsetHeightVector)
		;
		
		switch(TraceDirectionType)
		{
			case EAgentRaytraceDirection::TracingFront:
				
				TraceDirectionDelegate = AgentProperties.RaytraceFrontDelegate;
				RelativeForward = Forward;
				RelativeRight = Right;
				break;
			case EAgentRaytraceDirection::TracingLeft:
				TraceDirectionDelegate = AgentProperties.RaytraceLeftDelegate;
				RelativeForward = -Right;
				RelativeRight = -Forward;
				break;
			case EAgentRaytraceDirection::TracingRight:
				TraceDirectionDelegate = AgentProperties.RaytraceRightDelegate;
				RelativeForward = Right;
				RelativeRight = Forward;
				break;
		}

		const FVector ForwardOffset = RelativeForward * (AgentProperties.CapsuleRadius + 1.0f);
		const FVector RightOffset = RelativeRight * StartOffsetWidth;
		const FVector StartingPoint = (
			AgentLocation +
				(RelativeForward * (AgentProperties.CapsuleRadius + 1.0f)) +
				(RelativeRight * StartOffsetWidth) +
				StartOffsetHeightVector
		);

		for(uint8 i = 0; i < Rows; i++) // Horizontal Traces
		{
			for(uint8 j = 0; j < Columns; j++) // Vertical Traces
			{
				const FVector CurrentStartPoint = StartingPoint +
					((-RelativeRight * WidthIncrementSize) * j) +
					((FVector(0.0f, 0.0f, -1.0f) * HeightIncrementSize) * i);
				const FVector CurrentEndPoint = CurrentStartPoint + (RelativeForward * 50.0f);

				WorldRef->AsyncLineTraceByChannel(
					EAsyncTraceType::Single, CurrentStartPoint, CurrentEndPoint, ECollisionChannel::ECC_Visibility,
					FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam,
					&TraceDirectionDelegate
				);
			}
		}
		
	};

	DoTraceTask(EAgentRaytraceDirection::TracingFront);
	DoTraceTask(EAgentRaytraceDirection::TracingLeft);
	DoTraceTask(EAgentRaytraceDirection::TracingRight);
}

FVector ANAIAgentManager::GetAgentGoalLocationFromType(const EAgentType& AgentType, const FVector& PlayerLocation) const
{
	switch(AgentType)
	{
		case EAgentType::PathToPlayer:
			return PlayerLocation;
		case EAgentType::PathToLocation:
			return NULL_VECTOR;
		case EAgentType::Static:
			return NULL_VECTOR;
		case EAgentType::Dynamic:
			return NULL_VECTOR;
		case EAgentType::FollowCustomPath:
			return NULL_VECTOR;
	}
	// We only get here if the AgentType was null 
	return NULL_VECTOR;
}

#undef NULL_VECTOR

void ANAIAgentManager::Initialize()
{
	if(WorldRef == nullptr)
		WorldRef = GetWorld();
	if(WorldRef)
	{
		if(NavSysRef == nullptr)
		{
			NavSysRef = UNavigationSystemV1::GetCurrent(WorldRef);
			if(NavDataRef == nullptr && NavSysRef)
			{
				NavDataRef = NavSysRef->MainNavData;
				if(NavQueryRef == nullptr && NavDataRef)
				{
					NavQueryRef.operator=(UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavDataRef));
				}
			}	
		}	
	}
}
