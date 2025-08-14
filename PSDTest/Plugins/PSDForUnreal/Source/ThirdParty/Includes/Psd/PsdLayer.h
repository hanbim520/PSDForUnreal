// Copyright 2011-2020, Molecular Matters GmbH <office@molecular-matters.com>
// See LICENSE.txt for licensing details (2-clause BSD License: https://opensource.org/licenses/BSD-2-Clause)

#pragma once

#include "PsdFixedSizeString.h"


PSD_NAMESPACE_BEGIN

struct Channel;
struct TransparencyMask;
struct LayerMask;
struct VectorMask;


/// \ingroup Types
/// \class Layer
/// \brief A struct representing a layer as stored in the Layer Mask Info section.
/// \sa LayerMaskSection Channel LayerMask VectorMask
struct Layer
{
	Layer* parent = nullptr;						///< The layer's parent layer, if any.
	util::FixedSizeString name;			///< The ASCII name of the layer. Truncated to 31 characters in PSD files.
	uint16_t* utf16Name = nullptr;				///< The UTF16 name of the layer.

	int32_t top = 0;						///< Top coordinate of the rectangle that encloses the layer.
	int32_t left = 0;						///< Left coordinate of the rectangle that encloses the layer.
	int32_t bottom = 0;						///< Bottom coordinate of the rectangle that encloses the layer.
	int32_t right = 0;						///< Right coordinate of the rectangle that encloses the layer.

	Channel* channels = nullptr;					///< An array of channels, having channelCount entries.
	unsigned int channelCount = 0;			///< The number of channels stored in the array.

	LayerMask* layerMask = nullptr;				///< The layer's user mask, if any.
	VectorMask* vectorMask = nullptr;				///< The layer's vector mask, if any.

	uint32_t blendModeKey = 0;				///< The key denoting the layer's blend mode. Can be any key described in \ref blendMode::Enum.
	uint8_t opacity = 0;					///< The layer's opacity value, with the range [0, 255] mapped to [0%, 100%].
	uint8_t clipping = 0;					///< The layer's clipping mode (not used yet).

	uint32_t type = 0;						///< The layer's type. Can be any of \ref layerType::Enum.
	bool isVisible =false;						///< The layer's visibility.
	bool isPassThrough = false;					///< If the layer is a pass-through group.
};

PSD_NAMESPACE_END
