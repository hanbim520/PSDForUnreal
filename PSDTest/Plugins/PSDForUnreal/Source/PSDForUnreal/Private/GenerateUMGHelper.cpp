// Fill out your copyright notice in the Description page of Project Settings.


#include "GenerateUMGHelper.h"

#include "WidgetBlueprintFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "WidgetBlueprint.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "FileHelpers.h"

//UMG 
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"


FGenerateUMGHelper::FGenerateUMGHelper()
{
}

FGenerateUMGHelper::~FGenerateUMGHelper()
{
}

UWidgetBlueprint* FGenerateUMGHelper::CreateUMGBP(const FString& AssetPath)
{
    const FString PackagePath = FPaths::GetPath(AssetPath);
    const FString AssetName = FPaths::GetBaseFilename(AssetPath);

    // 1. 检查并确保目录存在
    FString AbsolutePath;
    if (!FPackageName::TryConvertGameRelativePackagePathToLocalPath(PackagePath, AbsolutePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid package path: %s"), *PackagePath);
        return nullptr;
    }
    if (!IFileManager::Get().DirectoryExists(*AbsolutePath))
    {
        IFileManager::Get().MakeDirectory(*AbsolutePath, true);
    }

    // 2. 检查资产是否已存在，如果存在则删除
    FString PackageName = FPaths::Combine(*PackagePath, *AssetName);
    if (FPackageName::DoesPackageExist(PackageName))
    {
        // 获取物理文件路径
        FString ExistingFileName;
        if (FPackageName::DoesPackageExist(PackageName, &ExistingFileName))
        {
            // 删除文件前先确保编辑器已卸载该资产
            TArray<UPackage*> PackagesToUnload;
            UPackage* ExistingPackage = FindPackage(nullptr, *PackageName);
            if (ExistingPackage)
            {
                PackagesToUnload.Add(ExistingPackage);
            }

            // 删除文件
            IFileManager::Get().Delete(*ExistingFileName);
            UE_LOG(LogTemp, Log, TEXT("Deleted existing asset file: %s"), *ExistingFileName);
        }
    }

    // 3. 创建新资产
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
    Factory->ParentClass = UUserWidget::StaticClass();

    UObject* NewAsset = AssetTools.CreateAsset(*AssetName, *PackagePath, UWidgetBlueprint::StaticClass(), Factory);

    if (!NewAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create asset '%s' at path '%s'"), *AssetName, *PackagePath);
        return nullptr;
    }

    // 4. 标记为需要保存并通知资产注册表
    NewAsset->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewAsset);

    return Cast<UWidgetBlueprint>(NewAsset);
}

UWidget* FGenerateUMGHelper::MakeWidgetWithWBP(UClass* WidgetClass, UWidgetBlueprint* ParentWBP, const FString& WidgetName)
{
    UWidgetTree* WidgetTree = ParentWBP->WidgetTree;
    UWidget* CanvasPanel = WidgetTree->ConstructWidget<UWidget>(WidgetClass, *WidgetName);

    return CanvasPanel;
}

void FGenerateUMGHelper::SetWBPRootWidget(UWidgetBlueprint* ParentWBP, UWidget* Widget)
{
    if (ParentWBP && ParentWBP->WidgetTree && Widget)
    {
        ParentWBP->WidgetTree->RootWidget = Widget;
    }
}

void FGenerateUMGHelper::CompileAndSaveBP(UBlueprint* BPObject)
{
    if(!BPObject)
    {
        return;
    }

    // 1. 编译蓝图确保其状态有效
    FKismetEditorUtilities::CompileBlueprint(BPObject);

    UPackage* const Package = BPObject->GetPackage();
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find package for Blueprint: %s"), *BPObject->GetName());
        return;
    }
    Package->SetDirtyFlag(true);
    Package->FullyLoad();
    // 2. 只保存当前蓝图包
    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension());

    // 3. 使用UPackage::SavePackage直接保存这个包
    bool bSaved = UPackage::SavePackage(
        Package,
        BPObject,
        RF_Standalone,
        *PackageFileName,
        GError,
        nullptr,
        true,
        true,
        SAVE_NoError);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save Blueprint: %s"), *BPObject->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved Blueprint: %s"), *BPObject->GetName());
    }
   
}

bool FGenerateUMGHelper::ApplyInterfaceToBP(UBlueprint* BPObject, UClass* InterfaceClass)
{
    if (BPObject && InterfaceClass)
    {
        TArray<struct FBPInterfaceDescription> InterfaceDescription;
        FBPInterfaceDescription Description;
        Description.Interface = InterfaceClass;

        InterfaceDescription.Add(Description);
        BPObject->ImplementedInterfaces = InterfaceDescription;
        return true;
    }
    return false;
}

TSubclassOf<UObject> FGenerateUMGHelper::GetBPGeneratedClass(UBlueprint* BPObject)
{
    if (BPObject)
    {
        return BPObject->GeneratedClass;
    }
    return nullptr;
}

void FGenerateUMGHelper::SetWidgetCenterAlignment(UWidget* Widget, PanelContext* Info)
{
    if (!Widget)
    {
        return;
    }

    // Set anchors to center (0.5, 0.5)
   // Widget->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));

    // For slot-based widgets (like those in CanvasPanel)
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
    {
        Slot->SetSize(FVector2D(Widget->GetDesiredSize().X, Widget->GetDesiredSize().Y));

        FAnchorData AnchorData;
        AnchorData.Anchors = FAnchors(0.5, 0.5, 0.5, 0.5); // 全屏锚点
        AnchorData.Offsets = FMargin(0, 0, 0, 0);  // 无偏移
        AnchorData.Alignment = FVector2D(0.5f, 0.5f); // 中心对齐

        Slot->LayoutData = AnchorData;

        FVector2D ParentPos = FVector2D(0,0);
        if (Info == nullptr)
        {
            Slot->SetPosition(FVector2D(0,0));
            Slot->SetSize(SCREENSize);
        }
        else
        {
            Slot->SetSize(Info->Size());

            FVector2D WidgetPosition = Info->GetSelfUMGPosition();
            Slot->SetPosition(FVector2D(WidgetPosition.X, WidgetPosition.Y));
            
        }
    }
    // For other panel slots (like Vertical/Horizontal Box)
    else if (Widget->Slot)
    {

    }
}


// --- Main Orchestration Function ---
UWidgetBlueprint* FGenerateUMGHelper::GenerateUMGFromHierarchy(const FString& AssetPath, PanelContext* RootNode)
{
    if (!RootNode)
    {
        UE_LOG(LogTemp, Error, TEXT("RootNode is null. Cannot generate UMG."));
        return nullptr;
    }

    // 1. Create a new, empty Widget Blueprint asset.
    UWidgetBlueprint* WBP = CreateUMGBP(AssetPath);
    if (!WBP)
    {
        // CreateUMGBP will log the specific error. We just exit here.
        return nullptr;
    }

    // 2. Create a CanvasPanel to act as the root container.
    UCanvasPanel* RootCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    SetWBPRootWidget(WBP, RootCanvas);

    UCanvasPanel* RootAlignCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootAlignCanvas"));
    RootCanvas->AddChild(RootAlignCanvas);
    SetWidgetCenterAlignment(RootAlignCanvas,nullptr);

    // 3. Start the recursive process. The children of the root PSD node will be added to our RootCanvas.
    for (PanelContext* ChildNode : RootNode->Children)
    {
        if (ChildNode->IsControlInfo())
        {
            // Skip ControlInfo nodes, they are not actual UI elements.
            continue;
        }

        UWidget* Widget = CreateWidgetRecursive(WBP, RootAlignCanvas, ChildNode);
        SetWidgetCenterAlignment(Widget, ChildNode); // Center align the widget if needed
    }

    // 4. Compile and save the newly created asset.
    CompileAndSaveBP(WBP);

    UE_LOG(LogTemp, Log, TEXT("Successfully generated UMG asset: %s"), *AssetPath);
    return WBP;
}

void FGenerateUMGHelper::SupportUnrealType(UWidget* NewWidget, PanelContext* Node)
{
    if (Node->ControlType.Contains(TEXT("Button"),ESearchCase::IgnoreCase))
    {   
       
    } else if (Node->ControlType.Contains(TEXT("Text"), ESearchCase::IgnoreCase))
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(NewWidget))
        {
            TextBlock->SetText(FText::FromString(Node->ControlName));
        }
    }
    else if (Node->ControlType.Contains(TEXT("Image"), ESearchCase::IgnoreCase))
    {
        if (UImage* Image = Cast<UImage>(NewWidget))
        {
            // 这里可以设置图片的资源
            // Image->SetBrushFromTexture(...);
        }
    }
    else if (Node->ControlType.Contains(TEXT("Slider"), ESearchCase::IgnoreCase))
    {
        if (USlider* Slider = Cast<USlider>(NewWidget))
        {
            // 设置滑块的默认值或样式
        }
    }
    else if (Node->ControlType.Contains(TEXT("EditableTextBox"), ESearchCase::IgnoreCase))
    {
        if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(NewWidget))
        {
            EditableTextBox->SetHintText(FText::FromString(Node->ControlName));
        }
    }
    else
    {

    }
}

// --- Recursive Widget Creation Logic ---
UWidget* FGenerateUMGHelper::CreateWidgetRecursive(UWidgetBlueprint* WBP, UPanelWidget* ParentWidget, PanelContext* Node)
{
    if (!WBP || !ParentWidget || !Node) return nullptr;

    FString ObjectName = Node->ControlName;
    FString WidgetType = Node->ControlType;
    //ParseLayerName(Node->ControlName, ObjectName, WidgetType, Params);

    // Skip structural or irrelevant layers gracefully without warnings.
    if (ObjectName.Equals(TEXT("</Layer group>")) || ObjectName.Contains(TEXT("@")) || ObjectName.IsEmpty() || WidgetType.IsEmpty() || ObjectName.Contains(TEXT("ControlInfo"), ESearchCase::IgnoreCase))
    {
        return nullptr;
    }

    UClass* WidgetClass = GetUMGClassFromString(WidgetType);
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not find UMG class for type '%s'. Skipping layer '%s'."), *WidgetType, *Node->ControlName);
        return nullptr;
    }

    UWidget* NewWidget = WBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, FName(*ObjectName));
    if (!NewWidget) return nullptr;
    
    // Add the new widget as a child of its parent.
    if (UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget))
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ParentPanel->AddChild(NewWidget));
        if (CanvasSlot)
        {
            FAnchorData AnchorData;
            AnchorData.Anchors = FAnchors(0.5, 0.5, 0.5, 0.5); // 全屏锚点
            AnchorData.Offsets = FMargin(0, 0, 0, 0);  // 无偏移
            AnchorData.Alignment = FVector2D(0.5f, 0.5f); // 中心对齐

            CanvasSlot->LayoutData = AnchorData;

            const FVector2D Position = Node->GetSelfUMGPosition();
            const FVector2D Size = Node->Size();
            CanvasSlot->SetPosition(Position);
            CanvasSlot->SetSize(Size);
            UTextBlock* TextBlock = Cast<UTextBlock>(NewWidget);
            if (TextBlock)
            {
                CanvasSlot->SetAutoSize(true);
            }
            
        }
    }
    else if (UContentWidget* ParentContentWidget = Cast<UContentWidget>(ParentWidget))
    {
        ParentContentWidget->SetContent(NewWidget);
    }

    // After creating the widget, process its children to configure it or add to it.
    ConfigureWidgetFromChildren(WBP, NewWidget, Node);

    return NewWidget;
}

FString FGenerateUMGHelper::ConvertAbsolutePathToAssetPath(const FString& AbsolutePath)
{
    FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    return AbsolutePath.Replace(*ContentDir, TEXT("/Game/"));
}

void FGenerateUMGHelper::ConfigureWidgetFromChildren(UWidgetBlueprint* WBP, UWidget* WidgetToConfigure, PanelContext* Node)
{


    if (Node->ControlType.Equals(TEXT("Button"), ESearchCase::IgnoreCase))
    {
        SetButtonInfo(WBP, WidgetToConfigure, Node);
      
    }
    else if (Node->ControlType.Equals(TEXT("Image"), ESearchCase::IgnoreCase)
    || Node->ControlType.Contains(TEXT("Texture"), ESearchCase::IgnoreCase))
    {
        if (UImage* Image = Cast<UImage>(WidgetToConfigure))
        {
            SetImageOrTextInfo(WBP, Image, Node);
        }
    }
    else if (Node->ControlType.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
    {
        SetTextBlock(WBP, WidgetToConfigure, Node);
    }
    else if (Node->ControlType.Equals(TEXT("Slider"), ESearchCase::IgnoreCase))
    {
        if (USlider* Slider = Cast<USlider>(WidgetToConfigure))
        {
            // 设置滑块的默认值或样式
        }
    }
    else if (Node->ControlType.Equals(TEXT("EditableTextBox"), ESearchCase::IgnoreCase))
    {
        if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(WidgetToConfigure))
        {
            EditableTextBox->SetHintText(FText::FromString(Node->ControlName));
        }
    }
    else if (Node->ControlType.Equals(TEXT("Toggle"), ESearchCase::IgnoreCase))
    {
        if (UCheckBox* CheckBox = Cast<UCheckBox>(WidgetToConfigure))
        {
            // 设置复选框的默认状态或样式
        }
    }
    else if (Node->ControlType.Equals(TEXT("Common"), ESearchCase::IgnoreCase))
    {
        // Common类型可以是一个占位符，可能需要特殊处理
        // 这里可以添加一些通用的处理逻辑
    }
    else if (Node->ControlType.Equals(TEXT("PSDTestAtlas"), ESearchCase::IgnoreCase))
    {
        // 处理PSDTestAtlas类型的特殊逻辑
        // 这里可以添加一些特定的处理逻辑
    }
    else
    {
        if (UPanelWidget* Panel = Cast<UPanelWidget>(WidgetToConfigure))
        {
            for (PanelContext* ChildNode : Node->Children)
            {
                CreateWidgetRecursive(WBP, Panel, ChildNode);
            }
        }
    }

}
// --- Utility Functions ---
void FGenerateUMGHelper::ParseLayerName(const FString& FullName, FString& OutName, FString& OutType, FString& OutParams)
{
    FString RightPart;
    if (!FullName.Split(TEXT("@"), &OutName, &RightPart))
    {
        OutName = FullName;
        OutType = TEXT("Unknown");
        OutParams = TEXT("");
        return;
    }

    if (!RightPart.Split(TEXT(":"), &OutType, &OutParams))
    {
        OutType = RightPart;
        OutParams = TEXT("");
    }
}

UClass* FGenerateUMGHelper::GetUMGClassFromString(const FString& TypeString)
{
    if (TypeString.IsEmpty() || TypeString.Equals(TEXT("Unknown")))
    {
        // Default to a TextBlock for layers without a type, like "Title".
        return UTextBlock::StaticClass();
    }

    static const TMap<FString, FString> TypeMap = {
        {TEXT("Panel"),       TEXT("/Script/UMG.CanvasPanel")},
        {TEXT("Texture"),     TEXT("/Script/UMG.Image")},
        {TEXT("Text"),        TEXT("/Script/UMG.TextBlock")},
        {TEXT("Button"),      TEXT("/Script/UMG.Button")},
        {TEXT("Slider"),      TEXT("/Script/UMG.Slider")},
        {TEXT("InputField"),  TEXT("/Script/UMG.EditableTextBox")},
        {TEXT("Toggle"),      TEXT("/Script/UMG.CheckBox")},
        // Map your custom types here. We'll map them to UImage as a placeholder.
        {TEXT("PSDTestAtlas"),TEXT("/Script/UMG.Image")},
        {TEXT("Common"),      TEXT("/Script/UMG.Image")}
    };

    const FString* ClassPath = TypeMap.Find(TypeString);
    if (ClassPath)
    {
        return FindObject<UClass>(nullptr, **ClassPath);
    }
    return nullptr;
}

PanelContext* FGenerateUMGHelper::GetControlInfo(PanelContext* Self)
{
    if (!Self)
    {
        return nullptr;
    }

    return  Self->FindFirstChildWithControlInfo();
}


//------------set control info------------

void FGenerateUMGHelper::SetButtonInfo(UWidgetBlueprint* WBP, UWidget* Widget, PanelContext* Node)
{
    if (UButton* Button = Cast<UButton>(Widget))
    {
        for (PanelContext* ChildNode : Node->Children)
        {
            FString ChildType = ChildNode->ControlType;
            if (ChildType == TEXT("Text"))
            {
                CreateWidgetRecursive(WBP, Button, ChildNode);
                continue;
            }

            FString ImageName = ChildNode->ControlName;
            FString PSDPath = PSDHelper->GetPSDTexturePath();
            FString PackagePath = FPaths::Combine(PSDPath, ImageName + TEXT("_Texture"));
            FString AssetPath = ConvertAbsolutePathToAssetPath(PackagePath);

            // 3. 加载并设置纹理...
            if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
            {
                FButtonStyle ButtonStyle = Button->WidgetStyle;
                // 设置普通状态图片
                if (ImageName.Contains(TEXT("_Normal"), ESearchCase::IgnoreCase))
                {
                    ButtonStyle.Normal.SetResourceObject(Texture);
                    ButtonStyle.Normal.TintColor = FLinearColor::White;
                }
                else if (ImageName.Contains(TEXT("_Hovered"), ESearchCase::IgnoreCase))
                {
                    ButtonStyle.Normal.SetResourceObject(Texture);
                    ButtonStyle.Normal.TintColor = FLinearColor::Yellow;
                }
                else if (ImageName.Contains(TEXT("_Pressed"), ESearchCase::IgnoreCase))
                {
                    ButtonStyle.Normal.SetResourceObject(Texture);
                    ButtonStyle.Normal.TintColor = FLinearColor::White;
                }
                else if (ImageName.Contains(TEXT("_Disabled"), ESearchCase::IgnoreCase))
                {
                    ButtonStyle.Normal.SetResourceObject(Texture);
                    ButtonStyle.Normal.TintColor = FLinearColor::Gray;
                }

                // 应用新样式
                Button->SetStyle(ButtonStyle);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to load texture: %s"), *AssetPath);
            }
        }
    }
}

void FGenerateUMGHelper::SetImageOrTextInfo(UWidgetBlueprint* WBP, UWidget* Widget, PanelContext* Node)
{
    if (UImage* Image = Cast<UImage>(Widget))
    {
        FString ImageName = Node->ControlName;
        FString PSDPath = PSDHelper->GetPSDTexturePath();
        FString PackagePath = FPaths::Combine(PSDPath, ImageName + TEXT("_Texture"));
        FString AssetPath = ConvertAbsolutePathToAssetPath(PackagePath);

        // 3. 加载并设置纹理...
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath))
        {
            Image->SetBrushFromTexture(Texture);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load texture: %s"), *AssetPath);
        }
    }
    else if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        TextBlock->SetText(FText::FromString(Node->ControlName));
    }
}

void FGenerateUMGHelper::SetTextBlock(UWidgetBlueprint* WBP, UWidget* Widget, PanelContext* Node)
{
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        TextBlock->SetText(FText::FromString(Node->ControlName));
        // 可以设置其他文本属性，如字体、颜色等
        // TextBlock->SetFont(...);
        // TextBlock->SetColorAndOpacity(...);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Widget is not a TextBlock: %s"), *Node->ControlName);
    }
}