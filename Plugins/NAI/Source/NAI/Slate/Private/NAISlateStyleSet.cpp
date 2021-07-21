// Fill out your copyright notice in the Description page of Project Settings.

#include "NAI/Slate/Public/NAISlateStyleSet.h"

#include "Styling/SlateStyleRegistry.h"

TUniquePtr<FSlateStyleSet> FNAISlateStyleSet::MSStyleInstance = nullptr;

void FNAISlateStyleSet::Initialize()
{
	// Don't continue if the style already exists
	if(MSStyleInstance.IsValid())
	{
		return;
	}
	
	MSStyleInstance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*MSStyleInstance); // Register this custom slate style

	// Get a reference to the Small Logo
	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(
	MSStyleInstance->RootToContentDir(TEXT("TempLogoSmall.png")),
	FVector2D(260.0f, 90.0f));

	if(ThumbnailBrush) // Don't add this as a property if the image doesn't exist
	{
		MSStyleInstance->Set("MainNAIAgentClient_logo", ThumbnailBrush); // Create the slate property for the image
	}
}

void FNAISlateStyleSet::Shutdown()
{
	if (MSStyleInstance.IsValid()) // Nothing to do if the style doesn't exist
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*MSStyleInstance); // Unregister the style
		MSStyleInstance.Reset(); // Reset the static UniquePtr so it can be reinitialized during hot-reloads
	}
}

const ISlateStyle& FNAISlateStyleSet::Get()
{
	check(MSStyleInstance);
	return *MSStyleInstance;
}

// Set the StyleSetName here, but it is ofc a getter for that name also
FName FNAISlateStyleSet::GetStyleSetName()
{
	static FName StyleSetName(TEXT("NAIStyleSet")); // Create the name of this StyleSet as a static var
	return StyleSetName;
}

// Set the ContextName here, but it is ofc a getter for that name also
FName FNAISlateStyleSet::GetContextName()
{
	return FName("NAI");
}

TUniquePtr<FSlateStyleSet> FNAISlateStyleSet::Create()
{
	TUniquePtr<FSlateStyleSet> Style = MakeUnique<FSlateStyleSet>(GetStyleSetName());

	// Set the directory based on where the plugin is installed.
	// If it is in a project's plugin folder, then use that, otherwise use the engine's plugin folder
	FString SlateDirectoryPath = FPaths::ProjectPluginsDir() / TEXT("NAI/Resources");
	if(!FPaths::DirectoryExists(SlateDirectoryPath))
		SlateDirectoryPath = FPaths::EnginePluginsDir() / TEXT("NAI/Resources");
	
	Style->SetContentRoot(SlateDirectoryPath);
	check(Style); // Crash if something went wrong creating the style
	return Style;
}

