// Copyright Epic Games, Inc. All Rights Reserved.

#include "PSDForUnrealEditorMode.h"
#include "PSDForUnrealEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "PSDForUnrealEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/PSDForUnrealSimpleTool.h"
#include "Tools/PSDForUnrealInteractiveTool.h"

// step 2: register a ToolBuilder in FPSDForUnrealEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "PSDForUnrealEditorMode"

const FEditorModeID UPSDForUnrealEditorMode::EM_PSDForUnrealEditorModeId = TEXT("EM_PSDForUnrealEditorMode");

FString UPSDForUnrealEditorMode::SimpleToolName = TEXT("PSDForUnreal_ActorInfoTool");
FString UPSDForUnrealEditorMode::InteractiveToolName = TEXT("PSDForUnreal_MeasureDistanceTool");


UPSDForUnrealEditorMode::UPSDForUnrealEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UPSDForUnrealEditorMode::EM_PSDForUnrealEditorModeId,
		LOCTEXT("ModeName", "PSDForUnreal"),
		FSlateIcon(),
		true);
}


UPSDForUnrealEditorMode::~UPSDForUnrealEditorMode()
{
}


void UPSDForUnrealEditorMode::ActorSelectionChangeNotify()
{
}

void UPSDForUnrealEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FPSDForUnrealEditorModeCommands& SampleToolCommands = FPSDForUnrealEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UPSDForUnrealSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UPSDForUnrealInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UPSDForUnrealEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FPSDForUnrealEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UPSDForUnrealEditorMode::GetModeCommands() const
{
	return FPSDForUnrealEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
