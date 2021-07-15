// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "Styling/SlateStyle.h"

class NAI_API FCustomAgentClientDetailsPanel : public IDetailCustomization
{
private:
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	void OnMoveSpeedPropertyChanged();
	
	void OnMoveSpeedCommitted(
		const FText& InText,
		ETextCommit::Type InCommitType
	);

	TSharedPtr<IPropertyHandle> MoveSpeedHandle;

	TSharedPtr<SEditableTextBox> MoveSpeedTextBox;
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class NAI_API FCustomAgentManagerDetailsPanel : public IDetailCustomization
{
private:
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
