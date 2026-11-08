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

template<const Color& kAccentColor = Reference::Colors::acc1>
class NumberBoxWidget : public LabParameterWidget
{
    using BaseWidget = LabParameterWidget;

public:
    explicit NumberBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index]),
          fUsingMiddleSplitBar(d_isEqual(fParameter.min, -fParameter.max))
    {
        setActiveCursor(R::cursor);
        setName(fParameter.name);
        setDefault(fParameter.init);
        setRange(fParameter.min, fParameter.max);
        setStep(fParameter.step);
        setUsingLogScale(fParameter.isLogarithmic);
        setValue(fParameter.init, false);

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
        renderParameterValue(valuestr, fParameter, getValue());

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(m.name.fontSize);
        textLetterSpacing(m.name.letterSpacing);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + m.margin, m.border + m.margin, fParameter.name, nullptr);

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

            if (fUsingMiddleSplitBar)
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
    const FaustParameter& fParameter;
    const bool fUsingMiddleSplitBar;

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

    void updateSize(const bool updateChildren) final
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1>
class NumberBoxWithMeterWidget : public NumberBoxWidget<kAccentColor>
{
    using BaseWidget = NumberBoxWidget<kAccentColor>;
    using R = typename BaseWidget::R;

public:
    explicit NumberBoxWithMeterWidget(LabWidget* const parent,
                                      const FaustParameterIndex index,
                                      const FaustParameterIndex meter)
        : BaseWidget(parent, index),
          fMeter(kFaustParameters[meter]),
          fMeterIndex(meter),
          fMeterRange(fMeter.max - fMeter.min)
    {
    }

protected:
    void onNanoDisplay() override
    {
        BaseWidget::onNanoDisplay();

        const NumberBoxMetrics<R> m { this };

        const float normvalue = 1.f - (
            (this->fInterface->getParameterValue(kParametersMainStart + fMeterIndex) - fMeter.min) / fMeterRange
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

private:
    const FaustParameter& fMeter;
    const FaustParameterIndex fMeterIndex;
    const float fMeterRange;
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex, const Color& kAccentColor>
class StaticNumberBoxWidget final : public NumberBoxWidget<kAccentColor>
{
public:
    explicit StaticNumberBoxWidget(LabWidget* const parent)
        : NumberBoxWidget<kAccentColor>(parent, kParameterIndex) {}
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex, FaustParameterIndex kMeterIndex, const Color& kAccentColor>
class StaticNumberBoxWithMeterWidget final : public NumberBoxWithMeterWidget<kAccentColor>
{
public:
    explicit StaticNumberBoxWithMeterWidget(LabWidget* const parent)
        : NumberBoxWithMeterWidget<kAccentColor>(parent, kParameterIndex, kMeterIndex) {}
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
