// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

// --------------------------------------------------------------------------------------------------------------------
// generic parameter metadata

struct LibreAudioParameterEnumerationValue {
    const char* label;
    int value;
};

struct LibreAudioParameter {
    const char* bracket;
    const char* label;
    const char* shortlabel;
    const char* name;
    const char* symbol;
    const char* tooltip;
    const char* unit;
    const LibreAudioParameterEnumerationValue* scalePoints;
    float init;
    float min;
    float max;
    float step;
    uint8_t accentcolor;
    uint8_t scalePointCount;
    bool isBoolean : 1;
    bool isEasy : 1;
    bool isEnumerator : 1;
    bool isDynamic : 1;
    bool isInteger : 1;
    bool isLogarithmic : 1;
    bool isOutput : 1;
    bool isTrigger : 1;
};

// --------------------------------------------------------------------------------------------------------------------
// utilities

constexpr const char* getParameterLabel(const LibreAudioParameter& param)
{
    return param.label[0] != '\0' ? param.label : param.name;
}

constexpr const char* getParameterShortLabel(const LibreAudioParameter& param)
{
    return param.shortlabel[0] != '\0' ? param.shortlabel : getParameterLabel(param);
}

constexpr bool isParameterOutputOrTrigger(const LibreAudioParameter& param)
{
    return param.isOutput || param.isTrigger;
}

// --------------------------------------------------------------------------------------------------------------------
