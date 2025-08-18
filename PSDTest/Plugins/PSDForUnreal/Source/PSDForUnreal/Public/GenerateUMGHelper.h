// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WidgetBlueprint.h"
#include "PSDHelper.h"

/**
 * 
 */
class PSDFORUNREAL_API FGenerateUMGHelper
{
public:
	FGenerateUMGHelper();
	~FGenerateUMGHelper();

public:
	UWidgetBlueprint* CreateUMGBP(const FString& AssetPath);
	UWidget* MakeWidgetWithWBP(UClass* WidgetClass, UWidgetBlueprint* ParentWBP, const FString& WidgetName);
	void SetWBPRootWidget(UWidgetBlueprint* ParentWBP, UWidget* Widget);
	void CompileAndSaveBP(UBlueprint* BPObject);
	bool ApplyInterfaceToBP(UBlueprint* BPObject, UClass* InterfaceClass);
	TSubclassOf<UObject> GetBPGeneratedClass(UBlueprint* BPObject);
	void SetWidgetCenterAlignment(UWidget* Widget, PanelContext* Info);

	PanelContext* GetControlInfo(PanelContext* Self);

	
public:
	UWidgetBlueprint* GenerateUMGFromHierarchy(const FString& AssetPath, PanelContext* RootNode);
	UWidget* CreateWidgetRecursive(UWidgetBlueprint* WBP, class UPanelWidget* ParentWidget, PanelContext* Node);
	void ParseLayerName(const FString& FullName, FString& OutName, FString& OutType, FString& OutParams);
	UClass* GetUMGClassFromString(const FString& TypeString);
	void ConfigureWidgetFromChildren(UWidgetBlueprint* WBP, UWidget* WidgetToConfigure, PanelContext* Node);

};
