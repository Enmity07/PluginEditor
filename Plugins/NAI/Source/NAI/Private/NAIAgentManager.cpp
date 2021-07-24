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

	TArray<TSharedPtr<FName>> ProfileNames;
	UCollisionProfile::GetProfileNames(ProfileNames);
	if(ProfileNames.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("---Start---"));
		for(int i = 0; i < ProfileNames.Num(); i++)
		{
			UE_LOG(LogTemp, Warning, TEXT("Name: %s"), *ProfileNames[i].Get()->ToString());
		}
		UE_LOG(LogTemp, Warning, TEXT("---End---"));
	}

	
	if(WorldRef)
	{
		const int AgentCount = AgentMap.Num();
		if(AgentCount > 0)
		{
			for(int i = 0; i < AgentCount; i++)
			{
				
				// Get the next Guid and use it to get a copy of the agent
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
				if(Agent.Timers.bIsPathReady)
				{
					// Get the Goal Location from the agent type
					// the pathfinding can be radically different between types
					// so GetAgentGoalLocationFromType() determines what vector we need
					// for the async pathfinding task
					const FVector PlayerLocation = FVector(); // TODO: GET THE PLAYER!!!
					const EAgentType AgentType = Agent.AgentProperties.AgentType;
					const FVector GoalLocation = GetAgentGoalLocationFromType(AgentType, PlayerLocation);
					
					AgentPathTaskAsync(Guid, GoalLocation, AgentLocation); // start the task
					Agent.Timers.bIsPathReady = false; // Reset the timer
				}

				if(Agent.Timers.bIsMoveReady)
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
						Agent.AgentClient->SetActorLocation(NewLoc, true);

						// Turn the agent to face the direction it's moving in
						const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(AgentLocation, NewLoc);
						Agent.AgentClient->SetActorRotation(
							FMath::Lerp(
								Agent.AgentClient->GetActorRotation(),
								LookAtRotation,
								Agent.AgentProperties.LookAtRotationRate
							)
						);
					}
					
					Agent.Timers.bIsMoveReady = false;
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

void ANAIAgentManager::AgentPathTaskAsync(const FGuid& Guid, const FVector& Location, const FVector& Start)
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

//FTraceHandle UWorld::AsyncLineTraceByObjectType(
//    EAsyncTraceType InTraceType, 
//    const FVector& Start, 
//    const FVector& End, 
//    const FCollisionObjectQueryParams& ObjectQueryParams, 
//    const FCollisionQueryParams& Params /* = FCollisionQueryParams::DefaultQueryParam */, 
//    FTraceDelegate* InDelegate/* =NULL */, 
//    uint32 UserData /* = 0 */)
//{
//	return StartNewTrace(AsyncTraceState, FTraceDatum(this, FCollisionShape::LineShape, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams, DefaultCollisionChannel, UserData, InTraceType, Start, End, FQuat::Identity, InDelegate, AsyncTraceState.CurrentFrame));
//}
// FTraceHandle	AsyncLineTraceByChannel(
//    EAsyncTraceType InTraceType,
//	  const FVector&
//	  Start,const FVector& End,
//	  ECollisionChannel TraceChannel,
//	  const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam,
//	  const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam,
//	  FTraceDelegate * InDelegate=NULL, uint32 UserData = 0
//);


void ANAIAgentManager::AgentTraceTaskAsync(
	const FGuid& Guid, const FVector& Start, const FVector& End, const EAgentRaytraceDirection& Direction)
{
	// Get the required delegate
	FTraceDelegate TraceDirectionDelegate;
	switch(Direction)
	{
		case EAgentRaytraceDirection::TracingFront:
			TraceDirectionDelegate = AgentMap[Guid].RaytraceFrontDelegate;
			break;
		case EAgentRaytraceDirection::TracingLeft:
			TraceDirectionDelegate = AgentMap[Guid].RaytraceLeftDelegate;
			break;
		case EAgentRaytraceDirection::TracingRight:
			TraceDirectionDelegate = AgentMap[Guid].RaytraceRightDelegate;
			break;
		default:
			break;
	}

	//FCollisionObjectQueryParams ObjectQueryParams;
	
	//WorldRef->AsyncLineTraceByObjectType(
	//	EAsyncTraceType::Single,
	//	Start,
	//	End,
	//	,
	//	FCollisionResponseParams::DefaultResponseParam,
	//	&TraceDirectionDelegate
	//	
	//);
	
	WorldRef->AsyncLineTraceByChannel(
		EAsyncTraceType::Single,
		Start,
		End,
		ECollisionChannel::ECC_Pawn, // TODO: Don't forget this
		FCollisionQueryParams::DefaultQueryParam,
		FCollisionResponseParams::DefaultResponseParam,
		&TraceDirectionDelegate
	);
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
		default:
			return NULL_VECTOR;
	}
	// this is unreachable since the switch only returns and never breaks
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
