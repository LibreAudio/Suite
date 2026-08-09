// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NanoVG.hpp"

START_NAMESPACE_DISTRHO

class LibreAudioUIWidgetInterface;

// --------------------------------------------------------------------------------------------------------------------
// base widget class

template <class BaseWidget>
class LibreAudioBaseWidget : public BaseWidget
{
public:
    explicit LibreAudioBaseWidget(LibreAudioBaseWidget<NanoSubWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->fScaleFactor) {}

    explicit LibreAudioBaseWidget(LibreAudioBaseWidget<NanoTopLevelWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->fScaleFactor) {}

    explicit LibreAudioBaseWidget(Window& windowToMapTo, LibreAudioUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo),
          fInterface(iface),
          fScaleFactor(windowToMapTo.getScaleFactor()) {}

    void addIdleCallback(IdleCallback* const callback)
    {
        if constexpr (std::is_same_v<BaseWidget, NanoSubWidget>)
            BaseWidget::getWindow().addIdleCallback(callback);
        else
            addIdleCallback(callback);
    }

    double getTime() const
    {
        return BaseWidget::getApp().getTime();
    }

    bool timeEllapsed(const double lastTime, const double wantedTime) const
    {
        return d_isNotZero(lastTime) ? getTime() - lastTime >= wantedTime : false;
    }

    bool timeNotEllapsed(const double lastTime, const double wantedTime) const
    {
        return d_isNotZero(lastTime) ? getTime() - lastTime < wantedTime : false;
    }

protected:
    LibreAudioUIWidgetInterface* const fInterface;
    float fScaleFactor;

    friend class LibreAudioBaseWidget<NanoSubWidget>;
    // friend class LibreAudioBaseWidget<NanoTopLevelWidget>;
};

using LibreAudioWidget = LibreAudioBaseWidget<NanoSubWidget> ;
using LibreAudioTopLevelWidget = LibreAudioBaseWidget<NanoTopLevelWidget>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
