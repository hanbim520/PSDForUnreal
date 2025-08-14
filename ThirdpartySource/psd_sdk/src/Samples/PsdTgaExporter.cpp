// Copyright 2011-2020, Molecular Matters GmbH <office@molecular-matters.com>
// See LICENSE.txt for licensing details (2-clause BSD License: https://opensource.org/licenses/BSD-2-Clause)

#include "../Psd/Psd.h"
#include "../Psd/PsdPlatform.h"
#include "PsdTgaExporter.h"
#include "PsdDebug.h"
#include "stb_image_write.h"

#include <string>

#include <stdio.h>
#if defined(__APPLE__)
#include <codecvt>
#include <locale>
#endif

#if PSD_USE_MSVC
#include <windows.h> // Include for WideCharToMultiByte
#include <winnls.h>
#endif

#if defined( __linux)
#include <cwchar>
#include <cstring>
#include <cstdlib>

#endif



namespace
{
	struct TgaType
	{
		enum Enum
		{
			BGR_UNCOMPRESSED = 2,			// TGA file contains BGR triplets of color data.
			MONO_UNCOMPRESSED = 3			// TGA file contains grayscale values.
		};
	};


#pragma pack(push, 1)
	struct TgaHeader
	{
		uint8_t idLength;
		uint8_t paletteType;
		uint8_t type;
		uint16_t paletteOffset;
		uint16_t paletteLength;
		uint8_t bitsPerPaletteEntry;
		uint16_t originX;
		uint16_t originY;
		uint16_t width;
		uint16_t height;
		uint8_t bitsPerPixel;
		uint8_t attributes;
	};
#pragma pack(pop)


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	static TgaHeader CreateHeader(unsigned int width, unsigned int height, TgaType::Enum type, uint8_t bitsPerPixel)
	{
		TgaHeader header;
		header.idLength = 0u;
		header.paletteType = 0u;
		header.type = static_cast<uint8_t>(type);
		header.paletteOffset = 0u;
		header.paletteLength = 0u;
		header.bitsPerPaletteEntry = 0u;
		header.originX = 0u;
		header.originY = 0u;
		header.width = static_cast<uint16_t>(width);
		header.height = static_cast<uint16_t>(height);
		header.bitsPerPixel = bitsPerPixel;
		header.attributes = 0x20u;

		return header;
	}
}


namespace tgaExporter
{
	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	FILE* CreateFile(const wchar_t* filename)
	{
		FILE* file = nullptr;
#if PSD_USE_MSVC
		const errno_t success = _wfopen_s(&file, filename, L"wb");
        if (success != 0)
#elif defined(__APPLE__)
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
        std::string s = convert.to_bytes(filename);
        char const *cs = s.c_str();
        file = fopen(cs, "wb");
        if (file == NULL)
#elif defined(__linux)
		//In Linux
		char *buffer;
		size_t n = std::wcslen(filename) * 4 + 1;
		buffer = static_cast<char*>(std::malloc(n));
		std::memset(buffer,0,n);
		std::wcstombs(buffer,filename,n);
		file = fopen(buffer,"wb");
		std::free(buffer);
		if (file == NULL)
#endif
		{
			PSD_SAMPLE_LOG("Cannot create file for writing.");
			return nullptr;
		}

		return file;
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	void SaveMonochrome(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data)
	{
		FILE* file = CreateFile(filename);
		if (!file)
		{
			return;
		}

		const TgaHeader& header = CreateHeader(width, height, TgaType::MONO_UNCOMPRESSED, 8u);
		fwrite(&header, sizeof(TgaHeader), 1u, file);
		fwrite(data, sizeof(char), width*height, file);
		fclose(file);
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	void SaveRGB(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data)
	{
		FILE* file = CreateFile(filename);
		if (!file)
		{
			return;
		}

		const TgaHeader& header = CreateHeader(width, height, TgaType::BGR_UNCOMPRESSED, 24u);
		fwrite(&header, sizeof(TgaHeader), 1u, file);

		uint8_t* colors = new uint8_t[width*height*3u];
		for (unsigned int i=0u; i<height; ++i)
		{
			for (unsigned int j=0u; j<width; ++j)
			{
				const uint8_t r = data[(i*width + j) * 4u + 0u];
				const uint8_t g = data[(i*width + j) * 4u + 1u];
				const uint8_t b = data[(i*width + j) * 4u + 2u];

				colors[(i*width + j) * 3u + 2u] = r;
				colors[(i*width + j) * 3u + 1u] = g;
				colors[(i*width + j) * 3u + 0u] = b;
			}
		}

		fwrite(colors, sizeof(uint8_t)*3u, width*height, file);
		delete[] colors;
		fclose(file);
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	void SaveRGBA(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data)
	{
		FILE* file = CreateFile(filename);
		if (!file)
		{
			return;
		}

		const TgaHeader& header = CreateHeader(width, height, TgaType::BGR_UNCOMPRESSED, 32u);
		fwrite(&header, sizeof(TgaHeader), 1u, file);

		uint8_t* colors = new uint8_t[width*height*4u];
		for (unsigned int i=0u; i < height; ++i)
		{
			for (unsigned int j=0u; j < width; ++j)
			{
				const uint8_t r = data[(i*width + j) * 4u + 0u];
				const uint8_t g = data[(i*width + j) * 4u + 1u];
				const uint8_t b = data[(i*width + j) * 4u + 2u];
				const uint8_t a = data[(i*width + j) * 4u + 3u];

				colors[(i*width + j) * 4u + 2u] = r;
				colors[(i*width + j) * 4u + 1u] = g;
				colors[(i*width + j) * 4u + 0u] = b;
				colors[(i*width + j) * 4u + 3u] = a;
			}
		}

		fwrite(colors, sizeof(uint8_t)*4u, width*height, file);
		delete[] colors;
		fclose(file);
	}

    // ---------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------------------------------
    static std::string ConvertWCharToString(const wchar_t* w_filename)
    {
        size_t n = std::wcslen(w_filename) * 4 + 1;
        char* buffer = static_cast<char*>(std::malloc(n));
        std::memset(buffer, 0, n);
        std::wcstombs(buffer, w_filename, n);
        std::string result(buffer);
        std::free(buffer);
        return result;

// #if PSD_USE_MSVC
//         // On Windows, stbi_write_png can handle wchar_t if you use the internal _wfopen.
//         // However, to keep it consistent and simple, we convert to UTF-8, which works across all platforms.
//         int size_needed = WideCharToMultiByte(CP_UTF8, 0, w_filename, -1, NULL, 0, NULL, NULL);
//         std::string strTo(size_needed, 0);
//         WideCharToMultiByte(CP_UTF8, 0, w_filename, -1, &strTo[0], size_needed, NULL, NULL);
//         // The string constructor might add an extra null terminator, remove it if it exists and the string is not empty
//         if (size_needed > 1 && strTo.back() == '\0')
//         {
//             strTo.pop_back();
//         }
//         return strTo;
// #elif defined(__APPLE__)
//         std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
//         return convert.to_bytes(w_filename);
// #elif defined(__linux__)
//         // In Linux, convert wchar_t to a multibyte string (UTF-8)
//         size_t n = std::wcslen(w_filename) * 4 + 1;
//         char* buffer = static_cast<char*>(std::malloc(n));
//         std::memset(buffer, 0, n);
//         std::wcstombs(buffer, w_filename, n);
//         std::string result(buffer);
//         std::free(buffer);
//         return result;
// #else
//         // Fallback for other platforms, might not handle all characters correctly
//         size_t len = wcslen(w_filename);
//         std::string result;
//         result.resize(len);
//         for (size_t i = 0; i < len; ++i)
//         {
//             result[i] = static_cast<char>(w_filename[i]);
//         }
//         return result;
// #endif
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------------------------------
    void SavePNG(const wchar_t* filename, unsigned int width, unsigned int height, unsigned int channels, const uint8_t* data)
    {
        if (channels != 3 && channels != 4)
        {
            PSD_SAMPLE_LOG("PNG Exporter only supports 3 (RGB) or 4 (RGBA) channels.");
            return;
        }

        // Convert the wide-character filename to a standard string (UTF-8) that stbi_write_png can use.
        const std::string path = ConvertWCharToString(filename);
        if (path.empty())
        {
            PSD_SAMPLE_LOG("Cannot create file for writing due to invalid path.");
            return;
        }

        // The stride is the number of bytes from one row of pixels to the next.
        const int stride_in_bytes = width * channels;

        // stbi_write_png returns 0 on failure.
        const int success = stbi_write_png(path.c_str(), width, height, channels, data, stride_in_bytes);

        if (success == 0)
        {
            PSD_SAMPLE_LOG("Failed to write PNG file.");
        }
    }
}
