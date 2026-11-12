// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"
#include "../_lab/container.hpp"
#include "../_lab/interface.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class BackgroundPillToggleCellWidget final : public LabReferenceButtonWidget<Reference::Widgets::PillToggle::Cell, kCornerBoth>
{
    using R = Reference::Widgets::PillToggle::Cell;
    using BaseWidget = LabReferenceButtonWidget<Reference::Widgets::PillToggle::Cell, kCornerBoth>;

public:
    explicit BackgroundPillToggleCellWidget(LabWidget* const parent, const uint id, const char* const label)
        : BaseWidget(parent, id)
    {
        BaseWidget::setCheckable(true);
        BaseWidget::setName(label);
    }

protected:
    void onNanoDisplay() final
    {
        if (isCheckable() && isChecked())
        {
            drawReferenceBackground<R, R::backgroundColor〡selected, kCornerBoth>();
            drawReferenceText<R, R::color〡selected>(getName());
        }
        else
        {
            drawReferenceBackground<R, R::backgroundColor, kCornerBoth>();
            drawReferenceText<R, R::color>(getName());
        }

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

// --------------------------------------------------------------------------------------------------------------------

template <SizeHint sizeHint>
class PillToggleWidget : public LabReferenceContainerWidget<Reference::Widgets::PillToggle, Horizontal>,
                         private ButtonEventHandler::Callback,
                         private IdleCallback
{
    using R = Reference::Widgets::PillToggle;
    using BaseWidget = LabReferenceContainerWidget<R, Horizontal>;

    std::list<std::unique_ptr<BackgroundPillToggleCellWidget>> fCells;

public:
    explicit PillToggleWidget(LabWidget* const parent,
                              const FaustParameter& parameter,
                              const uint32_t id)
        : BaseWidget(parent)
    {
        addIdleCallback(this);
        setId(id);
        setName(parameter.label);

        for (uint i = 0; i < parameter.scalePointCount; ++i)
        {
            std::unique_ptr<BackgroundPillToggleCellWidget> widget {
                new BackgroundPillToggleCellWidget(
                    this, parameter.scalePoints[i].value, parameter.scalePoints[i].label)
            };
            widget->setCallback(this);
            widgets.push_back({ widget.get(), Fixed });
            fCells.emplace_back(std::move(widget));
        }

        if (parameter.scalePointCount != 0)
            fCells.front()->setChecked(true, false);

        updateSize(true);
    }

private:
    void addWidget() = delete;

    void buttonClicked(SubWidget* const widget, int) final
    {
        BackgroundPillToggleCellWidget* const button = static_cast<BackgroundPillToggleCellWidget*>(widget);

        if (! button->isChecked())
        {
            button->setChecked(true, false);
            return;
        }

        for (const std::unique_ptr<BackgroundPillToggleCellWidget>& cell : fCells)
            if (cell.get() != button)
                cell->setChecked(false, false);

        const uint id = getId();
        fInterface->parameterControlPressed(id);
        fInterface->parameterControlModified(id, button->getId());
        fInterface->parameterControlReleased(id);
    }

    void idleCallback() final
    {
        const uint value = d_roundToUnsignedInt(fInterface->getParameterValue(getId()));

        for (const std::unique_ptr<BackgroundPillToggleCellWidget>& cell : fCells)
            cell->setChecked(cell->getId() == value, false);
    }

    void updateSize(const bool updateChildren) final
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        // update children size first
        BaseWidget::updateSize(true);

        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        if constexpr (sizeHint == Fixed)
        {
            // make all cells have the same width
            uint cellWidth = 0;

            for (const std::unique_ptr<BackgroundPillToggleCellWidget>& cell : fCells)
                cellWidth = std::max(cellWidth, cell->getWidth());

            for (const std::unique_ptr<BackgroundPillToggleCellWidget>& cell : fCells)
                cell->setWidth(cellWidth);

            // set width and height
            uint width = (border + margin) * 2;
            uint height = width;

            if (const uint numWidgets = widgets.size())
            {
                width += padding * (numWidgets - 1);
                width += numWidgets * cellWidth;
            }

            if constexpr (R::height != 0)
                height += R::height * fScaleFactor;
            else if (! fCells.empty())
                height += d_roundToUnsignedInt(fCells.front()->getHeight());
            else
                height += d_roundToUnsignedInt(fScaleFactor);

            LabWidget::setSize(width, height);
        }
        else
        {
            if (const uint numCells = fCells.size())
            {
                uint cellWidth = (getWidth() - (border + margin) * 2 - padding * (numCells - 1)) / numCells;

                for (const std::unique_ptr<BackgroundPillToggleCellWidget>& cell : fCells)
                    cell->setWidth(cellWidth);
            }

            uint height = (border + margin) * 2;

            if constexpr (R::height != 0)
                height += R::height * fScaleFactor;
            else if (! fCells.empty())
                height += d_roundToUnsignedInt(fCells.front()->getHeight());
            else
                height += d_roundToUnsignedInt(fScaleFactor);

            LabWidget::setHeight(height);
        }

        // update everything else
        BaseWidget::updateSize(false);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
