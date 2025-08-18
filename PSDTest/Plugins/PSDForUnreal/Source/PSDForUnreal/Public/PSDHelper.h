// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <string>
#include <locale>
#include <codecvt>
#include <vector>
#include "Psd/Psd.h"
// for convenience reasons, we directly include the platform header from the PSD library.
// we could have just included <Windows.h> as well, but that is unnecessarily big, and triggers lots of warnings.
#include "Psd/PsdPlatform.h"

// in the sample, we use the provided malloc allocator for all memory allocations. likewise, we also use the provided
// native file interface.
// in your code, feel free to use whatever allocator you have lying around.
#include "Psd/PsdMallocAllocator.h"
#include "Psd/PsdNativeFile.h"

#include "Psd/PsdDocument.h"
#include "Psd/PsdColorMode.h"
#include "Psd/PsdLayer.h"
#include "Psd/PsdChannel.h"
#include "Psd/PsdChannelType.h"
#include "Psd/PsdLayerMask.h"
#include "Psd/PsdVectorMask.h"
#include "Psd/PsdLayerMaskSection.h"
#include "Psd/PsdImageDataSection.h"
#include "Psd/PsdImageResourcesSection.h"
#include "Psd/PsdParseDocument.h"
#include "Psd/PsdParseLayerMaskSection.h"
#include "Psd/PsdParseImageDataSection.h"
#include "Psd/PsdParseImageResourcesSection.h"
#include "Psd/PsdLayerCanvasCopy.h"
#include "Psd/PsdInterleave.h"
#include "Psd/PsdPlanarImage.h"
#include "Psd/PsdExport.h"
#include "Psd/PsdExportDocument.h"
#include "Psd/PsdAllocator.h"

#include "PsdTgaExporter.h"
#include "PsdDebug.h"
#include "nlohmann/json.hpp"

#include <vector>
#include <map>
#include <string>
#include <map>
#include <variant>
#include <optional>

#define SCREENSize FVector2D(1100,682)

// Foward declaration if necessary, to avoid circular dependencies.
struct PanelContext;
struct Layer;
class Allocator;

static const unsigned int CHANNEL_NOT_FOUND = UINT_MAX;
/**
 * @struct PanelContext
 * @brief 用来存储和表示UI控件的层级结构信息，通常用于从设计文件生成UMG。
 * This struct is used to store and represent the hierarchical information of UI controls,
 * often for generating UMG from design files.
 */
struct PanelContext
{

    /** @brief PSD文件的源路径 (The source path of the PSD file) */
    FString PsdPath;

    /** @brief 父控件的名称 (The name of the parent control) */
    PanelContext* Parent;

    /** @brief 当前控件的名称 (The name of the current control) */
    FString ControlName;

    /** @brief 当前控件的类型 (例如, "Button", "Text", "Image") (The type of the current control) */
    FString ControlType;

    /** @brief 控件在设计文件中的矩形区域 (位置和大小) (The rectangle area (position and size) of the control in the design file) */
    int Left, Top, Right, Bottom;

    bool bIsFullScreen = false;
   /* FVector2D Size = FVector2D(0.0f, 0.0f);*/

    /** @brief 子控件的列表 (A list of child controls) */
    std::vector<PanelContext*> Children;

    // Default constructor to initialize pointers and arrays
    PanelContext()
    {
        // It's a good practice to ensure pointers are null and arrays are empty on creation.
        Children.clear();
        Parent = nullptr;
        Left = 0;
        Top = 0;
        Right = 0;
        Bottom = 0;
    }

    // Destructor to clean up allocated memory for children
    ~PanelContext()
    {
        for (PanelContext* Child : Children)
        {
            delete Child;
            Child = nullptr;
        }
        Children.clear();
    }

    float Width()
    {
        return Right - Left;
    }

    float  Height()
    {
        return Bottom - Top;
    }

    FVector2D Size() 
    {
        PanelContext* ContextInfo = FindFirstChildWithControlInfo();
        if (ContextInfo)
        {
            return FVector2D(ContextInfo->Width(), ContextInfo->Height());
        }
        else
        {
            return FVector2D(Width(), Height());
        }
    }

    PanelContext* FindFirstChildOfType(const FString& TypeFilter) const {
        for (PanelContext* child : Children) {
            if (child->ControlType.Equals(TypeFilter, ESearchCase::IgnoreCase)) {
                return child;
            }
        }
        return nullptr;
    }

    std::vector<PanelContext*> FindChildrenOfType(const FString& TypeFilter) const {
        std::vector<PanelContext*> result;
        for (PanelContext* child : Children) {
            if (child->ControlType.Equals(TypeFilter, ESearchCase::IgnoreCase)) {
                result.push_back(child);
            }
        }
        return result;
    }


    std::vector<PanelContext*> FindChildrenWithControlInfo() const
    {
        std::vector<PanelContext*> result;

        // 遍历所有子控件
        for (PanelContext* child : Children)
        {
            // 检查控件名称是否包含"ControlInfo"（不区分大小写）
            if (child->ControlName.Contains(TEXT("ControlInfo"), ESearchCase::IgnoreCase))
            {
                result.push_back(child);
            }
        }

        return result;
    }

    PanelContext* FindFirstChildWithControlInfo() const
    {
        // 遍历所有子控件
        for (PanelContext* child : Children)
        {
            // 检查控件名称是否包含"ControlInfo"（不区分大小写）
            if (child->ControlName.Contains(TEXT("ControlInfo"), ESearchCase::IgnoreCase))
            {
                return child;
            }
        }

        return nullptr;
    }

    bool IsControlInfo() const
    {
        // 检查控件名称是否包含"ControlInfo"（不区分大小写）
        return ControlName.Contains(TEXT("ControlInfo"), ESearchCase::IgnoreCase);
    }

    FVector2D ConvertPsdToUnreal(
        const FVector2D& PsdPosition, // PSD中的左上角坐标
        const FVector2D& PsdSize     // PSD中的元素尺寸
        ) 
    {
        
        // 1. 转换为Unreal单位
//         const FVector2D UnrealSize = PsdSize ;
// 
//         // 2. 计算PSD中心点坐标（Y轴翻转）
//         const FVector2D PsdCenter(
//             PsdPosition.X + PsdSize.X / 2.0f,
//             SCREENSize.Y - (PsdPosition.Y + PsdSize.Y / 2.0f));
// 
//         // 3. 转换为Unreal坐标系（中心原点）
//         const FVector2D UnrealPosition(
//             PsdCenter.X  - SCREENSize.X / (2.0f),
//             PsdCenter.Y - SCREENSize.Y / (2.0f));
// 
//         return FVector2D(UnrealPosition.X, UnrealPosition.Y);
       // 1. 计算元素在PSD画布中的中心点坐标
        const FVector2D PsdCenter = PsdPosition + PsdSize / 2.0f;

        // 2. 计算PSD画布的中心点坐标
        const FVector2D CanvasCenter = SCREENSize / 2.0f;


        FVector2D UnrealPosition = PsdCenter - CanvasCenter;

        // 4. Y轴翻转，将向下增长的坐标系转换为向上增长的
       // UnrealPosition.Y = -UnrealPosition.Y;

        return UnrealPosition;
    }
    FVector2D ConvertPSDCoordinatesToUMGPosition(
        const FVector2D& ParentPos
    )
    {
        PanelContext* ControlInfo = FindFirstChildWithControlInfo();
        if (!ControlInfo)
        {
            FVector2D PSD_Position(static_cast<float>(Left), static_cast<float>(Top));
            FVector2D WidgetSize(static_cast<float>(Right - Left), static_cast<float>(Bottom - Top));
            FVector2D ChildUnrealPosition = ConvertPsdToUnreal(PSD_Position, WidgetSize);
            return ChildUnrealPosition - ParentPos;
        }
        else
        {
            FVector2D PSD_Position(static_cast<float>(ControlInfo->Left), static_cast<float>(ControlInfo->Top));
            FVector2D WidgetSize(static_cast<float>(ControlInfo->Right - ControlInfo->Left), static_cast<float>(ControlInfo->Bottom - ControlInfo->Top));
            FVector2D ChildUnrealPosition = ConvertPsdToUnreal(PSD_Position, WidgetSize);
            return ChildUnrealPosition - ParentPos;
        }
       
    }

    FVector2D GetSelfUMGPosition()
    {
        FVector2D ParentPos = FVector2D(0,0);
        if (Parent == nullptr)
        {
            ParentPos = FVector2D(0, 0);
        }
        else
        {
            ParentPos = Parent->GetSelfUMGPosition();
        }

        PanelContext* ControlInfo = FindFirstChildWithControlInfo();
        if (!ControlInfo)
        {
            FVector2D PSD_Position(static_cast<float>(Left), static_cast<float>(Top));
            FVector2D WidgetSize(static_cast<float>(Right - Left), static_cast<float>(Bottom - Top));
            FVector2D ChildUnrealPosition = ConvertPsdToUnreal(PSD_Position, WidgetSize);
            return ChildUnrealPosition - ParentPos;
        }
        else
        {
            FVector2D PSD_Position(static_cast<float>(ControlInfo->Left), static_cast<float>(ControlInfo->Top));
            FVector2D WidgetSize(static_cast<float>(ControlInfo->Right - ControlInfo->Left), static_cast<float>(ControlInfo->Bottom - ControlInfo->Top));
            FVector2D ChildUnrealPosition = ConvertPsdToUnreal(PSD_Position, WidgetSize);
            return ChildUnrealPosition - ParentPos;
        }
       
    }
};

using ParamValue = std::variant<int, std::string, bool>;
using Params = std::map<std::string, ParamValue>;
struct UIElement {
    std::string name;
    std::string type;
    nlohmann::json params;

    // 安全获取参数的通用方法
//     template<typename T>
//     std::optional<T> get(const std::string& key) const {
//         if (!params.contains(key)) return std::nullopt;
//         return safe_converter<T>::convert(params[key]);
//     }
};

/**
 * 
 */
class PSDFORUNREAL_API FPSDHelper
{
public:
	FPSDHelper();
	~FPSDHelper();


public:
    // Function to convert std::string (UTF-8) to std::wstring
    std::wstring string_to_wstring(const std::string& str) {
        // Deprecated in C++17, but still works on many compilers.
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    // Function to convert std::wstring to std::string (UTF-8)
    std::string wstring_to_string(const std::wstring& wstr) {
        // Deprecated in C++17.
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }

    unsigned int FindChannel(PSD_NAMESPACE_NAME::Layer* layer, int16_t channelType)
    {
        for (unsigned int i = 0; i < layer->channelCount; ++i)
        {
            PSD_NAMESPACE_NAME::Channel* channel = &layer->channels[i];
            if (channel->data && channel->type == channelType)
                return i;
        }

        return CHANNEL_NOT_FOUND;
    }

    template <typename T, typename DataHolder>
    static void* ExpandChannelToCanvas(PSD_NAMESPACE_NAME::Allocator* allocator, const DataHolder* layer, const void* data, unsigned int canvasWidth, unsigned int canvasHeight)
    {
        T* canvasData = static_cast<T*>(allocator->Allocate(sizeof(T) * canvasWidth * canvasHeight, 16u));
        memset(canvasData, 0u, sizeof(T) * canvasWidth * canvasHeight);

        PSD_NAMESPACE_NAME::imageUtil::CopyLayerData(static_cast<const T*>(data), canvasData, layer->left, layer->top, layer->right, layer->bottom, canvasWidth, canvasHeight);

        return canvasData;
    }


    static void* ExpandChannelToCanvas(const PSD_NAMESPACE_NAME::Document* document, PSD_NAMESPACE_NAME::Allocator* allocator, PSD_NAMESPACE_NAME::Layer* layer, PSD_NAMESPACE_NAME::Channel* channel)
    {
        if (document->bitsPerChannel == 8)
            return ExpandChannelToCanvas<uint8_t>(allocator, layer, channel->data, document->width, document->height);
        else if (document->bitsPerChannel == 16)
            return ExpandChannelToCanvas<uint16_t>(allocator, layer, channel->data, document->width, document->height);
        else if (document->bitsPerChannel == 32)
            return ExpandChannelToCanvas<float32_t>(allocator, layer, channel->data, document->width, document->height);

        return nullptr;
    }


    template <typename T>
    static void* ExpandMaskToCanvas(const PSD_NAMESPACE_NAME::Document* document, PSD_NAMESPACE_NAME::Allocator* allocator, T* mask)
    {
        if (document->bitsPerChannel == 8)
            return ExpandChannelToCanvas<uint8_t>(allocator, mask, mask->data, document->width, document->height);
        else if (document->bitsPerChannel == 16)
            return ExpandChannelToCanvas<uint16_t>(allocator, mask, mask->data, document->width, document->height);
        else if (document->bitsPerChannel == 32)
            return ExpandChannelToCanvas<float32_t>(allocator, mask, mask->data, document->width, document->height);

        return nullptr;
    }

    template <typename T>
    T* CreateInterleavedImage(PSD_NAMESPACE_NAME::Allocator* allocator, const void* srcR, const void* srcG, const void* srcB, unsigned int width, unsigned int height)
    {
        T* image = static_cast<T*>(allocator->Allocate(width * height * 4u * sizeof(T), 16u));

        const T* r = static_cast<const T*>(srcR);
        const T* g = static_cast<const T*>(srcG);
        const T* b = static_cast<const T*>(srcB);
        PSD_NAMESPACE_NAME::imageUtil::InterleaveRGB(r, g, b, T(0), image, width, height);

        return image;
    }


    template <typename T>
    T* CreateInterleavedImage(PSD_NAMESPACE_NAME::Allocator* allocator, const void* srcR, const void* srcG, const void* srcB, const void* srcA, unsigned int width, unsigned int height)
    {
        T* image = static_cast<T*>(allocator->Allocate(width * height * 4u * sizeof(T), 16u));

        const T* r = static_cast<const T*>(srcR);
        const T* g = static_cast<const T*>(srcG);
        const T* b = static_cast<const T*>(srcB);
        const T* a = static_cast<const T*>(srcA);
        PSD_NAMESPACE_NAME::imageUtil::InterleaveRGBA(r, g, b, a, image, width, height);

        return image;
    }


    std::wstring GetSampleOutputPath(void);
public:
	bool ResolvePSD(FString InPsdPath);
    void GenerateContext(PSD_NAMESPACE_NAME::LayerMaskSection* InLayerMaskSection);

    std::optional<UIElement> ParseUIElement(const std::string& input);
    void ProcessButtonTextures(PanelContext* RootNode);

    std::vector<PanelContext*>& GetRootNodes()
    {
        return rootNodes;
    }

   
    void  SavePNG_Unreal(const FString& FilePath, int32 Width, int32 Height, int32 Channels, const uint8_t* Data);
    FString GetPSDTexturePath()
    {
        return FPaths::ProjectContentDir() / TEXT("UI") / FileName;
    }

private:
    bool bGeneratedPNG = true;

    std::vector<PanelContext*> rootNodes;
    std::map<PSD_NAMESPACE_NAME::Layer*, PanelContext*> nodeMap;

   FString FileName = "";
};
