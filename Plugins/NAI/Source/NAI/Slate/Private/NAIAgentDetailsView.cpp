// Fill out your copyright notice in the Description page of Project Settings.

#include "NAI/Slate/Public/NAIAgentDetailsView.h"
#include "NAI.h"
#include "NAIAgentClient.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Styling/SlateStyleRegistry.h"

FReply FCustomAgentClientDetailsPanel::OnTestButtonClicked()
{
	if(LogoImage.IsValid())
		LogoImage.Get()->SetVisibility(EVisibility::Hidden);

	if(MainHorizontalBox.IsValid())
		MainHorizontalBox.Get()->RemoveSlot(LogoImage.ToSharedRef());
	
	if(MainVerticalBox.IsValid())
		MainVerticalBox.Get()->RemoveSlot(MainHorizontalBox.ToSharedRef());

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
	// Edit the Agent Category
	IDetailCategoryBuilder& CustomAgentCategory = DetailBuilder.EditCategory("Agent");
	// Grab the custom Slate Style for this module
	const ISlateStyle* SlateStyle = FSlateStyleRegistry::FindSlateStyle("NAIStyleSet");

	//// Get Property
	//MoveSpeedHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ANAIAgentClient, MoveSpeed));
	//check(MoveSpeedHandle.IsValid());
	//// Bind Delegate
	//MoveSpeedHandle.Get()->SetOnPropertyValueChanged(
	//FSimpleDelegate::CreateSP(this,
	//	&FCustomAgentClientDetailsPanel::OnMoveSpeedPropertyChanged)
	//);
	
	//IDetailPropertyRow* CustomPropertyRow = DetailBuilder.EditDefaultProperty(MoveSpeedHandle);

	//float Value;
	//MoveSpeedHandle.Get()->GetValue(Value);
	
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

	// Below is an example of how to add/remove slots through a button
	// Intended use is to be able to hide/show different sections of the details
	// panel. So a drop down of different categories could be used to toggle/switch
	// between different details sections
	
	
	// Add the custom Row
	CustomAgentCategory.AddCustomRow(FText::FromString("Agent Settings"))
	.WholeRowContent()
	.VAlign(VAlign_Center)
	[
		SAssignNew(MainVerticalBox, SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SAssignNew(MainHorizontalBox, SHorizontalBox)
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
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(SlateStyle->GetBrush("NAI.BackgroundColor"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("123")))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("F345")))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("F34534")))
				]
			]
		]

		+ SVerticalBox::Slot() // CreatePathToPlayer Details Section
		.HAlign(HAlign_Center)
		.FillHeight(true)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(true)
			[
				SNew(SBorder)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ForegroundColor(FSlateColor(FLinearColor(0.02f, 0, 1, 1)))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0, 0, 0, 1)))
				.ColorAndOpacity(FLinearColor(0.02f, 0, 1, 1))
				[
					SNew(SBox)
					.MinDesiredWidth(175)	// to enforce some minimum width, ideally we define the minimum, not a fixed width
					.HeightOverride(250)
					[
						SAssignNew(PathToPlayerInformation, STextBlock)
						.Text(FText::FromString(TEXT(
							"Information About this type of Ageghghfghfghfghfghfghnt.\n"
							"This is a test of tsomdddddddddddddddddddddddddddddddddd\n"
							"4523W34dddddddddddddddddddddddddddddddddddddddddddddgggg\n"
							"dfgghgjhjlkl;ddddddddddddddddddddddddddddddddddddddddddd\n"
						)))
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FLinearColor(99, 255, 150, 1))
					]
				]
			]
		]


		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		[
			SAssignNew(SpinBoxTest, SSpinBox<uint16>)
			.MinValue(1)
			.MaxValue(500)
			.Value(50)
			//.OnValueChanged_Lambda(
			//	[this](uint16 InValue) { NewFaderStartingAddress = InValue; }
			//)
			.MinDesiredWidth(60.0f)
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
