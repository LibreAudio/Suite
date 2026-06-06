// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct FaustParameter
{
    const char* label;
    const char* symbol;
    const char* tooltip;
    const char* unit;
    float init;
    float min;
    float max;
    float step;
    bool isBoolean : 1;
    bool isInteger : 1;
    bool isLogarithmic : 1;
    bool isOutput : 1;
    bool isTrigger : 1;
};

template<int count>
using FaustParameters = FaustParameter[count];

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
