// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Color.hpp"
#include "Geometry.hpp"
// #include "EventHandlers.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

struct Reference {
    struct AlphaIntensivity {
        static constexpr const float main = 0.7f;
        static constexpr const float mid = 0.45f;
        static constexpr const float minor = 0.3f;
        static constexpr const float other = 0.22f;

        static constexpr const float glow = mid;
    };

    struct Common {
        static constexpr const float fontSize = 17;
        static constexpr const float letterSpacing = fontSize * 0.01;
        // static constexpr const uint margin = 12;

        static constexpr const uint margin = 8;
        static constexpr const uint padding = 6;

        static constexpr const uint rowHeight = 40;
    };

    struct Colors {
        static constexpr const Color blue { 0xc3, 0xd9, 0xff }; // blue
        static constexpr const Color red { 0xff, 0xbf, 0xcb }; // red
        static constexpr const Color yellow { 0xff, 0xdf, 0xad }; // yellow
        static constexpr const Color green { 0xd2, 0xfd, 0xd3 }; // green
        static constexpr const Color babyblue { 0xbe, 0xf1, 0xff }; // baby blue
        static constexpr const Color pink { 0xda, 0xc1, 0xf3 }; // pink
        static constexpr const Color orange { 0xff, 0x7e, 0x1c };

        static constexpr const Color ink { 0xf6, 0xf6, 0xf8 };
        static constexpr const Color ink2 { 0xc8, 0xca, 0xd0 };
        static constexpr const Color ink3 { 0x8c, 0x8e, 0x96 };
        static constexpr const Color acc1 = blue;
        static constexpr const Color acc2 = red;
        static constexpr const Color acc3 = yellow;
        static constexpr const Color acc4 = green;
        static constexpr const Color acc5 = babyblue;
        static constexpr const Color acc6 = pink;
        // static constexpr const Color orange { 0xff, 0x7e, 0x1c };
        static constexpr const Color bg0 { 0x16, 0x16, 0x18 };
        static constexpr const Color bg1 { 0x1c, 0x1c, 0x20 };
        static constexpr const Color bg2 { 0x26, 0x26, 0x2b };
        static constexpr const Color track { 0x13, 0x13, 0x16 };
        // static constexpr const Color line { 0x2a, 0x2a, 0x30 };
        // static constexpr const Color line2 { 0x3b, 0x3b, 0x44 };
        static constexpr const Color line2 { 0x64, 0x64, 0x70 };
        static constexpr const Color bracket { 0xd6, 0xd8, 0xdf, AlphaIntensivity::minor };

        static constexpr const Color borderColorDark { 0, 0, 0, AlphaIntensivity::other };
        static constexpr const Color borderColorLight { 0xff, 0xff, 0xff, 0.06f };
        static constexpr const Color borderColorLight〡hovered { 0xff, 0xff, 0xff, 0.15f };

        static constexpr const Color transparent { 0.f, 0.f, 0.f, 0.f };
    };

    struct Zero {
        static constexpr const Color backgroundColor = Colors::transparent;
        static constexpr const Color backgroundColor〡checked = Colors::transparent;
        static constexpr const Color backgroundColor〡disabled = Colors::transparent;
        static constexpr const Color backgroundColor〡hover = Colors::transparent;
        static constexpr const Color borderColor = Colors::transparent;
        static constexpr const Color borderHighlightColor = Colors::transparent;
        static constexpr const Color borderShadowColor = Colors::transparent;
        static constexpr const Color color = Colors::transparent;
        static constexpr const Color color〡checked = Colors::transparent;
        static constexpr const Color color〡disabled = Colors::transparent;
        static constexpr const Color color〡hover = Colors::transparent;
        static constexpr const Color shadowColor = Colors::transparent;
        static constexpr const Point<int> shadowOffset = { 0, 0 };
        static constexpr const Size<int> shadowSize = { 0, 0 }; // FIXME inset vs outset naming
        static constexpr const MouseCursor cursor = kMouseCursorArrow;
        static constexpr const float fontSize = Common::fontSize;
        static constexpr const float letterSpacing = Common::letterSpacing;
        static constexpr const uint border = 0;
        static constexpr const uint borderRadius = 0;
        static constexpr const uint height = 0;
        static constexpr const uint padding = 0;
        static constexpr const uint margin = 0;
        static constexpr const uint shadowFeather = 0;
        static constexpr const uint width = 0;
    };

    struct Window : Zero {
        static constexpr const Color backgroundGradientStart { 0x2c, 0x2c, 0x34 };
        static constexpr const Color backgroundGradientStop { 0x2a, 0x2a, 0x2f };
        static constexpr const Color borderColor { 0, 0, 0, AlphaIntensivity::minor };
        static constexpr const uint border = 1;
        static constexpr const uint margin = 12;
        static constexpr const uint padding = margin;
    };

    struct TopBar : Zero {
        static constexpr const uint height = 28;
        static constexpr const uint padding = Common::margin;
    };

    struct MainArea : Zero {
        static constexpr const uint padding = Window::padding;
    };

    struct IOMeter : Zero {
        static constexpr const Color backgroundColor = Colors::track;
        static constexpr const Color borderColor { 0, 0, 0, AlphaIntensivity::other };
        static constexpr const MouseCursor cursor = kMouseCursorUpDown;
        static constexpr const uint border = 1;
        static constexpr const uint borderRadius = 7;
        static constexpr const uint margin = 3;
        static constexpr const uint width = 26;

        struct Slider {
            static constexpr const Color color = Colors::acc1;
            static constexpr const Color colorGradientStart = { Colors::acc1, AlphaIntensivity::glow };
            static constexpr const Color colorGradientStop { colorGradientStart, 0.f };
            static constexpr const uint height = 3;
        };

        struct Tick {
            static constexpr const Color color { Colors::ink3, 0.15f };
            static constexpr const Color colorMaj { Colors::ink2, 0.26f };
            static constexpr const uint height = 1;
            static constexpr const uint width = 18;
        };

        struct Track {
            static constexpr const Color colorGradientStart = Colors::acc1;
            static constexpr const Color colorGradientStop = { colorGradientStart, AlphaIntensivity::glow };
            static constexpr const uint width = 9;
        };

        struct Value {
            static constexpr const Color color = Colors::ink2;
            static constexpr const float fontSize = 11;
            // static constexpr const float letterSpacing = fontSize * 0.02f;
            // font-mono
        };

        struct Unit {
            static constexpr const Color color = Colors::ink3;
            static constexpr const float fontSize = 10;
            // static constexpr const float letterSpacing = fontSize * 0.02f;
            // font-ui
        };
    };

    struct CentralArea : Zero {
        static constexpr const uint border = 1;
        static constexpr const Color borderColor { 0, 0, 0, AlphaIntensivity::other };
        static constexpr const uint borderRadius = 7;
        static constexpr const uint margin = Common::margin;
        static constexpr const uint padding = Common::margin;
    };

    struct PillTogglesArea : Zero {
        static constexpr const Color backgroundColor = Colors::transparent;
        static constexpr const Color borderColor = Colors::transparent;
    };

    struct Widgets {
        template<uint kNumRows = 1, bool kIncludeHeader = false>
        struct BoxFrame : Zero {
            static_assert(kNumRows != 0, "box frame must have at least 1 row");

            static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
            static constexpr const Color borderColor = backgroundColor;
            static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
            static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 8;
            static constexpr const uint margin = Common::margin;
            static constexpr const uint padding = Common::padding;

            // TESTING
            static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
            static constexpr const Point<int> shadowOffset = { 0, 5 };
            // static constexpr const Size<int> shadowSize = { -6, -6 };
            static constexpr const Size<int> shadowSize = { -12, -12 };
            static constexpr const uint shadowFeather = 14;

            struct Header : Zero {
                static constexpr const uint margin = 0;
                static constexpr const uint padding = BoxFrame::padding;
                static constexpr const uint height = 18;

                template<const Color& kColor>
                struct Title : Reference::Zero {
                    static constexpr const Color color = kColor;
                    static constexpr const float fontSize = Header::height * 0.75f;
                    static constexpr const float height = Header::height;
                };
            };

            struct Row : Zero {
                static constexpr const uint margin = 0;
                static constexpr const uint padding = BoxFrame::padding;
                static constexpr const uint height = Common::rowHeight;
            };

            static constexpr const uint height = (border + margin) * 2
                                               + padding * (kNumRows - 1)
                                               + Row::height * kNumRows
                                               + (kIncludeHeader ? Header::height + padding : 0);
        };

        struct Card : Zero {
            static constexpr const Color backgroundColor = { Colors::ink3, AlphaIntensivity::glow };
            static constexpr const Color backgroundColor〡selected = { Colors::acc5, AlphaIntensivity::glow };
            static constexpr const Color borderColor = Colors::ink3;
            static constexpr const Color borderColor〡selected = Colors::acc5;
            static constexpr const Color glowColor { borderColor〡selected, AlphaIntensivity::glow };
            static constexpr const MouseCursor cursor = kMouseCursorHand;
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 11;
            static constexpr const uint margin = Common::margin;
            static constexpr const uint height = 80;
            struct Title {
                static constexpr const Color color = Colors::ink;
                static constexpr const Color color〡selected = Colors::acc5;
                static constexpr const float fontSize = Common::fontSize;
                static constexpr const float letterSpacing = Common::letterSpacing;
            };
            struct Subtitle {
                static constexpr const Color color = Colors::ink2;
                static constexpr const float fontSize = Common::fontSize;
                static constexpr const float letterSpacing = Common::letterSpacing;
            };
        };

        struct CardGroup : Zero {
            // static constexpr const uint margin = Common::margin;
            static constexpr const uint padding = Common::margin;
            static constexpr const uint height = 80; // FIXME
        };

        template<const Color& kAccentColor = Colors::acc1>
        struct CheckBox : Zero {
            static constexpr const Color color = Colors::ink;
            static constexpr const Color color〡checked = kAccentColor;
            static constexpr const MouseCursor cursor = kMouseCursorHand;
            static constexpr const uint padding = Common::margin / 2;
            static constexpr const float fontSize = 13.f;
            static constexpr const float letterSpacing = fontSize * 0.08f;
            struct Box : Zero {
                static constexpr const Color backgroundColor = Colors::track;
                static constexpr const Color borderColor = Colors::borderColorLight;
                static constexpr const Color borderColor〡hovered = Colors::borderColorLight〡hovered;
                static constexpr const uint border = 1;
                static constexpr const uint borderRadius = 4;
                static constexpr const float fontSize = CheckBox::fontSize;
                static constexpr const float letterSpacing = CheckBox::letterSpacing;
            };
            static constexpr const uint height = std::max<uint>(
                (margin + border) * 2 + fontSize,
                (margin + border + Box::border + Box::margin) * 2 + Box::fontSize
            );
        };

        template<const Color& kAccentColor = Colors::acc1>
        struct ComboBox : Zero {
            static constexpr const Color color = kAccentColor;
            static constexpr const MouseCursor cursor = kMouseCursorHand;
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 5;
            static constexpr const uint padding = Common::margin / 2;
            static constexpr const float fontSize = 13.f;
            static constexpr const float letterSpacing = fontSize * 0.08f;
            struct Box : Zero {
                static constexpr const Color backgroundColor = Colors::track;
                static constexpr const Color borderColor = Colors::borderColorLight;
                static constexpr const Color borderColor〡hovered = Colors::borderColorLight〡hovered;
                static constexpr const uint border = 1;
                static constexpr const uint borderRadius = 4;
                static constexpr const uint margin = ComboBox::padding;
                static constexpr const float fontSize = ComboBox::fontSize;
                static constexpr const float letterSpacing = ComboBox::letterSpacing;
            };
            static constexpr const uint height = std::max<uint>(
                (margin + border) * 2 + fontSize,
                (margin + border + Box::border + Box::margin) * 2 + Box::fontSize
            );
        };

        struct DualSlider : Zero {
            static constexpr const Color color = Colors::acc6;
            static constexpr const Color color〡deactivated = Colors::ink3;
            static constexpr const uint height = 30 + 18;
            struct Name {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 14;
                // font-ui
            };
            struct Value {
                static constexpr const Color color = Colors::ink2;
                static constexpr const float fontSize = 15;
                // font-mono
            };
        };

        struct EasyKnob : Zero {
            static constexpr const Color backgroundColor = Colors::transparent;
            static constexpr const Color borderColor = Colors::transparent;
            // static constexpr const Color color = { 0xff, 0xff, 0xff };
            static constexpr const uint height = (68 + 20) * 2;
            static constexpr const uint width = 68 * 2;
            struct Name {
                static constexpr const Color color = Colors::acc1;
                static constexpr const float fontSize = 23;
                static constexpr const float letterSpacing = Common::letterSpacing;
                // text-shadow
            };
            struct Value {
                static constexpr const Color color = Colors::ink2;
                static constexpr const float fontSize = 20;
                // font-mono
            };
            struct Unit {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 19;
                static constexpr const float letterSpacing = fontSize * 0.02f;
                // font-ui
            };
        };

        struct FloatingLineSlider : Zero {
            static constexpr const MouseCursor cursor = kMouseCursorUpDown;
            static constexpr const float fontSize = 14.f;
            static constexpr const float letterSpacing = 1.f;
            static constexpr const uint margin = 4;
            static constexpr const uint padding = 7;
        };

        struct Grid : Zero {
            // static constexpr const Color color = Color(0x64, 0x64, 0x70);
            static constexpr const Color color = Colors::line2;
            struct Label : Zero {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 10.f;
                static constexpr const float letterSpacing = 0.f;
                static constexpr const uint margin = 2;
                static constexpr const uint padding = 4;
                // font-mono
            };
        };

        struct Knob : Zero {
            static constexpr const Color backgroundColor = Colors::transparent;
            static constexpr const Color borderColor = Colors::transparent;
            // static constexpr const Color color = { 0xff, 0xff, 0xff };
            static constexpr const uint height = 58 + 20;
            static constexpr const uint width = 58;
            struct Name {
                static constexpr const Color color = Colors::acc1;
                static constexpr const float fontSize = 17;
                static constexpr const float letterSpacing = Common::letterSpacing;
                // text-shadow
            };
            struct Unit {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 14;
                static constexpr const float letterSpacing = fontSize * 0.02f;
                // font-ui
            };
            struct Value {
                static constexpr const Color color = Colors::ink2;
                static constexpr const float fontSize = 15;
                // font-mono
            };
        };

        struct KnobGroup : Zero {
            static constexpr const Color backgroundColor = Colors::transparent;
            static constexpr const Color borderColor = Colors::transparent;
            static constexpr const uint margin = Common::margin;
            struct Bracket {
                static constexpr const Color color = Colors::bracket;
                static constexpr const float fontSize = 15;
                static constexpr const float letterSpacing = fontSize * 0.01;
                static constexpr const uint height = 10;
                static constexpr const uint margin = 0;
                static constexpr const uint padding = 6;
                static constexpr const uint width = 1;
            };
        };

        struct Menu : Zero {
            static constexpr const Color backgroundColor = Color(0x1e, 0x1f, 0x24, 0.96f);
            static constexpr const Color borderColor = Color(0.f, 0.f, 0.f, 0.45f);
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 5;
            static constexpr const uint margin = 4;
            static constexpr const uint padding = 4;
            struct Item : Zero {
                static constexpr const Color color = Colors::ink2;
                static constexpr const Color color〡checked = Colors::track;
                static constexpr const MouseCursor cursor = kMouseCursorHand;
                static constexpr const float fontSize = 15;
                static constexpr const uint border = 1;
                static constexpr const uint borderRadius = 5;
                static constexpr const uint height = 18;
                static constexpr const uint margin = 4;
            };
        };

        template<const Color& kAccentColor = Colors::acc1>
        struct NumberBox : Zero {
            static constexpr const Color backgroundColor = Colors::track;
            // static constexpr const Color borderColor = Color(0.f, 0.f, 0.f, 0.45f);
            static constexpr const Color borderColor = Colors::borderColorLight;
            static constexpr const Color borderColor〡hovered = Colors::borderColorLight〡hovered;
            static constexpr const Color insetShadingColor = Color(0.f, 0.f, 0.f, 0.55f);
            static constexpr const MouseCursor cursor = kMouseCursorUpDown;
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 5;
            static constexpr const uint margin = 4;
            static constexpr const uint height = Common::rowHeight;
            struct Name {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 11.f;
                static constexpr const float letterSpacing = fontSize * 0.08f;
            };
            struct Value {
                static constexpr const Color color = kAccentColor;
                static constexpr const float fontSize = 18.f;
                static constexpr const float letterSpacing = 0.f;
            };
            struct Unit {
                static constexpr const Color color = Colors::ink3;
                static constexpr const float fontSize = 11.f;
                static constexpr const float letterSpacing = 0.f;
            };
            struct Bar {
                static constexpr const Color color = kAccentColor;
                static constexpr const Color color〡deactivated = Color(0.f, 0.f, 0.f, 0.5f);
                static constexpr const uint height = 2;
            };
        };

        struct PluginName : Zero {
            static constexpr const Color color = Colors::acc1;
            static constexpr const float fontSize = 20;
            static constexpr const float letterSpacing = fontSize * 0.05;
        };

        struct PillToggle : Zero {
            static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.07f };
            static constexpr const Color borderColor = { backgroundColor, 0.04f };
            static constexpr const Color borderHighlightColor = { backgroundColor, 0.12f };
            // static constexpr const Color deactivatedColor = Colors::ink3;
            // static constexpr const Color color = Colors::acc;
            // static constexpr const float fontSize = Common::fontSize;
            // static constexpr const float letterSpacing = Common::letterSpacing;
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 8;
            static constexpr const uint margin = 1;
            static constexpr const uint padding = 2;
            struct Cell : Zero {
                static constexpr const Color backgroundColor = Colors::transparent;
                static constexpr const Color backgroundColor〡selected = { 0xff, 0xff, 0xff, 0.12 };
                static constexpr const Color color = Colors::ink2;
                static constexpr const Color color〡deactivated = Colors::ink3;
                static constexpr const Color color〡selected = Colors::acc1;
                static constexpr const MouseCursor cursor = kMouseCursorHand;
                static constexpr const float fontSize = 17;
                static constexpr const float letterSpacing = Common::letterSpacing;
                static constexpr const uint borderRadius = 6;
                static constexpr const uint height = 26;
                static constexpr const uint margin = 3;
            };
        };

        struct SmallKnob : Zero {
            static constexpr const uint height = 50 + 18;
            static constexpr const uint width = 50;
            struct Name : Knob::Name {};
            struct Unit : Knob::Unit {};
            struct Value : Knob::Value {};
        };

        template<const Color& kAccentColor = Colors::acc1>
        struct ThresholdMeter : Zero {
            static constexpr const MouseCursor cursor = kMouseCursorUpDown;
            static constexpr const uint padding = 1;
            struct Meter {
                static constexpr const Color backgroundColor = Colors::track;
                static constexpr const Color borderColor = Color(0.f, 0.f, 0.f, 0.45f);
                static constexpr const Color insetShadingColor = Color(0.f, 0.f, 0.f, 0.55f);
                static constexpr const uint border = 1;
                static constexpr const uint borderRadius = 5;
            };
            struct Slider {
                static constexpr const Color color = kAccentColor;
                static constexpr const uint width = 8;
            };
            struct Tick {
                static constexpr const Color color { Colors::ink3, 0.15f };
                static constexpr const Color colorMaj { Colors::ink2, 0.26f };
                static constexpr const uint height = 1;
            };
            struct Track {
                static constexpr const Color colorGradientStart = kAccentColor;
                static constexpr const Color colorGradientStop = Color(kAccentColor, 0.45f);
                static constexpr const uint width = 8;
            };
            static constexpr const uint width = (border + margin + Track::width) * 2 + padding + Slider::width;
        };

        template <uint8_t accent, bool small = false>
        struct ToggleSwitch : std::conditional_t<small, SmallKnob, Knob> {
            static constexpr const Color color = Colors::acc2;
            static constexpr const MouseCursor cursor = kMouseCursorHand;
            static constexpr const uint margin = small ? 0 : 4;
            static constexpr const float fontSize = 17;
            static constexpr const float letterSpacing = Common::letterSpacing;

            struct Switch {
                static constexpr const Color backgroundColor = Colors::bg2;
                static constexpr const Color backgroundColor〡selected = ToggleSwitch::color;
                static constexpr const Color borderColor = Colors::bg0;
                static constexpr const uint border = 1;
                static constexpr const uint borderRadius = 10;
                static constexpr const uint width = 36;
                static constexpr const uint height = 20;
                static constexpr const uint margin = 1;

                struct Ball {
                    static constexpr const Color backgroundColor = Colors::ink3;
                    static constexpr const Color borderColor =  { Colors::bg0, 0.4f };
                    static constexpr const uint border = 1;
                    static constexpr const uint dotSize = 2;
                };
            };
        };

        struct TopBarButton : Zero {
            static constexpr const Color backgroundColor = { 0x45, 0x45, 0x4f };
            static constexpr const Color color = { 0xa9, 0xc4, 0xf0 };
            static constexpr const Color color〡disabled = Colors::ink3;
            static constexpr const MouseCursor cursor = kMouseCursorHand;
            static constexpr const uint borderRadius = 11;
            static constexpr const uint margin = 9;
            // FIXME move some to common colors?
            static constexpr const Color backgroundColor〡checked = color;
            static constexpr const Color color〡checked = backgroundColor;
            static constexpr const Color color〡hover = color;
            // Color(backgroundColor, color, 0.5f);
        };

        struct TopBarButton〡Bypass : TopBarButton {
            static constexpr const Color backgroundColor〡checked = { 0x5a, 0x35, 0x3c };
            static constexpr const Color color〡checked = { 0xff, 0x5d, 0x5d };
            static constexpr const Color color〡hover = color〡checked;
        };

        struct TopBarButtonGroup : Zero {
            static constexpr const uint borderRadius = 6;
            static constexpr const uint padding = 1;

            // TESTING
            static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
            static constexpr const Point<int> shadowOffset = { 0, 4 };
            static constexpr const Size<int> shadowSize = { -12, -12 };
            static constexpr const uint shadowFeather = 14;
        };
    };

    // helpers
    // template<Orientation orientation, uint size, class Base = Zero>
    // struct AutoSize : Base {
    //     static constexpr const uint width = orientation == Vertical ? size : 0;
    //     static constexpr const uint height = orientation == Horizontal ? size : 0;
    // };

    template<uint _height, class Base = Zero>
    struct Height : Base {
        static constexpr const uint height = _height;
    };

    template<uint _margin, class Base = Zero>
    struct Margin : Base {
        static constexpr const uint margin = _margin;
    };

    template<uint _padding, class Base = Zero>
    struct Padding : Base {
        static constexpr const uint padding = _padding;
    };

    template<uint _width, class Base = Zero>
    struct Width : Base {
        static constexpr const uint width = _width;
    };

    template<class Base>
    struct Transparent : Base {
        static constexpr const Color backgroundColor = Colors::transparent;
        static constexpr const Color backgroundColor〡checked = Colors::transparent;
        static constexpr const Color backgroundColor〡disabled = Colors::transparent;
        static constexpr const Color backgroundColor〡hover = Colors::transparent;
        static constexpr const Color borderColor = Colors::transparent;
        static constexpr const Color borderHighlightColor = Colors::transparent;
        static constexpr const Color borderShadowColor = Colors::transparent;
        static constexpr const Color color = Colors::transparent;
        static constexpr const Color color〡checked = Colors::transparent;
        static constexpr const Color color〡disabled = Colors::transparent;
        static constexpr const Color color〡hover = Colors::transparent;
        static constexpr const Color shadowColor = Colors::transparent;
    };

    static constexpr const Color kAccentColors[] = {
        Colors::acc1,
        Colors::acc2,
        Colors::acc3,
        Colors::acc4,
        Colors::acc5,
        Colors::acc6,
    };
};

static_assert(Reference::IOMeter::width == (
    Reference::IOMeter::border +
    Reference::IOMeter::margin +
    Reference::IOMeter::Track::width) * 2, "incorrect meter size");

static_assert(Reference::IOMeter::Tick::width == (
        Reference::IOMeter::width - (Reference::IOMeter::border + Reference::IOMeter::margin) * 2
    ), "incorrect meter tick size");

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
