// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Styling/SlateStyle.h"

class FNAISlateStyleSet
{
public:
	// Initializes the value of MenuStyleInstance and registers it with the Slate Style Registry.
	static void Initialize();

	// Unregisters the Slate Style Set and then resets the MenuStyleInstance pointer.
	static void Shutdown();

	// Retrieves a reference to the Slate Style pointed to by MenuStyleInstance.
	static const class ISlateStyle& Get();

	// Retrieves the name of the Style Set.
	static FName GetStyleSetName();

	static FName GetContextName();
private:
	// Creates the Style Set.
	static TUniquePtr<class FSlateStyleSet> Create(); 

	// Singleton instance used for our Style Set.
	static TUniquePtr<class FSlateStyleSet> MSStyleInstance;
};
