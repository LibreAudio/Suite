// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/ui.hpp"

#include "ui/bot/eq-ui.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::EqRootWidget>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
