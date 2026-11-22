// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/toggle-switch.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

// template<const Color& kAccentColor = Reference::Colors::acc1>
// struct PowerBoxReference : Reference::Widgets::NumberBox<kAccentColor> {
//     struct Icon {
//         static constexpr const Color color = Reference::Colors::ink3;
//         static constexpr const Color color〡hover = Reference::Colors::ink2;
//         static constexpr const Color color〡checked = kAccentColor;
//         // the image is drawn at half its pixel size, for high resolution screens
//         static constexpr const int imageScale = 2;
//     };
//     // how long the bar takes to fill or empty, in seconds
//     static constexpr const double barTime = 0.05;
// };

// kHeight, in unscaled pixels, may make the box taller than a number box, leaving its contents their size
// template<const Color& kAccentColor = Reference::Colors::acc1, uint kHeight = Reference::Widgets::NumberBox<>::height>

template<class R>
struct SwitchBoxMetrics : LabMetrics<R> {
    using LabMetrics<R>::LabMetrics;

    const LabMetricsHeight<R, typename R::Bar> bar { *this };
    const LabMetricsFontSize<R, typename R::Name> name { *this };
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex index>
using SwitchBoxReference = Reference::Widgets::SwitchBox<Reference::AccentColor<index>::color>;

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex>
class SwitchBoxWidget : public LabToggleSwitchBaseWidget
{
    using R = SwitchBoxReference<kParameterIndex>;
    using BaseWidget = LabToggleSwitchBaseWidget;
    using BaseWidget::BaseWidget;

    static constexpr const LibreAudioParameter& kParameter = kFaustParameters[kParameterIndex];
    static_assert(kParameter.isBoolean, "switch-box parameter must be boolean");

    const NanoImage fImage = createImageFromMemory(IMAGES_POWER_28_PNG_DATA,
                                                   IMAGES_POWER_28_PNG_LEN,
                                                   IMAGE_GENERATE_MIPMAPS);
    float fImageWidth;
    float fImageHeight;

    float fCurrentValue = kParameter.init;
    float fNextValue = fCurrentValue;

    bool fAnimating = false;
    double fAnimationStartTime;

public:
    explicit SwitchBoxWidget(LabWidget* const parent)
        : BaseWidget(parent, kParametersMainStart + kParameterIndex)
    {
        setActiveCursor(R::cursor);
        updateSize(false);
    }

protected:
    void updateSize(const bool updateChildren) override
    {
        fImageWidth = fImage.getWidth() * fScaleFactor / 2.f;
        fImageHeight = fImage.getHeight() * fScaleFactor / 2.f;

        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }

private:
    void idleCallback() final
    {
        BaseWidget::idleCallback();

        double time = 0;
        if (const float value = isChecked() ? kParameter.max : kParameter.min; d_isNotEqual(fNextValue, value))
        {
            fNextValue = value;
            fAnimationStartTime = time = getTime();
            fAnimating = true;
        }

        if (fAnimating)
        {
            if (d_isZero(time))
                time = getTime();

            // 0.125s duration
            const float normalized = std::clamp<float>(time - fAnimationStartTime, 0.f, 0.125f) * 8.f;

            fCurrentValue = fNextValue * normalized + fCurrentValue * (1.f - normalized);
            repaint();

            if (d_isEqual(normalized, 1.f))
                fAnimating = false;
        }
    }

    void onNanoDisplay() final
    {
        const SwitchBoxMetrics<R> m { this };

        const float normvalue = (fCurrentValue - kParameter.min) / (kParameter.max - kParameter.min);

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
            if ((getState() & kButtonStateActiveHover) != 0)
                drawReferenceBorder<R, R::borderColor〡hovered>();
        }

        // ------------------------------------------------------------------------------------------------------------
        // name

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(m.name.fontSize);
        textLetterSpacing(m.name.letterSpacing);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(m.border * 2 + m.margin, m.border + m.margin, kParameter.name, nullptr);

        // ------------------------------------------------------------------------------------------------------------
        // power image

        {
            const float x = (m.width - fImageWidth) * 0.5f;
            const float y = (m.height - fImageHeight + m.name.fontSize) * 0.5f;

            save();
            globalTint(Color(R::Name::color, R::Bar::color, normvalue));

            beginPath();
            rect(x, y, fImageWidth, fImageHeight);
            fillPaint(imagePattern(x, y, fImageWidth, fImageHeight, 0.f, fImage, 1.f));
            fill();

            restore();
        }

        // ------------------------------------------------------------------------------------------------------------
        // bottom bar

        {
            save();

            scissor(0, m.height - m.bar.height, m.width, m.bar.height);
            beginPath();
            roundedRect(0, 0, m.width, m.height, m.borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            scissor(0, m.height - m.bar.height, normvalue * m.width, m.bar.height);

            beginPath();
            roundedRect(0, 0, m.width, m.height, m.borderRadius);
            fillColor(R::Bar::color);
            fill();

            restore();
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
