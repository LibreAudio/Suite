// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Color.hpp"
#include "EventHandlers.hpp"

struct Colors {
    static constexpr const Color backgroundGradientStart { 0x2c, 0x2c, 0x34 };
    static constexpr const Color backgroundGradientStop { 0x2a, 0x2a, 0x2f };
    static constexpr const Color border { 0, 0, 0, 0.35f };

    static constexpr const Color ink { 0xf6, 0xf6, 0xf8 };
    static constexpr const Color ink2 { 0xc8, 0xca, 0xd0 };
    static constexpr const Color ink3 { 0x8c, 0x8e, 0x96 };
    static constexpr const Color acc { 0xc3, 0xd9, 0xff };
    static constexpr const Color accGlow { 0xc3, 0xd9, 0xff, 0.45f };
    static constexpr const Color orange { 0xff, 0x7e, 0x1c };
    static constexpr const Color bg0 { 0x16, 0x16, 0x18 };
    static constexpr const Color bg1 { 0x1c, 0x1c, 0x20 };
    static constexpr const Color bg2 { 0x26, 0x26, 0x2b };
    static constexpr const Color track { 0x13, 0x13, 0x16 };
    static constexpr const Color line { 0x2a, 0x2a, 0x30 };
    static constexpr const Color line2 { 0x3b, 0x3b, 0x44 };

    // const Color& get(const bool isEnabled, const bool isChecked, const bool isHover) const
    // {
    //     if (! isEnabled)
    //         return ink3;
    //     if (isChecked)
    //         return ink;
    //     if (isHover)
    //         return acc;
    //     return ink2;
    // }

    // const Color& get(const ButtonEventHandler* const button) const
    // {
    //     if (! button->isEnabled())
    //         return ink3;
    //     if (button->isChecked())
    //         return ink;
    //     if (button->isHovered())
    //         return acc;
    //     return ink2;
    // }
};
