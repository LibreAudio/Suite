// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "button.hpp"
#include "container.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class R>
class LabReferenceButtonGroupWidget : public LabReferenceContainerWidget<R, Horizontal>,
                                      protected IdleCallback,
                                      private ButtonEventHandler::Callback
{
    using BaseWidget = LabReferenceContainerWidget<R, Horizontal>;

public:
    explicit LabReferenceButtonGroupWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        BaseWidget::addIdleCallback(this);
    }

protected:
    template<class B,
             SizeHint sizeHint = Fixed,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabButtonBaseWidget, B>>>
    std::shared_ptr<B> addButton(const uint id, const char* const name, Args... args)
    {
        std::shared_ptr<B> widget { new B(this, id, args...) };
        widget->setCallback(this);
        widget->setCheckable(true);
        widget->setName(name);
        checkButtonState(widget);
        this->widgets.push_back({ widget.get(), sizeHint });
        fWidgets.push_back(widget);
        if (widget->getSize().isNull())
            d_stderr2("Error: addButton called but widget %u: '%s' does not have a known size", id, name);
        return widget;
    }

    template<class W, SizeHint sizeHint = Expanding>
    void addSpacer() = delete;

    template<class W, SizeHint sizeHint = Fixed, typename... Args>
    void addWidget(Args... args) = delete;

protected:
    std::list<std::shared_ptr<LabButtonBaseWidget>> fWidgets;

    void idleCallback() override
    {
        for (const std::shared_ptr<LabButtonBaseWidget>& widget : fWidgets)
            checkButtonState(widget);
    }

    void updateSize(const bool updateChildren) override
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        const LabMetrics<R, uint> m { this };

        // update children size first
        BaseWidget::updateSize(true);

        // set width and height
        uint width = (m.border + m.margin) * 2;

        if (const uint numWidgets = fWidgets.size())
        {
            width += m.padding * (numWidgets - 1);

            for (const std::shared_ptr<LabButtonBaseWidget>& widget : fWidgets)
                width += widget->getWidth();
        }

        LabWidget::setWidth(width);

        // update everything else
        BaseWidget::updateSize(false);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        const uint id = widget->getId();

        if (id >= kWidgetIdsStart)
        {
            this->fInterface->buttonClicked(id);

            // other UI buttons might change state when this one changes, so update everything
            idleCallback();
        }
        else
        {
            const float value = static_cast<LabButtonBaseWidget*>(widget)->isChecked() ? 1.f : 0.f;
            this->fInterface->parameterControlPressed(id);
            this->fInterface->parameterControlModified(id, value);
            this->fInterface->parameterControlReleased(id);
        }
    }

    void checkButtonState(const std::shared_ptr<LabButtonBaseWidget>& widget)
    {
        const uint id = widget->getId();

        if (id >= kWidgetIdsStart)
        {
            widget->setChecked(this->fInterface->isButtonChecked(id), false);
            widget->setEnabled(this->fInterface->isButtonEnabled(id));
        }
        else
        {
            widget->setChecked(d_isNotZero(this->fInterface->getParameterValue(id)), false);
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
