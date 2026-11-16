// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"

#include "djFilter-parameters.hpp"
#include "mbComp5-parameters.hpp"
#include "reverb-parameters.hpp"
#include "springReverb-parameters.hpp"
#include "vocalDoubler-parameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

inline constexpr const uint32_t kNumPresets = 4;

struct PresetMetadata {
    const char* title;
    const char* subtitle;
};

// --------------------------------------------------------------------------------------------------------------------
// empty data as fallback

inline constexpr const std::array<PresetMetadata, 0> _kPresetMetadata = {};

inline constexpr const std::array<float[], 0> _kPresetValues = {};

// --------------------------------------------------------------------------------------------------------------------

namespace djFilter {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Clean Cut", "Non-resonant 12dB Highpass and Lowpass" },
    { "Two Thirds Resonance", "Clean but singing resonant filters" },
    { "Spacial Filters", "Drowning in reverb on the edges" },
    { "Whiny Baby", "Resonant distortion on the extreme" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        0.0, 0.0, 0.0, 0.0,
    },
    {
        0.0, 66.0, 0.0, 0.0,
    },
    {
        0.0, 20.0, 0.0, 100.0,
    },
    {
        0.0, 100.0, 100.0, 0.0,
    },
}};

}

#if 0
// --------------------------------------------------------------------------------------------------------------------

namespace mbComp5 {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
}};

}

// --------------------------------------------------------------------------------------------------------------------

namespace reverb {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
}};

}

// --------------------------------------------------------------------------------------------------------------------

namespace springReverb {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
}};

}

// --------------------------------------------------------------------------------------------------------------------

namespace vocalDoubler {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
}};

}
#endif

// --------------------------------------------------------------------------------------------------------------------
// export presets if available, otherwise fallback to none

namespace LIBREAUDIO_PLUGIN_ID {

inline constexpr const auto kPresetMetadata = _kPresetMetadata;
inline constexpr const auto kPresetValues = _kPresetValues;

}

// --------------------------------------------------------------------------------------------------------------------
