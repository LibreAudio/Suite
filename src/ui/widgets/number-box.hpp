// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"

#include "../reference.hpp"
#include "parameter-value.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1>
class NumberBoxWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::NumberBox<kAccentColor>;
    using BaseWidget = LabKnobWidget;

public:
    explicit NumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : BaseWidget(parent, kParametersMainStart + id),
          fParameter(kFaustParameters[id]),
          fDecimals(ParameterValue::decimalsForStep(fParameter.step))
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
    const int fDecimals;

    bool handleScroll(const Widget::ScrollEvent& ev) final
    {
        if (!(fParameter.step > 0))
            return BaseWidget::handleScroll(ev);
        if (!isEnabled() || !contains(ev.pos) || d_isZero(ev.delta.getY()))
            return false;
        const float next = ParameterValue::scrollStep(getValue(), fParameter.min, fParameter.max,
                                                     fParameter.step, ev.delta.getY() > 0 ? 1 : -1);
        if (d_isNotEqual(next, getValue()))
        {
            fInterface->parameterControlPressed(getId());
            setValue(next, true);
            fInterface->parameterControlReleased(getId());
        }
        return true;
    }

    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();
        // drawReferenceBorder<R>();

        const float border = R::border * fScaleFactor;
        const float border2x = border * 2;
        const float borderRadius = R::borderRadius * fScaleFactor;
        const float margin = R::margin * fScaleFactor;
        const float bh = R::Bar::height * fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        char valuestr[32];
        // Avoid displaying a negative zero caused by floating-point rounding.
        const float displayValue = std::abs(getValue()) < 0.5 * std::pow(10.0, -fDecimals) ? 0.f : getValue();
        std::snprintf(valuestr, sizeof(valuestr), "%.*f", fDecimals, displayValue);

        // inset shading
        beginPath();
        roundedRect(0, 0, w, h, borderRadius);
        fillPaint(linearGradient(0, 0, 0, margin, R::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::backgroundColor);
        strokeWidth(border * 0.5f);
        stroke();

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(R::Name::fontSize * fScaleFactor);
        textLetterSpacing(R::Name::letterSpacing * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + margin, border + margin, fParameter.name, nullptr);

        fillColor(R::Value::color);
        fontFace("mono");
        fontSize(R::Value::fontSize * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        textLetterSpacing(R::Value::letterSpacing * fScaleFactor);

        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        text(border2x + margin, h - bh - margin, valuestr, nullptr);

        if (*fParameter.unit != '\0')
        {
            fillColor(R::Unit::color);
            fontFace("regular");
            fontSize(R::Unit::fontSize * fScaleFactor);
            textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            textLetterSpacing(R::Unit::letterSpacing * fScaleFactor);
            text(w - border2x - margin, h - bh - margin, fParameter.unit, nullptr);
        }

        // bottom bar
        {
            save();

            scissor(0, h - bh, w, bh);
            beginPath();
            roundedRect(0, 0, w, h, borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            scissor(0, h - bh, border2x + getNormalizedValue() * (w - border2x), bh);
            beginPath();
            roundedRect(0, 0, w, h, borderRadius);
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
