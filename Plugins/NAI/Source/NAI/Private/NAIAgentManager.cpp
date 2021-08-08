// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentManager.h"

#include "DrawDebugHelpers.h"
#include "NAIAgentClient.h"
#include "Async/Async.h"
#include "Kismet/KismetMathLibrary.h"

#define NULL_VECTOR FVector(125.0f, 420.0f, 317.4f)
#define MAX_AGENT_PRE_ALLOC 1024

ANAIAgentManager::ANAIAgentManager()
{
	MaxAgentCount = MAX_AGENT_PRE_ALLOC;
	
	WorldRef = nullptr;
	NavSysRef = nullptr;
	NavDataRef = nullptr;
	 
	PrimaryActorTick.bCanEverTick = true;
}

void ANAIAgentManager::BeginPlay()
{
	Super::BeginPlay();

	AgentMap.Reserve(MAX_AGENT_PRE_ALLOC);
}

#undef MAX_AGENT_PRE_ALLOC

/**
 * If this is enabled with a lot of agents active in the game it will crash the editor,
 * so use it with caution.
*/
#define ENABLE_DEBUG_DRAW_LINE true

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

				// If the Agent has been set to stop, don't do anything
		        if(Agent.bIsHalted)
		        {
		            continue;
		        }

				// Kick of an Async Path Task if the agent is ready for one
				if(Agent.Timers.PathTime.bIsReady)
				{
					// Get the Goal Location from the Agent type
					// the pathfinding can be radically different between types
					// so GetAgentGoalLocationFromType() determines what vector we need
					// for the async pathfinding task
					const FVector PlayerLocation = GetActorLocation(); // TODO: GET THE PLAYER!!!
					const EAgentType AgentType = Agent.AgentProperties.AgentType;
					const FVector GoalLocation = GetAgentGoalLocationFromType(AgentType, PlayerLocation);
					
					AgentPathTaskAsync(AgentLocation, GoalLocation, Agent.AgentProperties.NavigationProperties); // start the task
					Agent.Timers.PathTime.Reset(); // Reset the timer
				}

				const FVector AgentForward = Agent.AgentClient->GetActorForwardVector();
				const FVector AgentRight = Agent.AgentClient->GetActorRightVector();
				
				// Kick off an Async Trace Task if the agent is ready for one
				if(Agent.Timers.TraceTime.bIsReady)
				{
					AgentAvoidanceTraceTaskAsync(
						AgentLocation,
						AgentForward,
						AgentRight,
						Agent.AgentProperties
					);
					
					Agent.Timers.TraceTime.Reset();
				}
				
				if(Agent.Timers.FloorCheckTime.bIsReady)
				{
					const FVector StartPoint = FVector(
						AgentLocation.X, AgentLocation.Y,
						(AgentLocation.Z - (Agent.AgentProperties.CapsuleHalfHeight + 1.0f)));
					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);

					
					WorldRef->AsyncLineTraceByChannel(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ECollisionChannel::ECC_Visibility,
						FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam,
						&Agent.AgentProperties.NavigationProperties.FloorCheckTraceDelegate
					);
					
					Agent.Timers.FloorCheckTime.Reset();
				}

				if(Agent.Timers.StepCheckTime.bIsReady)
				{
					const FVector StartPoint =
						AgentLocation +
							(AgentForward * Agent.AgentProperties.NavigationProperties.StepProperties.ForwardOffset) +
							(FVector(0.0f, 0.0f, -1.0f) * Agent.AgentProperties.NavigationProperties.StepProperties.DownwardOffset);

					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);

					FCollisionObjectQueryParams ObjectQueryParams;
					ObjectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);
					
					WorldRef->AsyncLineTraceByObjectType(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ECollisionChannel::ECC_WorldStatic,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent.AgentProperties.NavigationProperties.StepCheckTraceDelegate
					);
						
					Agent.Timers.StepCheckTime.Reset();
				}
				
				// Handle the movement/rotation of the agent only if it is ready for one
				if(Agent.Timers.MoveTime.bIsReady)
				{
					// No reason to move if we don't have a path
					if(Agent.CurrentPath.Num() > 1) // Need at least 2 path points for it to be a path
					{			
						// Calculate the new FVector for this move update
						const FVector Start = Agent.CurrentPath[0];
						const FVector End = Agent.CurrentPath[1];
#if (ENABLE_DEBUG_DRAW_LINE)
						DrawDebugLine(WorldRef, Start, End, FColor(0, 255, 0),
                        	false, 2, 0, 2.0f);
#endif
						// Get the direction in a normalized format
						const FVector Direction = (End - Start).GetSafeNormal();

						FVector NewLoc = (AgentLocation + (Direction * (Agent.AgentProperties.MoveSpeed * DeltaTime)));

						/**
						 * We need to check the step before the floor, since if there is a step
						 * we want to adjust our height for that. But if we don't have a step detected
						 * then we can just use the floor height.
						 */
						if(Agent.LatestStepCheckResult.bStepWasDetected)
						{
							NewLoc.Z = (Agent.LatestStepCheckResult.StepHeight +
								(Agent.AgentProperties.CapsuleHalfHeight + 5.0f)); // an offset is applied to avoid the agent getting stuck on the floor
						}
						else if(Agent.LatestFloorCheckResult.bIsValidResult)
						{			
							NewLoc.Z = (Agent.LatestFloorCheckResult.DetectedZPoint +
								(Agent.AgentProperties.CapsuleHalfHeight + 5.0f)); // an offset is applied to avoid the agent getting stuck on the floor
						}

						// Calculate the difference in movement
						const FVector MoveDelta = NewLoc - AgentLocation;

						// Calculate our new rotation
						FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(AgentLocation, NewLoc);
						LookAtRotation.Roll	= 0.0f; LookAtRotation.Pitch = 0.0f;
						const FRotator LerpRotation = FMath::Lerp(Agent.AgentClient->GetActorRotation(),
							LookAtRotation, Agent.AgentProperties.LookAtRotationRate
						);

						// We directly move the agent by moving it's root component as it avoids a shit load
						// of unnecessary wrapper functions, along with extra GetActorLocation() function calls
						// when we already have the agents location in this scope, before we finally get to this function
						// so we're directly calling it here instead of SetActorLocation() / SetActorRotation()
						Agent.AgentClient->GetRootComponent()->MoveComponent(MoveDelta, LerpRotation, true);
					}
					
					Agent.Timers.MoveTime.Reset();
				}
				
				// Update the agent in the map to apply the changes we've made this tick
				AgentMap[Guid] = Agent;
			}
		}
	}
	else
	{
		Initialize();
	}
}

#undef ENABLE_DEBUG_DRAW_LINE

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

void ANAIAgentManager::AgentPathTaskAsync(const FVector& Start, const FVector& Goal,
                                          const FAgentNavigationProperties& NavigationProperties) const
{
	FPathFindingQuery PathfindingQuery;
	PathfindingQuery.StartLocation = Start;
	PathfindingQuery.EndLocation = Goal;
	PathfindingQuery.QueryFilter = NavQueryRef;
	PathfindingQuery.NavData = NavDataRef;
	
	NavSysRef->FindPathAsync(
		NavigationProperties.NavAgentProperties,
		PathfindingQuery,
		NavigationProperties.NavPathQueryDelegate,
		EPathFindingMode::Regular
	);
}

void ANAIAgentManager::AgentAvoidanceTraceTaskAsync(
	const FVector& AgentLocation, const FVector& Forward, const FVector& Right,
	const FAgentProperties& AgentProperties) const
{
	// Lambda for each trace to avoid creating another function.
	// We can capture the variables passed to this function by reference
	// so no need to pass them trough to another function, which would be a mess
	auto DoTraceTask = [&](const EAgentRaytraceDirection& TraceDirectionType)
	{
		FTraceDelegate TraceDirectionDelegate;
		FVector RelativeForward;
		FVector RelativeRight;

		// Initialize everything...
		// We check the TraceDirectionType several times instead of using an if statement for all of them,
		// because we want the variables to be const. The compiler will optimize out the multiple identical
		// condition checks anyway

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
			default:
				break;
		}

		const FVector ForwardOffset = RelativeForward * (AgentProperties.CapsuleRadius + 1.0f);
		const FVector RightOffset = RelativeRight * StartOffsetWidth;
		const FVector StartingPoint = (((AgentLocation +ForwardOffset) + RightOffset) + StartOffsetHeightVector);

		for(uint8 i = 0; i < Rows; i++) // Vertical Traces
		{
			for(uint8 j = 0; j < Columns; j++) // Horizontal Traces
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

	// Use the above lambda function to do each trace direction
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
