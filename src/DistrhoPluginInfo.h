// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"

#if defined(__has_include) && __has_include("config-custom.h")
#include "config-custom.h"
#endif

// optional common IO
#ifdef _DARKGLASS_DEVICE_PABLITO
#define LIBREAUDIO_WANT_COMMON_IO 0
#endif
#ifndef LIBREAUDIO_WANT_COMMON_IO
#define LIBREAUDIO_WANT_COMMON_IO 1
#endif

// optional time controls
#ifndef LIBREAUDIO_WANT_TIMEPOS_BPM
#define LIBREAUDIO_WANT_TIMEPOS_BPM 0
#elif LIBREAUDIO_WANT_TIMEPOS_BPM
#define DISTRHO_PLUGIN_WANT_TIMEPOS 1
#endif

// optional speech/voice-activity detection
#ifndef LIBREAUDIO_WANT_SPEECH_DETECTION
#define LIBREAUDIO_WANT_SPEECH_DETECTION 0
#elif LIBREAUDIO_WANT_SPEECH_DETECTION
#define LIBREAUDIO_WANT_EXTRA_PROCESSOR 1
#define LIBREAUDIO_EXTRA_DSP_INCLUDE "dsp-cpp/speech-detection-dsp.hpp"
#define LIBREAUDIO_EXTRA_PARAMETERS_INCLUDE "dsp-cpp/speech-detection-parameters.hpp"
#endif

// optional speech/voice isolation
#ifndef LIBREAUDIO_WANT_VOICE_ISOLATION
#define LIBREAUDIO_WANT_VOICE_ISOLATION 0
#elif LIBREAUDIO_WANT_VOICE_ISOLATION
#define LIBREAUDIO_WANT_EXTRA_PROCESSOR 1
#define LIBREAUDIO_EXTRA_DSP_INCLUDE "bot/voice-isolation-dsp.hpp"
#define LIBREAUDIO_EXTRA_PARAMETERS_INCLUDE "dsp-cpp/voice-isolation-parameters.hpp"
#endif

// optional latency output
#ifndef DISTRHO_PLUGIN_WANT_LATENCY
#define DISTRHO_PLUGIN_WANT_LATENCY 0
#endif

// optional UI graphs (fft analyzer, waveform, etc)
#ifndef LIBREAUDIO_WANT_GRAPH
#define LIBREAUDIO_WANT_GRAPH 0
#define LIBREAUDIO_WANT_GRAPH_ANALYZER 0
#define LIBREAUDIO_WANT_GRAPH_WAVEFORM 0
#define LIBREAUDIO_WANT_GRAPH_IO_COUNT 0
#define LIBREAUDIO_WANT_GRAPH_INPUT 0
#define LIBREAUDIO_WANT_GRAPH_OUTPUT 0
#endif

// optional extra (non-faust dsp) processor
#ifndef LIBREAUDIO_WANT_EXTRA_PROCESSOR
#define LIBREAUDIO_WANT_EXTRA_PROCESSOR 0
#endif

#define DISTRHO_PLUGIN_BRAND "Libre Audio"
#define DISTRHO_PLUGIN_BRAND_ID LiAu
#define DISTRHO_PLUGIN_HOMEPAGE "https://libreaudio.org/"
#define DISTRHO_PLUGIN_LICENSE "GPL-3.0-or-later"
#define DISTRHO_PLUGIN_MAKER "Libre Audio"

#ifdef LIBREAUDIO_CUSTOM_UI
#define DISTRHO_PLUGIN_HAS_UI 1
#define DISTRHO_PLUGIN_WANT_STATE 1
#ifndef DISTRHO_UI_DEFAULT_WIDTH
#define DISTRHO_UI_DEFAULT_WIDTH 910
#endif
#ifndef DISTRHO_UI_DEFAULT_HEIGHT
#define DISTRHO_UI_DEFAULT_HEIGHT 475
#endif
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_USER_RESIZABLE 1
#define DISTRHO_UI_USE_CUSTOM 1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "ui/_lab/ui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE LabUIWidget
#else
#undef DISTRHO_UI_DEFAULT_WIDTH
#undef DISTRHO_UI_DEFAULT_HEIGHT
#endif

#define DISTRHO_PLUGIN_IS_RT_SAFE 1

#if DISTRHO_PLUGIN_NUM_INPUTS != DISTRHO_PLUGIN_NUM_OUTPUTS
#error DISTRHO_PLUGIN_NUM_INPUTS != DISTRHO_PLUGIN_NUM_OUTPUTS
#endif

inline constexpr const char* const _constexpr_DISTRHO_PLUGIN_NAME = DISTRHO_PLUGIN_NAME;
static_assert(_constexpr_DISTRHO_PLUGIN_NAME[0] == 'L', "Name does not start with 'LA '");
static_assert(_constexpr_DISTRHO_PLUGIN_NAME[1] == 'A', "Name does not start with 'LA '");
static_assert(_constexpr_DISTRHO_PLUGIN_NAME[2] == ' ', "Name does not start with 'LA '");

inline constexpr const char* const _constexpr_DISTRHO_PLUGIN_LABEL = DISTRHO_PLUGIN_LABEL;
static_assert(_constexpr_DISTRHO_PLUGIN_LABEL[0] != '\0', "Label must not be empty");
