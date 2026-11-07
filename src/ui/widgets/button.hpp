// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"

#include "las-resources.h"

namespace LibreAudio {

#if 0

inline constexpr const char kTextA[] = "A";
inline constexpr const char kTextB[] = "B";
inline constexpr const char kTextC[] = "C";
inline constexpr const char kTextD[] = "D";
inline constexpr const char kTextEasy[] = "Easy";
inline constexpr const char kTextExpert[] = "Expert";

// --------------------------------------------------------------------------------------------------------------------

template<Corner kCorner, const uchar* kImageData, uint kImageDataSize>
using TopBarBypassButtonWidget = ImageButtonWidget<Reference::Widgets::TopBarButton〡Bypass, kCorner, kImageData, kImageDataSize>;

template<Corner kCorner, const uchar* kImageData, uint kImageDataSize>
using TopBarImageButtonWidget = ImageButtonWidget<Reference::Widgets::TopBarButton, kCorner, kImageData, kImageDataSize>;

template<Corner kCorner, const uchar* kImageData, uint kImageDataSize, const uchar* kImageData2, uint kImageDataSize2>
using TopBarDualImageButtonWidget = DualImageButtonWidget<Reference::Widgets::TopBarButton〡Bypass, kCorner, kImageData, kImageDataSize>;

    // template<Corner corner, const uchar* imageData, uint imageDataSize, const uchar* imageData2, uint imageDataSize2>
    // using DualButton = DualImageButtonWidget<Reference::Widgets::TopBarButton, corner, imageData, imageDataSize, imageData2, imageDataSize2>;

template<Corner kCorner, const char* kText>
using TopBarStaticTextButtonWidget = StaticTextButtonWidget<Reference::Widgets::TopBarButton, kCorner, kText>;

// --------------------------------------------------------------------------------------------------------------------

template<Corner kCorner>
using TopBarUndoButtonWidget = TopBarButtonWidget<kCorner, IMAGES_UNDO_28_PNG_DATA, IMAGES_UNDO_28_PNG_LEN>>;

template<Corner kCorner>
using TopBarRedoButtonWidget = TopBarButtonWidget<kCorner, IMAGES_REDO_28_PNG_DATA, IMAGES_REDO_28_PNG_LEN>>;

template<Corner kCorner>
using TopBarSnapshotSlotAButtonWidget = TopBarStaticTextButtonWidget<kCorner, kTextA>>;

template<Corner kCorner>
using TopBarSnapshotSlotBButtonWidget = TopBarStaticTextButtonWidget<kCorner, kTextB>>;

template<Corner kCorner>
using TopBarSnapshotSlotCButtonWidget = TopBarStaticTextButtonWidget<kCorner, kTextC>>;

template<Corner kCorner>
using TopBarSnapshotSlotDButtonWidget = TopBarStaticTextButtonWidget<kCorner, kTextD>>;

// WidgetPair<kWidgetSnapshotCopy, DualButton<kCornerLeft, IMAGES_X_28_PNG_DATA, IMAGES_X_28_PNG_LEN, IMAGES_COPY_28_PNG_DATA, IMAGES_COPY_28_PNG_LEN>>,
// WidgetPair<kWidgetSnapshotSlotA, TextButton<kCornerNone, kA>>,
// WidgetPair<kWidgetSnapshotSlotB, TextButton<kCornerNone, kB>>,
// WidgetPair<kWidgetSnapshotSlotC, TextButton<kCornerNone, kC>>,
// WidgetPair<kWidgetSnapshotSlotD, TextButton<kCornerRight, kD>>

#endif

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
