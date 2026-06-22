// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoDetails.hpp"

#include "common_input-parameters.hpp"
#include "common_output-parameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint32_t kCommonIOParameters = 1;

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
};

enum Parameters {
    kParametersCommonStart,
    kParametersCommonEnd = kParametersCommonStart + kCommonParameterCount - 1,
    kParametersInputStart,
    kParametersInputEnd = kParametersInputStart + common_input::kFaustParameterCount - 1,
    kParametersOutputStart,
    kParametersOutputEnd = kParametersOutputStart + common_output::kFaustParameterCount - 1 - kCommonIOParameters,
    kParametersMainStart,
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
