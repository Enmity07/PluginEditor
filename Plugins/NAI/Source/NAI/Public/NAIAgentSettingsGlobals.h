#pragma once

UENUM()
enum class EAgentType : uint8
{
	PathToPlayer UMETA(DisplayName = "PathToPlayer"),
	PathToLocation UMETA(DisplayName = "PathToLocation"),
	Static UMETA(DisplayName = "StaticLocation"),
	Dynamic UMETA(DisplayName = "DynamicLocation"), 
	FollowCustomPath UMETA(DisplayName = "FollowCustomPath"),
};