// Standalone build of speexdsp's resampler, used by LibreAudioVoiceIsolation to
// run RNNoise at 48 kHz whatever the host rate.
// OUTSIDE_SPEEX and RANDOM_PREFIX come from the las-speex-resampler target, so
// that users of speex_resampler.h see the same symbol names.
//
// The SIMD path is chosen from the compiler's own target macros, so universal
// macOS builds get the right one per architecture.
// SPDX-License-Identifier: BSD-3-Clause (speexdsp)

#define FLOATING_POINT
#define EXPORT

#if defined(__SSE__)
#define USE_SSE
#endif
#if defined(__SSE2__)
#define USE_SSE2
#endif
#if defined(__aarch64__) && defined(__ARM_NEON)
#define USE_NEON
#endif

#include "speexdsp/libspeexdsp/resample.c"
