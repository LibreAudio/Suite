// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base.hpp"
#include "../_lab/container.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R, Orientation orientation>
class ReferencePageWidget : public LabReferenceContainerWidget<R, orientation>
{
    using BaseWidget = LabReferenceContainerWidget<R, Vertical>;
    using BaseWidget::BaseWidget;
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
