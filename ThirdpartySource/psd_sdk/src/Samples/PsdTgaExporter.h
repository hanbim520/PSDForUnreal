// Copyright 2011-2020, Molecular Matters GmbH <office@molecular-matters.com>
// See LICENSE.txt for licensing details (2-clause BSD License: https://opensource.org/licenses/BSD-2-Clause)

#pragma once

#include "../Psd/Psdstdint.h"


namespace tgaExporter
{
	/// Assumes 8-bit single-channel data.
	void SaveMonochrome(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data);

	/// Assumes 8-bit RGBA data, but ignores alpha (32-bit data is assumed for performance reasons).
	void SaveRGB(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data);

	/// Assumes 8-bit RGBA data.
	void SaveRGBA(const wchar_t* filename, unsigned int width, unsigned int height, const uint8_t* data);


    /// <summary>
	/// Saves image data as a PNG file.
	/// </summary>
	/// <param name="filename">The path to the output file.</param>
	/// <param name="width">The width of the image.</param>
	/// <param name="height">The height of the image.</param>
	/// <param name="channels">The number of channels in the image data (e.g., 3 for RGB, 4 for RGBA).</param>
	/// <param name="data">A pointer to the raw image data.</param>
    void SavePNG(const wchar_t* filename, unsigned int width, unsigned int height, unsigned int channels, const uint8_t* data);
}
