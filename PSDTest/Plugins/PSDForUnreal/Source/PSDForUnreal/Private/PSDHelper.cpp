// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDHelper.h"
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

#include "PsdTgaExporter.h"
#include "PsdDebug.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorReimportHandler.h"


#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ObjectTools.h" // 如果需要导入特定类型（如纹理），可能需要这个
#include "Factories/Factory.h" // UFactory的基类
 

PSD_PUSH_WARNING_LEVEL(0)
// disable annoying warning caused by xlocale(337): warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning(disable:4530)
#include <string>
#include <sstream>
#include <functional>
#include <iostream>
#include "../../../../../../../Source/Editor/UnrealEd/Public/AssetImportTask.h"

PSD_POP_WARNING_LEVEL


class FMyAssetTools
{
public:
#if WITH_EDITOR
    /**
    * @brief [简单] 自动导入一个资产到内容浏览器。
    * @param SourceAssetPath 要导入的源文件在硬盘上的绝对路径 (例如 "D:/MyTextures/T_Rock.png")
    * @param GameDestinationPath 资产在游戏内容目录中的目标【文件夹】路径 (例如 "/Game/Textures/Rocks")
    * @return 返回成功导入后创建的UObject对象，如果失败则返回nullptr
    * @note 资产的名称将根据SourceAssetPath的文件名自动生成。例如 "T_Rock.png" 会生成名为 "T_Rock" 的资产。
    */
    static UObject* ImportAsset(const FString& SourceAssetPath, const FString& GameDestinationPath)
    {
        // --- 1. 检查输入参数是否有效 ---
        if (!FPaths::FileExists(SourceAssetPath))
        {
            UE_LOG(LogTemp, Error, TEXT("无法导入资产：源文件不存在！路径: %s"), *SourceAssetPath);
            return nullptr;
        }

        if (!GameDestinationPath.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Error, TEXT("无法导入资产：目标路径必须以 /Game/ 开头！路径: %s"), *GameDestinationPath);
            return nullptr;
        }

        // --- 2. 加载AssetTools模块 ---
        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        IAssetTools& AssetTools = AssetToolsModule.Get();

        // --- 3. 设置导入任务 ---
        TArray<FString> FilesToImport;
        FilesToImport.Add(SourceAssetPath);

        // ImportAssets会处理导入并返回创建的资产对象
        TArray<UObject*> ImportedObjects = AssetTools.ImportAssets(FilesToImport, GameDestinationPath);

        // --- 4. 检查导入结果 ---
        if (ImportedObjects.Num() > 0 && ImportedObjects[0] != nullptr)
        {
            UE_LOG(LogTemp, Log, TEXT("资产导入成功！路径: %s"), *ImportedObjects[0]->GetPathName());
            return ImportedObjects[0];
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("资产导入失败！源文件: %s"), *SourceAssetPath);
            return nullptr;
        }
    }

    /**
     * @brief [高级] 使用FAssetImportTask进行更精细的控制，可以指定导入后的资产名称。
     * @param SourceAssetPath 要导入的源文件在硬盘上的绝对路径。
     * @param GameDestinationPath 资产在游戏内容目录中的目标【文件夹】路径。
     * @param DestinationAssetName 导入后资产的【确切名称】。如果留空，则根据源文件名自动生成。
     * @return 返回成功导入后创建的UObject对象，如果失败则返回nullptr
     */
    static UObject* ImportAssetWithTask(const FString& SourceAssetPath, const FString& GameDestinationPath, const FString& DestinationAssetName = TEXT(""))
    {
        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

        // 创建一个导入任务
        UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
        ImportTask->bSave = true;                   // 导入后自动保存
        ImportTask->bAutomated = true;              // 自动化模式，不弹出对话框
        ImportTask->bReplaceExisting = true;        // 如果已存在同名资产，则覆盖
        ImportTask->Filename = SourceAssetPath;     // 源文件路径
        ImportTask->DestinationPath = GameDestinationPath; // 目标文件夹路径
        ImportTask->DestinationName = DestinationAssetName; // 【重要】指定资产名称
        // ImportTask->Factory = ...;               // 可以指定一个特定的Factory，通常留空自动选择
        // ImportTask->Options = ...;               // 可以设置特定类型的导入选项（如FBX导入设置）

        TArray<UAssetImportTask*> TasksToImport;
        TasksToImport.Add(ImportTask);

        // 执行导入
        AssetToolsModule.Get().ImportAssetTasks(TasksToImport);

        // 检查结果
        if (ImportTask->Result.Num() > 0 && ImportTask->Result[0] != nullptr)
        {
            UObject* ImportedObject = Cast<UObject>(ImportTask->Result[0]);
            UE_LOG(LogTemp, Log, TEXT("资产通过Task导入成功！路径: %s"), *ImportedObject->GetPathName());
            return ImportedObject;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("资产通过Task导入失败！源文件: %s"), *SourceAssetPath);
            return nullptr;
        }
    }
#endif
};

FPSDHelper::FPSDHelper()
{
}

FPSDHelper::~FPSDHelper()
{
}
std::wstring FPSDHelper::GetSampleOutputPath(void)
{
   return L"";
}

void FPSDHelper::SavePNG_Unreal(const FString& FilePath, int32 Width, int32 Height, int32 Channels, const uint8_t* Data)
{
    // 获取 IImageWrapper 模块
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

    // 创建 PNG 包装器
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create PNG image wrapper."));
        return;
    }

    // 设置图像数据（Unreal 要求 RGBA 格式）
    if (Channels == 4)
    {
        ImageWrapper->SetRaw(Data, Width * Height * Channels, Width, Height, ERGBFormat::RGBA, 8);
    }
    else if (Channels == 3)
    {
        // 如果输入是 RGB，需要手动转换为 RGBA
        TArray<uint8> RGBA_Data;
        RGBA_Data.AddUninitialized(Width * Height * 4);
        for (int32 i = 0; i < Width * Height; i++)
        {
            RGBA_Data[i * 4 + 0] = Data[i * 3 + 0]; // R
            RGBA_Data[i * 4 + 1] = Data[i * 3 + 1]; // G
            RGBA_Data[i * 4 + 2] = Data[i * 3 + 2]; // B
            RGBA_Data[i * 4 + 3] = 255;             // A (不透明)
        }
        ImageWrapper->SetRaw(RGBA_Data.GetData(), Width * Height * 4, Width, Height, ERGBFormat::RGBA, 8);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unsupported number of channels: %d"), Channels);
        return;
    }

    // 获取 PNG 压缩后的数据
    const TArray64<uint8>& PNG_Data = ImageWrapper->GetCompressed();
    if (PNG_Data.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to compress PNG data."));
        return;
    }

    // 写入文件
    if (!FFileHelper::SaveArrayToFile(PNG_Data, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save PNG to: %s"), *FilePath);
    }
    ReimportAllAssets(FilePath);
}

bool FPSDHelper::ResolvePSD(FString InPsdPath)
{   

    FileName = FPaths::GetBaseFilename(InPsdPath);

    const std::string srcPathAnsi = TCHAR_TO_UTF8(*InPsdPath);
    const std::wstring srcPath = string_to_wstring(srcPathAnsi);

    PSD_NAMESPACE_NAME::MallocAllocator allocator;
    PSD_NAMESPACE_NAME::NativeFile file(&allocator);

    // try opening the file. if it fails, bail out.
    if (!file.OpenRead(srcPath.c_str()))
    {
        PSD_SAMPLE_LOG("Cannot open file.\n");
        return 1;
    }

    // create a new document that can be used for extracting different sections from the PSD.
    // additionally, the document stores information like width, height, bits per pixel, etc.
    PSD_NAMESPACE_NAME::Document* document = CreateDocument(&file, &allocator);
    if (!document)
    {
        PSD_SAMPLE_LOG("Cannot create document.\n");
        file.Close();
        return 1;
    }
    
    PanelContext* context = new PanelContext();

    // the sample only supports RGB colormode
    if (document->colorMode != PSD_NAMESPACE_NAME::colorMode::RGB)
    {
        PSD_SAMPLE_LOG("Document is not in RGB color mode.\n");
        PSD_NAMESPACE_NAME::DestroyDocument(document, &allocator);
        file.Close();
        return 1;
    }

    // extract image resources section.
    // this gives access to the ICC profile, EXIF data and XMP metadata.
    {
        PSD_NAMESPACE_NAME::ImageResourcesSection* imageResourcesSection = ParseImageResourcesSection(document, &file, &allocator);
        PSD_SAMPLE_LOG("XMP metadata:\n");
        PSD_SAMPLE_LOG(imageResourcesSection->xmpMetadata);
        PSD_SAMPLE_LOG("\n");
        DestroyImageResourcesSection(imageResourcesSection, &allocator);
    }

    // extract all layers and masks.
    bool hasTransparencyMask = false;
    PSD_NAMESPACE_NAME::LayerMaskSection* layerMaskSection = PSD_NAMESPACE_NAME::ParseLayerMaskSection(document, &file, &allocator);
    if (layerMaskSection)
    {
        hasTransparencyMask = layerMaskSection->hasTransparencyMask;
        GenerateContext(layerMaskSection);
        // extract all layers one by one. this should be done in parallel for maximum efficiency.
        for (unsigned int i = 0; i < layerMaskSection->layerCount; ++i)
        {
            PSD_NAMESPACE_NAME::Layer* layer = &layerMaskSection->layers[i];
            PSD_NAMESPACE_NAME::ExtractLayer(document, &file, &allocator, layer);

            // check availability of R, G, B, and A channels.
            // we need to determine the indices of channels individually, because there is no guarantee that R is the first channel,
            // G is the second, B is the third, and so on.
            const unsigned int indexR = FindChannel(layer, PSD_NAMESPACE_NAME::channelType::R);
            const unsigned int indexG = FindChannel(layer, PSD_NAMESPACE_NAME::channelType::G);
            const unsigned int indexB = FindChannel(layer, PSD_NAMESPACE_NAME::channelType::B);
            const unsigned int indexA = FindChannel(layer, PSD_NAMESPACE_NAME::channelType::TRANSPARENCY_MASK);


            // interleave the different pieces of planar canvas data into one RGB or RGBA image, depending on what channels
            // we found, and what color mode the document is stored in.
            uint8_t* image8 = nullptr;
            uint16_t* image16 = nullptr;
            float32_t* image32 = nullptr;

            const unsigned int layerWidth = layer->right - layer->left;
            const unsigned int layerHeight = layer->bottom - layer->top;

           

            if (!bGeneratedPNG)
            {
                continue; // Skip layer processing if PNG generation is not enabled
            }
          

            // note that channel data is only as big as the layer it belongs to, e.g. it can be smaller or bigger than the canvas,
            // depending on where it is positioned. therefore, we use the provided utility functions to expand/shrink the channel data
            // to the canvas size. of course, you can work with the channel data directly if you need to.
            //void* canvasData[4] = {};
            unsigned int channelCount = 0u;
            if ((indexR != CHANNEL_NOT_FOUND) && (indexG != CHANNEL_NOT_FOUND) && (indexB != CHANNEL_NOT_FOUND))
            {
                // RGB channels were found.
                channelCount = 3u;

                if (indexA != CHANNEL_NOT_FOUND)
                {
                    // A channel was also found.
                    channelCount = 4u;
                }

                const void* r_data = layer->channels[indexR].data;
                const void* g_data = layer->channels[indexG].data;
                const void* b_data = layer->channels[indexB].data;

                if (channelCount == 3u)
                {
                    if (document->bitsPerChannel == 8)
                    {
                        image8 = CreateInterleavedImage<uint8_t>(&allocator, r_data, g_data, b_data, layerWidth, layerHeight);
                    }
                    else if (document->bitsPerChannel == 16)
                    {
                        image16 = CreateInterleavedImage<uint16_t>(&allocator, r_data, g_data, b_data, layerWidth, layerHeight);
                    }
                    else if (document->bitsPerChannel == 32)
                    {
                        image32 = CreateInterleavedImage<float32_t>(&allocator, r_data, g_data, b_data, layerWidth, layerHeight);
                    }
                }
                else if (channelCount == 4u)
                {
                    const void* a_data = layer->channels[indexA].data;
                    if (document->bitsPerChannel == 8)
                    {
                        image8 = CreateInterleavedImage<uint8_t>(&allocator, r_data, g_data, b_data, a_data, layerWidth, layerHeight);
                    }
                    else if (document->bitsPerChannel == 16)
                    {
                        image16 = CreateInterleavedImage<uint16_t>(&allocator, r_data, g_data, b_data, a_data, layerWidth, layerHeight);
                    }
                    else if (document->bitsPerChannel == 32)
                    {
                        image32 = CreateInterleavedImage<float32_t>(&allocator, r_data, g_data, b_data, a_data, layerWidth, layerHeight);
                    }
                }
            }
            
            std::wstringstream layerName;
            if (layer->utf16Name)
            {
#ifdef _WIN32
                //In Windows wchar_t is utf16
                PSD_STATIC_ASSERT(sizeof(wchar_t) == sizeof(uint16_t));
                layerName << reinterpret_cast<wchar_t*>(layer->utf16Name);

#else
                //In Linux, wchar_t is utf32
                //Convert code from https://stackoverflow.com/questions/23919515/how-to-convert-from-utf-16-to-utf-32-on-linux-with-std-library#comment95663809_23920015
                auto is_surrogate = [](uint16_t uc) -> bool
                    {
                        return ((uc - 0xd800u) < 2048u);
                    };
                auto is_high_surrogate = [](uint16_t uc) -> bool
                    {
                        return ((uc & 0xfffffc00) == 0xd800);
                    };
                auto is_low_surrogate = [](uint16_t uc) -> bool
                    {
                        return ((uc & 0xfffffc00) == 0xdc00);
                    };
                auto surrogate_to_utf32 = [](uint16_t high, uint16_t low) -> wchar_t
                    {
                        return ((high << 10) + low - 0x35fdc00);
                    };
                PSD_STATIC_ASSERT(sizeof(wchar_t) == sizeof(uint32_t));

                //Begin convert
                size_t u16len = 0;
                const uint16_t* cur = layer->utf16Name;
                while (*cur != uint16_t('\0')) {
                    cur++;
                    u16len++;
                }
                //Len it

                const uint16_t* const end = layer->utf16Name + u16len;
                const uint16_t* input = layer->utf16Name;
                while (input < end) {
                    const uint16_t uc = *input++;
                    if (!is_surrogate(uc)) {
                        layerName << wchar_t(uc);
                    }
                    else {
                        if (is_high_surrogate(uc) && input < end && is_low_surrogate(*input)) {
                            layerName << wchar_t(surrogate_to_utf32(uc, *input++));
                        }
                        else {
                            // Error
                            // Impossible
                            std::abort();
                        }
                    }
                }
#endif
            }
            else
            {
                layerName << layer->name.c_str();

    
                context->ControlName = UTF8_TO_TCHAR(layer->name.c_str());
            }

            FString UnrealLayerName = FString(layerName.str().c_str());
            if (UnrealLayerName.Contains(TEXT("ControlInfo"), ESearchCase::IgnoreCase) || UnrealLayerName.Contains(TEXT("Font"), ESearchCase::IgnoreCase))
            {
                continue;
            }
            
            // at this point, image8, image16 or image32 store either a 8-bit, 16-bit, or 32-bit image, respectively.
            // the image data is stored in interleaved RGB or RGBA, and has the size "document->width*document->height".
            // it is up to you to do whatever you want with the image data. in the sample, we simply write the image to a .TGA file.
            if (channelCount == 3u)
            {
                if (document->bitsPerChannel == 8u)
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"layer";
//                     filename << layerName.str();
//                     filename << L".tga";
//                     tgaExporter::SaveRGB(filename.str().c_str(), layerWidth, layerHeight, image8);
                    FString UnrealFilePath = GetPSDTexturePath() / FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)image8);
                }
            }
            else if (channelCount == 4u)
            {
                if (document->bitsPerChannel == 8u)
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"layer";
//                     filename << layerName.str();
//                     filename << L".tga";
//                     tgaExporter::SaveRGBA(filename.str().c_str(), layerWidth, layerHeight, image8);

//                     std::wstringstream filenamePng;
//                     filenamePng << GetSampleOutputPath();
//                     filenamePng << L"layer";
//                     filenamePng << layerName.str();
//                     filenamePng << L".png";

                    FString UnrealFilePath = GetPSDTexturePath()/ FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)image8);
                }
            }

            allocator.Free(image8);
            allocator.Free(image16);
            allocator.Free(image32);

            // in addition to the layer data, we also want to extract the user and/or vector mask.
            // luckily, this has been handled already by the ExtractLayer() function. we just need to check whether a mask exists.
            if (layer->layerMask)
            {
                // a layer mask exists, and data is available. work out the mask's dimensions.
                const unsigned int width = static_cast<unsigned int>(layer->layerMask->right - layer->layerMask->left);
                const unsigned int height = static_cast<unsigned int>(layer->layerMask->bottom - layer->layerMask->top);

                // similar to layer data, the mask data can be smaller or bigger than the canvas.
                // the mask data is always single-channel (monochrome), and has a width and height as calculated above.
                void* maskData = layer->layerMask->data;
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"layer";
//                     filename << layerName.str();
//                     filename << L"_usermask.tga";
//                     tgaExporter::SaveMonochrome(filename.str().c_str(), width, height, static_cast<const uint8_t*>(maskData));

                    FString UnrealFilePath = GetPSDTexturePath() / FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)maskData);

                }

                // use ExpandMaskToCanvas create an image that is the same size as the canvas.
                void* maskCanvasData = ExpandMaskToCanvas(document, &allocator, layer->layerMask);
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"canvas";
//                     filename << layerName.str();
//                     filename << L"_usermask.tga";
//                     tgaExporter::SaveMonochrome(filename.str().c_str(), layerWidth, layerHeight, static_cast<const uint8_t*>(maskCanvasData));
                    FString UnrealFilePath = GetPSDTexturePath() / FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)maskCanvasData);
                }

                allocator.Free(maskCanvasData);
            }

            if (layer->vectorMask)
            {
                // accessing the vector mask works exactly like accessing the layer mask.
                const unsigned int width = static_cast<unsigned int>(layer->vectorMask->right - layer->vectorMask->left);
                const unsigned int height = static_cast<unsigned int>(layer->vectorMask->bottom - layer->vectorMask->top);

                void* maskData = layer->vectorMask->data;
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"layer";
//                     filename << layerName.str();
//                     filename << L"_vectormask.tga";
//                     tgaExporter::SaveMonochrome(filename.str().c_str(), width, height, static_cast<const uint8_t*>(maskData));
                    FString UnrealFilePath = GetPSDTexturePath() / FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)maskData);
                }

                void* maskCanvasData = ExpandMaskToCanvas(document, &allocator, layer->vectorMask);
                {
//                     std::wstringstream filename;
//                     filename << GetSampleOutputPath();
//                     filename << L"canvas";
//                     filename << layerName.str();
//                     filename << L"_vectormask.tga";
//                     tgaExporter::SaveMonochrome(filename.str().c_str(), layerWidth, layerHeight, static_cast<const uint8_t*>(maskCanvasData));

                    FString UnrealFilePath = GetPSDTexturePath() / FString(layerName.str().c_str()) + TEXT(".png");
                    SavePNG_Unreal(UnrealFilePath, layerWidth, layerHeight, channelCount, (const uint8_t*)maskCanvasData);
                }

                allocator.Free(maskCanvasData);
            }
        }

        DestroyLayerMaskSection(layerMaskSection, &allocator);
    }

    // extract the image data section, if available. the image data section stores the final, merged image, as well as additional
    // alpha channels. this is only available when saving the document with "Maximize Compatibility" turned on.
    if (document->imageDataSection.length != 0)
    {
        unsigned int channelCount = 0u;
        PSD_NAMESPACE_NAME::ImageDataSection* imageData = ParseImageDataSection(document, &file, &allocator);
        if (imageData)
        {
            // interleave the planar image data into one RGB or RGBA image.
            // store the rest of the (alpha) channels and the transparency mask separately.
            const unsigned int imageCount = imageData->imageCount;

            // note that an image can have more than 3 channels, but still no transparency mask in case all extra channels
            // are actual alpha channels.
            bool isRgb = false;
            if (imageCount == 3)
            {
                // imageData->images[0], imageData->images[1] and imageData->images[2] contain the R, G, and B channels of the merged image.
                // they are always the size of the canvas/document, so we can interleave them using imageUtil::InterleaveRGB directly.
                isRgb = true;
            }
            else if (imageCount >= 4)
            {
                // check if we really have a transparency mask that belongs to the "main" merged image.
                if (hasTransparencyMask)
                {
                    // we have 4 or more images/channels, and a transparency mask.
                    // this means that images 0-3 are RGBA, respectively.
                    isRgb = false;
                    channelCount = 3;
                }
                else
                {
                    // we have 4 or more images stored in the document, but none of them is the transparency mask.
                    // this means we are dealing with RGB (!) data, and several additional alpha channels.
                    isRgb = true;
                    channelCount = 4;
                }
            }

            uint8_t* image8 = nullptr;
            uint16_t* image16 = nullptr;
            float32_t* image32 = nullptr;
            if (isRgb)
            {
                // RGB
                if (document->bitsPerChannel == 8)
                {
                    image8 = CreateInterleavedImage<uint8_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, document->width, document->height);
                }
                else if (document->bitsPerChannel == 16)
                {
                    image16 = CreateInterleavedImage<uint16_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, document->width, document->height);
                }
                else if (document->bitsPerChannel == 32)
                {
                    image32 = CreateInterleavedImage<float32_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, document->width, document->height);
                }
            }
            else
            {
                // RGBA
                if (document->bitsPerChannel == 8)
                {
                    image8 = CreateInterleavedImage<uint8_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, imageData->images[3].data, document->width, document->height);
                }
                else if (document->bitsPerChannel == 16)
                {
                    image16 = CreateInterleavedImage<uint16_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, imageData->images[3].data, document->width, document->height);
                }
                else if (document->bitsPerChannel == 32)
                {
                    image32 = CreateInterleavedImage<float32_t>(&allocator, imageData->images[0].data, imageData->images[1].data, imageData->images[2].data, imageData->images[3].data, document->width, document->height);
                }
            }

            if (document->bitsPerChannel == 8)
            {
                std::wstringstream filename;
                filename << GetSampleOutputPath();
                filename << L"merged.tga";
//                 if (isRgb)
//                 {
//                     tgaExporter::SaveRGB(filename.str().c_str(), document->width, document->height, image8);
//                 }
//                 else
//                 {
//                     tgaExporter::SaveRGBA(filename.str().c_str(), document->width, document->height, image8);
//                 }


                FString UnrealFilePath = GetPSDTexturePath() / FString("merged") + TEXT(".png");
                SavePNG_Unreal(UnrealFilePath, document->width, document->height, channelCount, (const uint8_t*)image8);
            }

            allocator.Free(image8);
            allocator.Free(image16);
            allocator.Free(image32);

            // extract image resources in order to acquire the alpha channel names.
            PSD_NAMESPACE_NAME::ImageResourcesSection* imageResources = ParseImageResourcesSection(document, &file, &allocator);
            if (imageResources)
            {
                // store all the extra alpha channels. in case we have a transparency mask, it will always be the first of the
                // extra channels.
                // alpha channel names can be accessed using imageResources->alphaChannels[index].
                // loop through all alpha channels, and skip all channels that were already merged (either RGB or RGBA).
                const unsigned int skipImageCount = isRgb ? 3u : 4u;
                for (unsigned int i = 0u; i < imageCount - skipImageCount; ++i)
                {
                    PSD_NAMESPACE_NAME::AlphaChannel* channel = imageResources->alphaChannels + i;

                    if (document->bitsPerChannel == 8)
                    {
//                         std::wstringstream filename;
//                         filename << GetSampleOutputPath();
//                         filename << L"extra_channel_";
//                         filename << channel->asciiName.c_str();
//                         filename << L".tga";
// 
//                         tgaExporter::SaveMonochrome(filename.str().c_str(), document->width, document->height, static_cast<const uint8_t*>(imageData->images[i + skipImageCount].data));

                        if (channel->mode == PSD_NAMESPACE_NAME::AlphaChannel::Mode::ALPHA)
                        {
                            channelCount = 4;
                        }
                        else
                        {
                            channelCount = 3;
                        }
                        FString UnrealFilePath = GetPSDTexturePath() / FString::Printf(TEXT("extra_channel_%s"),UTF8_TO_TCHAR(channel->asciiName.c_str())) + TEXT(".png");
                        SavePNG_Unreal(UnrealFilePath, document->width, document->height, channelCount, (const uint8_t*)image8);
                    }
                }

                PSD_NAMESPACE_NAME::DestroyImageResourcesSection(imageResources, &allocator);
            }

            PSD_NAMESPACE_NAME::DestroyImageDataSection(imageData, &allocator);
        }
    }

    // don't forget to destroy the document, and close the file.
    PSD_NAMESPACE_NAME::DestroyDocument(document, &allocator);
    file.Close();


  

    return 0;

}

void FPSDHelper::ReimportAllAssets(const FString& FilePath)
{
    FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    FString GamePathWithFile = FilePath.Replace(*ContentDir, TEXT("/Game/"));

    FString DestinationFolder = FPaths::GetPath(GamePathWithFile);
    FString BaseFilename = FPaths::GetBaseFilename(FilePath);

    FString SanitizedAssetName = ObjectTools::SanitizeObjectName(BaseFilename);

    FMyAssetTools::ImportAssetWithTask(FilePath, DestinationFolder, SanitizedAssetName);
    
}

void FPSDHelper::GenerateContext(PSD_NAMESPACE_NAME::LayerMaskSection* InLayerMaskSection)
{
    for (unsigned int i = 0; i < InLayerMaskSection->layerCount; ++i)
    {
        PSD_NAMESPACE_NAME::Layer* layer = &InLayerMaskSection->layers[i];

        // Create a new context for this layer
        PanelContext* context = new PanelContext();

        // Populate the context with data from the layer
        context->Left = layer->left;
        context->Top = layer->top;
        context->Right = layer->right;
        context->Bottom = layer->bottom;

        // Convert layer name (handles both UTF-16 and legacy names)
        if (layer->utf16Name)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
            const char16_t* utf16_ptr = reinterpret_cast<const char16_t*>(layer->utf16Name);
            std::string utf8_string = converter.to_bytes(utf16_ptr);
            context->ControlName = FString(utf8_string.c_str()); // Assuming Unreal's FString

            std::optional<UIElement> Element = ParseUIElement(utf8_string);
            if (Element.has_value())
            {

                context->Element = Element;
                context->ControlName = UTF8_TO_TCHAR(Element->name.c_str());
                context->ControlType = UTF8_TO_TCHAR(Element->type.c_str());
                if (Element->params.contains("fullscreen"))
                {
                    context->bIsFullScreen = Element->params["fullscreen"].get<bool>();
                }
                else
                {
                    context->bIsFullScreen = false;
//                     if (Element->params.contains("size") && Element->params["size"].is_array())
//                     {
//                         const auto& size_array = Element->params["size"];
//                         if (size_array.size() != 2) 
//                         {
//                             UE_LOG(LogTemp, Error, TEXT("UMG control size error ,size only set one"));
//                             return ;
//                         }
//                         context->Size = FVector2D(size_array[0], size_array[1]);
//                     }
//                     else
//                     {
//                         context->Size = FVector2D(200,100);
//                         UE_LOG(LogTemp, Error, TEXT("UMG control size error ,size is (0,0)"));
//                     }
                }
               
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to parse UI element from layer name: %s"), *context->ControlName);
            }

        }
        else
        {
            context->ControlName = FString(layer->name.c_str());
            std::optional<UIElement> Element = ParseUIElement(layer->name.c_str());
            if (Element.has_value())
            {
                context->Element = Element;
                context->ControlName = UTF8_TO_TCHAR(Element->name.c_str());
                context->ControlType = UTF8_TO_TCHAR(Element->type.c_str());
                if (Element->params.contains("fullscreen"))
                {
                    context->bIsFullScreen = Element->params["fullscreen"].get<bool>();
                }
                else
                {
//                     context->bIsFullScreen = false;
//                     if (Element->params.contains("size") && Element->params["size"].is_array())
//                     {
//                         const auto& size_array = Element->params["size"];
//                         if (size_array.size() != 2)
//                         {
//                             UE_LOG(LogTemp, Error, TEXT("UMG control size error ,size only set one"));
//                             return;
//                         }
//                         context->Size = FVector2D(size_array[0], size_array[1]);
//                     }
//                     else
//                     {
//                         UE_LOG(LogTemp, Error, TEXT("UMG control size error ,size is (0,0)"));
//                     }
                }

            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to parse UI element from layer name: %s"), *context->ControlName);
            }

        }

        // Add the new node to our map, using the original layer pointer as the key
        nodeMap[layer] = context;
    }


    // -- STEP 2: Link the nodes together into a hierarchy --
    for (unsigned int i = 0; i < InLayerMaskSection->layerCount; ++i)
    {
        PSD_NAMESPACE_NAME::Layer* layer = &InLayerMaskSection->layers[i];

        // Find the PanelContext node that corresponds to this layer
        PanelContext* currentNode = nodeMap[layer];

        if (layer->parent == nullptr)
        {
            // This is a top-level layer (it has no parent).
            // It belongs in our list of root nodes.
            rootNodes.push_back(currentNode);
        }
        else
        {
            // This is a child layer. Find its parent in our map.
            PanelContext* parentNode = nodeMap[layer->parent];

            // Establish the two-way link:
            // 1. Set the current node's parent.
            currentNode->Parent = parentNode;

            // 2. Add the current node to its parent's list of children.
            parentNode->Children.push_back(currentNode);
        }

//         if (currentNode->ControlType.Equals(TEXT("Button"), ESearchCase::IgnoreCase)) {
//             ProcessButtonTextures(currentNode);
//         }
    }

    std::function<void(PanelContext*, int)> printHierarchy =
        [&](PanelContext* node, int depth) {

        FString indent(TEXT(""));
        for (int i = 0; i < depth; ++i)
        {
            indent += TEXT("  ");
        }
        UE_LOG(LogTemp,Log,TEXT("%s- %s\n"), *indent, *node->ControlName);

        for (PanelContext* child : node->Children)
        {
            printHierarchy(child, depth + 1);
        }
        };
    UE_LOG(LogTemp, Log, TEXT("--- Reconstructed PSD Hierarchy ---\n"));
    for (PanelContext* root : rootNodes)
    {
        printHierarchy(root, 0);
    }
}

void FPSDHelper::ProcessButtonTextures(PanelContext* RootNode)
{
    if (!RootNode || !RootNode->ControlType.Equals(TEXT("Button"), ESearchCase::IgnoreCase)) {
        return;
    }

    // 查找所有Texture类型的子节点
    std::vector<PanelContext*> textureChildren = RootNode->FindChildrenOfType(TEXT("Texture"));

    for (PanelContext* textureChild : textureChildren) {
        // 确保纹理数据有效
        RootNode->Top = textureChild->Top;
        RootNode->Bottom = textureChild->Bottom;
        RootNode->Left = textureChild->Left;
        RootNode->Right = textureChild->Right;

        break;
    }
}

std::optional<UIElement> FPSDHelper::ParseUIElement(const std::string& input)
{
    // 1. 查找必须存在的分隔符 '@'
    size_t at_pos = input.find('@');

    // 如果没有找到 '@'，则格式无效
    if (at_pos == std::string::npos) {
        return std::nullopt;
    }

    try {
        UIElement element;

        // 2. 提取名称 (从开头到 '@')
        element.name = input.substr(0, at_pos);

        // 3. 查找可选的分隔符 ':'，注意从 '@' 之后开始查找
        size_t colon_pos = input.find(':', at_pos + 1);

        if (colon_pos == std::string::npos) {
            // 4a. 如果没有找到 ':'，则 '@' 之后的所有内容都作为类型
            element.type = input.substr(at_pos + 1);
            // JSON 参数视为空对象
            element.params = nlohmann::json::object();
        }
        else {
            // 4b. 如果找到了 ':'
            // 验证类型是否为空 (例如 "name@:{}")
            if (colon_pos <= at_pos + 1) {
                return std::nullopt; // 类型不能为空
            }
            // 提取类型 (从 '@' 和 ':' 之间)
            element.type = input.substr(at_pos + 1, colon_pos - at_pos - 1);

            // 提取 ':' 之后的 JSON 字符串
            std::string json_str = input.substr(colon_pos + 1);

            // 如果 JSON 字符串不为空则解析，否则视为空对象
            if (!json_str.empty()) {
                element.params = nlohmann::json::parse(json_str);
            }
            else {
                element.params = nlohmann::json::object();
            }
        }

        return element;
    }
    catch (const nlohmann::json::parse_error& e) {
        // JSON 解析失败
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return std::nullopt;
    }
    catch (const std::out_of_range& e) {
        // 字符串截取越界，通常在格式错误时发生
        std::cerr << "String parsing error: " << e.what() << "\n";
        return std::nullopt;
    }
    catch (...) {
        // 捕获其他未知异常
        std::cerr << "Unknown error occurred\n";
        return std::nullopt;
    }
}