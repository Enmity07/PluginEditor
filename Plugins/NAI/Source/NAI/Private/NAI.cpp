// Copyright Epic Games, Inc. All Rights Reserved.

#include "NAI.h"
#include "NAIAgentClient.h"
#include "NAIAgentManager.h"
#include "NAI/Slate/Public/NAISlateStyleSet.h"
#include "NAI/Slate/Public/NAIAgentDetailsView.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FNAIModule"

void FNAIModule::StartupModule()
{
	// Get a reference to the PropertyModule to add the custom DetailsViews
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Bind the custom details view for the AgentClient
	PropertyModule.RegisterCustomClassLayout(
		ANAIAgentClient::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FCustomAgentClientDetailsPanel::MakeInstance)
	);
	
	// Bind the custom details view for the AgentManager
	PropertyModule.RegisterCustomClassLayout(
		ANAIAgentManager::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FCustomAgentManagerDetailsPanel::MakeInstance)
	);

	FNAISlateStyleSet::Initialize();
}

void FNAIModule::ShutdownModule()
{
	FNAISlateStyleSet::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNAIModule, NAI)