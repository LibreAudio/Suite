// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/bot/knob.hpp"

#include "LibreAudioUtils.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template <class R>
class DrawableKnobWidget final : public DrawableKnobWidget_Bot<R>
{
    using BaseWidget = DrawableKnobWidget_Bot<R>;
    using State = typename BaseWidget::State;

    static constexpr const double kTimeForShowingHostParameterChanges = 1;
    static constexpr const double kTimeForValueFadeout = 0.1;

public:
    DrawableKnobWidget(LabWidget* const parent, const FaustParameter& parameter, const uint32_t id)
        : BaseWidget(parent, parameter, id),
          fParameter(parameter)
    {
        BaseWidget::setActiveCursor(R::cursor);
        BaseWidget::setName(parameter.label);
        BaseWidget::setDefault(parameter.init);
        BaseWidget::setRange(parameter.min, parameter.max);
        BaseWidget::setStep(parameter.step);
        BaseWidget::setUsingLogScale(parameter.isLogarithmic);
        BaseWidget::setValue(parameter.init, false);

        updateSize(false);
    }

    const FaustParameter& getParameter() const noexcept
    {
        return fParameter;
    }

private:
    const FaustParameter& fParameter;

    void onNanoDisplay() final
    {
        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();

        char textBuffer[24];

        if (const double timeNow = BaseWidget::getTime();
            (BaseWidget::getState() & BaseWidget::kKnobStateDraggingHover) != 0 ||
            BaseWidget::timeNotEllapsed(fLastParameterChangedByHostTime, kTimeForShowingHostParameterChanges, timeNow) ||
            BaseWidget::timeNotEllapsed(fLastStateChangedTime, kTimeForValueFadeout, timeNow))
        {
            renderParameterValue(textBuffer, fParameter, BaseWidget::getValue());

            BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_BOTTOM);

            BaseWidget::fillColor(BaseWidget::isEnabled() ? R::Value::color : this->fKnobStyle.colorDisabled);
            BaseWidget::fontFace("mono");
            BaseWidget::fontSize(R::Value::fontSize * this->fScaleFactor);

            if (*fParameter.unit == '\0')
            {
                // no unit, draw value in the center
                BaseWidget::text(w * 0.5f, h, textBuffer);
            }
            else
            {
                // has unit, put value on the left
                BaseWidget::textAlign(BaseWidget::ALIGN_RIGHT | BaseWidget::ALIGN_BOTTOM);
                BaseWidget::text(w - fUnitTextWidth - fUnitTextSpacing, h, textBuffer);

                // then unit on the right
                BaseWidget::fillColor(BaseWidget::isEnabled() ? R::Unit::color : this->fKnobStyle.colorDisabled);
                BaseWidget::fontSize(R::Unit::fontSize * this->fScaleFactor);
                BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_BOTTOM);
                BaseWidget::text(w - fUnitTextWidth, h, fParameter.unit);
            }
        }
        else
        {
            BaseWidget::fillColor(BaseWidget::isEnabled() ? R::Name::color : this->fKnobStyle.colorDisabled);
            BaseWidget::fontSize(R::Name::fontSize * this->fScaleFactor);
            BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_BOTTOM);

            std::snprintf(textBuffer,
                          sizeof(textBuffer),
                          "%.8s",
                          *fParameter.label != '\0' ? fParameter.label : fParameter.name);
            textBuffer[sizeof(textBuffer) - 1] = '\0';
            BaseWidget::text(w * 0.5f, h, textBuffer);
        }

        BaseWidget::onNanoDisplay();
    }

    void idleCallback() final
    {
        BaseWidget::idleCallback();

        if (BaseWidget::timeEllapsed(fLastParameterChangedByHostTime, kTimeForShowingHostParameterChanges))
        {
            fLastParameterChangedByHostTime = 0.0;
            BaseWidget::repaint();
        }

        if (BaseWidget::timeEllapsed(fLastStateChangedTime, kTimeForValueFadeout))
        {
            fLastStateChangedTime = 0.0;
            BaseWidget::repaint();
        }
    }

    void parameterChangedByHost() final
    {
        fLastParameterChangedByHostTime = BaseWidget::getTime();
    }

    void stateChanged(const State state, const State oldState) final
    {
        BaseWidget::stateChanged(state, oldState);

        fLastStateChangedTime = BaseWidget::getTime();
    }

    void updateSize(const bool updateChildren) final
    {
        BaseWidget::template updateReferenceSize<R>();

        if (*fParameter.unit != '\0')
        {
            Rectangle<float> bounds;
            BaseWidget::fontFace("mono");
            BaseWidget::fontSize(R::Unit::fontSize * this->fScaleFactor);
            BaseWidget::textAlign(0);
            // textLetterSpacing(R::Unit::letterSpacing * this->fScaleFactor);
            BaseWidget::textBounds(0, 0, fParameter.unit, nullptr, bounds);

            fUnitTextSpacing = std::abs(bounds.getX() * 2);
            fUnitTextWidth = bounds.getWidth();
        }

        BaseWidget::updateSize(updateChildren);
    }

    double fLastParameterChangedByHostTime = 0.0;
    double fLastStateChangedTime = 0.0;

    float fUnitTextSpacing = 0.f;
    float fUnitTextWidth = 0.f;
};

using EasyKnobWidget = DrawableKnobWidget<Reference::Widgets::EasyKnob>;
using SmallKnobWidget = DrawableKnobWidget<Reference::Widgets::Knob>;
using SmallestKnobWidget = DrawableKnobWidget<Reference::Widgets::SmallKnob>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
