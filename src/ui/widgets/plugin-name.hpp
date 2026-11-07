// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"
#include "../_lab/metrics.hpp"

#include "../reference.hpp"

#include "DistrhoPluginInfo.h"

#include <cctype>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class PluginNameWidget final : public LabReferenceWidget<Reference::Widgets::PluginName>
{
    using R = Reference::Widgets::PluginName;
    using BaseWidget = LabReferenceWidget<R>;

    char fName[sizeof(DISTRHO_PLUGIN_NAME) - 3];

public:
    PluginNameWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        std::memcpy(fName, _constexpr_DISTRHO_PLUGIN_NAME + 3, sizeof(DISTRHO_PLUGIN_NAME) - 3);
        fName[sizeof(DISTRHO_PLUGIN_NAME) - 4] = '\0';

        for (uint i = 0; i < sizeof(DISTRHO_PLUGIN_NAME) - 3; ++i)
            fName[i] = std::toupper(fName[i]);

        updateSize(false);
    }

private:
    void onNanoDisplay() final
    {
        const LabMetrics<R> m { this };

        BaseWidget::onNanoDisplay();

        fillColor(R::color);
        fontSize(m.fontSize);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textLetterSpacing(m.letterSpacing);
        text(m.width * 0.5f, m.height * 0.5f, fName);
    }

    void updateSize(const bool updateChildren) final
    {
        const LabMetrics<R> m { this };
        const float borderAndMarginSize = (m.border + m.margin) * 2;

        Rectangle<float> bounds;
        fontSize(m.fontSize);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        textLetterSpacing(m.letterSpacing);
        textBounds(0, 0, fName, nullptr, bounds);
        // FIXME
        // setSize(bounds.getWidth() + borderAndMarginSize, bounds.getHeight() + borderAndMarginSize);
        setWidth(bounds.getWidth() + borderAndMarginSize);

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
