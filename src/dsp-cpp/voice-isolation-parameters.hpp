// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioParameter.hpp"
#include "DistrhoPluginInfo.h"

#include <array>

namespace voice_isolation {

enum ParameterIndex : uint16_t {
    kParameterCount
};

inline constexpr const std::array<LibreAudioParameter, kParameterCount> kParameters = {{
}};

}

#if LIBREAUDIO_WANT_EXTRA_PROCESSOR

namespace extra {
using voice_isolation::kParameterCount;
using voice_isolation::kParameters;
}

#endif
