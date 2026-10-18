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
// An on / off parameter drawn like a number box: its name at the top left, and under it, centred, a power icon, grey
// when off, lighter grey when hovered and the accent colour when on, over the number box's bottom bar, full when on
// and empty when off. A click anywhere on the box toggles it.

template<const Color& kAccentColor = Reference::Colors::acc1>
struct PowerBoxReference : Reference::Widgets::NumberBox<kAccentColor> {
    struct Icon {
        static constexpr const Color color = Reference::Colors::ink3;
        static constexpr const Color color〡hover = Reference::Colors::ink2;
        static constexpr const Color color〡checked = kAccentColor;
        // the image is drawn at half its pixel size, for high resolution screens
        static constexpr const int imageScale = 2;
    };
};

template<const Color& kAccentColor = Reference::Colors::acc1>
class PowerBoxWidget final : public ToggleSwitchBaseWidget
{
    using R = PowerBoxReference<kAccentColor>;
    using BaseWidget = ToggleSwitchBaseWidget;

public:
    // name, if given, replaces the parameter's name, such as a shorter one that fits the box
    explicit PowerBoxWidget(LabWidget* const parent, const FaustParameterIndex id, const char* const name = nullptr)
        : BaseWidget(parent, kParametersMainStart + id, name != nullptr ? name : kFaustParameters[id].name)
    {
        setChecked(d_isNotZero(kFaustParameters[id].init), false);
        updateSize(false);
    }

private:
    const NanoImage fImage {
        createImageFromMemory(IMAGES_POWER_28_PNG_DATA, IMAGES_POWER_28_PNG_LEN, IMAGE_GENERATE_MIPMAPS)
    };

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

        // bottom bar, full when on and empty when off
        {
            save();

            scissor(0, h - bh, w, bh);
            beginPath();
            roundedRect(0, 0, w, h, borderRadius);
            fillColor(isChecked() ? R::Bar::color : R::Bar::color〡deactivated);
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
