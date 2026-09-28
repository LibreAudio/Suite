// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "las-resources.h"

#include "../reference.hpp"
#include "../widgets/shader-clean.hpp"
#include "../_lab/root.hpp"

#include <list>

START_NAMESPACE_DISTRHO
class LibreAudioBaseUI;
END_NAMESPACE_DISTRHO

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class RootBaseWidget : public ReferenceRootWidget<Reference::Window, kVertical>
{
    using BaseWidget = ReferenceRootWidget<Reference::Window, kVertical>;

public:
    RootBaseWidget(Window& window, LabUIWidgetInterface* const iface)
        : BaseWidget(window, iface)
    {
        createFontFromMemory("regular",
                             FONTS_INTER_18PT_REGULAR_TTF_DATA,
                             FONTS_INTER_18PT_REGULAR_TTF_LEN,
                             false);
        createFontFromMemory("mono",
                             FONTS_SPLINESANSMONO_REGULAR_TTF_DATA,
                             FONTS_SPLINESANSMONO_REGULAR_TTF_LEN,
                             false);
    }

    virtual void enableShaders(const std::list<ShaderBaseWidget*>& shaders)
    {
        fShaders = shaders;
        updateSize(false);
    }

protected:
    std::list<ShaderBaseWidget*> fShaders;

    friend class DISTRHO_NAMESPACE::LibreAudioBaseUI;
};

// --------------------------------------------------------------------------------------------------------------------

template <class TopBar, class MainArea>
class RootWidget : public RootBaseWidget
{
public:
    RootWidget(Window& window, LabUIWidgetInterface* const iface)
        : RootBaseWidget(window, iface) {}

protected:
    std::shared_ptr<TopBar> fTopBar = addWidget<TopBar>();
    std::shared_ptr<MainArea> fMainArea = addWidget<MainArea, Expanding>();

    void updateSize(const bool updateChildren) final
    {
        RootBaseWidget::updateSize(updateChildren);

        const Point<int> pos = fMainArea->getMainAreaAbsolutePos();
        const Size<uint> size = fMainArea->getMainAreaSize();
        const float borderRadius = fMainArea->getMainAreaBorderRadius();

        for (ShaderBaseWidget* const sw : fShaders)
        {
            sw->setAbsolutePos(pos);
            sw->setSize(size);
            sw->setBorderRadius(borderRadius);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
