// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

// kHeight, in unscaled pixels, may make the box taller than the reference, leaving its contents their size
template<const Color& kAccentColor = Reference::Colors::acc1, uint kHeight = Reference::Widgets::NumberBox<>::height>
class NumberBoxWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::NumberBox<kAccentColor>;
    using BaseWidget = LabKnobWidget;

public:
    explicit NumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : NumberBoxWidget(parent, kParametersMainStart + id, &kFaustParameters[id]) {}

    // For a host parameter outside the main dsp, such as the common dry/wet. The description is passed by pointer, as
    // addWidget copies its arguments, and must outlive the widget.
    explicit NumberBoxWidget(LabWidget* const parent, const uint32_t parameterId, const FaustParameter* const parameter)
        : BaseWidget(parent, parameterId),
          fParameter(*parameter)
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

        // whole numbers for a step of 1, as the knob shows them
        char valuestr[32];
        if (isInteger())
            std::snprintf(valuestr, sizeof(valuestr), "%d", d_roundToInt(getValue()));
        else
            std::snprintf(valuestr, sizeof(valuestr), "%.1f", getValue());

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
        setHeight(d_roundToUnsignedInt(kHeight * fScaleFactor));
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
