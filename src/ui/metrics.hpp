// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NanoVG.hpp"

struct Metrics {
    static constexpr const uint fontSize = 20;

    struct Window {
        static constexpr const uint padding = 0;
        static constexpr const uint margin = 0;
    };

    struct TopBar {
        static constexpr const uint height = 46;
        static constexpr const uint padding = 10;
        static constexpr const uint margin = 0;
        static constexpr const uint marginLeft = margin + 7;
        static constexpr const uint marginRight = margin + 14;
        static constexpr const uint smallImageSize = 14;
        struct Logo {
            static constexpr const uint imageSize = 34;
        };
        struct PluginName {
        //     static constexpr const uint size = 34;
        };
        struct Cluster {
            static constexpr const uint width = 610;
            static constexpr const uint padding = 60;
            static constexpr const uint margin = 0;
            struct UndoRedo {
                static constexpr const uint width = 40;
                static constexpr const uint padding = 12;
                static constexpr const uint margin = 0;
            };
            struct Snapshots {
                static constexpr const uint width = 104;
                static constexpr const uint padding = 10;
                static constexpr const uint margin = 0;
            };
            struct EasyExpert {
                static constexpr const uint width = 92;
                static constexpr const uint padding = 10;
                static constexpr const uint margin = 0;
            };
            struct Menu {
                static constexpr const uint width = 45;
                static constexpr const uint padding = 9;
            };
        };
    };

    struct BottomBar {
        static constexpr const uint height = 46;
    };
};
