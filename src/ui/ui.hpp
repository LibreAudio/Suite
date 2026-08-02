// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// NOTE this is the file that gets imported by DPF in a custom include
// to keep build times reasonable only include the necessary files for a LibreAudioArea widget

#pragma once

#include "base/container.hpp"
#include "reference.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

using LibreAudioUIWidget = LibreAudioContainer<LibreAudioReference::Window, kVertical, LibreAudioTopLevelWidget>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
