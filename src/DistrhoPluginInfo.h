// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"

#define DISTRHO_PLUGIN_HOMEPAGE "https://libreaudio.org/"
#define DISTRHO_PLUGIN_LICENSE "GPL-3.0-or-later"
#define DISTRHO_PLUGIN_MAKER "Libre Audio"

// TODO
#define DISTRHO_PLUGIN_HAS_UI 0

// #define DISTRHO_PLUGIN_DESCRIPTION ""
#define DISTRHO_PLUGIN_UNIQUE_ID LASu

#if DISTRHO_PLUGIN_NUM_INPUTS != 2
#error DISTRHO_PLUGIN_NUM_INPUTS != 2
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS != 2
#error DISTRHO_PLUGIN_NUM_OUTPUTS != 2
#endif
