// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

enum class EAgentType : uint8
{
	PathToPlayer UMETA(DisplayName = "PathToPlayer"),
	PathToLocation UMETA(DisplayName = "PathToLocation"),
	FollowCustomPath UMETA(DisplayName = "FollowCustomPath"),
};

enum class EAgentPathToLocationType : uint8
{
	Static UMETA(DisplayName = "StaticLocation"),
	Dynamic UMETA(DisplayName = "DynamicLocation"),
};

// Custom details view for the AgentClient class
class NAI_API FCustomAgentClientDetailsPanel : public IDetailCustomization
{
private:
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	TSharedPtr<SVerticalBox> MainVerticalBox;
	TSharedPtr<SHorizontalBox> MainHorizontalBox;
	
	TSharedPtr<SImage> LogoImage;
	
	FReply OnTestButtonClicked();

	TSharedPtr<STextBlock> PathToPlayerInformation;
	
	// ----------------------------
	// Class Property Customization
	// ----------------------------

	// MoveSpeed Property
	
	TSharedPtr<IPropertyHandle> MoveSpeedHandle;
	TSharedPtr<SEditableTextBox> MoveSpeedTextBox;
	
	// Delegate function executed when the MoveSpeed class property changes
	void OnMoveSpeedPropertyChanged();
	// Delegate function executed when when the MoveSpeedTextBox loses focus/enter is pressed
	void OnMoveSpeedCommitted(const FText& InText, ETextCommit::Type InCommitType);

public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	
private:
	void CreatePathToPlayerDetails();
	void CreatePathToLocationDetails();
	void CreateFollowCustomPathDetails();

	void CreateStaticPathToLocationDetails();
	void CreateDynamicPathToLocationDetails();
};

// Custom details view for the AgentManager class
class NAI_API FCustomAgentManagerDetailsPanel : public IDetailCustomization
{
private:
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
