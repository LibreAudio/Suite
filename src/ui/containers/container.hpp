// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../reference.hpp"
#include "../_lab/container.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// transparent container widget class

template<Orientation orientation>
using TransparentContainerWidget = ReferenceContainerWidget<Reference::Zero, orientation>;

using TransparentHorizontalContainerWidget = TransparentContainerWidget<kHorizontal>;
using TransparentVerticalContainerWidget = TransparentContainerWidget<kVertical>;

// --------------------------------------------------------------------------------------------------------------------
// transparent container widget class with a known padding

template<Orientation orientation, uint padding>
using TransparentPaddedContainerWidget = ReferenceContainerWidget<Reference::Padding<padding>, orientation>;

// --------------------------------------------------------------------------------------------------------------------
// transparent container widget class with a known size (width or height depending on orientation)

// NOTE horizontal containers get fixed height, vertical containers get fixed width
template<Orientation orientation, uint size>
using TransparentSizedContainerReference = std::conditional_t<orientation == kHorizontal,
                                                              Reference::Height<size>,
                                                              Reference::Width<size>>;

template<Orientation orientation, uint size>
using TransparentSizedContainerWidget = ReferenceContainerWidget<TransparentSizedContainerReference<orientation, size>,
                                                                 orientation>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
