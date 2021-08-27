// Copyright NoxxProjects and Primrose Taylor. All rights reserved.

#include "NAIAgentManager.h"

#include "DrawDebugHelpers.h"
#include "NAIAgentClient.h"
#include "Async/Async.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#define NULL_VECTOR FVector(125.0f, 420.0f, -31700.4f)
#define MAX_AGENT_PRE_ALLOC 1024

#define ZERO_QUAT FQuat(0.0f, 0.0f, 0.0f, 0.0f)

bool UAgentManagerStatics::bManagerExists = false;
ANAIAgentManager* UAgentManagerStatics::CurrentManager = nullptr;

void UAgentManagerStatics::SetManagerReference(ANAIAgentManager* InManager)
{
	if(!InManager)
		return;
	CurrentManager = InManager;
	bManagerExists = true;
}

ANAIAgentManager* UAgentManagerStatics::GetManagerReference()
{
	return CurrentManager;
}

void UAgentManagerStatics::Reset()
{
	CurrentManager = nullptr;
	bManagerExists = false;
}

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

	// TODO: Throw some error if there is more than 1 AgentManager in the level

	/** Add this manager as the current active manager. */
	UAgentManagerStatics::SetManagerReference(this);
	
	AgentMap.Reserve(MAX_AGENT_PRE_ALLOC);
}

void ANAIAgentManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/** Remove the reference to this manager. */
	UAgentManagerStatics::Reset();
}

#undef MAX_AGENT_PRE_ALLOC

/**
 * If this is enabled with a lot of agents active in the game it will crash the editor,
 * so use it with caution.
*/
#define ENABLE_DEBUG_DRAW_LINE false

void ANAIAgentManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);


	
	if(WorldRef)
	{
		const int AgentCount = AgentMap.Num();
		if(AgentCount > 0)
		{
			for(int i = 0; i < AgentCount; i++)
			{
				// Get the Guid for this iteration, and use it to get a ref to the agent
				const FGuid Guid = AgentGuids[i];
				/**
				 * Grab a reference to the Agent we want to work on.
				 * We don't take a copy to avoid having to unbox it after
				 */
				FAgent *Agent = &AgentMap[Guid];

				// Update all the agents timers for their tasks
				Agent->UpdateTimers(DeltaTime);

				const FVector AgentLocation = Agent->AgentClient->GetActorLocation();
				// Calculate the Agents Speed and update the property on the AgentClient object
				Agent->CalculateAndUpdateSpeed(AgentLocation, DeltaTime);

				// If the Agent has been set to stop, don't do anything
		        if(Agent->bIsHalted)
		        {
		            continue;
		        }

				/** Execute the Pathfinding Task TODO: Doc this properly */
				if(Agent->PathTask.IsReady())
				{
					// Get the Goal Location from the Agent type
					const FVector PlayerLocation = GetActorLocation(); // TODO: GET THE PLAYER!!!
					const EAgentType AgentType = Agent->AgentProperties.AgentType;
					const FVector GoalLocation = GetAgentGoalLocationFromType(AgentType, PlayerLocation);
					
					AgentPathTaskAsync(AgentLocation, GoalLocation,
						Agent->AgentProperties.NavigationProperties.NavAgentProperties,
						Agent->PathTask.GetOnCompleteDelegate());
					Agent->PathTask.Reset(); // Reset the timer
				}

				const FVector AgentForward = Agent->AgentClient->GetActorForwardVector();
				const FVector AgentRight = Agent->AgentClient->GetActorRightVector();
				
				/** Execute the Front avoidance task TODO: Doc this properly */
				if(Agent->AvoidanceFrontTask.IsReady())
				{
					AgentAvoidanceTraceTaskAsync(
						EAgentAvoidanceTraceDirection::TracingFront,
						AgentLocation, AgentForward, AgentRight, Agent->AgentProperties,
						Agent->AvoidanceFrontTask.GetOnCompleteDelegate()
					);
					
					Agent->AvoidanceFrontTask.Reset();
				}

				/** Is there something in front of the Agent? */
				if(Agent->AvoidanceFrontTask.GetResult().IsBlocked())
				{
					/** Check both sides to see which way the Agent can strafe. */
					
					if(Agent->AvoidanceRightTask.IsReady())
					{
						AgentAvoidanceTraceTaskAsync(
							EAgentAvoidanceTraceDirection::TracingRight,
							AgentLocation, AgentForward, AgentRight, Agent->AgentProperties,
							Agent->AvoidanceRightTask.GetOnCompleteDelegate()
						);

						Agent->AvoidanceRightTask.Reset();
					}
					
					if(Agent->AvoidanceLeftTask.IsReady())
					{
						AgentAvoidanceTraceTaskAsync(
							EAgentAvoidanceTraceDirection::TracingLeft,
							AgentLocation, AgentForward, AgentRight, Agent->AgentProperties,
							Agent->AvoidanceLeftTask.GetOnCompleteDelegate()
						);

						Agent->AvoidanceLeftTask.Reset();
					}
				}

				/**
				 * The Object types to query for AsyncLineTraceByObjectType calls.
				 * This converts each added ECC_XXXX channel to a bitfield, add more channels with the |
				 * operator. Read the comments on the FCollisionObjectQueryParams type for more info.
				 */
				const FCollisionObjectQueryParams ObjectQueryParams =
					(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));

				/** Conduct the floor check TODO: Doc this properly */
				if(Agent->FloorCheckTask.IsReady())
				{
					const FVector StartPoint = FVector(
						AgentLocation.X, AgentLocation.Y,
						(AgentLocation.Z - (Agent->AgentProperties.CapsuleHalfHeight - 1.0f)));
					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);
					
					WorldRef->AsyncLineTraceByObjectType(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ObjectQueryParams,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent->FloorCheckTask.GetOnCompleteDelegate()
					);
					
					Agent->FloorCheckTask.Reset();
				}

				/** Execute the step check task TODO: Doc this properly */
				if(Agent->StepCheckTask.IsReady())
				{
					const FVector StartPoint =
						AgentLocation +
							(AgentForward * Agent->AgentProperties.NavigationProperties.StepProperties.ForwardOffset) +
							(FVector(0.0f, 0.0f, -1.0f) * Agent->AgentProperties.NavigationProperties.StepProperties.DownwardOffset);
					const FVector EndPoint = StartPoint - FVector(0.0f, 0.0f, 100.0f);
					
					WorldRef->AsyncLineTraceByObjectType(
						EAsyncTraceType::Multi, StartPoint, EndPoint, ObjectQueryParams,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent->StepCheckTask.GetOnCompleteDelegate()
					);
				}
				
				const FAgentVirtualCapsuleSweepProperties CapsuleSweepProperties
					= Agent->AgentProperties.NavigationProperties.LocalBoundsCheckProperties; 

				if(Agent->LocalBoundsCheckTask.IsReady())
				{
					// const FVector StartPoint = AgentLocation + FVector(0.0f, 0.0f, CapsuleSweepProperties.HalfHeight);
					const FVector EndPoint = AgentLocation - FVector(0.0f, 0.0f, (CapsuleSweepProperties.HalfHeight * 2.0f));
					
					WorldRef->AsyncSweepByObjectType(
						EAsyncTraceType::Multi, AgentLocation, EndPoint, ZERO_QUAT,
						ObjectQueryParams,
						CapsuleSweepProperties.VirtualCapsule,
						FCollisionQueryParams::DefaultQueryParam,
						&Agent->LocalBoundsCheckTask.GetOnCompleteDelegate()
					);

					const FVector DebugLocation = AgentLocation - FVector(0.0f, 0.0f, CapsuleSweepProperties.HalfHeight);
					UKismetSystemLibrary::DrawDebugCapsule(WorldRef, DebugLocation,
					CapsuleSweepProperties.HalfHeight, CapsuleSweepProperties.Radius,
						FRotator(0.0f), FLinearColor(0, 100, 255), 2.0f, 1.0f
					);

					Agent->LocalBoundsCheckTask.Reset();
				}
				
				/** Execute the movement task TODO: Doc this properly */
				if(Agent->MoveTask.IsReady())
				{
					const TArray<FNavPathPoint> PathPoints = Agent->PathTask.GetResult().Points;
					
					// No reason to move if we don't have a path
					if(PathPoints.Num() > 1) // Need at least 2 path points for it to be a path
					{			
						// Calculate the new FVector for this move update
						const FVector Start = PathPoints[0].Location;
						const FVector End = PathPoints[1].Location;
#if (ENABLE_DEBUG_DRAW_LINE)
						DrawDebugLine(WorldRef, Start, End, FColor(0, 255, 0),
                        	false, 2.0f, 0, 2.0f);
#endif
						// Get the direction in a normalized format
						const FVector Direction = (End - Start).GetSafeNormal();

						FVector NewLoc = (AgentLocation + (Direction * (Agent->AgentProperties.MoveSpeed * DeltaTime)));


						if(Agent->LocalBoundsCheckTask.GetResult().bIsValidResult)
						{
							const float NewZ = (Agent->LocalBoundsCheckTask.GetResult().GetHighestHitPoint().Z +
								(Agent->AgentProperties.CapsuleHalfHeight + 1.0f)
							);

							NewLoc.Z = NewZ;
						}
						else
						{
							// Try get height of ground with the simple checks
						}

						
						/** 
						 * We need to check the step before the floor, since if there is a step
						 * we want to adjust our height for that. But if we don't have a step detected
						 * then we can just use the floor height.
						 * TODO: This can be better..
						 */
						//if(Agent->StepCheckTask.GetResult().bIsValidResult)
						//{
						//	NewLoc.Z = (Agent->StepCheckTask.GetResult().DetectedHitLocation.Z +
						//		(Agent->AgentProperties.CapsuleHalfHeight + 1.0f)); // an offset is applied to avoid the agent getting stuck on the floor
						//}
						//else if(Agent->FloorCheckTask.GetResult().bIsValidResult)
						//{			
						//	NewLoc.Z = (Agent->FloorCheckTask.GetResult().DetectedHitLocation.Z +
						//		(Agent->AgentProperties.CapsuleHalfHeight + 1.0f)); // an offset is applied to avoid the agent getting stuck on the floor
						//}

						// Calculate the difference in movement
						// This difference calc is also done inside the FindLookAtRotation function below,
						// so we're doing this twice...replace the function
						const FVector MoveDelta = NewLoc - AgentLocation;

						// Calculate our new rotation
						FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(AgentLocation, NewLoc);
						LookAtRotation.Roll	= 0.0f; LookAtRotation.Pitch = 0.0f;
						const FRotator LerpRotation = FMath::Lerp(Agent->AgentClient->GetActorRotation(),
							LookAtRotation, Agent->AgentProperties.LookAtRotationRate
						);

						// FHitResult Hit; // TODO: Perhaps can get rid of the simgple floor/step check thanks to this 

						// We directly move the agent by moving it's root component as it avoids a shit load
						// of function calls, along with extra GetActorLocation() function calls
						// when we already have the agents location in this scope, before we finally get to this function
						// so we're directly calling it here instead of SetActorLocation() / SetActorRotation()
						Agent->AgentClient->GetRootComponent()->MoveComponent(
							MoveDelta, LerpRotation, true);
					}
					
					Agent->MoveTask.Reset();
				}
			}
		}
	}
	else
	{
		Initialize();
	}
}

#define ENABLE_DEBUG_PRINT_SCREEN			false 

#if (ENABLE_DEBUG_PRINT_SCREEN)
	#define ENABLE_FLOOR_DEBUG_PRINT_SCREEN		false
#else
	#define ENABLE_FLOOR_DEBUG_PRINT_SCREEN		false
#endif

void ANAIAgentManager::OnAsyncPathComplete(
	uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer,
	FGuid Guid)
{
	if(ResultType == ENavigationQueryResult::Success)
	{
		const FAgentPathResult NewResult = FAgentPathResult(true, NavPointer.Get()->GetPathPoints());
		UpdateAgentPathResult(Guid, NewResult);
	}
	else
	{
		// Handle Pathfinding failed 
	}
}

void ANAIAgentManager::OnAvoidanceTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data,
	FGuid Guid, EAgentAvoidanceTraceDirection Direction)
{
	if(!Handle.IsValid())
		return;
	
	if(Data.OutHits.Num() == 0)
	{
		UpdateAgentAvoidanceResult(Guid, Direction, false);
	}
	else
	{
		UpdateAgentAvoidanceResult(Guid, Direction,
	CheckIfBlockedByAgent(Data.OutHits, Guid));
	}
#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
}

void ANAIAgentManager::OnFloorCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data, FGuid Guid)
{
	UE_LOG(LogTemp, Warning, TEXT("Guid: %s"), *Guid.ToString());

	if(!Handle.IsValid())
		return;

#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
	if(Data.OutHits.Num() == 0)
	{
		UpdateAgentFloorCheckResult(Guid, FVector::ZeroVector, false);
		return;
	}
	
	const TArray<FVector> Locations = GetAllHitLocationsNotFromAgents(Data.OutHits);
#if (ENABLE_FLOOR_DEBUG_PRINT_SCREEN)
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(
			-1, 1.0f, FColor::Yellow,
			FString::Printf(TEXT("Total number non agents locs: %d"),
			Locations.Num()));
#endif
	if(Locations.Num() == 0)
	{
		UpdateAgentFloorCheckResult(Guid, FVector::ZeroVector, false);
		return;
	}
	FVector HighestVector = FVector::ZeroVector;
	for(int i = 0; i < Locations.Num(); i++)
	{
		if(Locations[i].Z > HighestVector.Z)
		{
			HighestVector = Locations[i];
		}
	}
	UpdateAgentFloorCheckResult(Guid, HighestVector, true);
}

void ANAIAgentManager::OnStepCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data, FGuid Guid)
{
	if(!Handle.IsValid())
		return;

#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
	if(Data.OutHits.Num() == 0)
	{
		UpdateAgentStepCheckResult(Guid, FVector::ZeroVector, false);
		return; 
	}
	const TArray<FVector> Locations = GetAllHitLocationsNotFromAgents(Data.OutHits);
#if (ENABLE_DEBUG_PRINT_SCREEN)
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(
			-1, 1.0f, FColor::Yellow,
			FString::Printf(TEXT("Total number non agents locs: %d"),
			Locations.Num()));
#endif
	if(Locations.Num() == 0)
	{
		UpdateAgentStepCheckResult(Guid, FVector::ZeroVector, false);
		return;
	}

	FVector HighestVector = FVector::ZeroVector;
	for(int i = 0; i < Locations.Num(); i++)
	{
		if(Locations[i].Z > HighestVector.Z)
		{
			HighestVector = Locations[i];
		}
	}
	UpdateAgentStepCheckResult(Guid, HighestVector, true);
}

#define ENABLE_LOCAL_BOUNDS_DEBUG true

void ANAIAgentManager::OnLocalBoundsCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data, FGuid Guid)
{
	if(!Handle.IsValid())
		return;

	const int HitResultCount = Data.OutHits.Num();
	
	/**
	 * We didn't hit anything at all... this means we're in mid-air...
	 * Reset the result with the type's default constructor
	 */
	if(HitResultCount == 0)
	{
		AgentMap[Guid].UpdateLocalBoundsCheckResult(/* Default */);
		return;
	}
	
	/** 
	 * Handle Singular Hit output
	 * We pass the only impact point as the highest vector since there are no others
	 * This can be either the floor height, or the location of an object the agent collided with
	 * in mid-air
	 */
	if(HitResultCount == 1)
	{
		AgentMap[Guid].UpdateLocalBoundsCheckResult(
			FAgentLocalBoundsCheckResult(true, Data.OutHits.Last().ImpactPoint)
		);
		
		return;
	}

	/** IF we got here it means we have multiple hits. */

	// Get the highest vector from the list of hits
	FVector HighestVector = FVector::ZeroVector;
	FNAICalculator::GetHighestHitPoint(Data.OutHits, HighestVector);

	// assemble a list of vectors representing the hit locations
	TArray<FVector> HitPoints;
	for(int i = 0; i < HitResultCount; i++)
	{
		HitPoints.Add(Data.OutHits[i].ImpactPoint);
	}
	// Update the result
	AgentMap[Guid].UpdateLocalBoundsCheckResult(
		FAgentLocalBoundsCheckResult(true, HighestVector, true, HitPoints)
	);

	
#if (ENABLE_LOCAL_BOUNDS_DEBUG)
	if(GEngine)
	{
		// Print the highest vector to screen as a string
		GEngine->AddOnScreenDebugMessage(
			-1, 1.0f, FColor::Yellow,
			FString::Printf(TEXT("Highest Vector Hit: %s"),
			*HighestVector.ToString())
		);

		// Draw a small capsule at the location of the highest vector
		UKismetSystemLibrary::DrawDebugCapsule(WorldRef, HighestVector,
	5.0f, 5.0f,
	FRotator(0.0f), FLinearColor(255, 0, 150), 5.0f, 1.0f
		);
	}
#endif
}

bool ANAIAgentManager::CheckIfBlockedByAgent(const TArray<FHitResult>& Objects, const FGuid& Guid) const
{
	for(int i = 0; i < Objects.Num(); i++)
	{
		if(Objects[i].Actor.IsValid())
		{
			if(Objects[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
			{
				// TODO: This check is not needed for line traces since the they should always start outside the agents collider
				if(Guid != Cast<ANAIAgentClient>(Objects[i].Actor.Get())->GetGuid())
				{
					// If we got here then it means with DID hit an agent, and it WAS NOT this one
					return true; // Don't need to check any others since only one needs to be in the way
				}
			}
		}
	}
	return false;
}

TArray<FVector> ANAIAgentManager::GetAllHitLocationsNotFromAgents(const TArray<FHitResult>& HitResults) const
{
	TArray<FVector> Locations;
	
	for(int i = 0; i < HitResults.Num(); i++)
	{
		if(HitResults[i].Actor.IsValid())
		{
			if(!HitResults[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
			{
				Locations.Add(HitResults[i].ImpactPoint);
			}
		}
	}
	
	return Locations; 
}

#undef ENABLE_DEBUG_DRAW_LINE
#undef ENABLE_FLOOR_DEBUG_PRINT_SCREEN
#undef ENABLE_DEBUG_PRINT_SCREEN

void ANAIAgentManager::AgentPathTaskAsync(const FVector& Start, const FVector& Goal,
	const FNavAgentProperties& NavAgentProperties, const FNavPathQueryDelegate& PathDelegate) const
{
	FPathFindingQuery PathfindingQuery;
	PathfindingQuery.StartLocation = Start;
	PathfindingQuery.EndLocation = Goal;
	PathfindingQuery.QueryFilter = NavQueryRef;
	PathfindingQuery.NavData = NavDataRef;
	
	NavSysRef->FindPathAsync(
		NavAgentProperties,
		PathfindingQuery,
		PathDelegate,
		EPathFindingMode::Regular
	);
}

void ANAIAgentManager::AgentAvoidanceTraceTaskAsync(
	const EAgentAvoidanceTraceDirection& TraceDirection,
	const FVector& AgentLocation, const FVector& Forward, const FVector& Right,
	const FAgentProperties& AgentProperties, FTraceDelegate& TraceDirectionDelegate) const
{
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
			
			RelativeForward = Forward;
			RelativeRight = Right;
			break;
		case EAgentAvoidanceTraceDirection::TracingLeft:
			RelativeForward = -Right;
			RelativeRight = -Forward;
			break;
		case EAgentAvoidanceTraceDirection::TracingRight:
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
