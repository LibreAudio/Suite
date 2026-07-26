// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Color.hpp"
#include "EventHandlers.hpp"

static const struct Colors {
    Color background = Color::fromHTML("#28282d");
    Color outline = Color::fromHTML("#3a3a42");

    Color ink = Color::fromHTML("#f2f2f4");
    Color ink2 = Color::fromHTML("#b7b9bf");
    Color ink3 = Color::fromHTML("#74767e");
    Color acc = Color::fromHTML("#c5d9ff");

    Color bg0 = Color::fromHTML("#161618");
    Color bg1 = Color::fromHTML("#1c1c20");
    Color bg2 = Color::fromHTML("#26262b");
    Color bg3 = Color::fromHTML("#303036");
    Color line = Color::fromHTML("#2a2a30");
    Color line2 = Color::fromHTML("#3b3b44");

    const Color& get(const bool isEnabled, const bool isChecked, const bool isHover) const
    {
        if (! isEnabled)
            return ink3;
        if (isChecked)
            return ink;
        if (isHover)
            return acc;
        return ink2;
    }

    const Color& get(const ButtonEventHandler* const button) const
    {
        if (! button->isEnabled())
            return ink3;
        if (button->isChecked())
            return ink;
        if (button->isHovered())
            return acc;
        return ink2;
    }
} gColors;
