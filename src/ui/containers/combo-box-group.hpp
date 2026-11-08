// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../widgets/combo-box.hpp"

#include "../_lab/container.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R, class CB, FaustParameterIndex... Indexes>
class ComboBoxGroupWidget : public LabReferenceContainerWidget<R, Vertical>
{
    using BaseWidget = LabReferenceContainerWidget<R, Vertical>;

    const std::array<std::shared_ptr<CB>, sizeof...(Indexes)> comboBoxes = {
        this->template addWidget<CB, Expanding>(Indexes)...
    };

public:
    ComboBoxGroupWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(true);
    }

protected:
    void updateSize(const bool updateChildren) override
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        // update children size first
        BaseWidget::updateSize(true);

        // find width for label and value fields
        uint wlabel = 0;
        uint wvalue = 0;
        for (const std::shared_ptr<CB>& cb : comboBoxes)
            cb->maxBounds(wlabel, wvalue);

        // set the same width for all fields
        for (const std::shared_ptr<CB>& cb : comboBoxes)
            cb->setBounds(wlabel, wvalue);

        // update children size again
        BaseWidget::updateSize(true);

        // force height if needed
        if constexpr (R::height != 0)
        {
            const LabMetrics<R, uint> m { this };

            const uint itemheight = (
                m.height - (m.border + m.margin) * 2 - m.padding * (comboBoxes.size() - 1)
            ) / comboBoxes.size();

            for (const std::shared_ptr<CB>& cb : comboBoxes)
                cb->setHeight(itemheight);
        }

        // set parent width the same as the children (plus margins)
        const LabMetrics<R> m { this };
        LabWidget::setWidth((m.border + m.margin) * 2 + comboBoxes[0]->getWidth());
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
