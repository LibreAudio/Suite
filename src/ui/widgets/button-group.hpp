// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "button.hpp"
#include "Layout.hpp"

#include <memory>
#include <type_traits>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioButtonGroupWidget : public LibreAudioContainer<LibreAudioReference::Widgets::ButtonGroup>
{
    using R = LibreAudioReference::Widgets::ButtonGroup;
    // using LibreAudioButtonWidget::Corner kCornerLeft = LibreAudioButtonWidget::kCornerLeft;
    // using LibreAudioButtonWidget::Corner kCornerRight = LibreAudioButtonWidget::kCornerRight;

public:
    explicit LibreAudioButtonGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioContainer(parent)
    {
    }

    void done(ButtonEventHandler::Callback* const callback)
    {
        const uint border = R::border * fScaleFactor;
        const uint margin = R::margin * fScaleFactor;
        const uint padding = R::padding * fScaleFactor;

        uint width = (border + margin) * 2;
        if (const uint numWidgets = widgets.size())
        {
            width += padding * (numWidgets - 1);

            for (const SubWidgetWithSizeHint& widgetWithSizeHint : widgets)
            {
                width += widgetWithSizeHint.widget->getWidth();
                static_cast<LibreAudioButtonWidget*>(widgetWithSizeHint.widget)->setCallback(callback);
            }

            if (numWidgets == 1)
            {
                static_cast<LibreAudioButtonWidget*>(widgets.front().widget)->setCorner(LibreAudioButtonWidget::kCornerLeft | LibreAudioButtonWidget::kCornerRight);
            }
            else
            {
                static_cast<LibreAudioButtonWidget*>(widgets.front().widget)->setCorner(LibreAudioButtonWidget::kCornerLeft);
                static_cast<LibreAudioButtonWidget*>(widgets.back().widget)->setCorner(LibreAudioButtonWidget::kCornerRight);
            }
        }

        LibreAudioWidget::setWidth(width);
    }

protected:
    template<class B,
             SizeHint sizeHint = Fixed,
             typename = std::enable_if_t<std::is_base_of_v<LibreAudioButtonWidget, B>>>
    std::unique_ptr<LibreAudioButtonWidget> addButton()
    {
        std::unique_ptr<LibreAudioButtonWidget> widget { new B(this) };
        widget->setCheckable(true);
        widgets.push_back({ widget.get(), Fixed });
        return widget;
    }

private:
    void onNanoDisplay() final
    {
        // TODO divider??
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
