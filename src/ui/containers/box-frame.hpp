// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "container.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/_lab/text.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<const Color& color, const char text[]>
using BoxFrameTitle = LabReferenceStaticTextWidget<Reference::Widgets::BoxFrame<>::Header::Title<color>, text>;

template<Orientation kOrientation, uint kNumRows = 1, bool kIncludeHeader = false>
using BoxFrameWidget = LabReferenceContainerWidget<
    Reference::Widgets::BoxFrame<kNumRows, kIncludeHeader>, kOrientation
>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
