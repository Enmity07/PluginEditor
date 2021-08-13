// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentClient.h"

#include "DrawDebugHelpers.h"
#include "NAIAgentManager.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

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
	
	AgentManager = nullptr;
	WorldRef = nullptr;
	
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	
	PrimaryActorTick.bCanEverTick = false;
}

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
		
		Agent.Timers.MoveTime.TickRate = MoveTickInterval;

		Agent.PathTask.InitializeTask(PathfindingTickInterval, 0.5f);
		Agent.AvoidanceFrontTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.AvoidanceRightTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.AvoidanceLeftTask.InitializeTask(AvoidanceTickInterval, 0.5f);
		Agent.FloorCheckTask.InitializeTask(0.01f, 0.5f);
		Agent.StepCheckTask.InitializeTask(0.01f, 0.5f);
		
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

		// Bind the OnAsyncPathComplete function to the Async Navigation Query
		Agent.AgentProperties.NavigationProperties.NavPathQueryDelegate.BindUObject(this, &ANAIAgentClient::OnAsyncPathComplete);
		Agent.AgentProperties.NavigationProperties.FloorCheckTraceDelegate.BindUObject(this, &ANAIAgentClient::OnFloorCheckTraceComplete);
		Agent.AgentProperties.NavigationProperties.StepCheckTraceDelegate.BindUObject(this, &ANAIAgentClient::OnStepCheckTraceComplete);
		Agent.AgentProperties.RaytraceFrontDelegate.BindUObject(this, &ANAIAgentClient::OnFrontTraceCompleted);
		Agent.AgentProperties.RaytraceRightDelegate.BindUObject(this, &ANAIAgentClient::OnRightTraceCompleted);
		Agent.AgentProperties.RaytraceLeftDelegate.BindUObject(this, &ANAIAgentClient::OnLeftTraceCompleted);
		// Add the agent to the TMap<FGuid, FAgent> AgentMap, which is for all active agents
		AgentManager->AddAgent(Agent);
	}
}

#define ENABLE_DEBUG_DRAW_LINE true
#define ENABLE_FLOOR_DEBUG_PRINT_SCREEN false
#define ENABLE_DEBUG_PRINT_SCREEN false

void ANAIAgentClient::OnFloorCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager)
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
		AgentManager->UpdateAgentFloorCheckResult(Guid, FVector(), false);
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
		AgentManager->UpdateAgentFloorCheckResult(Guid, FVector(), false);
		return;
	}
	
	FVector HighestVector = FVector();
	for(int i = 0; i < Locations.Num(); i++)
	{
		if(Locations[i].Z > HighestVector.Z)
		{
			HighestVector = Locations[i];
		}
	}
	AgentManager->UpdateAgentFloorCheckResult(Guid, HighestVector, true);
}

void ANAIAgentClient::OnStepCheckTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager)
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
		AgentManager->UpdateAgentStepCheckResult(Guid, FVector(), false);
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
		AgentManager->UpdateAgentStepCheckResult(Guid, FVector(), false);
		return;
	}

	FVector HighestVector = FVector();
	for(int i = 0; i < Locations.Num(); i++)
	{
		if(Locations[i].Z > HighestVector.Z)
		{
			HighestVector = Locations[i];
		}
	}
	AgentManager->UpdateAgentStepCheckResult(Guid, HighestVector, true);
}

void ANAIAgentClient::OnAsyncPathComplete(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPointer)
{
	if(!AgentManager)
	{
		return;
	}
	
	if(ResultType == ENavigationQueryResult::Success)
	{
		const FAgentPathResult NewResult = FAgentPathResult(true, NavPointer.Get()->GetPathPoints());
		AgentManager->UpdateAgentPath(Guid, NewResult);
	}
	else
	{
		// Handle Pathfinding failed
	}
}

void ANAIAgentClient::OnFrontTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager)
		return;

	if(Data.OutHits.Num() == 0)
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingFront, false);
	}
	else
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingFront,
	CheckIfBlockedByAgent(Data.OutHits));
	}
#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
}

void ANAIAgentClient::OnRightTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager) //|| Data.OutHits.Num() < 1)
		return;
	
	if(Data.OutHits.Num() == 0)
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingRight, false);
	}
	else
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingRight,
	CheckIfBlockedByAgent(Data.OutHits));
	}
#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
}

void ANAIAgentClient::OnLeftTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager)
		return;

	if(Data.OutHits.Num() == 0)
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingLeft, false);
	}
	else
	{
		AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentAvoidanceTraceDirection::TracingLeft,
	CheckIfBlockedByAgent(Data.OutHits));
	}
#if (ENABLE_DEBUG_DRAW_LINE)
	if(WorldRef)
	{
		DrawDebugLine(WorldRef, Data.Start, Data.End, FColor(0, 255, 0),
			false, 2, 0, 2.0f);
	}
#endif
}

bool ANAIAgentClient::CheckIfBlockedByAgent(const TArray<FHitResult>& Objects)
{
	for(int i = 0; i < Objects.Num(); i++)
	{
		if(Objects[i].Actor.IsValid())
		{
			if(Objects[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
			{
				// TODO: This check might not be needed since the traces should always start outside the agents collider
				if(Guid != Cast<ANAIAgentClient>(Objects[i].Actor.Get())->Guid)
				{
					// If we got here then it means with DID hit an agent, and it WAS NOT this one
					return true; // Don't need to check any others since only one needs to be in the way
				}
			}
		}
	}
	return false;
}

TArray<FVector> ANAIAgentClient::GetAllHitLocationsNotFromAgents(const TArray<FHitResult>& HitResults)
{
	TArray<FVector> Locations;
	
	for(int i = 0; i < HitResults.Num(); i++)
	{
		if(HitResults[i].Actor.IsValid())
		{
			if(!HitResults[i].Actor.Get()->IsA(ANAIAgentClient::StaticClass()))
			{
				Locations.Add(HitResults[i].Location);
			}
		}
	}
	
	return Locations; 
}

#undef ENABLE_DEBUG_DRAW_LINE
