// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/button.hpp"
#include "../reference.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBackgroundButtonWidget : public LibreAudioButtonWidget
{
    using R = LibreAudioReference::Widgets::Button;

public:
    explicit LibreAudioBackgroundButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonWidget(parent)
    {
    }

protected:
    virtual const Color& getBackgroundColor() const noexcept
    {
        if (isCheckable())
            return isChecked() ? R::foregroundColor : R::backgroundColor;

        return R::backgroundColor;
    }

    virtual const Color& getForegroundColor() const noexcept
    {
        if (! isEnabled())
            return R::deactivatedColor;

        if (isCheckable())
            return isChecked() ? R::backgroundColor : R::foregroundColor;

        return R::foregroundColor;
    }

    void onNanoDisplay() override
    {
        const float w = getWidth();
        const float h = getHeight();

        fillColor(getBackgroundColor());

        if (fCorner != 0)
        {
            beginPath();
            roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
            fill();

            if ((fCorner & kCornerLeft) == 0)
            {
                beginPath();
                rect(0, 0, w * 0.5f, h);
                fill();
            }
            if ((fCorner & kCornerRight) == 0)
            {
                beginPath();
                rect(w * 0.5f, 0, w * 0.5f, h);
                fill();
            }
        }
        else
        {
            beginPath();
            rect(0, 0, w, h);
            fill();
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const uchar* imageData, uint imageDataSize, uint imageScale = 2>
class LibreAudioImageButtonWidget : public LibreAudioBackgroundButtonWidget
{
    using R = LibreAudioReference::Widgets::Button;

public:
    explicit LibreAudioImageButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioBackgroundButtonWidget(parent)
    {
        const uint margin = (R::border + R::margin) * fScaleFactor;

        updateImageSize();
        setSize(fImageWidth + margin * 2, fImageHeight + margin * 2);
    }

private:
    const NanoImage fImage { createImageFromMemory(imageData, imageDataSize, IMAGE_GENERATE_MIPMAPS) };
    double fImageWidth;
    double fImageHeight;

    void onNanoDisplay() final
    {
        LibreAudioBackgroundButtonWidget::onNanoDisplay();

        const float w = getWidth();
        const float h = getHeight();

        save();
        globalTint(getForegroundColor());

        beginPath();
        rect((w - fImageWidth) * 0.5,  (h - fImageHeight) * 0.5, fImageWidth, fImageHeight);
        fillPaint(imagePattern((w - fImageWidth) * 0.5,
                               (h - fImageHeight) * 0.5,
                               fImageWidth,
                               fImageHeight,
                               0.f,
                               fImage,
                               1.f));
        fill();

        restore();
    }

    void updateImageSize()
    {
        fImageWidth = fImage.getWidth() * fScaleFactor / imageScale;
        fImageHeight = fImage.getHeight() * fScaleFactor / imageScale;
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const uchar* image1Data, uint imageData1Size, const uchar* image2Data, uint imageData2Size, uint imageScale = 2>
class LibreAudioDualImageButtonWidget : public LibreAudioBackgroundButtonWidget
{
    using R = LibreAudioReference::Widgets::Button;

public:
    explicit LibreAudioDualImageButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioBackgroundButtonWidget(parent)
    {
        const uint margin = (R::border + R::margin) * fScaleFactor;

        setCheckable(true);
        updateImageSize();
        setSize(fImageWidth + margin * 2, fImageHeight + margin * 2);
    }

private:
    const NanoImage fImage1 { createImageFromMemory(image1Data, imageData1Size, IMAGE_GENERATE_MIPMAPS) };
    const NanoImage fImage2 { createImageFromMemory(image2Data, imageData2Size, IMAGE_GENERATE_MIPMAPS) };
    double fImageWidth;
    double fImageHeight;

    void onNanoDisplay() final
    {
        LibreAudioBackgroundButtonWidget::onNanoDisplay();

        const float w = getWidth();
        const float h = getHeight();

        save();
        globalTint(getForegroundColor());

        beginPath();
        rect((w - fImageWidth) * 0.5,  (h - fImageHeight) * 0.5, fImageWidth, fImageHeight);
        fillPaint(imagePattern((w - fImageWidth) * 0.5,
                               (h - fImageHeight) * 0.5,
                               fImageWidth,
                               fImageHeight,
                               0.f,
                               isChecked() ? fImage1 : fImage2,
                               1.f));
        fill();

        restore();
    }

    void updateImageSize()
    {
        fImageWidth = fImage1.getWidth() * fScaleFactor / imageScale;
        fImageHeight = fImage1.getHeight() * fScaleFactor / imageScale;
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const char _text[]>
class LibreAudioTextButtonWidget : public LibreAudioBackgroundButtonWidget
{
    using R = LibreAudioReference::Widgets::Button;

public:
    explicit LibreAudioTextButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioBackgroundButtonWidget(parent)
    {
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor) * 2;

        Rectangle<float> bounds;
        fontSize(R::fontSize * fScaleFactor);
        textAlign(0);
        textBounds(0, 0, _text, nullptr, bounds);
        setWidth(bounds.getWidth() + margin);
    }

private:
    void onNanoDisplay() final
    {
        LibreAudioBackgroundButtonWidget::onNanoDisplay();

        // const float w = getWidth();
        // const float h = getHeight();

        fillColor(getForegroundColor());
        fontSize(R::fontSize * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        text(getWidth() * 0.5f, getHeight() * 0.5f, _text);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBypassButtonWidget final : public LibreAudioImageButtonWidget<IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN>
{
    using R = LibreAudioReference::Widgets::Button;

public:
    explicit LibreAudioBypassButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioImageButtonWidget(parent)
    {
        setCheckable(true);
    }

private:
    const Color& getBackgroundColor() const noexcept final
    {
        return isChecked() ? R::bypassBackgroundColor : R::backgroundColor;
    }

    const Color& getForegroundColor() const noexcept final
    {
        return isChecked() || isHovered() ? R::bypassForegroundColor : R::foregroundColor;
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
