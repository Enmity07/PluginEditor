// Copyright NoxxProjects and Primrose Taylor. All rights reserved.

#include "NAIAgentClient.h"

#include "DrawDebugHelpers.h"
#include "NAIAgentManager.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

class UAgentManagerStatics;

ANAIAgentClient::ANAIAgentClient()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->AreaClass = nullptr;
	CapsuleComponent->SetCollisionProfileName(TEXT("AgentClient"));
	CapsuleComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	CapsuleComponent->SetReceivesDecals(false);
	CapsuleComponent->PrimaryComponentTick.bCanEverTick = false;
	RootComponent = CapsuleComponent;
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMeshComponent->SetupAttachment(CapsuleComponent);
	SkeletalMeshComponent->SetReceivesDecals(false);
	SkeletalMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	AgentType = EAgentType::PathToPlayer;
	MoveSpeed = 50.0f;
	LookAtRotationRate = 0.5f;
	MaxStepHeight = 25.0f;

	MoveTickInterval = 0.033f;
	PathfindingTickInterval = 0.5f;
	AvoidanceTickInterval = 0.3f;
	
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	
	PrimaryActorTick.bCanEverTick = false;
}

void ANAIAgentClient::BeginPlay()
{
	Super::BeginPlay();

	ANAIAgentManager* AgentManager = (UAgentManagerStatics::bManagerExists)
		? (UAgentManagerStatics::GetManagerReference()) : (nullptr);
	
	if(AgentManager)
	{
		// Create the Agent guid at Runtime, not in the constructor
		Guid = FGuid::NewGuid();
		UE_LOG(LogTemp, Warning, TEXT("Hey my Guid: %s"), *Guid.ToString());
		
		// Init the agent we're going to add here
		FAgent Agent;

		const float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
		const float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		
		Agent.Guid = Guid;
		Agent.AgentClient = this;
		Agent.AgentManager = AgentManager;
		// Set the agents properties
		Agent.AgentProperties.AgentType = AgentType;
		Agent.AgentProperties.CapsuleRadius = CapsuleRadius;
		Agent.AgentProperties.CapsuleHalfHeight = CapsuleHalfHeight;
		Agent.AgentProperties.MoveSpeed = MoveSpeed;
		Agent.AgentProperties.LookAtRotationRate = LookAtRotationRate;
		Agent.AgentProperties.MaxStepHeight = MaxStepHeight;

		Agent.PathTask.InitializeTask(PathfindingTickInterval, 0.5f);
		Agent.AvoidanceFrontTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.AvoidanceRightTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.AvoidanceLeftTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.FloorCheckTask.InitializeTask(0.01f, 0.5f);
		Agent.StepCheckTask.InitializeTask(0.01f, 0.5f);
		
		Agent.MoveTask.InitializeTask(MoveTickInterval);
		
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.AgentRadius =
			Agent.AgentProperties.CapsuleRadius;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.AgentHeight =
			(Agent.AgentProperties.CapsuleHalfHeight * 2.0f);
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanFly = false;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanJump = false;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanSwim = false;
		
		/** Set up avoidance settings. */
		Agent.AgentProperties.AvoidanceProperties.Initialize(
			AvoidanceLevel,
			CapsuleRadius, CapsuleHalfHeight
		);

		/** Set up stepping settings. */
		Agent.AgentProperties.NavigationProperties.StepProperties.Initialize(
			CapsuleRadius, CapsuleHalfHeight,
			Agent.AgentProperties.MaxStepHeight
		);

		Agent.bIsHalted = false;

		Agent.PathTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnAsyncPathComplete, Guid
		);

		Agent.AvoidanceFrontTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnAvoidanceTraceComplete,
			Guid, EAgentAvoidanceTraceDirection::TracingFront
		);

		Agent.AvoidanceRightTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnAvoidanceTraceComplete,
			Guid, EAgentAvoidanceTraceDirection::TracingRight
		);

		Agent.AvoidanceLeftTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnAvoidanceTraceComplete,
			Guid, EAgentAvoidanceTraceDirection::TracingLeft
		);
		
		Agent.FloorCheckTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnFloorCheckTraceComplete, Guid
		);

		Agent.StepCheckTask.GetOnCompleteDelegate().BindUObject(
			AgentManager, &ANAIAgentManager::OnStepCheckTraceComplete, Guid
		);

		//  Add the agent to the TMap<FGuid, FAgent> AgentMap, which is for all active agents
		AgentManager->AddAgent(Agent);
	}
}
