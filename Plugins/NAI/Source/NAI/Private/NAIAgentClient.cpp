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
		Agent.Timers.FloorCheckTime.TickRate = 0.01f;

		Agent.AgentProperties.NavigationProperties.NavAgentProperties.AgentRadius =
			Agent.AgentProperties.CapsuleRadius;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.AgentHeight =
			(Agent.AgentProperties.CapsuleHalfHeight * 2.0f);
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanFly = false;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanJump = false;
		Agent.AgentProperties.NavigationProperties.NavAgentProperties.bCanSwim = false;
		
		// Setup avoidance settings
		Agent.AgentProperties.AvoidanceProperties.Initialize(
			AvoidanceLevel,
			CapsuleComponent->GetScaledCapsuleRadius(),
			CapsuleComponent->GetUnscaledCapsuleHalfHeight()
		);

		Agent.bIsHalted = false;

		// Bind the PathCompleteDelegate function to the Async Navigation Query
		Agent.AgentProperties.NavigationProperties.NavPathQueryDelegate.BindUObject(this, &ANAIAgentClient::PathCompleteDelegate);
		Agent.AgentProperties.NavigationProperties.FloorCheckTraceDelegate.BindUObject(this, &ANAIAgentClient::OnFloorCheckTraceComplete);
		Agent.AgentProperties.RaytraceFrontDelegate.BindUObject(this, &ANAIAgentClient::OnFrontTraceCompleted);
		Agent.AgentProperties.RaytraceRightDelegate.BindUObject(this, &ANAIAgentClient::OnRightTraceCompleted);
		Agent.AgentProperties.RaytraceLeftDelegate.BindUObject(this, &ANAIAgentClient::OnLeftTraceCompleted);
		// Add the agent to the TMap<FGuid, FAgent> AgentMap, which is for all active agents
		AgentManager->AddAgent(Agent);
	}
}

#define ENABLE_DEBUG_DRAW_LINE true
#define ENABLE_DEBUG_PRINT_SCREEN true

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
		AgentManager->UpdateAgentFloorCheckResult(Guid, 0.0f, false);
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
		AgentManager->UpdateAgentFloorCheckResult(Guid, 0.0f, false);
		return;
	}
	
	TArray<float> HitZPoints;
	for(int i = 0; i < Locations.Num(); i++)
	{
		HitZPoints.Add(Locations[i].Z);
	}
	// If this sort doesn't go the correct way look at using this lambda
	// HitZPoints.Sort([](const float& a, const float& b) { return a.field < b.field; });
	// src: kukikuilla on Reddit https://www.reddit.com/r/unrealengine/comments/d6fjii/how_does_one_sort_a_tarray_in_descending_order/
	HitZPoints.Sort();
	const float LowestValue = HitZPoints[0];
	AgentManager->UpdateAgentFloorCheckResult(Guid, LowestValue, true);

}

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

// TODO: These delegate functions only ended up here because we used the bind UObject function...
// TODO: the FAgent container for each agent is where they ideally will belong
void ANAIAgentClient::OnFrontTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if(!Handle.IsValid() || !AgentManager) //|| Data.OutHits.Num() < 1)
		return;

	AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentRaytraceDirection::TracingFront,
		CheckIfBlockedByAgent(Data.OutHits));
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
	
	AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentRaytraceDirection::TracingRight,
		CheckIfBlockedByAgent(Data.OutHits));
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
	if(!Handle.IsValid() || !AgentManager) //|| Data.OutHits.Num() < 1)
		return;
	
	AgentManager->UpdateAgentAvoidanceResult(Guid, EAgentRaytraceDirection::TracingLeft,
		CheckIfBlockedByAgent(Data.OutHits));

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
	TArray<FVector> Locations = TArray<FVector>();
	
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
