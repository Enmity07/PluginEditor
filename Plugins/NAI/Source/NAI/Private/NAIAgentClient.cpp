// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentClient.h"

#include "DrawDebugHelpers.h"
#include "NAIAgentManager.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"



// Sets default values
ANAIAgentClient::ANAIAgentClient()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->AreaClass = nullptr;
	CapsuleComponent->SetCollisionProfileName(TEXT("AgentClient"));
	CapsuleComponent->SetReceivesDecals(false);
	CapsuleComponent->PrimaryComponentTick.bCanEverTick = false;
	RootComponent = CapsuleComponent;
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMeshComponent->SetupAttachment(CapsuleComponent);
	SkeletalMeshComponent->SetReceivesDecals(false);
	SkeletalMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	AgentType = EAgentType::PathToPlayer;
	MoveSpeed = 50.0f;

	MoveTickInterval = 0.033f;
	PathfindingTickInterval = 0.5f;
	AvoidanceTickInterval = 0.3f;
	
	AgentManager = nullptr;
	WorldRef = nullptr;
	
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ANAIAgentClient::BeginPlay()
{
	Super::BeginPlay();

	if(AgentManagerVariable)
		AgentManager = AgentManagerVariable;

	if(WorldRef == nullptr)
		WorldRef = GetWorld();
	
	if(AgentManager)
	{
		// Create the Agent guid at Runtime, not in the constructor
		Guid = FGuid::NewGuid();
		
		// Init the agent we're going to add here
		FAgent Agent;
		
		Agent.Guid = Guid;
		Agent.AgentClient = this;
		Agent.AgentManager = AgentManager;
		// Set the agents properties
		Agent.AgentProperties.AgentType = AgentType;
		Agent.AgentProperties.CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
		Agent.AgentProperties.CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		Agent.AgentProperties.MoveSpeed = MoveSpeed;
		Agent.AgentProperties.LookAtRotationRate = LookAtRotationRate;
		Agent.Timers.MoveTime.TickRate = MoveTickInterval;
		Agent.Timers.PathTime.TickRate = PathfindingTickInterval;
		Agent.Timers.TraceTime.TickRate = AvoidanceTickInterval;

		// Setup avoidance settings
		Agent.AgentProperties.AvoidanceProperties.Initialize(
			AvoidanceLevel,
			CapsuleComponent->GetScaledCapsuleRadius(),
			CapsuleComponent->GetUnscaledCapsuleHalfHeight()
		);

		Agent.bIsHalted = false;

		// Bind the PathCompleteDelegate function to the Async Navigation Query
		Agent.NavPathQueryDelegate.BindUObject(this, &ANAIAgentClient::PathCompleteDelegate);
		Agent.AgentProperties.RaytraceFrontDelegate.BindUObject(this, &ANAIAgentClient::OnFrontTraceCompleted);
		Agent.AgentProperties.RaytraceRightDelegate.BindUObject(this, &ANAIAgentClient::OnRightTraceCompleted);
		Agent.AgentProperties.RaytraceLeftDelegate.BindUObject(this, &ANAIAgentClient::OnLeftTraceCompleted);
		
		// Add the agent to the TMap<FGuid, FAgent> AgentMap, which is for all active agents
		AgentManager->AddAgent(Agent);
	}
}

// Local Delegate which is called when an Async pathfinding request has completed for this agent
void ANAIAgentClient::PathCompleteDelegate(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer)
{
	if(ResultType == ENavigationQueryResult::Success)
	{	
		if(AgentManager)
		{
			const TArray<FNavPathPoint> PathPoints = NavPointer.Get()->GetPathPoints();
			AgentManager->UpdateAgentPath(Guid, PathPoints);
		}
	}
}

void ANAIAgentClient::OnFrontTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid()) //|| Data.OutHits.Num() < 1)
		return;
	
	for(int i = 0; i < Data.OutHits.Num(); i++)
	{
		if(Data.OutHits[i].Actor.Get()->IsValidLowLevelFast())
		{
			if(Data.OutHits[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
			{
				if(Guid != Cast<ANAIAgentClient>(Data.OutHits[i].Actor.Get())->Guid)
				{
					// If we got here then it means with DID hit an agent, and it WAS NOT this one
		
					
					break; // Don't need to check any others since only one needs to be in the way
				}
			}
		} 
	}
	if(WorldRef)
	{
		DrawDebugLine(
			WorldRef,
			Data.Start,
			Data.End,
			FColor(0, 255, 0),
			false,
			2,
			0,
			2.0f
		);
	}
}

void ANAIAgentClient::OnRightTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid()) //|| Data.OutHits.Num() < 1)
		return;
	
	if(WorldRef)
	{
		DrawDebugLine(
			WorldRef,
			Data.Start,
			Data.End,
			FColor(0, 255, 0),
			false,
			2,
			0,
			2.0f
		);
	}
}

void ANAIAgentClient::OnLeftTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid()) //|| Data.OutHits.Num() < 1)
		return;
	
	if(WorldRef)
	{
		DrawDebugLine(
			WorldRef,
			Data.Start,
			Data.End,
			FColor(0, 255, 0),
			false,
			2,
			0,
			2.0f
		);
	}
}
