// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NanoVG.hpp"

// --------------------------------------------------------------------------------------------------------------------
// base widget class

class LibreAudioWidget : public NanoSubWidget
{
public:
    LibreAudioWidget(NanoTopLevelWidget* const parent)
        : NanoSubWidget(parent),
          fScaleFactor(parent->getScaleFactor()) {}

    LibreAudioWidget(NanoSubWidget* const parent)
        : NanoSubWidget(parent),
          fScaleFactor(parent->getTopLevelWidget()->getScaleFactor()) {}

protected:
    double fScaleFactor;
};

// --------------------------------------------------------------------------------------------------------------------
// base widget classes

template<const uchar* imageData, uint imageDataSize, uint imageScale = 2>
class LibreAudioImageWidget : public LibreAudioWidget
{
public:
    LibreAudioImageWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        updateImageSize();
        setSize(fImageWidth, fImageHeight);
    }

protected:
    void onNanoDisplay() final
    {
        const uint width = getWidth();
        const uint height = getHeight();

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - fImageWidth) * 0.5,
                               (height - fImageHeight) * 0.5,
                               fImageWidth,
                               fImageHeight,
                               0.f,
                               fImage,
                               1.f));
        fill();
    }

private:
    const NanoImage fImage { createImageFromMemory(imageData, imageDataSize, IMAGE_GENERATE_MIPMAPS) };
    double fImageWidth;
    double fImageHeight;

    void updateImageSize()
    {
        fImageWidth = fImage.getWidth() * fScaleFactor / imageScale;
        fImageHeight = fImage.getHeight() * fScaleFactor / imageScale;
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const char string[], bool uppercase = false>
class LibreAudioTextWidget : public LibreAudioWidget
{
public:
    LibreAudioTextWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        if constexpr (uppercase)
            fText.toUpper();
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(gColors.ink2);
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), fText);
    }

private:
    String fText { string };
};

// --------------------------------------------------------------------------------------------------------------------
