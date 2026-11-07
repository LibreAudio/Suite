// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"

#include "EventHandlers.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class ButtonBaseWidget : public LabWidget,
                         public ButtonEventHandler
{
    using BaseWidget = LabWidget;

public:
    explicit ButtonBaseWidget(LabWidget* const parent)
        : BaseWidget(parent),
          ButtonEventHandler(this) {}

private:
    bool onMouse(const MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return BaseWidget::onMouse(ev);
    }

    bool onMotion(const MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return BaseWidget::onMotion(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// reference button widget class

template<class R, Corner corner>
class ReferenceButtonWidget : public ButtonBaseWidget
{
    using BaseWidget = ButtonBaseWidget;

public:
    explicit ReferenceButtonWidget(LabWidget* const parent, const uint id)
        : BaseWidget(parent)
    {
        setId(id);
    }

protected:
    [[nodiscard]] const Color& getBackgroundColor() const noexcept
    {
        if (! isEnabled())
        {
            if constexpr (d_isNotZero(R::backgroundColor〡disabled.alpha))
                return R::backgroundColor〡disabled;
        }

        if (isCheckable() && isChecked())
        {
            if constexpr (d_isNotZero(R::backgroundColor〡checked.alpha))
                return R::backgroundColor〡checked;
        }

        if (isHovered())
        {
            if constexpr (d_isNotZero(R::backgroundColor〡hover.alpha))
                return R::backgroundColor〡hover;
        }

        return R::backgroundColor;
    }

    [[nodiscard]] const Color& getForegroundColor() const noexcept
    {
        if (! isEnabled())
        {
            if constexpr (d_isNotZero(R::color〡disabled.alpha))
                return R::color〡disabled;
        }

        if (isCheckable() && isChecked())
        {
            if constexpr (d_isNotZero(R::color〡checked.alpha))
                return R::color〡checked;
        }

        if (isHovered())
        {
            if constexpr (d_isNotZero(R::color〡hover.alpha))
                return R::color〡hover;
        }

        return R::color;
    }

    template <class TR>
    void drawReferenceText(const char* const text)
    {
        // NOTE same logic as getForegroundColor, duplicated for being compatible with template args
        if (! isEnabled())
        {
            if constexpr (d_isNotZero(R::color〡disabled.alpha))
                return drawReferenceText<TR, TR::color〡disabled>(text);
        }

        if (isCheckable() && isChecked())
        {
            if constexpr (d_isNotZero(R::color〡checked.alpha))
                return drawReferenceText<TR, TR::color〡checked>(text);
        }

        if (isHovered())
        {
            if constexpr (d_isNotZero(R::color〡hover.alpha))
                return drawReferenceText<TR, TR::color〡hover>(text);
        }

        return drawReferenceText<TR, TR::color>(text);
    }

    template <class TR, const Color& color>
    void drawReferenceText(const char* const text)
    {
        BaseWidget::drawReferenceText<TR, color>(text);
    }

    void onNanoDisplay() override
    {
        // NOTE same logic as getBackgroundColor, duplicated for being compatible with template args
        do
        {
            if (! isEnabled())
            {
                if constexpr (d_isNotZero(R::backgroundColor〡disabled.alpha))
                {
                    drawReferenceBackground<R, R::backgroundColor〡disabled, corner>();
                    break;
                }
            }

            if (isCheckable() && isChecked())
            {
                if constexpr (d_isNotZero(R::backgroundColor〡checked.alpha))
                {
                    drawReferenceBackground<R, R::backgroundColor〡checked, corner>();
                    break;
                }
            }

            if (isHovered())
            {
                if constexpr (d_isNotZero(R::backgroundColor〡hover.alpha))
                {
                    drawReferenceBackground<R, R::backgroundColor〡hover, corner>();
                    break;
                }
            }

            drawReferenceBackground<R, R::backgroundColor, corner>();
        }
        while (false);

        drawReferenceBorder<R, R::borderColor>();
    }

    void stateChanged(const State state, const State oldState) override
    {
        if constexpr (R::cursor != kMouseCursorArrow)
        {
            if ((state & kButtonStateHover) != 0)
            {
                if ((oldState & kButtonStateHover) == 0)
                    setCursor(R::cursor);
            }
            else
            {
                if ((oldState & kButtonStateHover) != 0)
                    resetCursor();
            }
        }

        if ((state & kButtonStateActive) != 0)
        {
            if ((oldState & kButtonStateActive) == 0)
                fInterface->setActiveWidget(this);
        }
        else
        {
            if ((oldState & kButtonStateActive) != 0)
                fInterface->setActiveWidget(nullptr);
        }
    }

    void updateSize(const bool updateChildren) override
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
