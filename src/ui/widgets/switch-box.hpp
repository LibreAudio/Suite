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
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex index>
using SwitchBoxReference = Reference::Widgets::SwitchBox<Reference::AccentColor<index>::color>;

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex>
class SwitchBoxWidget final : public LabToggleSwitchBaseWidget
{
    using R = SwitchBoxReference<kParameterIndex>;
    using BaseWidget = LabToggleSwitchBaseWidget;
    using BaseWidget::BaseWidget;

    static constexpr const FaustParameter& kParameter = kFaustParameters[kParameterIndex];
    static_assert(kParameter.isBoolean, "switch-box parameter must be boolean");

    const NanoImage fImage = createImageFromMemory(IMAGES_POWER_28_PNG_DATA,
                                                   IMAGES_POWER_28_PNG_LEN,
                                                   IMAGE_GENERATE_MIPMAPS);

    float fCurrentValue = kParameter.init;
    float fNextValue = fCurrentValue;

    bool fAnimating = false;
    double fAnimationStartTime;

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

            const float normalized = std::clamp<double>(time - fAnimationStartTime, 0.0, 1.0); // NOTE 1s duration

            fCurrentValue = kParameter.min + normalized * (kParameter.max - kParameter.min);
            repaint();

            if (d_isEqual(normalized, 1.f))
                fAnimating = false;
        }
    }

    void onNanoDisplay() final
    {
        const SwitchBoxMetrics<R> m { this };

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

        // ------------------------------------------------------------------------------------------------------------
        // bottom bar

        {
            const float normvalue = (fCurrentValue - kParameter.min) / (kParameter.max - kParameter.min);

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

#if 0
public:
    // name, if given, replaces the parameter's name, such as a shorter one that fits the box
    explicit PowerBoxWidget(LabWidget* const parent, const FaustParameterIndex id, const char* const name = nullptr)
        : BaseWidget(parent, kParametersMainStart + id, name != nullptr ? name : kFaustParameters[id].name)
    {
        setChecked(d_isNotZero(kFaustParameters[id].init), false);
        fBar = isChecked() ? 1.f : 0.f;
        updateSize(false);
    }
#endif

#if 0
private:
    // bar fill as drawn, 0 to 1, moving towards the checked state
    float fBar = 0.f;
    double fLastTime = 0;

    void idleCallback() final
    {
        BaseWidget::idleCallback();

        const double now = getApp().getTime();
        const double dt = fLastTime != 0 ? now - fLastTime : 0;
        fLastTime = now;

        const float target = isChecked() ? 1.f : 0.f;

        if (d_isEqual(fBar, target))
            return;

        const float step = static_cast<float>(dt / R::barTime);
        fBar = target > fBar ? std::min(target, fBar + step) : std::max(target, fBar - step);
        repaint();
    }

    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();

        const float s = fScaleFactor;
        const float border = R::border * s;
        const float border2x = border * 2;
        const float borderRadius = R::borderRadius * s;
        const float margin = R::margin * s;
        const float bh = R::Bar::height * s;
        const float w = getWidth();
        const float h = getHeight();

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
        fontSize(R::Name::fontSize * s);
        textLetterSpacing(R::Name::letterSpacing * s);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + margin, border + margin, getName(), nullptr);

        // the icon, centred under the name
        const float iw = fImage.getWidth() * s / R::Icon::imageScale;
        const float ih = fImage.getHeight() * s / R::Icon::imageScale;
        const float nameBottom = border + margin + R::Name::fontSize * s;
        const float ix = std::round((w - iw) * 0.5f);
        const float iy = std::round(nameBottom + (h - bh - margin - nameBottom - ih) * 0.5f);

        save();
        globalTint(isChecked() ? R::Icon::color〡checked : isHovered() ? R::Icon::color〡hover : R::Icon::color);
        beginPath();
        rect(ix, iy, iw, ih);
        fillPaint(imagePattern(ix, iy, iw, ih, 0.f, fImage, 1.f));
        fill();
        restore();

        // bottom bar, filled to fBar, with nothing filled when off
        {
            save();

            scissor(0, h - bh, w, bh);
            beginPath();
            roundedRect(0, 0, w, h, borderRadius);
            fillColor(R::Bar::color〡deactivated);
            fill();

            if (fBar > 0.f)
            {
                scissor(0, h - bh, fBar * w, bh);
                beginPath();
                roundedRect(0, 0, w, h, borderRadius);
                fillColor(R::Bar::color);
                fill();
            }

            restore();
        }
    }

    void updateSize(const bool updateChildren) final
    {
        setHeight(d_roundToUnsignedInt(kHeight * fScaleFactor));
        BaseWidget::updateSize(updateChildren);
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
