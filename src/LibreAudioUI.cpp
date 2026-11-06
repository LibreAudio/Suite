// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/main-area.hpp"
#include "ui/containers/parameter-dump-area.hpp"
#include "ui/containers/root.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

#ifdef SHADER_PLUGIN
using RootWidgetUI = RootWidget<TopBar, MainAreaContainerWidget<>>;
#else
using RootWidgetUI = RootWidget<TopBar, ParameterDumpArea>;
#endif

}

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::RootWidgetUI>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
