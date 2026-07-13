// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"

#if defined(__has_include) && __has_include("config-custom.h")
#include "config-custom.h"
#endif

#define DISTRHO_PLUGIN_BRAND "Libre Audio"
#define DISTRHO_PLUGIN_BRAND_ID LiAu
#define DISTRHO_PLUGIN_HOMEPAGE "https://libreaudio.org/"
#define DISTRHO_PLUGIN_LICENSE "GPL-3.0-or-later"
#define DISTRHO_PLUGIN_MAKER "Libre Audio"

#define DISTRHO_PLUGIN_HAS_UI      1
#define DISTRHO_PLUGIN_IS_RT_SAFE  1
#define DISTRHO_PLUGIN_WANT_STATE  1

#define DISTRHO_UI_FILE_BROWSER    0
#define DISTRHO_UI_USER_RESIZABLE  1

#ifndef DISTRHO_UI_USE_CUSTOM
#define DISTRHO_UI_USE_CUSTOM      1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#endif

#ifndef DISTRHO_UI_DEFAULT_WIDTH
#define DISTRHO_UI_DEFAULT_WIDTH 500
#endif

#ifndef DISTRHO_UI_DEFAULT_HEIGHT
#define DISTRHO_UI_DEFAULT_HEIGHT 620
#endif

#if DISTRHO_PLUGIN_NUM_INPUTS != 2
#error DISTRHO_PLUGIN_NUM_INPUTS != 2
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS != 2
#error DISTRHO_PLUGIN_NUM_OUTPUTS != 2
#endif
