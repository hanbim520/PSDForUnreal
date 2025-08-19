#include "CoreMinimal.h"
#include "PSDHelperFunctionLibrary.h"
#include "PSDHelper.h"
#include "GenerateUMGHelper.h"

static void HandleConvertPSDCommand(const TArray<FString>& Args)
{
    if (Args.Num() < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("Usage: PSD.ConvertToUMG <PSDPath> <AssetPath>"));
        UE_LOG(LogTemp, Error, TEXT("Example: PSD.ConvertToUMG \"E:/PSDForUnreal/psd/Sample.psd\" \"/Game/WBP_FromPSD\""));
        return;
    }

    FString PSDPath = Args[0];
    FString AssetPath = Args[1];

    // Remove quotes if present
    PSDPath = PSDPath.Replace(TEXT("\""), TEXT(""));
    AssetPath = AssetPath.Replace(TEXT("\""), TEXT(""));

    // Clean up path formatting using FPaths
    PSDPath = FPaths::ConvertRelativePathToFull(PSDPath);

    UE_LOG(LogTemp, Log, TEXT("Starting PSD to UMG conversion..."));
    UE_LOG(LogTemp, Log, TEXT("PSD Path: %s"), *PSDPath);
    UE_LOG(LogTemp, Log, TEXT("Asset Path: %s"), *AssetPath);

    // Validate file exists using FPaths
    if (!FPaths::FileExists(PSDPath))
    {
        UE_LOG(LogTemp, Error, TEXT("PSD file does not exist: %s"), *PSDPath);
        UE_LOG(LogTemp, Error, TEXT("Absolute path: %s"), *FPaths::ConvertRelativePathToFull(PSDPath));
        return;
    }

    // Execute PSD to UMG conversion
    FPSDHelper* PSDHelper = new FPSDHelper();
    
    // Check ResolvePSD return value (0 = success, 1 = failure)
    int32 Result = PSDHelper->ResolvePSD(PSDPath);
    if (Result != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to resolve PSD file: %s"), *PSDPath);
        delete PSDHelper;
        return;
    }

    // Check root nodes
    const auto& RootNodes = PSDHelper->GetRootNodes();
    if (RootNodes.size() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No root nodes found in PSD: %s"), *PSDPath);
        delete PSDHelper;
        return;
    }

    // Get main panel node
    PanelContext* MainPanelNode = RootNodes[0];
    if (!MainPanelNode)
    {
        UE_LOG(LogTemp, Error, TEXT("Main panel node is null: %s"), *PSDPath);
        delete PSDHelper;
        return;
    }

    // Generate UMG
    FGenerateUMGHelper Helper;
    Helper.SetPSDHepler(PSDHelper);
    Helper.GenerateUMGFromHierarchy(AssetPath, MainPanelNode);

    delete PSDHelper;
}

static FAutoConsoleCommand ConvertPSDCommand(
    TEXT("PSD.ConvertToUMG"),
    TEXT("Convert PSD file to UMG widget. Usage: PSD.ConvertToUMG <PSDPath> <AssetPath>")
    TEXT("<PSDPath>: D:\\PSDForUnreal\\psd\\SamplePS.psd")
    TEXT("<AssetPath>: /Game/WBP_SamplePS")
    TEXT("Example: PSD.ConvertToUMG \"D:\\PSDForUnreal\\psd\\SamplePS.psd\" \"/Game/WBP_SamplePS\""),
    FConsoleCommandWithArgsDelegate::CreateStatic(&HandleConvertPSDCommand)
);