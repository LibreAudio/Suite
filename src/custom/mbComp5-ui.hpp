// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/bot/mbComp5-ui.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

inline bool isBypassed(const LabUIWidgetInterface* const iface) noexcept
{
    return d_isNotZero(iface->getParameterValue(kCommonParameterBypass));
}

inline float getParameterValue(const LabUIWidgetInterface* const iface, const FaustParameterIndex index) noexcept
{
    return iface->getParameterValue(kParametersMainStart + index);
}

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
