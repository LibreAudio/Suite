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

        for (uint32_t i = 0; i < count && i < 4; ++i)
            addCard(kWidgetPresetA + i, fInterface->getPresetTitle(i), fInterface->getPresetSubtitle(i));
    }
};

}
