// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

#include "FaustParameter.hpp"

#ifndef M_PIf
#define M_PIf static_cast<float>(M_PI)
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// define a few useful constants, so we don't need to calculate them on runtime

// log(10)
inline constexpr const float kLog10f = 2.302585092994046f;

// log(1000)
inline constexpr const float kLog1000f = 6.907755278982137f;

// --------------------------------------------------------------------------------------------------------------------
// dB level related functions

inline constexpr const float kDbScale = 20.f;
inline constexpr const float kDbScaleHalf = kDbScale * 0.5f;
inline constexpr const float kDbScaleHalf__log = kLog10f; // std::log(kDbScaleHalf)

[[nodiscard]] inline constexpr float db2coef(const float db, const float min)
{
    return db > min ? std::pow(kDbScaleHalf, db / kDbScale) : 0.f;
}

[[nodiscard]] inline constexpr float coef2db(const float coef, const float min)
{
    return d_isNotZero(coef) ? std::max(min, std::log(coef) / kDbScaleHalf__log * kDbScale) : min;
}

// --------------------------------------------------------------------------------------------------------------------
// Hz frequency related helpers

inline constexpr const int kFreqMin = 20;
inline constexpr const int kFreqMax = 20'000;
inline constexpr const int kFreqScale = 1000;
inline constexpr const float kFreqMinf = kFreqMin;
inline constexpr const float kFreqMaxf = kFreqMax;
inline constexpr const float kFreqScalef = kFreqScale;
inline constexpr const float kFreqScale__log = kLog1000f; // std::log(kFreqScale)

[[nodiscard]] inline constexpr float freq2norm(const float freq)
{
    return std::log(freq / kFreqMin) / kFreqScale__log;
}

[[nodiscard]] inline constexpr float norm2freq(const float norm)
{
    return kFreqMin * std::pow(kFreqScale, norm);
}

// --------------------------------------------------------------------------------------------------------------------

template<size_t size>
static inline void _renderParameterValue(char str[size], const FaustParameter& param, const float value)
{
    if (param.scalePointCount != 0)
    {
        if (const int ivalue = d_roundToInt(value); value >= 0 && value < param.scalePointCount)
        {
            std::snprintf(str, size, "%s", param.scalePoints[ivalue].label);
            str[size - 1] = '\0';
            return;
        }
    }

    /**/ if (param.step >= 1.f)
        std::snprintf(str, size, "%d", d_roundToInt(value));
    else if (param.step >= 0.1f)
        std::snprintf(str, size, "%.1f", d_roundToInt(value * 10.f) * 0.1f);
    else
        std::snprintf(str, size, "%.2f", d_roundToInt(value * 100.f) * 0.01f);

    str[size - 1] = '\0';
}

#define renderParameterValue(str, param, value) _renderParameterValue<std::size(str)>(str, param, value)

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
