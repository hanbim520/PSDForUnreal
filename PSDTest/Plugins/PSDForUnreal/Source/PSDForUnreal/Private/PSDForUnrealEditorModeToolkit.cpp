// Copyright Epic Games, Inc. All Rights Reserved.

#include "PSDForUnrealEditorModeToolkit.h"
#include "PSDForUnrealEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "PSDForUnrealEditorModeToolkit"

FPSDForUnrealEditorModeToolkit::FPSDForUnrealEditorModeToolkit()
{
}

void FPSDForUnrealEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FPSDForUnrealEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FPSDForUnrealEditorModeToolkit::GetToolkitFName() const
{
	return FName("PSDForUnrealEditorMode");
}

FText FPSDForUnrealEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "PSDForUnrealEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
