// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"
#include "interface.hpp"

#include "EventHandlers.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class LabKnobWidget : public LabWidget,
                      public KnobEventHandler,
                      protected IdleCallback,
                      private KnobEventHandler::Callback
{
    static constexpr const float kMouseDeceleration = 500.f;
    static constexpr const Modifier kGroupModifier = kModifierAlt; // Option
   #ifdef DISTRHO_OS_MAC
    static constexpr const Modifier kResetModifier = kModifierSuper; // Command
   #else
    static constexpr const Modifier kResetModifier = kModifierControl;
   #endif
    static constexpr const Modifier kSlowModifier = kModifierShift;

public:
    explicit LabKnobWidget(LabWidget* const parent, const uint32_t id)
        : LabWidget(parent),
          KnobEventHandler(this)
    {
        setId(id);
        setCallback(this);
        setMouseDeceleration(kMouseDeceleration);
        setOrientation(Vertical);
        setResetModifier(kResetModifier);
        setSlowModifier(kSlowModifier);

        addIdleCallback(this);
    }

    void setLinkedParameters(const std::vector<uint32_t>& linked)
    {
        DISTRHO_SAFE_ASSERT_RETURN(std::find(linked.cbegin(), linked.cend(), getId()) == linked.end(),);

        fLinkedParameters =  linked;
    }

protected:
    void idleCallback() override
    {
        // NOTE this only triggers updates if the value doesnt match
        if (setValue(fInterface->getParameterValue(getId())))
            parameterChangedByHost();
    }

    virtual void parameterChangedByHost() {}

private:
    bool fDragStarted = false;
    bool fDragShiftMod = false;
    bool fDraggingMulti = false;
    float fLastMotionValue = 0.f;
    std::vector<uint32_t> fLinkedParameters;

    void knobDragStarted(SubWidget* const widget) final
    {
        fDragStarted = true;
        fLastMotionValue = getValue();
        fInterface->parameterControlPressed(widget->getId());
    }

    void knobDragFinished(SubWidget* const widget) final
    {
        fDragStarted = false;

        if (fDraggingMulti)
        {
            fDraggingMulti = false;

            for (uint32_t id : fLinkedParameters)
                fInterface->parameterControlReleased(id);
        }

        fInterface->parameterControlReleased(widget->getId());
    }

    void knobValueChanged(SubWidget* const widget, const float value) final
    {
        if (fDragShiftMod)
        {
            if (! fDraggingMulti)
            {
                fDraggingMulti = true;

                for (uint32_t id : fLinkedParameters)
                    fInterface->parameterControlPressed(id);
            }

            if (! fLinkedParameters.empty())
            {
                const float diff = value - fLastMotionValue;
                const float min = getMinimum();
                const float max = getMaximum();

                for (uint32_t id : fLinkedParameters)
                {
                    const float old = fInterface->getParameterValue(id);

                    if (const float nvalue = std::clamp(old + diff, min, max); d_isNotEqual(nvalue, old))
                        fInterface->parameterControlModified(id, nvalue);
                }
            }
        }

        fLastMotionValue = value;
        fInterface->parameterControlModified(widget->getId(), value);
    }

    void knobDoubleClicked(SubWidget* const widget) final
    {
        fInterface->parameterControlPressed(widget->getId());
        setValue(getDefault(), true);
        fInterface->parameterControlReleased(widget->getId());
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev, fScaleFactor))
            return true;
        return LabWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        fDragShiftMod = fDragStarted && (ev.mod & kGroupModifier) != 0;

        if (motionEvent(ev, fScaleFactor))
            return true;

        return LabWidget::onMotion(ev);
    }

    bool onScroll(const Widget::ScrollEvent& ev) final
    {
        if (contains(ev.pos))
        {
            fDragShiftMod = (ev.mod & kGroupModifier) != 0;
            knobDragStarted(this);
            scrollEvent(ev);
            knobDragFinished(this);
            fDragShiftMod = false;
            return true;
        }

        return LabWidget::onScroll(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
