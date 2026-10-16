// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

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

END_NAMESPACE_DISTRHO
