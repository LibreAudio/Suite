// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base.hpp"

#include "EventHandlers.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioButtonWidget : public LibreAudioWidget,
                               public ButtonEventHandler
{
public:
    explicit LibreAudioButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this)
    {
    }

private:
    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

enum Corner : uint8_t {
    kCornerNone = 0,
    kCornerLeft = 0x1,
    kCornerRight = 0x2,
    kCornerBoth = 0x1 | 0x2,
};

template <class R, uint8_t corner>
class LibreAudioReferenceButtonWidget : public LibreAudioButtonWidget
{
public:
    explicit LibreAudioReferenceButtonWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonWidget(parent)
    {
    }

protected:
    [[nodiscard]] virtual const Color& getBackgroundColor() const noexcept
    {
        if (isCheckable())
            return isChecked() ? R::color : R::backgroundColor;

        return R::backgroundColor;
    }

    [[nodiscard]] virtual const Color& getForegroundColor() const noexcept
    {
        if (! isEnabled())
            return R::color〡deactivated;

        if (isCheckable())
            return isChecked() ? R::backgroundColor : R::color;

        return R::color;
    }

    void onNanoDisplay() override
    {
        const float w = getWidth();
        const float h = getHeight();

        const Color& bgcolor = getBackgroundColor();
        DISTRHO_SAFE_ASSERT_RETURN(d_isEqual(bgcolor.alpha, 1.f),);

        fillColor(bgcolor);

        if constexpr (corner != 0)
        {
            beginPath();
            roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
            fill();

            if constexpr ((corner & kCornerLeft) == 0)
            {
                beginPath();
                rect(0, 0, w * 0.5f, h);
                fill();
            }
            if constexpr ((corner & kCornerRight) == 0)
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

END_NAMESPACE_DISTRHO
