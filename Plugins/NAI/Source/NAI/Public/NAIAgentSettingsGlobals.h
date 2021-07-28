#pragma once

UENUM(BlueprintType, Blueprintable)
enum  class EAgentType : uint8
{
	PathToPlayer UMETA(DisplayName = "PathToPlayer"),
	PathToLocation UMETA(DisplayName = "PathToLocation"),
	Static UMETA(DisplayName = "StaticLocation"),
	Dynamic UMETA(DisplayName = "DynamicLocation"), 
	FollowCustomPath UMETA(DisplayName = "FollowCustomPath"),
};

UENUM(BlueprintType, Blueprintable)
enum class EAgentAvoidanceLevel : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Advanced UMETA(DisplayName = "Advanced"),
};