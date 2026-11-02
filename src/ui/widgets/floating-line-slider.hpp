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
    static_assert(R::padding != 0, "floating line padding must not be 0, required for space between line dots");

public:
    explicit FloatingLineSliderWidget(LabWidget* const parent,
                                      const FaustParameterIndex index,
                                      const float extraMinimum = 0.f)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index])
    {
        setActiveCursor(R::cursor);
        setName(fParameter.name);
        setDefault(fParameter.init);
        setRange(fParameter.min - extraMinimum, fParameter.min, fParameter.max);
        setStep(fParameter.step);
        setUsingLogScale(fParameter.isLogarithmic);
        setValue(fParameter.init, false);

        if (*fParameter.unit != '\0')
        {
            fUnitWithSpacePrefix = " ";
            fUnitWithSpacePrefix += fParameter.unit;
        }
    }

    void setRange(const float min, const float start, const float max)
    {
        DISTRHO_SAFE_ASSERT_RETURN(min <= start,);
        DISTRHO_SAFE_ASSERT_RETURN(start < max,);

        fPaddedMinimum = (start - min) / (max - min);
        LabKnobWidget::setRange(start, max);
    }

private:
    const FaustParameter& fParameter;
    float fPaddedMinimum = 0.f;
    String fUnitWithSpacePrefix;

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

        const bool lit = (getState() & kKnobStateDraggingHover) != 0;
        const float normvalue = std::clamp(getNormalizedValue(), 0.f, 1.f);
        const float radius = (lit != 0 ? 2.f : 1.5f) * m.s;
        const float usableHeight = m.height - fPaddedMinimum * m.height;
        const float y = usableHeight * (1.f - normvalue);
        int align;
        float sy;

        // background
        drawReferenceBackground<R>();

        // slider
        for (float x = m.border + m.margin, limit = m.width - m.border - m.margin; x < limit; x += m.padding)
        {
            beginPath();
            circle(x, y, radius);
            fillColor(rainbow(x / limit).withAlpha(lit ? 1.f : 0.85f));
            fill();
        }

        // name and value
        if (y > m.height * 0.5f)
        {
            align = ALIGN_BOTTOM;
            sy = y - 4.f * m.s;
        }
        else
        {
            align = ALIGN_TOP;
            sy = y + 4.f * m.s;
        }

        char valuestr[48];
        _renderParameterValue<32>(valuestr, fParameter, getValue());

        if (! fUnitWithSpacePrefix.isEmpty())
            std::strncat(valuestr, fUnitWithSpacePrefix.buffer(), 48 - 32);

        fillColor(Reference::kAccentColors[5].withAlpha(lit ? 1.f : 0.85f));
        fontSize(m.fontSize);
        textLetterSpacing(m.letterSpacing);

        fontFace("regular");
        textAlign(ALIGN_LEFT | align);
        text(m.border + m.margin + m.padding * 2, sy, getName(), nullptr);

        fontFace("mono");
        textAlign(ALIGN_RIGHT | align);
        text(m.width - m.border - m.margin - m.padding * 2, sy, valuestr, nullptr);

        // border
        drawReferenceBorder<R>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
