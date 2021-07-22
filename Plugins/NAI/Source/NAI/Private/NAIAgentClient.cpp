// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentClient.h"
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
	
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
}

// Called when the game starts or when spawned
void ANAIAgentClient::BeginPlay()
{
	Super::BeginPlay();

	if(AgentManagerVariable)
		AgentManager = AgentManagerVariable;
	
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
		Agent.AgentProperties.MoveSpeed = MoveSpeed;
		Agent.AgentProperties.LookAtRotationRate = LookAtRotationRate;
		// Set the actual tick rate to be an amount in seconds
		Agent.AgentProperties.MoveTickRate = MoveTickInterval;
		Agent.AgentProperties.PathfindingTickRate = PathfindingTickInterval;
		Agent.AgentProperties.TracingTickRate = AvoidanceTickInterval;

		// Timer settings
		Agent.Timers.bIsMoveReady = true;
		Agent.Timers.bIsPathReady = true;
		Agent.Timers.bIsTraceReady = true;
		
		Agent.bIsHalted = false;

		// Bind the PathDelegate function to the Async Navigation Query
		Agent.NavPathQueryDelegate.BindUObject(this, &ANAIAgentClient::PathDelegate);
		
		// Add the agent to the TMap<FGuid, FAgent> AgentMap, which is for all active agents
		AgentManager->AddAgent(Agent);
	}
}

// Local Delegate which is called when an Async pathfinding request has completed for this agent
void ANAIAgentClient::PathDelegate(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer)
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