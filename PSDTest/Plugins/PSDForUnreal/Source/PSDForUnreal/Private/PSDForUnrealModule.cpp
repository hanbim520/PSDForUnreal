// Copyright Epic Games, Inc. All Rights Reserved.

#include "PSDForUnrealModule.h"
#include "PSDForUnrealEditorModeCommands.h"


#define LOCTEXT_NAMESPACE "PSDForUnrealModule"

void FPSDForUnrealModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FPSDForUnrealEditorModeCommands::Register();



}

void FPSDForUnrealModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FPSDForUnrealEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPSDForUnrealModule, PSDForUnrealEditorMode)