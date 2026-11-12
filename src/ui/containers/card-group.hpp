// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button-group.hpp"

#include "../widgets/card.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class CardGroupWidget : public LabReferenceButtonGroupWidget<Reference::Widgets::CardGroup>
{
    using R = Reference::Widgets::CardGroup;
    using BaseWidget = LabReferenceButtonGroupWidget<R>;

public:
    explicit CardGroupWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
    }

    std::shared_ptr<CardWidget> addCard(const uint id, const char* const title, const char* const subtitle)
    {
        return BaseWidget::addButton<CardWidget, Expanding>(id, title, subtitle);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        // update children size first
        BaseWidget::updateSize(true);

        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        uint cardsHeight;

        if constexpr (R::height != 0)
            cardsHeight = d_roundToUnsignedInt(R::height * fScaleFactor);
        else if (! fWidgets.empty())
            cardsHeight = fWidgets.front()->getHeight();
        else
            cardsHeight = d_roundToUnsignedInt(fScaleFactor);

        LabWidget::setHeight((border + margin) * 2 + cardsHeight);

        // update everything else
        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
