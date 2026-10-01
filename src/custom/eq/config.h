// Native dynamic EQ configuration. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#define DISTRHO_PLUGIN_CLAP_ID "org.libreaudio.eq"
#define DISTRHO_PLUGIN_DESCRIPTION "Mid/side parametric EQ modelled on x42-eq (fil4.lv2)"
#define DISTRHO_PLUGIN_LABEL "eq"
#define DISTRHO_PLUGIN_NAME "LA EQ42"
#define DISTRHO_PLUGIN_NUM_INPUTS 2
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2
#define DISTRHO_PLUGIN_UNIQUE_ID LAeq
#define DISTRHO_PLUGIN_URI "https://libreaudio.org/plugins/eq"

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
#endif

// optional speech/voice-activity detection control
#ifndef LIBREAUDIO_WANT_SPEECH_DETECTION
#define LIBREAUDIO_WANT_SPEECH_DETECTION 0
#endif

// optional latency output
#ifndef DISTRHO_PLUGIN_WANT_LATENCY
#define DISTRHO_PLUGIN_WANT_LATENCY 0
#endif

#define LIBREAUDIO_WANT_DRYWET 0

// FIXME remove this
#define LIBREAUDIO_PLUGIN eq

#define LIBREAUDIO_PLUGIN_DSP_INCLUDE "eq-dsp.hpp"
#define LIBREAUDIO_PLUGIN_PARAMETERS_INCLUDE "eq-parameters.hpp"
#define LIBREAUDIO_PLUGIN__eq

namespace eq {}
using namespace eq;

