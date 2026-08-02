// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NanoVG.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// base widget class

template <class BaseWidget = NanoSubWidget>
class LibreAudioBaseWidget : public BaseWidget
{
public:
    explicit LibreAudioBaseWidget(NanoSubWidget* const parent)
        : BaseWidget(parent),
          fScaleFactor(parent->getTopLevelWidget()->getScaleFactor()) {}

    explicit LibreAudioBaseWidget(NanoTopLevelWidget* const parent)
        : BaseWidget(parent),
          fScaleFactor(parent->getScaleFactor()) {}

    explicit LibreAudioBaseWidget(Window& windowToMapTo)
        : BaseWidget(windowToMapTo),
          fScaleFactor(windowToMapTo.getScaleFactor()) {}

protected:
    float fScaleFactor;
};

typedef LibreAudioBaseWidget<NanoSubWidget> LibreAudioWidget;
typedef LibreAudioBaseWidget<NanoTopLevelWidget> LibreAudioTopLevelWidget;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
