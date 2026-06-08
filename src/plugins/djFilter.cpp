// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioPlugin.hpp"

#include "djFilter.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new LibreAudioPlugin<djFilter, kNumParameters>(kParameters);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
