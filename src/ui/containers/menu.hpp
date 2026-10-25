// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"
#include "../_lab/container.hpp"

#include "LibreAudioParameters.hpp"

#include <string>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class MenuWidget final : public LabWidget
{
    class ItemWidget;
    class ItemContainerWidget;

    using BaseWidget = LabWidget;

    class ItemContainerWidget final : public ReferenceContainerWidget<Reference::Widgets::Menu, Vertical>,
                                      private ButtonEventHandler::Callback,
                                      private IdleCallback
    {
        using R = Reference::Widgets::Menu;
        using BaseWidget = ReferenceContainerWidget<R, Vertical>;
        using Layout = typename BaseWidget::Layout;

        static_assert(R::height == 0, "menu items must have zero height");
        static_assert(R::width == 0, "menu items must have zero width");

    public:
        explicit ItemContainerWidget(MenuWidget* const parent,
                                     const uint id,
                                     const std::vector<const char*>& items)
            : BaseWidget(parent),
              fIsParameter(id < fInterface->getParameterCount()),
              fLabels(items)
        {
            addIdleCallback(this);
            setId(id);

            fSelectedIndex = fIsParameter ? d_roundToUnsignedInt(fInterface->getParameterValue(id)) : 0;

            for (uint i = 0, count =  items.size(); i < count; ++i)
            {
                std::shared_ptr<ItemWidget> widget { new ItemWidget(this, i, fLabels[i]) };
                widget->setCallback(this);
                widget->setChecked(fSelectedIndex == i, false);
                Layout::widgets.push_back({ widget.get(), Fixed });
                fItems.emplace_back(std::move(widget));
            }

            updateSize(true);

            if (fIsParameter)
                fInterface->parameterControlPressed(id);
        }

        ~ItemContainerWidget() final
        {
            if (fIsParameter)
                fInterface->parameterControlReleased(getId());
        }

        bool shouldClose() const noexcept
        {
            return fShouldClose;
        }

        void updateSize(const bool updateChildren) final
        {
            BaseWidget::updateSize(updateChildren);

            uint itemwidth = 0;

            if (updateChildren)
            {
                for (std::shared_ptr<ItemWidget> item : fItems)
                {
                    item->updateSize(true);
                    itemwidth = std::max(itemwidth, item->getWidth());
                }

                for (std::shared_ptr<ItemWidget> item : fItems)
                    item->setWidth(itemwidth);
            }
            else if (! fItems.empty())
            {
                itemwidth = fItems.front()->getWidth();
            }

            const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
            const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
            const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);
            const uint width = itemwidth + (border + margin) * 2;

            uint height = (border + margin) * 2;

            if (const uint numItems = fItems.size())
                height += padding * (numItems - 1) + fItems.front()->getHeight() * numItems;

            LabWidget::setSize(width, height);
        }

    private:
        const bool fIsParameter;
        bool fShouldClose = false;
        const std::vector<const char*> fLabels;

        uint fSelectedIndex = 0;
        std::list<std::shared_ptr<ItemWidget>> fItems;

        void buttonClicked(SubWidget* const widget, int) final
        {
            if (fIsParameter)
            {
                fInterface->parameterControlModified(getId(), widget->getId());
                fShouldClose = true;
            }
        }

        void idleCallback() final
        {
            const uint id = getId();

            if (fIsParameter)
            {
                if (const uint index = d_roundToUnsignedInt(fInterface->getParameterValue(id)); fSelectedIndex != index)
                {
                    fSelectedIndex = index;

                    for (std::shared_ptr<ItemWidget> item : fItems)
                        item->setChecked(index == item->getId(), false);
                }
            }
        }
    };

    class ItemWidget final : public ReferenceButtonWidget<Reference::Widgets::Menu::Item, kCornerAuto>
    {
        using R = Reference::Widgets::Menu::Item;
        using BaseWidget = ReferenceButtonWidget<R, kCornerAuto>;

        static_assert(R::height != 0, "menu items must have non-zero height");
        static_assert(R::width == 0, "menu items must have zero width");

        friend class ItemContainerWidget;

    public:
        explicit ItemWidget(ItemContainerWidget* const parent, const uint id, const char* const text)
            : BaseWidget(parent, id)
        {
            setCheckable(true);
            setName(text);
            updateSize(false);
        }

    private:
        void onNanoDisplay() final
        {
            if (isChecked())
                drawReferenceBackground<R, R::backgroundColor〡checked, kCornerAuto>();
            else if (isHovered())
                drawReferenceBackground<R, R::backgroundColor〡hover, kCornerAuto>();
            else
                drawReferenceBackground<R, R::backgroundColor, kCornerAuto>();

            if (isChecked())
                drawReferenceText<R, R::color〡checked>(getName());
            else
                drawReferenceText<R, R::color>(getName());

            drawReferenceBorder<R, R::borderColor>();
        }

        void updateSize(const bool updateChildren) final
        {
            const uint margin = d_roundToUnsignedInt(R::margin * this->fScaleFactor) * 2;

            Rectangle<float> bounds;
            BaseWidget::fontSize(R::fontSize * this->fScaleFactor);
            BaseWidget::textAlign(0);
            BaseWidget::textLetterSpacing(R::letterSpacing * this->fScaleFactor);
            BaseWidget::textBounds(0, 0, getName(), nullptr, bounds);
            BaseWidget::setWidth(bounds.getWidth() + margin);

            BaseWidget::updateSize(updateChildren);
        }
    };

public:
    explicit MenuWidget(LabTopLevelWidget* const parent,
                        const Point<int>& pos,
                        const uint id,
                        const std::vector<const char*>& items)
        : BaseWidget(parent)
    {
        setId(id);
        setSize(parent->getSize());

        fContainer.reset(new ItemContainerWidget(this, id, items));

        static_cast<SubWidget*>(fContainer.get())->setAbsolutePos(pos.getX(),
                                                                  pos.getY() - fContainer->getHeight());

        fContainer->updateSize(true);
    }

    bool shouldClose() const noexcept
    {
        return fShouldClose || fContainer->shouldClose();
    }

private:
    std::shared_ptr<ItemContainerWidget> fContainer;
    bool fShouldClose = false;

    void onNanoDisplay() final
    {
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (BaseWidget::onMouse(ev))
            return true;
        fShouldClose = true;
        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
