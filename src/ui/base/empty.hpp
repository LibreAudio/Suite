// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// empty widget, useful for making space and alignment of other widgets

struct LibreAudioEmptyReference {
    static constexpr const Color backgroundColor { 0.f, 0.f, 0.f, 0.f };
    static constexpr const Color borderColor { 0.f, 0.f, 0.f, 0.f };
    static constexpr const uint border = 0;
    static constexpr const uint borderRadius = 0;
    static constexpr const uint height = 0;
    static constexpr const uint width = 0;
};

template<class R = LibreAudioEmptyReference>
class LibreAudioEmptyWidget final : public LibreAudioWidget
{
public:
    explicit LibreAudioEmptyWidget(LibreAudioWidget* const parent)
        : LibreAudioWidget(parent)
    {
        _initSize();
    }

    explicit LibreAudioEmptyWidget(LibreAudioTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        _initSize();
    }

private:
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

        beginPath();

        if constexpr (R::borderRadius != 0)
            roundedRect(0, 0, w, h, R::borderRadius * this->fScaleFactor);
        else
            rect(0, 0, w, h);

        if constexpr (d_isNotZero(R::backgroundColor.alpha))
        {
            fillColor(R::backgroundColor);
            fill();
        }

        if constexpr (R::border != 0 && d_isNotZero(R::borderColor.alpha))
        {
            strokeColor(R::borderColor);
            strokeWidth(R::border * 2 * this->fScaleFactor);
            stroke();
        }
    }

    void _initSize()
    {
        if constexpr (R::width != 0)
            setWidth(d_roundToUnsignedInt(R::width * this->fScaleFactor));

        if constexpr (R::height != 0)
            setHeight(d_roundToUnsignedInt(R::height * this->fScaleFactor));
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
