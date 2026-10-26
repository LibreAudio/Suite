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

template<class R>
struct NumberBoxMetrics : LabMetrics<R> {
    using LabMetrics<R>::LabMetrics;

    const LabMetricsHeight<R, typename R::Bar> bar { *this };
    const LabMetricsFontSize<R, typename R::Name> name { *this };
    const LabMetricsFontSize<R, typename R::Unit> unit { *this };
    const LabMetricsFontSize<R, typename R::Value> value { *this };
};

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1, uint height = Reference::Widgets::NumberBox<>::height>
class NumberBoxWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::NumberBox<kAccentColor, height>;
    using BaseWidget = LabKnobWidget;

public:
    explicit NumberBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index]),
          fMiddleSplitBar(d_isEqual(fParameter.min, -fParameter.max))
    {
        setName(fParameter.name);
        setDefault(fParameter.init);
        setRange(fParameter.min, fParameter.max);
        setStep(fParameter.step);
        setUsingLogScale(fParameter.isLogarithmic);
        setValue(fParameter.init, false);

        updateSize(false);
    }

private:
    const FaustParameter& fParameter;
    const bool fMiddleSplitBar;

    void knobDoubleClicked(SubWidget*) final
    {
        char valuestr[64];
        renderParameterValue(valuestr, fParameter, getValue());

        fInterface->openTextInput(this,
                                  valuestr,
                                  getAbsoluteArea(),
                                  R::Name::color,
                                  R::Value::color,
                                  R::borderRadius,
                                  R::Value::fontSize,
                                  true);
    }

    void onNanoDisplay() final
    {
        const NumberBoxMetrics<R> m { this };

        const float border2x = m.border * 2;

        drawReferenceBackground<R>();
        // drawReferenceBorder<R>();

        char valuestr[32];
        renderParameterValue(valuestr, fParameter, getValue());

        // inset shading
        beginPath();
        roundedRect(0, 0, m.width, m.height, m.borderRadius);
        fillPaint(linearGradient(0, 0, 0, m.margin, R::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::backgroundColor);
        strokeWidth(m.border * 0.5f);
        stroke();

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(m.name.fontSize);
        textLetterSpacing(m.name.letterSpacing);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + m.margin, m.border + m.margin, fParameter.name, nullptr);

        fillColor(R::Value::color);
        fontFace("mono");
        fontSize(m.value.fontSize);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        textLetterSpacing(m.value.letterSpacing);
        text(border2x + m.margin, m.height - m.bar.height - m.margin, valuestr, nullptr);

       #ifdef LIBREAUDIO_PLUGIN__mbComp5
        const char* const unit = std::strcmp(fParameter.name, "Ratio") == 0 ? ":1"
                               : fParameter.unit;
       #else
        const char* const unit = fParameter.unit;
       #endif
        if (*unit != '\0')
        {
            fillColor(R::Unit::color);
            fontFace("regular");
            fontSize(m.unit.fontSize);
            textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            textLetterSpacing(m.unit.letterSpacing);
            text(m.width - border2x - m.margin, m.height - m.bar.height - m.margin, unit, nullptr);
        }

        // bottom bar
        {
            const float normvalue = getNormalizedValue();

            save();

            scissor(0, m.height - m.bar.height, m.width, m.bar.height);
            beginPath();
            roundedRect(0, 0, m.width, m.height, m.borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            if (fMiddleSplitBar)
            {
                if (normvalue < 0.5f)
                {
                    const float x = normvalue * m.width;
                    scissor(x, m.height - m.bar.height, m.width * 0.5f - x, m.bar.height);
                }
                else
                {
                    scissor(m.width * 0.5f, m.height - m.bar.height, (normvalue - 0.5f) * m.width, m.bar.height);
                }
            }
            else
            {
                scissor(0, m.height - m.bar.height, normvalue * m.width, m.bar.height);
            }

            beginPath();
            roundedRect(0, 0, m.width, m.height, m.borderRadius);
            fillColor(R::Bar::color);
            fill();

            restore();
        }
    }

    void updateSize(const bool updateChildren) final
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
