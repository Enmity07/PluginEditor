// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Widgets/Input/SSpinBox.h"

#include "NAIAgentSettingsGlobals.h"

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
	// TSharedPtr<SSpinBox<uint16>> SpinBoxTest;
	
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
