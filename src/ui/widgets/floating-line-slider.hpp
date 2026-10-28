// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"
#include "../_lab/metrics.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"
#include "LibreAudioUtils.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class FloatingLineSliderWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::FloatingLineSlider;
    using BaseWidget = LabKnobWidget;

    static_assert(R::width == 0, "floating line slider must have 0 width, size is adjusted by its parent");
    static_assert(R::height == 0, "floating line slider must have 0 height, size is adjusted by its parent");

public:
    explicit FloatingLineSliderWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index])
    {
        setName(fParameter.name);
        setDefault(fParameter.init);
        setRange(fParameter.min, fParameter.max);
        setStep(fParameter.step);
        setUsingLogScale(fParameter.isLogarithmic);
        setValue(fParameter.init, false);
    }

private:
    const FaustParameter& fParameter;

    // FIXME not very good
    static Color rainbow(const float t) noexcept
    {
        const float p = std::clamp(t, 0.f, 1.f) * (std::size(Reference::kAccentColors) - 2);
        const size_t i = std::min(std::size(Reference::kAccentColors) - 2, static_cast<size_t>(p));
        return Color(Reference::kAccentColors[i + 1], Reference::kAccentColors[i + 2], p - i);
    }

    void onNanoDisplay() final
    {
        const LabMetrics<R> m { this };

        drawReferenceBackground<R>();

        const bool lit = (getState() & kKnobStateDraggingHover) != 0;
        const float normvalue = getNormalizedValue();
        const float gap = 8.f * m.s;
        const float radius = (lit != 0 ? 2.f : 1.5f) * m.s;
        const float y = m.height * (1.f - normvalue);

        char valuestr[48];
        _renderParameterValue<32>(valuestr, fParameter, getValue());

        if (*fParameter.unit != '\0')
        {
            std::strncat(valuestr, " ", 2);
            std::strncat(valuestr, fParameter.unit, 10);
        }

        for (float x = m.border + m.margin + gap * 0.5f, limit = m.width - (m.border - m.margin) * 2; x < limit; x += gap)
        {
            beginPath();
            circle(x, y, radius);
            fillColor(rainbow(x / limit).withAlpha(lit ? 1.f : 0.85f));
            fill();
        }

        fontSize(m.fontSize);
        textLetterSpacing(m.letterSpacing);
        fillColor(Reference::kAccentColors[5].withAlpha(lit ? 1.f : 0.85f));

        float sy;
        if (y > m.height * 0.5f)
        {
            sy = y - 4.f * m.s;
            textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            fontFace("regular");
            text(m.width - 8.f * m.s, sy, getName(), nullptr);
            fontFace("mono");
            text(m.width - 8.f * m.s, sy - m.fontSize, valuestr, nullptr);
        }
        else
        {
            sy = y + 4.f * m.s;
            textAlign(ALIGN_RIGHT | ALIGN_TOP);
            fontFace("mono");
            text(m.width - 8.f * m.s, sy + m.fontSize, valuestr, nullptr);
            fontFace("regular");
            text(m.width - 8.f * m.s, sy, getName(), nullptr);
        }

        drawReferenceBorder<R>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
