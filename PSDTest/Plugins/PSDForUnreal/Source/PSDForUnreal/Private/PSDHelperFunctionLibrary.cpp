// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDHelperFunctionLibrary.h"
#include "PSDHelper.h"
#include "GenerateUMGHelper.h"


void UPSDHelperFunctionLibrary::ConvertPSDToUMG()
{
    FPSDHelper* PSDHelper = new FPSDHelper();

    PSDHelper->ResolvePSD(TEXT("E:/PSDForUnreal/psd/Sample.psd"));


    FGenerateUMGHelper Helper;
    FString AssetPath = TEXT("/Game/WBP_FromPSD"); // Define your output path

    // Assuming the first root node represents the entire panel.
    PanelContext* MainPanelNode = PSDHelper->GetRootNodes()[0];
    Helper.GenerateUMGFromHierarchy(AssetPath, MainPanelNode);
}