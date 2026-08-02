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
    void onNanoDisplay() override
    {
    }

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

class LibreAudioPluginName : public LibreAudioWidget
{
public:
    LibreAudioPluginName(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        std::memcpy(fName, DISTRHO_PLUGIN_NAME + 2, sizeof(DISTRHO_PLUGIN_NAME) - 2);
        fName[sizeof(DISTRHO_PLUGIN_NAME) - 3] = '\0';

        for (uint i = 0; i < sizeof(DISTRHO_PLUGIN_NAME) - 2; ++i)
            fName[i] = std::toupper(fName[i]);

        Rectangle<float> bounds;
        textAlign(0);
        textLetterSpacing(Metrics::TopBar::PluginName::letterSpacing * fScaleFactor);
        textBounds(0, 0, fName, nullptr, bounds);
        setWidth(bounds.getWidth());
    }

protected:
    void onNanoDisplay() final
    {
        fillColor(Colors::acc);
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textLetterSpacing(Metrics::TopBar::PluginName::letterSpacing * fScaleFactor);
        text(getWidth() * 0.5f, getHeight() * 0.5f, fName);
    }

private:
    char fName[sizeof(DISTRHO_PLUGIN_NAME) - 2];
};

// --------------------------------------------------------------------------------------------------------------------
