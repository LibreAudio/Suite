// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioParameter.hpp"

#include <array>

namespace extra_dsp {

enum ParameterIndex : uint16_t {
    kParameterVAD,
    kParameterCount
};

inline constexpr const std::array<LibreAudioParameter, kParameterCount> kParameters = {{
    {
        .bracket = "",
        .label = "",
        .shortlabel = "",
        .name = "VAD",
        .symbol = "vad",
        .tooltip = "",
        .unit = "",
        .scalePoints = nullptr,
        .init = 0.0,
        .min = 0.0,
        .max = 1.0,
        .step = 0.1,
        .accentcolor = 0,
        .scalePointCount = 0,
        .isBoolean = false,
        .isEasy = false,
        .isEnumerator = false,
        .isDynamic = false,
        .isInteger = false,
        .isLogarithmic = false,
        .isOutput = true,
        .isTrigger = false,
    },
}};

}
