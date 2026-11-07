// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "container.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class LabRootWidgetInterface
{
public:
    virtual ~LabRootWidgetInterface() = default;
    virtual void updateScaleFactorAndSize() = 0;

    void setActiveWidget(LabWidget* const widget) noexcept
    {
        fActiveWidget = widget;
    }

    const Widget::MotionEvent& getLastMotion() const noexcept
    {
        return fLastMotion;
    }

protected:
    bool shortcutCharacterInput(const Widget::CharacterInputEvent& ev)
    {
        return fActiveWidget == nullptr ? false : fActiveWidget->onCharacterInput(ev);
    }

    bool shortcutKeyboard(const Widget::KeyboardEvent& ev)
    {
        return fActiveWidget == nullptr ? false : fActiveWidget->onKeyboard(ev);
    }

    bool shortcutMouse(const Widget::MouseEvent& ev)
    {
        if (fActiveWidget == nullptr)
            return false;

        Widget::MouseEvent ev2(ev);
        ev2.pos = {
            ev.absolutePos.getX() - fActiveWidget->getAbsoluteX(),
            ev.absolutePos.getY() - fActiveWidget->getAbsoluteY()
        };
        return fActiveWidget->onMouse(ev2);
    }

    bool shortcutMotion(const Widget::MotionEvent& ev)
    {
        fLastMotion = ev;

        if (fActiveWidget == nullptr)
            return false;

        Widget::MotionEvent ev2(ev);
        ev2.pos = {
            ev.absolutePos.getX() - fActiveWidget->getAbsoluteX(),
            ev.absolutePos.getY() - fActiveWidget->getAbsoluteY()
        };
        return fActiveWidget->onMotion(ev2);
    }

    bool shortcutScroll(const Widget::ScrollEvent& ev)
    {
        if (fActiveWidget == nullptr)
            return false;

        Widget::ScrollEvent ev2(ev);
        ev2.pos = {
            ev.absolutePos.getX() - fActiveWidget->getAbsoluteX(),
            ev.absolutePos.getY() - fActiveWidget->getAbsoluteY()
        };
        return fActiveWidget->onScroll(ev2);
    }

private:
    LabWidget* fActiveWidget = nullptr;
    Widget::MotionEvent fLastMotion;
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, Orientation orientation>
class LabReferenceRootWidget : public LabReferenceContainerTopLevelWidget<R, orientation>,
                               public LabRootWidgetInterface
{
    using BaseWidget = LabReferenceContainerTopLevelWidget<R, orientation>;
    using CharacterInputEvent = typename BaseWidget::CharacterInputEvent;
    using KeyboardEvent = typename BaseWidget::KeyboardEvent;
    using MouseEvent = typename BaseWidget::MouseEvent;
    using MotionEvent = typename BaseWidget::MotionEvent;
    using ResizeEvent = typename BaseWidget::ResizeEvent;
    using ScrollEvent = typename BaseWidget::ScrollEvent;
    using BaseWidget::BaseWidget;

public:
    void resendLastMotion()
    {
        const Widget::MotionEvent ev = getLastMotion();
        onMotion(ev);
    }

    void updateScaleFactorAndSize() final
    {
        BaseWidget::updateScaleFactor(this->fInterface->getInterfaceScaleFactor());
        this->updateSize(true);
    }

protected:
    bool onCharacterInput(const CharacterInputEvent& ev) override
    {
        if (shortcutCharacterInput(ev))
            return true;
        return BaseWidget::onCharacterInput(ev);
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (shortcutKeyboard(ev))
            return true;
        return BaseWidget::onKeyboard(ev);
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (shortcutMouse(ev))
        {
            // release event: resend last motion to allow next hover to be handled
            if (! ev.press)
                resendLastMotion();

            return true;
        }
        return BaseWidget::onMouse(ev);
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (shortcutMotion(ev))
            return true;
        return BaseWidget::onMotion(ev);
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        if (shortcutScroll(ev))
            return true;
        return BaseWidget::onScroll(ev);
    }

private:
    void onResize(const ResizeEvent& ev) final
    {
        BaseWidget::onResize(ev);
        updateScaleFactorAndSize();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
