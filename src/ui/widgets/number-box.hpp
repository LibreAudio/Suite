// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class NumberBoxWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::NumberBox;
    using BaseWidget = LabKnobWidget;

    const FaustParameter& fParameter;

public:
    explicit NumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : BaseWidget(parent, kParametersMainStart + id),
          fParameter(kFaustParameters[id])
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
    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();
        drawReferenceBorder<R>();

        const float border = R::border * fScaleFactor;
        const float border2x = border * 2;
        const float borderRadius = R::borderRadius * fScaleFactor;
        const float margin = R::margin * fScaleFactor;
        const float bh = R::Bar::height * fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        char valuestr[32];
        std::snprintf(valuestr, sizeof(valuestr), "%.1f", getValue());

        // inset shading
        beginPath();
        roundedRect(0, 0, w, h, borderRadius);
        fillPaint(linearGradient(0, 0, 0, margin, Color(0.f, 0.f, 0.f, 0.55f), Reference::Colors::transparent));
        fill();

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(R::Name::fontSize * fScaleFactor);
        textLetterSpacing(R::Name::letterSpacing * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border + margin, border + margin, fParameter.name, nullptr);

        fillColor(R::Value::color);
        fontFace("mono");
        fontSize(R::Value::fontSize * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        textLetterSpacing(R::Value::letterSpacing * fScaleFactor);

        Rectangle<float> bounds;
        textBounds(border + margin, 0, "-88.8", nullptr, bounds);

        textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        // const float vw =
        text(bounds.getX() + bounds.getWidth(), h - bh - margin, valuestr, nullptr);

        if (*fParameter.unit != '\0')
        {
            fillColor(R::Unit::color);
            fontFace("regular");
            fontSize(R::Unit::fontSize * fScaleFactor);
            textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            textLetterSpacing(R::Unit::letterSpacing * fScaleFactor);
            text(w - border - margin, h - bh - margin, fParameter.unit, nullptr);
        }

        // bottom bar
        {
            save();

            scissor(0, h - border - bh, w, bh);
            beginPath();
            roundedRect(0, 0, w, h, borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            scissor(0, h - border - bh, border2x + getNormalizedValue() * (w - border2x), bh);
            beginPath();
            roundedRect(border, border, w - border2x, h - border2x, borderRadius);
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
