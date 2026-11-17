// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base.hpp"
#include "card-group.hpp"

namespace LibreAudio {

class PresetCardsWidget : public CardGroupWidget
{
public:
    PresetCardsWidget(LabWidget* const parent)
        : CardGroupWidget(parent)
    {
        const uint32_t count = fInterface->getPresetCount();
        if (count == 0)
            return;

        addCard<Reference::Colors::acc2>(kWidgetPresetA,
                                         fInterface->getPresetTitle(0),
                                         fInterface->getPresetSubtitle(0));

        if (count >= 2)
        {
            addCard<Reference::Colors::acc3>(kWidgetPresetB,
                                             fInterface->getPresetTitle(1),
                                             fInterface->getPresetSubtitle(1));

            if (count >= 3)
            {
                addCard<Reference::Colors::acc4>(kWidgetPresetC,
                                                 fInterface->getPresetTitle(2),
                                                 fInterface->getPresetSubtitle(2));

                if (count >= 4)
                    addCard<Reference::Colors::acc6>(kWidgetPresetD,
                                                     fInterface->getPresetTitle(3),
                                                     fInterface->getPresetSubtitle(3));
            }
        }
    }
};

}
