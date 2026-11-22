// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/parameter.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R,
         class BaseWidget,
         FaustParameterIndex kLeftMeterIndex,
         FaustParameterIndex kRightMeterIndex>
class ThresholdMeterBaseWidget : public BaseWidget,
                                 private std::conditional_t<std::is_base_of_v<IdleCallback, BaseWidget>, Unused, IdleCallback>
{
    static constexpr const float kTicks[] = { -6, -12, -24, -36, -48 };
    static constexpr const uint kWidth = (R::border + R::margin + R::Track::width) * 2 + R::padding + R::Slider::width;

public:
    explicit ThresholdMeterBaseWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        if constexpr (! std::is_base_of_v<IdleCallback, BaseWidget>)
            BaseWidget::addIdleCallback(this);
    }

    explicit ThresholdMeterBaseWidget(LabWidget* const parent, const uint32_t id)
        : BaseWidget(parent, id) {}

protected:
    static constexpr const LibreAudioParameter& kParameterMeterL = kFaustParameters[kLeftMeterIndex];
    static constexpr const LibreAudioParameter& kParameterMeterR = kFaustParameters[kRightMeterIndex];

    static_assert(kParameterMeterL.max == kParameterMeterR.max, "meter L vs R max mismatch");
    static_assert(kParameterMeterL.min == kParameterMeterR.min, "meter L vs R min mismatch");

    void idleCallback() override
    {
        if (const float valueL = std::clamp(this->fInterface->getParameterValue(kParametersMainStart + kLeftMeterIndex),
                                            kParameterMeterL.min,
                                            kParameterMeterL.max); d_isNotEqual(fMeterValueL, valueL))
        {
            fMeterValueL = valueL;
            BaseWidget::repaint();
        }

        if (const float valueR = std::clamp(this->fInterface->getParameterValue(kParametersMainStart + kRightMeterIndex),
                                            kParameterMeterR.min,
                                            kParameterMeterR.max); d_isNotEqual(fMeterValueR, valueR))
        {
            fMeterValueR = valueR;
            BaseWidget::repaint();
        }

        if constexpr (std::is_base_of_v<IdleCallback, BaseWidget>)
            BaseWidget::idleCallback();
    }

    void updateSize(const bool updateChildren) override
    {
        BaseWidget::template updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }

private:
    float fMeterValueL = kParameterMeterL.min;
    float fMeterValueR = kParameterMeterR.min;

    [[nodiscard]] float meterLevelToHeight(const float db, const float height) noexcept
    {
        const float normalized = 1.f - d_clamp((db - kParameterMeterL.min) / (kParameterMeterL.max - kParameterMeterL.min), 0.f, 1.f);
        // const float normalized = 1.f - d_clamp(std::pow(10.f, db * 0.05f), 0.f, 1.f);
        return d_roundToIntPositive(height * normalized);
    }

    void onNanoDisplay() final
    {
        const float border = R::border * this->fScaleFactor;
        const float margin = R::margin * this->fScaleFactor;
        const float padding = R::padding * this->fScaleFactor;
        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();

        const float sx = border + margin + (w - kWidth * this->fScaleFactor) * 0.5f;
        const float sw = R::Slider::width * this->fScaleFactor;

        const float mborder = R::Meter::border * this->fScaleFactor;
        const float mx = sx + sw + padding;
        const float my = border + margin;
        const float mw = w - sx * 2 - sw - padding;
        const float mh = h - (border + margin) * 2;

        const float tx = mx + mborder;
        const float ty = my + mborder;
        const float tw2 = mw - mborder * 2;
        const float tw1 = R::Track::width * this->fScaleFactor - 1.5f * this->fScaleFactor;
        const float th = mh - mborder * 2;

        // ------------------------------------------------------------------------------------------------------------
        // draw background

        BaseWidget::template drawReferenceBackground<R>();

        // ------------------------------------------------------------------------------------------------------------
        // meter area

        // background
        BaseWidget::beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            BaseWidget::roundedRect(mx, my, mw, mh, R::Meter::borderRadius * this->fScaleFactor);
        else
            BaseWidget::rect(mx, my, mw, mh);

        BaseWidget::fillColor(R::Meter::backgroundColor);
        BaseWidget::fill();

        // meter tracks
        BaseWidget::beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            BaseWidget::roundedRect(tx, ty, tw2, th, R::Meter::borderRadius * this->fScaleFactor);
        else
            BaseWidget::rect(tx, ty, tw2, th);

        if (d_isNotEqual(fMeterValueL, kParameterMeterL.min))
        {
            const float vy = meterLevelToHeight(fMeterValueL, th);
            const float vh = th - vy;
            const float ly = ty + vy;

            BaseWidget::fillPaint(BaseWidget::linearGradient(0, ly, 0, ly + vh, R::Track::colorGradientStart, R::Track::colorGradientStop));

            BaseWidget::save();
            BaseWidget::scissor(tx, ly, tw1, vh);
            BaseWidget::fill();
            BaseWidget::restore();
        }

        if (d_isNotEqual(fMeterValueR, kParameterMeterR.min))
        {
            const float vy = meterLevelToHeight(fMeterValueR, th);
            const float vh = th - vy;
            const float ly = ty + vy;

            BaseWidget::fillPaint(BaseWidget::linearGradient(0, ly, 0, ly + vh, R::Track::colorGradientStart, R::Track::colorGradientStop));

            BaseWidget::save();
            BaseWidget::scissor(w - sx + margin - mborder - tw1, ly, tw1, vh);
            BaseWidget::fill();
            BaseWidget::restore();
        }

        BaseWidget::beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            BaseWidget::roundedRect(mx, my, mw, mh, R::Meter::borderRadius * BaseWidget::fScaleFactor);
        else
            BaseWidget::rect(mx, my, mw, mh);

        // inset shading
        // FIXME define sizes elsewhere
        BaseWidget::fillPaint(BaseWidget::linearGradient(0, my, 0, my + 4 * BaseWidget::fScaleFactor, R::Meter::insetShadingColor, Reference::Colors::transparent));
        BaseWidget::fill();

        // border
        if constexpr (R::Meter::border != 0)
        {
            BaseWidget::strokeColor(R::backgroundColor);
            BaseWidget::strokeWidth(mborder * 0.5f);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw ticks

        BaseWidget::strokeColor(R::Tick::color);
        BaseWidget::strokeWidth(R::Tick::height * this->fScaleFactor);

        for (float tick : kTicks)
        {
            const float lh = meterLevelToHeight(tick, th);

            BaseWidget::beginPath();
            BaseWidget::moveTo(tx, ty + lh);
            BaseWidget::lineTo(tx + tw2, ty + lh);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // slider

        if constexpr (std::is_base_of_v<KnobEventHandler, BaseWidget>)
        {
            const float sy = border + margin + meterLevelToHeight(BaseWidget::getValue(), th);

            BaseWidget::beginPath();
            BaseWidget::moveTo(sx, sy - (sw - padding) * 0.5f);
            BaseWidget::lineTo(sx + sw - padding, sy);
            BaseWidget::lineTo(sx, sy + (sw - padding) * 0.5f);
            BaseWidget::closePath();
            BaseWidget::fillColor(R::Slider::color);
            BaseWidget::fill();

            BaseWidget::beginPath();
            BaseWidget::moveTo(sx + sw + padding, sy);
            BaseWidget::lineTo(w - sx, sy);
            BaseWidget::strokeColor(R::Slider::color);
            BaseWidget::strokeWidth(1 * this->fScaleFactor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw border

        BaseWidget::template drawReferenceBorder<R>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kLeftMeterIndex,
         FaustParameterIndex kRightMeterIndex,
         class R = Reference::Widgets::ThresholdMeter<>>
class MeterWidget final : public ThresholdMeterBaseWidget<R, LabWidget, kLeftMeterIndex, kRightMeterIndex>
{
    using BaseWidget = ThresholdMeterBaseWidget<R, LabWidget, kLeftMeterIndex, kRightMeterIndex>;

public:
    MeterWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        BaseWidget::setId(kParametersMainStart + kLeftMeterIndex);
        BaseWidget::setName(BaseWidget::kParameterMeterL.name);
        BaseWidget::updateSize(false);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex,
         FaustParameterIndex kLeftMeterIndex,
         FaustParameterIndex kRightMeterIndex,
         class R = Reference::Widgets::ThresholdMeter<>>
class ThresholdMeterWidget final : public ThresholdMeterBaseWidget<R, LabParameterWidget, kLeftMeterIndex, kRightMeterIndex>
{
    using BaseWidget = ThresholdMeterBaseWidget<R, LabParameterWidget, kLeftMeterIndex, kRightMeterIndex>;

    static constexpr const LibreAudioParameter& kParameterSlider = kFaustParameters[kParameterIndex];

    static_assert(kParameterSlider.max <= BaseWidget::kParameterMeterL.max, "gain vs meter max mismatch");
    static_assert(kParameterSlider.min >= BaseWidget::kParameterMeterL.min, "gain vs meter min mismatch");

public:
    ThresholdMeterWidget(LabWidget* const parent)
        : BaseWidget(parent, kParametersMainStart + kParameterIndex)
    {
        BaseWidget::setActiveCursor(R::cursor);
        BaseWidget::setName(kParameterSlider.name);
        BaseWidget::setDefault(kParameterSlider.init);
        BaseWidget::setRange(kParameterSlider.min, kParameterSlider.max);
        BaseWidget::setStep(kParameterSlider.step);
        BaseWidget::setUsingLogScale(kParameterSlider.isLogarithmic);
        BaseWidget::setValue(kParameterSlider.init, false);
        BaseWidget::updateSize(false);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
