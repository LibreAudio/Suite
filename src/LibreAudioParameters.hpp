// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoPluginInfo.h"
#include "DistrhoDetails.hpp"

#include LIBREAUDIO_PLUGIN_PARAMETERS_INCLUDE

#if LIBREAUDIO_WANT_COMMON_IO
#include "common_input-parameters.hpp"
#include "common_output-parameters.hpp"
#endif

#if LIBREAUDIO_WANT_EXTRA_PROCESSOR
#include LIBREAUDIO_EXTRA_PARAMETERS_INCLUDE
#endif

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

inline constexpr const uint32_t kCommonIOParameters = 1;

enum CommonParameters {
    kCommonParameterBypass,
    kCommonParameterReset,
   #if LIBREAUDIO_WANT_DRYWET
    kCommonParameterDryWet,
   #endif
    kCommonParameterCount
};

#if LIBREAUDIO_WANT_COMMON_IO
enum Groups {
    kGroupInput,
    kGroupOutput,
    kGroupMain,
};
#endif

enum Parameters {
    kParametersCommonStart,
    kParametersCommonEnd = kParametersCommonStart + kCommonParameterCount - 1,
   #if LIBREAUDIO_WANT_COMMON_IO
    kParametersInputStart,
    kParametersInputEnd = kParametersInputStart + common_input::kFaustParameterCount - 1,
    kParametersOutputStart,
    kParametersOutputEnd = kParametersOutputStart + common_output::kFaustParameterCount - 1 - kCommonIOParameters,
   #endif
    kParametersMainStart,
    kParametersMainEnd = kParametersMainStart + kFaustParameterCount - 1,
   #if LIBREAUDIO_WANT_EXTRA_PROCESSOR
    kParametersExtraStart,
    kParametersCount = kParametersExtraStart + extra::kParameterCount - 1
   #else
    kParametersCount
   #endif
};

#if LIBREAUDIO_WANT_DRYWET
inline constexpr const uint32_t kFirstUsableParameterSymbol = kCommonParameterDryWet;
#else
inline constexpr const uint32_t kFirstUsableParameterSymbol = kCommonParameterCount;
#endif

inline constexpr const char* kCommonParameterSymbols[kCommonParameterCount] = {
    ParameterDesignationSymbols::bypass,
    ParameterDesignationSymbols::reset,
   #if LIBREAUDIO_WANT_DRYWET
    "dry_wet",
   #endif
};

#if LIBREAUDIO_WANT_DRYWET
inline constexpr const LibreAudioParameter kCommonDryWetParameter = {
    .bracket = "",
    .label = "",
    .shortlabel = "",
   #ifdef _DARKGLASS_DEVICE_PABLITO
    .name = "Blend",
   #else
    .name = "Dry / Wet",
   #endif
    .symbol = kCommonParameterSymbols[kCommonParameterDryWet],
    .tooltip = "",
    .unit = "%",
    .scalePoints = nullptr,
    .init = 50.0,
    .min = 0.0,
    .max = 100.0,
    .step = 0.1,
    .accentcolor = 0,
    .scalePointCount = 0,
    .isBoolean = false,
    .isEasy = false,
    .isEnumerator = false,
    .isDynamic = false,
    .isInteger = false,
    .isLogarithmic = false,
    .isOutput = false,
    .isTrigger = false,
};
#endif

// --------------------------------------------------------------------------------------------------------------------

inline void initCommonParameterValuesToDefault(float values[kCommonParameterCount])
{
    for (uint32_t i = 0; i < kCommonParameterCount; ++i)
    {
        switch (static_cast<CommonParameters>(i))
        {
        case kCommonParameterBypass:
        case kCommonParameterReset:
            values[i] = 0.f;
            break;
       #if LIBREAUDIO_WANT_DRYWET
        case kCommonParameterDryWet:
            values[i] = kCommonDryWetParameter.init;
       #endif
            break;
        case kCommonParameterCount:
            break;
        }
    }
}

inline constexpr const LibreAudioParameter _kFallback = {};

inline constexpr const LibreAudioParameter& getParameter(const uint32_t index) noexcept
{
    return
       #if LIBREAUDIO_WANT_EXTRA_PROCESSOR
        index >= kParametersExtraStart ? extra::kParameters[index - kParametersExtraStart] :
       #endif
        index >= kParametersMainStart ? kFaustParameters[index - kParametersMainStart] :
       #if LIBREAUDIO_WANT_COMMON_IO
        index >= kParametersOutputStart ? common_output::kFaustParameters[index - kParametersOutputStart + kCommonIOParameters] :
        index >= kParametersInputStart ? common_input::kFaustParameters[index - kParametersInputStart] :
       #endif
       #if LIBREAUDIO_WANT_DRYWET
        index == kCommonParameterDryWet ? kCommonDryWetParameter :
       #endif
        _kFallback;
}

inline constexpr const char* getParameterSymbol(const uint32_t index) noexcept
{
    return getParameter(index).symbol;
}

// --------------------------------------------------------------------------------------------------------------------

inline constexpr bool isParameterOutputOrTrigger(const uint32_t index) noexcept
{
    return isParameterOutputOrTrigger(getParameter(index));
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
