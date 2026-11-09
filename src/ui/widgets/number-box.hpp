// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/parameter.hpp"
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

template<FaustParameterIndex kParameterIndex, uint8_t _accentColorIndex = UINT8_MAX>
class NumberBoxWidget : public LabParameterWidget
{
    using BaseWidget = LabParameterWidget;

    static constexpr const FaustParameter& kParameter = kFaustParameters[kParameterIndex];
    static constexpr const uint8_t kAccentColorIndex = _accentColorIndex != UINT8_MAX
                                                     ? _accentColorIndex
                                                     : kParameter.accentcolor;
    static constexpr const Color kAccentColor = Reference::kAccentColors[kAccentColorIndex];
    static constexpr const bool kUsingMiddleSplitBar = d_isEqual(kParameter.min, -kParameter.max);

public:
    explicit NumberBoxWidget(LabWidget* const parent)
        : BaseWidget(parent, kParametersMainStart + kParameterIndex)
    {
        setActiveCursor(R::cursor);
        setName(kParameter.name);
        setDefault(kParameter.init);
        setRange(kParameter.min, kParameter.max);
        setStep(kParameter.step);
        setUsingLogScale(kParameter.isLogarithmic);
        setValue(kParameter.init, false);

        updateSize(false);
    }

protected:
    using R = Reference::Widgets::NumberBox<kAccentColor>;

    void onNanoDisplay() override
    {
        const NumberBoxMetrics<R> m { this };

        const float border2x = m.border * 2;

        // ------------------------------------------------------------------------------------------------------------
        // background

        drawReferenceBackground<R>();

        // ------------------------------------------------------------------------------------------------------------
        // inset shading

        // FIXME make more generic

        beginPath();
        roundedRect(0, 0, m.width, m.height, m.borderRadius);
        fillPaint(linearGradient(0, 0, 0, m.margin, R::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::backgroundColor);
        strokeWidth(m.border * 0.5f);
        stroke();

        // ------------------------------------------------------------------------------------------------------------
        // border

        if constexpr (R::border != 0)
        {
            if ((getState() & kKnobStateDraggingHover) != 0)
                drawReferenceBorder<R, R::borderColor〡hovered>();
        }

        // ------------------------------------------------------------------------------------------------------------
        // name

        char valuestr[32];
        renderParameterValue(valuestr, kParameter, getValue());

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(m.name.fontSize);
        textLetterSpacing(m.name.letterSpacing);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + m.margin, m.border + m.margin, kParameter.name, nullptr);

        // ------------------------------------------------------------------------------------------------------------
        // value

        fillColor(R::Value::color);
        fontFace("mono");
        fontSize(m.value.fontSize);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        textLetterSpacing(m.value.letterSpacing);
        text(border2x + m.margin, m.height - m.bar.height - m.margin, valuestr, nullptr);

        // ------------------------------------------------------------------------------------------------------------
        // unit

       #ifdef LIBREAUDIO_PLUGIN__mbComp5
        const char* const unit = std::strcmp(kParameter.name, "Ratio") == 0 ? ":1"
                               : kParameter.unit;
       #else
        const char* const unit = kParameter.unit;
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

        // ------------------------------------------------------------------------------------------------------------
        // bottom bar

        {
            const float normvalue = getNormalizedValue();

            save();

            scissor(0, m.height - m.bar.height, m.width, m.bar.height);
            beginPath();
            roundedRect(0, 0, m.width, m.height, m.borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            if constexpr (kUsingMiddleSplitBar)
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

private:
    void knobDoubleClicked(SubWidget*) final
    {
        char valuestr[64];
        renderParameterValue(valuestr, kParameter, getValue());

        fInterface->openTextInput(this,
                                  valuestr,
                                  getAbsoluteArea(),
                                  R::Name::color,
                                  R::Value::color,
                                  R::borderRadius,
                                  R::Value::fontSize,
                                  true);
    }

    void updateSize(const bool updateChildren) final
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex, FaustParameterIndex kMeterIndex>
class NumberBoxWithMeterWidget : public NumberBoxWidget<kParameterIndex>
{
    using BaseWidget = NumberBoxWidget<kParameterIndex>;
    using BaseWidget::BaseWidget;
    using R = typename BaseWidget::R;

    static constexpr const FaustParameter& kMeter = kFaustParameters[kMeterIndex];

protected:
    void onNanoDisplay() override
    {
        BaseWidget::onNanoDisplay();

        const NumberBoxMetrics<R> m { this };

        const float normvalue = 1.f - (
            (this->fInterface->getParameterValue(kParametersMainStart + kMeterIndex) - kMeter.min) / (kMeter.max - kMeter.min)
        );
        const float x = normvalue * m.width;

        BaseWidget::save();

        BaseWidget::scissor(0, 0, m.width, m.bar.height);
        BaseWidget::beginPath();
        BaseWidget::roundedRect(0, 0, m.width, m.height, m.borderRadius);
        BaseWidget::fillColor(R::Bar::color〡deactivated);
        BaseWidget::fill();

        BaseWidget::scissor(x, 0, m.width - x, m.bar.height);

        BaseWidget::beginPath();
        BaseWidget::roundedRect(0, 0, m.width, m.height, m.borderRadius);
        BaseWidget::fillColor(Reference::Colors::orange);
        BaseWidget::fill();

        BaseWidget::restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
