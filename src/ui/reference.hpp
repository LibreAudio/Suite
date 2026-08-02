// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Color.hpp"
// #include "EventHandlers.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct LibreAudioReference {
    struct Colors {
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

        static constexpr const Color transparent { 0.f, 0.f, 0.f, 0.f };
    };

    struct Window {
        static constexpr const Color backgroundGradientStart { 0x2c, 0x2c, 0x34 };
        static constexpr const Color backgroundGradientStop { 0x2a, 0x2a, 0x2f };
        static constexpr const Color borderColor { 0, 0, 0, 0.35f };
        static constexpr const uint border = 1;
        static constexpr const uint padding = 0;
        static constexpr const uint height = 0;
        static constexpr const uint margin = 0;
        static constexpr const uint width = 0;
    };

    struct TopBar {
        static constexpr const Color backgroundColor = Colors::transparent;
        static constexpr const Color borderColor = Colors::transparent;
        static constexpr const uint border = 0;
        static constexpr const uint borderRadius = 0;
        static constexpr const uint height = 40;
        static constexpr const uint padding = 20;
        static constexpr const uint margin = 6;
        static constexpr const uint width = 0;
        struct PluginName {
            static constexpr const Color color = Colors::acc;
            static constexpr const float letterSpacing = 14 * 0.05;
            static constexpr const uint fontSize = 20;
        };
    };

    struct MainArea {
        static constexpr const Color backgroundColor = Colors::transparent;
        static constexpr const Color borderColor = Colors::transparent;
        static constexpr const uint border = 0;
        static constexpr const uint borderRadius = 0;
        static constexpr const uint height = 0;
        static constexpr const uint padding = 12;
        static constexpr const uint margin = 12;
        static constexpr const uint width = 0;
    };

    struct Meter {
        static constexpr const Color backgroundColor = Colors::track;
        static constexpr const Color borderColor { 0, 0, 0, 0.22f };
        static constexpr const uint border = 1;
        static constexpr const uint borderRadius = 7;
        static constexpr const uint height = 0;
        // static constexpr const uint padding = 0;
        static constexpr const uint margin = 3;
        static constexpr const uint width = 26;

        struct Tick {
            static constexpr const Color color { Colors::ink3, 0.15f };
            static constexpr const Color colorMaj { Colors::ink2, 0.26f };
            static constexpr const uint height = 1;
            static constexpr const uint width = 18;
        };

        struct Track {
            static constexpr const Color colorGradientStart = Colors::acc;
            static constexpr const Color colorGradientStop = Colors::accGlow;
            static constexpr const uint width = 9;
        };
    };

    struct Stage {
        static constexpr const Color backgroundColor = Colors::track;
        static constexpr const uint border = 1;
        static constexpr const Color borderColor { 0, 0, 0, 0.22f };
        static constexpr const uint borderRadius = 7;
        static constexpr const uint height = 0;
        static constexpr const uint width = 0;
    };

    struct Widgets {
        struct Button {
            static constexpr const Color backgroundColor = { 0x42, 0x42, 0x5a };
            static constexpr const Color deactivatedColor = Colors::ink3;
            static constexpr const Color foregroundColor = Colors::acc;
            // #4e4e5a
            static constexpr const uint border = 0;
            static constexpr const uint borderRadius = 6;
            static constexpr const uint fontSize = 16;
            static constexpr const uint margin = 6;
        };

        struct ButtonGroup {
            static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.08f };
            static constexpr const Color borderColor = Colors::transparent;
            static constexpr const uint border = 0;
            static constexpr const uint borderRadius = 6;
            static constexpr const uint height = 0;
            static constexpr const uint padding = 1;
            static constexpr const uint margin = 0;
            static constexpr const uint width = 0;
        };
    };
};

static_assert(LibreAudioReference::Meter::width == (
        LibreAudioReference::Meter::border * 2 +
        LibreAudioReference::Meter::margin * 2 +
        // LibreAudioReference::Meter::padding +
        LibreAudioReference::Meter::Track::width * 2
    ), "incorrect meter size");

static_assert(LibreAudioReference::Meter::Tick::width == (
        LibreAudioReference::Meter::width - (LibreAudioReference::Meter::border + LibreAudioReference::Meter::margin) * 2
    ), "incorrect meter tick size");

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
