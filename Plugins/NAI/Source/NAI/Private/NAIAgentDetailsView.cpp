﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "NAIAgentDetailsView.h"
#include "NAI.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "NAIAgentClient.h"
// #include "../../../../../../../../UE_4.26/Engine/Plugins/Experimental/AlembicImporter/Source/AlembicLibrary/Public/AbcFile.h"
#include "Styling/SlateStyleRegistry.h"

FReply FCustomAgentClientDetailsPanel::OnTestButtonClicked()
{
	if(LogoImage.IsValid())
		LogoImage->SetVisibility(EVisibility::Hidden);
	return FReply::Handled();
}

void FCustomAgentClientDetailsPanel::OnMoveSpeedPropertyChanged()
{
	if(MoveSpeedTextBox.IsValid())
	{
		if(MoveSpeedHandle.IsValid())
		{
			float Value;
			MoveSpeedHandle.Get()->GetValue(Value);
			MoveSpeedTextBox.Get()->SetText(FText::FromString(FString::SanitizeFloat(Value)));
		}
	}
}

void FCustomAgentClientDetailsPanel::OnMoveSpeedCommitted(
	const FText& InText,
	ETextCommit::Type InCommitType)
{
	if(MoveSpeedHandle.IsValid())
	{
		MoveSpeedHandle.Get()->SetValue(FCString::Atof(*InText.ToString()));	
	}
}

TSharedRef<IDetailCustomization> FCustomAgentClientDetailsPanel::MakeInstance()
{
	return MakeShareable(new FCustomAgentClientDetailsPanel);
}

void FCustomAgentClientDetailsPanel::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	/* Below is a load of example code for editing the details view.
	 * To edit an entire category, simply hide it then create a new custom one
	 * Use the DetailBuilder to get the properties of the class, bind the appropriate delegates,
	 * then edit the property, again through the DetailBuilder, to get the IDetailPropertyRow
	 * pointer, which is a pointer to the properties widget/row in the details panel,
	 * create a custom widget for the property, which will override it's original one.
	 * Do this property by property until you've created a full custom detail view
	 */
	
	// Add a custom row to contain the logo
	
	// Edit the Agent Category
	IDetailCategoryBuilder& CustomAgentCategory = DetailBuilder.EditCategory("Agent");
	// Grab the custom Slate Style for this module
	const ISlateStyle* SlateStyle = FSlateStyleRegistry::FindSlateStyle("NAIStyleSet");

	// Get Property
	MoveSpeedHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ANAIAgentClient, MoveSpeed));
	check(MoveSpeedHandle.IsValid());
	// Bind Delegate
	MoveSpeedHandle.Get()->SetOnPropertyValueChanged(
	FSimpleDelegate::CreateSP(this,
		&FCustomAgentClientDetailsPanel::OnMoveSpeedPropertyChanged)
	);
	
	IDetailPropertyRow* CustomPropertyRow = DetailBuilder.EditDefaultProperty(MoveSpeedHandle);

	float Value;
	MoveSpeedHandle.Get()->GetValue(Value);
	
	//CustomPropertyRow->CustomWidget()
	//.NameContent()
	//[
	//	SNew(STextBlock)
	//	.Text(FText::FromString("Name"))
	//];
	//.ValueContent()
	//[
	//	SAssignNew(MoveSpeedTextBox, SEditableTextBox)
	//	.Text(FText::FromString(FString::SanitizeFloat(Value)))
	//	.OnTextCommitted(this, &FCustomAgentClientDetailsPanel::OnMoveSpeedCommitted)
	//];
	
	// Add the custom Row
	CustomAgentCategory.AddCustomRow(FText::FromString("Agent Settings"))
	.WholeRowContent()
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			[
				// Create the SImage to put the logo image in
				SAssignNew(LogoImage, SImage)
				.Image(SlateStyle->GetBrush(TEXT("MainNAIAgentClient_logo")))
			]
		]
		+ SVerticalBox::Slot()
		.Padding(FMargin(0.0f, 10.0f, 0.0f, 10.0f))
		.MaxHeight(25.0f)
		[
			SNew(SButton)
			.ContentScale(FVector2D())
			.Text(FText::FromString(TEXT("A Button")))
			.OnClicked_Raw(this, &FCustomAgentClientDetailsPanel::OnTestButtonClicked)
		]
	];
	

}

void FCustomAgentClientDetailsPanel::CreatePathToPlayerDetails()
{
}

void FCustomAgentClientDetailsPanel::CreatePathToLocationDetails()
{
}

void FCustomAgentClientDetailsPanel::CreateFollowCustomPathDetails()
{
}

void FCustomAgentClientDetailsPanel::CreateStaticPathToLocationDetails()
{
}

void FCustomAgentClientDetailsPanel::CreateDynamicPathToLocationDetails()
{
}

TSharedRef<IDetailCustomization> FCustomAgentManagerDetailsPanel::MakeInstance()
{
	return MakeShareable(new FCustomAgentManagerDetailsPanel);
}

void FCustomAgentManagerDetailsPanel::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{

}
