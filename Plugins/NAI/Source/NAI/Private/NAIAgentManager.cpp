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

void ANAIAgentManager::Tick(const float DeltaTime)
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
				// Get the Guid for this iteration, and use it to get a copy of the agent
				const FGuid Guid = AgentGuids[i];
				FAgent Agent = AgentMap[Guid];

				// Update all the agents timers for their tasks
				Agent.UpdateTimers(DeltaTime);

				const FVector AgentLocation = Agent.AgentClient->GetActorLocation();
				// Calculate the Agents Speed and update the property on the AgentClient object
				Agent.CalculateAndUpdateSpeed(AgentLocation, DeltaTime);

				// If the Agent has been set to stop, don't do anything
		        if(Agent.bIsHalted)
		        {
		            continue;
		        }

				/** Execute the Pathfinding Task TODO: Doc this properly */
				if(Agent.PathTask.IsReady())
				{
					// Get the Goal Location from the Agent type
					const FVector PlayerLocation = GetActorLocation(); // TODO: GET THE PLAYER!!!
					const EAgentType AgentType = Agent.AgentProperties.AgentType;
					const FVector GoalLocation = GetAgentGoalLocationFromType(AgentType, PlayerLocation);
					
					AgentPathTaskAsync(AgentLocation, GoalLocation, Agent.AgentProperties.NavigationProperties); // start the task
					Agent.PathTask.Reset(); // Reset the timer
				}

				const FVector AgentForward = Agent.AgentClient->GetActorForwardVector();
				const FVector AgentRight = Agent.AgentClient->GetActorRightVector();
				
				/** Execute the Front avoidance task TODO: Doc this properly */
				if(Agent.AvoidanceFrontTask.IsReady())
				{
					AgentAvoidanceTraceTaskAsync(
						EAgentAvoidanceTraceDirection::TracingFront,
						AgentLocation, AgentForward, AgentRight, Agent.AgentProperties
					);
					
					Agent.AvoidanceFrontTask.Reset();
				}

				/** Is there something in front of the Agent? */
				if(Agent.AvoidanceFrontTask.GetResult().IsBlocked())
				{
					/** Check both sides to see which way the Agent can strafe. */
					
					if(Agent.AvoidanceRightTask.IsReady())
					{
						AgentAvoidanceTraceTaskAsync(
							EAgentAvoidanceTraceDirection::TracingRight,
							AgentLocation, AgentForward, AgentRight, Agent.AgentProperties
						);

						Agent.AvoidanceRightTask.Reset();
					}
					
					if(Agent.AvoidanceLeftTask.IsReady())
					{
						AgentAvoidanceTraceTaskAsync(
							EAgentAvoidanceTraceDirection::TracingLeft,
							AgentLocation, AgentForward, AgentRight, Agent.AgentProperties
						);

						Agent.AvoidanceLeftTask.Reset();
					}
				}

				/**
				 * The Object types to query for AsyncLineTraceByObjectType calls.
				 * This convert each ECC_XXXX channel to a bitfield, add more channels with the |
				 * operator. Read the comments on the FCollisionObjectQueryParams type for more info.
				 */
				const FCollisionObjectQueryParams ObjectQueryParams =
					(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));

				/** Conduct the floor check TODO: Doc this properly */
				if(Agent.FloorCheckTask.IsReady())
				{
					const FVector StartPoint = FVector(
						AgentLocation.X, AgentLocation.Y,
						(AgentLocation.Z - (Agent.AgentProperties.CapsuleHalfHeight - 1.0f)));
					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);
					
					WorldRef->AsyncLineTraceByObjectType(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ObjectQueryParams,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent.AgentProperties.NavigationProperties.FloorCheckTraceDelegate
					);
					
					Agent.FloorCheckTask.Reset();
				}

				/** Execute the step check task TODO: Doc this properly */
				if(Agent.StepCheckTask.IsReady())
				{
					const FVector StartPoint =
						AgentLocation +
							(AgentForward * Agent.AgentProperties.NavigationProperties.StepProperties.ForwardOffset) +
							(FVector(0.0f, 0.0f, -1.0f) * Agent.AgentProperties.NavigationProperties.StepProperties.DownwardOffset);
					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);
					
					WorldRef->AsyncLineTraceByObjectType(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ObjectQueryParams,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent.AgentProperties.NavigationProperties.StepCheckTraceDelegate
					);
						
					Agent.StepCheckTask.Reset();
				}
				
				/** Execute the movement task TODO: Doc this properly */
				if(Agent.Timers.MoveTime.bIsReady)
				{
					const TArray<FNavPathPoint> PathPoints = Agent.PathTask.GetResult().Points;
					
					// No reason to move if we don't have a path
					if(PathPoints.Num() > 1) // Need at least 2 path points for it to be a path
					{			
						// Calculate the new FVector for this move update
						const FVector Start = PathPoints[0].Location;
						const FVector End = PathPoints[1].Location;
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
						 * TODO: This can be better..
						 */
						if(Agent.StepCheckTask.GetResult().bIsValidResult)
						{
							NewLoc.Z = (Agent.StepCheckTask.GetResult().DetectedHitLocation.Z +
								(Agent.AgentProperties.CapsuleHalfHeight + 5.0f)); // an offset is applied to avoid the agent getting stuck on the floor
						}
						else if(Agent.FloorCheckTask.GetResult().bIsValidResult)
						{			
							NewLoc.Z = (Agent.FloorCheckTask.GetResult().DetectedHitLocation.Z +
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
						// of function calls, along with extra GetActorLocation() function calls
						// when we already have the agents location in this scope, before we finally get to this function
						// so we're directly calling it here instead of SetActorLocation() / SetActorRotation()
						Agent.AgentClient->GetRootComponent()->MoveComponent(MoveDelta, LerpRotation, true);
					}
					
					Agent.Timers.MoveTime.Reset();
				}
				
				/** Update the agent in the map to apply the changes we've made this tick */ // TODO: Don't need to copy the whole thing, just the task changes
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
	const EAgentAvoidanceTraceDirection& TraceDirection,
	const FVector& AgentLocation, const FVector& Forward, const FVector& Right,
	const FAgentProperties& AgentProperties) const
{
	FTraceDelegate TraceDirectionDelegate;
	FVector RelativeForward;
	FVector RelativeRight;

	// Initialize everything...
	// We check the TraceDirection several times instead of using an if statement for all of them,
	// because we want the variables to be const. The compiler will optimize out the multiple identical
	// condition checks anyway

	const uint8 Columns = (TraceDirection == EAgentAvoidanceTraceDirection::TracingFront) ?
		(AgentProperties.AvoidanceProperties.GridColumns) :
		(AgentProperties.AvoidanceProperties.SideColumns);

	const uint8 Rows = (TraceDirection == EAgentAvoidanceTraceDirection::TracingFront) ?
		(AgentProperties.AvoidanceProperties.GridRows) :
		(AgentProperties.AvoidanceProperties.SideRows)
	;
	
	const float WidthIncrementSize =
		(TraceDirection == EAgentAvoidanceTraceDirection::TracingFront) ?
			(AgentProperties.AvoidanceProperties.WidthIncrementSize) :
			(AgentProperties.AvoidanceProperties.SideWidthIncrementSize)
	;
	
	const float HeightIncrementSize = AgentProperties.AvoidanceProperties.HeightIncrementSize;
	
	const float StartOffsetWidth =
		(TraceDirection == EAgentAvoidanceTraceDirection::TracingFront) ?
			(AgentProperties.AvoidanceProperties.StartOffsetWidth) :
			(AgentProperties.AvoidanceProperties.SideStartOffsetWidth)
	;

	// We don't need to calculate the Height every time like the width,
	// since the "up" vector is always the same amount in the Z axis,
	// so the height is precalculated in the Initialize() function of the
	// FAgentAvoidanceProperties struct in NAIAgentManager.h
	const FVector StartOffsetHeightVector =
		(TraceDirection == EAgentAvoidanceTraceDirection::TracingFront) ?
			(AgentProperties.AvoidanceProperties.StartOffsetHeightVector) :
			(AgentProperties.AvoidanceProperties.SideStartOffsetHeightVector)
	;
	
	switch(TraceDirection)
	{
		case EAgentAvoidanceTraceDirection::TracingFront:
			
			TraceDirectionDelegate = AgentProperties.RaytraceFrontDelegate;
			RelativeForward = Forward;
			RelativeRight = Right;
			break;
		case EAgentAvoidanceTraceDirection::TracingLeft:
			TraceDirectionDelegate = AgentProperties.RaytraceLeftDelegate;
			RelativeForward = -Right;
			RelativeRight = -Forward;
			break;
		case EAgentAvoidanceTraceDirection::TracingRight:
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

			WorldRef->AsyncLineTraceByObjectType(
				EAsyncTraceType::Single, CurrentStartPoint, CurrentEndPoint, ECC_Pawn,
				FCollisionQueryParams::DefaultQueryParam,
				&TraceDirectionDelegate
			);
		}
	}
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
