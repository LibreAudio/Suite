// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"

#include "delay-parameters.hpp"
#include "djFilter-parameters.hpp"
#include "glueBus-parameters.hpp"
#include "limiter-parameters.hpp"
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

inline constexpr const std::array<float[0], 0> _kPresetValues = {};

inline constexpr const std::array<FaustParameterIndex, 0> _kIgnoredPresetParameters = {};

// --------------------------------------------------------------------------------------------------------------------

namespace delay {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Wide Quaters", "Warm, laid back and stereo widened quater echoes" },
    { "Slap 16th", "Tightly modulated and de-essed slap echo" },
    { "Ping Pong", "Relaxed dotted 8th ping-pong" },
    { "Echo Extreme", "High repetition filtered echo madness" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        2.0, 0.0, 1.0, 1.0, 375.0, 500.0, 120.0, 3.0, 3.0, 2.0, 2.0, 35.0, 0.0, 30.0, 0.20200000703334808, 1.5, 90.0, 180.0, 20000.0, 40.0, 20.0, 100.0, 100.0, -0,-0,-0,
    },
    {
        1.0, 0.0, 1.0, 1.0, 375.0, 500.0, 120.0, 9.0, 9.0, 0.0, 0.0, 0.0, 0.0, 60.0, 0.5020000338554382, 0.09999999403953552, 90.0, 80.0, 10000.0, 80.0, 0.0, 100.0, 100.0, -0,-0,-0,
    },
    {
        0.0, 2.0, 1.0, 1.0, 375.0, 500.0, 120.0, 5.0, 5.0, 1.0, 2.0, 50.0, 30.0, 60.0, 0.5020000338554382, 0.0, 90.0, 80.0, 7000.0, 40.0, 20.0, 100.0, 100.0, -0,-0,-0,
    },
    {
        2.0, 0.0, 0.0, 0.0, 489.0, 1289.0, 120.0, 5.0, 5.0, 0.0, 0.0, 70.0, 100.0, 0.0, 0.5020000338554382, 1.5999999046325684, 90.0, 80.0, 1999.0, 40.0, 0.0, 130.0, 100.0, -0,-0,-0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterDrywet,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace djFilter {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Clean Cut", "Non-resonant 12dB\nHighpass and Lowpass" },
    { "66% Resonance", "Clean but singing\nresonant filters" },
    { "Spacial Filters", "Drowning in reverb\non the edges" },
    { "Whiny Baby", "Resonant distortion\non the extreme" },
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

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterKnob,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace glueBus {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Gentle Glue", "Subtle compression and a bit of color" },
    { "Tight Punch", "Snappy bus glue for punchy drums" },
    { "Heavy Parallel", "Smashed parallel compression blended for massive weight" },
    { "Analog Glue", "Warm console-style glue with subtle saturation" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        0.10000002384185791, 100.0, 0.0, 2.0, 0.0, 100.0, 1000.0, 0.0, 0.0, 1000.0, 0.0, 0.0, -0,-0,-0,-0,-0,-0,
    },
    {
        0.10000002384185791, 100.0, 0.0, 4.0, 0.0, 100.0, 100.0, 5.0, -5.0, 5001.0, 0.0, 5.0, -0,-0,-0,-0,-0,-0,
    },
    {
        0.5999999046325684, 100.0, 0.0, 10.0, 0.0, 30.0, 100.0, 0.0, 20.0, 5001.0, 0.0, 20.0, -0,-0,-0,-0,-0,-0,
    },
    {
        -0.5, 100.0, 0.0, 1.0, 0.0, 80.0, 80.0, 3.0, 8.0, 2846.0, 0.0, -3.0, -0,-0,-0,-0,-0,-0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterDrywet,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace limiter {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Clean & Safe", "No coloration, no distortion" },
    { "Loud & Colored", "Punchy, saturated limiting for bold masters" },
    { "Ultimate Softness", "Slow response with looong time constants" },
    { "Hot Punch", "Loud and snappy transients" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        0.0, -1.0, 1.0, 0.0, 1.0, 50.0, 50.0, 50.0, 0.0, 0.0, -0,-0,
    },
    {
        0.0, -1.0, 1.0, 0.0, 0.10000000149011612, 100.0, 3.0, 20.0, 0.0, 0.0, -0,-0,
    },
    {
        0.0, -1.0, 1.0, 0.0, 2.0, 0.0, 200.0, 20.0, 0.0, 0.0, -0,-0,
    },
    {
        0.0, -1.0, 1.0, 0.0, 0.30000001192092896, 100.0, 30.0, 100.0, 0.0, 0.0, -0,-0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterThreshold,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace mbComp5 {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Pop Master", "Gentle multiband control for polished masters" },
    { "Soft Smoother", "Slow, transparent leveling with lookahead precision" },
    { "Smooth Vocals", "Even vocal tone with tamed sibilance" },
    { "FM Radio", "Heavy band compression for that on-air polish" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        3.0, 6.0, 5.0, 100.0, 100.0, 0.0, 1.0, 0.0, 100.0, 89.73214721679688, 350.41461181640625, 1532.569091796875, 5994.70556640625, -18.0, 2.0, 30.0, 300.0, 0.0, 0.0, 0.0, -22.0, 2.0, 20.0, 250.0, 0.0, 0.0, 0.0, -26.0, 2.0, 10.0, 180.0, 0.0, 0.0, 0.0, -28.0, 2.0, 6.0, 120.0, 0.0, 0.0, 0.0, -32.0, 2.0, 4.0, 90.0, 0.0, 0.0, 0.0, -0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,
    },
    {
        16.0, 6.0, 20.0, 100.0, 66.0, 10.0, 0.0, 10.0, 100.0, 89.73214721679688, 350.41461181640625, 1532.569091796875, 5994.70556640625, -22.0, 2.0, 40.0, 400.0, 2.0, 0.0, 0.0, -26.0, 2.200000047683716, 35.0, 360.0, 2.0, 0.0, 0.0, -30.0, 2.4000000953674316, 30.0, 320.0, 2.0, 0.0, 0.0, -32.0, 2.6000001430511475, 28.0, 280.0, 2.0, 0.0, 0.0, -35.99998474121094, 2.799999952316284, 25.0, 240.0, 2.0, 0.0, 0.0, -0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,
    },
    {
        2.8999998569488525, 60.0, 2.0, 100.0, 100.0, 0.0, 1.0, 0.0, 100.0, 159.6293182373047, 793.8001098632812, 2752.155029296875, 8000.98779296875, -24.0, 8.0, 30.0, 20.0, 0.0, 0.0, 0.0, -24.0, 8.0, 20.0, 15.0, 1.0, 0.0, 0.0, -30.0, 8.0, 10.0, 10.0, 0.0, 0.0, 0.0, -36.0, 8.0, 6.0, 5.0, 1.0, 0.0, 0.0, -40.0, 8.0, 4.0, 1.0, 3.0, 0.0, 0.0, -0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,
    },
    {
        20.0, 60.0, 25.0, 50.0, 50.0, 0.0, 2.0, 5.0, 100.0, 85.2732162475586, 350.41461181640625, 1532.569091796875, 5994.70556640625, -22.0, 20.0, 24.99999237060547, 150.0, 4.0, 0.0, 0.0, -26.0, 20.0, 19.999996185302734, 100.0, 2.5, 0.0, 0.0, -30.0, 20.0, 14.999994277954102, 40.0, 3.0, 0.0, 0.0, -32.0, 20.0, 13.000000953674316, 15.0, 1.0, 0.0, 0.0, -35.99998474121094, 20.0, 10.000000953674316, 5.0, 2.0, 0.0, 0.0, -0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,-0,
    },
}};

}

// --------------------------------------------------------------------------------------------------------------------

namespace reverb {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Large Hall", "Expansive concert hall with long, lush decay" },
    { "Warm Medium Hall", "Mid-sized concert space with gentle warmth" },
    { "Dark Piano Hall", "Warm and smooth, long tail" },
    { "Small Theatre", "Intimate stage ambience with warm wood tones" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        1.399999976158142, 50.0, 2000.0, 70.0, -5.0, 2.700000047683716, 1.899999976158142, 50.0, 70.0, 30.0, 0.0, 1.5, 465.0, 4001.0, 0.4000000059604645, 4.0, 80.0, 20.0, 25.0, 5500.0, 20.0, 25.0, 0.0, 110.0, 100.0, -0,-0,-0,-0,-0,-0,
    },
    {
        1.2000000476837158, 50.0, 2500.0, 70.0, -3.0, 1.6999999284744263, 1.7000000476837158, 30.0, 23.0, 25.0, 0.0, 1.2999999523162842, 400.0, 3300.0, 0.4000000059604645, 6.0, 80.0, 20.0, 40.0, 4001.0, 5.0, 20.0, 0.0, 100.0, 100.0, -0,-0,-0,-0,-0,-0,
    },
    {
        1.0, 50.0, 1970.0, 70.0, -5.0, 2.1599998474121094, 1.7000000476837158, 40.0, 23.0, 34.0, 0.0, 1.4399999380111694, 400.0, 3148.0, 0.4000000059604645, 8.0, 80.0, 0.0, 40.0, 3670.0, 10.0, 20.0, 0.0, 100.0, 100.0, -0,-0,-0,-0,-0,-0,
    },
    {
        1.2000000476837158, 30.0, 2500.0, 50.0, -6.0, 0.75, 1.4000000953674316, 20.0, 15.0, 15.0, 0.0, 1.1999999284744263, 540.0, 4001.0, 0.4000000059604645, 4.0, 50.0, 20.0, 25.0, 8001.0, 10.0, 10.0, 0.0, 100.0, 100.0, -0,-0,-0,-0,-0,-0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterDrywet,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace springReverb {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Large Wide Vocals", "Spacious stereo spring for lush vocals" },
    { "Icy Stage", "Shimmering cold tank with mid-length tail" },
    { "Guitar Amp Tank", "Old-school amp reverb, narrow and warm" },
    { "Huge Ducked Sustain", "Giant wash with sidechain breathing" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        30.0, 100.0, 15.0, 30.0, -0,-0,0.0, 40.0, 120.0, 100.0, -0,-0,
    },
    {
        10.0, 60.0, 30.0, 60.0, -0,-0,0.0, 0.0, 100.0, 100.0, -0,-0,
    },
    {
        0.0, 15.0, 0.0, 20.0, -0,-0,0.0, 0.0, 40.0, 100.0, -0,-0,
    },
    {
        50.0, 100.0, 0.0, 40.0, -0,-0,90.0, 70.0, 140.0, 100.0, -0,-0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterMix,
};

}

// --------------------------------------------------------------------------------------------------------------------

namespace vocalDoubler {

inline constexpr const std::array<PresetMetadata, kNumPresets> _kPresetMetadata = {{
    { "Wide Pop", "Polished double-tracked width for modern pop" },
    { "Bright Detune", "Crisp micro-pitch spread for sparkling vocals" },
    { "Hookline Doubles", "Human-like doubles with subtle imperfections" },
    { "Lennon Overdub", "Vintage tape ADT with classic 60s character" },
}};

inline constexpr const std::array<float[kFaustParameterCount], kNumPresets> _kPresetValues = {{
    {
        0.0, 18.0, 2.0, 0.5999999642372131, 1.5, 0.0, 0.9999999403953552, 20.0, 14.0, 0.25, 6.0, 1.0, 25.0, 15.0, 10.0, 40.0, 1.0, 50.0, 60.0, 3105.0, 1.0, -20.0, -0,
    },
    {
        1.0, 18.0, 2.0, 0.5999999642372131, 1.5, 0.0, 0.9999999403953552, 13.0, 9.0, 0.25, 6.0, 1.0, 25.0, 15.0, 10.0, 40.0, 1.0, 40.0, 104.0, 20000.0, 0.0, 0.0, -0,
    },
    {
        2.0, 18.0, 2.0, 0.5999999642372131, 1.5, 0.0, 0.9999999403953552, 13.0, 9.0, 0.25, 6.0, 1.0, 25.0, 15.0, 10.0, 40.0, 1.0, 20.0, 104.0, 3460.0, 0.0, 0.0, -0,
    },
    {
        0.0, 25.0, 1.0, 0.4999999701976776, 2.0, 0.0, 0.9999999403953552, 13.0, 9.0, 0.25, 6.0, 1.0, 25.0, 15.0, 10.0, 40.0, 1.0, 50.0, 591.0, 7160.0, 2.0, 0.0, -0,
    },
}};

inline constexpr const std::array _kIgnoredPresetParameters = {
    kFaustParameterMix,
};

}

// --------------------------------------------------------------------------------------------------------------------
// export presets if available, otherwise fallback to none

namespace LIBREAUDIO_PLUGIN_ID {

inline constexpr const decltype(_kPresetMetadata) kPresetMetadata = _kPresetMetadata;
inline constexpr const decltype(_kPresetValues) kPresetValues = _kPresetValues;
inline constexpr const decltype(_kIgnoredPresetParameters) kIgnoredPresetParameters = _kIgnoredPresetParameters;

}

// --------------------------------------------------------------------------------------------------------------------
