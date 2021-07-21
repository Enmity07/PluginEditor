// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentManager.h"
#include "NAIAgentClient.h"

#include "Kismet/KismetMathLibrary.h"

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

// Called every frame
void ANAIAgentManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
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
					AgentPathTask(Guid, PlayerLocation, AgentLocation);
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
	// So if the Agent already exists in the map it will
	// get overwritten with this new one
	AgentMap.Add(Agent.Guid, Agent);
}

void ANAIAgentManager::UpdateAgentPath(const FGuid& Guid, const TArray<FNavPathPoint>& PathPoints)
{
	AgentMap[Guid].UpdatePathPoints(PathPoints);
}

void ANAIAgentManager::AgentPathTask(const FGuid& Guid, const FVector& Location, const FVector& Start)
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
