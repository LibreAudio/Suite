// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"
#include "DistrhoDetails.hpp"

#if LIBREAUDIO_WANT_COMMON_IO
#include "common_input-parameters.hpp"
#include "common_output-parameters.hpp"
#endif

#include LIBREAUDIO_PLUGIN_PARAMETERS_INCLUDE

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

inline constexpr const uint32_t kCommonIOParameters = 1;
inline constexpr const uint32_t kExtraParameterCount = 0;

enum CommonParameters {
    kCommonParameterBypass,
    kCommonParameterReset,
   #if LIBREAUDIO_WANT_DRYWET
    kCommonParameterDryWet,
   #endif
    kCommonParameterCount
};

enum Groups {
    kGroupInput,
    kGroupOutput,
    kGroupMain,
    kGroupExtra,
};

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
    kParametersExtraStart,
    kParametersCount = kParametersExtraStart + kExtraParameterCount
};

inline constexpr const char* kCommonParameterSymbols[kCommonParameterCount] = {
    ParameterDesignationSymbols::bypass,
    ParameterDesignationSymbols::reset,
   #if LIBREAUDIO_WANT_DRYWET
    "dry_wet",
   #endif
};

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
            values[i] = 50.f;
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
        index >= kParametersMainStart ? kFaustParameters[index - kParametersMainStart] :
       #if LIBREAUDIO_WANT_COMMON_IO
        index >= kParametersOutputStart ? common_output::kFaustParameters[index - kParametersOutputStart + kCommonIOParameters] :
        index >= kParametersInputStart ? common_input::kFaustParameters[index - kParametersInputStart] :
       #endif
        _kFallback;
}

inline constexpr float getParameterInit(const uint32_t index) noexcept
{
    return getParameter(index).init;
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
