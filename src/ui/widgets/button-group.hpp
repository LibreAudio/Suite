// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../reference.hpp"
#include "../base/button.hpp"
#include "../base/container.hpp"

#include "Layout.hpp"

#include <memory>
#include <type_traits>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioButtonGroupWidget : public LibreAudioContainerWidget<LibreAudioReference::Widgets::ButtonGroup>
{
    using R = LibreAudioReference::Widgets::ButtonGroup;

public:
    explicit LibreAudioButtonGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioContainerWidget(parent)
    {
    }

    void done(ButtonEventHandler::Callback* const callback)
    {
        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

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
                DISTRHO_CUSTOM_SAFE_ASSERT(
                    "Single button must have corner = both",
                    static_cast<LibreAudioButtonWidget*>(widgets.front().widget)->getCorner() == LibreAudioButtonWidget::kCornerBoth);
            }
            else
            {
                DISTRHO_CUSTOM_SAFE_ASSERT(
                    "First button must have corner = left",
                    static_cast<LibreAudioButtonWidget*>(widgets.front().widget)->getCorner() == LibreAudioButtonWidget::kCornerLeft);
                DISTRHO_CUSTOM_SAFE_ASSERT(
                    "First button must have corner = right",
                    static_cast<LibreAudioButtonWidget*>(widgets.back().widget)->getCorner() == LibreAudioButtonWidget::kCornerRight);
            }
        }

        LibreAudioWidget::setWidth(width);
    }

protected:
    template<class B, typename = std::enable_if_t<std::is_base_of_v<LibreAudioButtonWidget, B>>>
    std::unique_ptr<LibreAudioButtonWidget> addButton(const uint id)
    {
        std::unique_ptr<LibreAudioButtonWidget> widget { new B(this) };
        widget->setId(id);
        widgets.push_back({ widget.get(), Fixed });
        if (widget->getSize().isNull())
            d_stderr2("Error: addButton called but widget '%s' does not have a known size", widget->getName());
        return widget;
    }

    void addWidget() = delete;

private:
    void onNanoDisplay() final
    {
        // TODO divider??
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
