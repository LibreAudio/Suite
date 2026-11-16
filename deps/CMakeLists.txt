# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# kiss-fft library

add_library(las-kiss-fft STATIC)

set_target_properties(las-kiss-fft
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(las-kiss-fft
  PUBLIC
    DISABLE_DEBUG_FLOAT
    FLOAT_APPROX
    RNNOISE_EXPORT=
)

target_include_directories(las-kiss-fft
  PUBLIC
    rnnoise/src
)

target_sources(las-kiss-fft
  PRIVATE
    rnnoise/src/kiss_fft.c
)

# ---------------------------------------------------------------------------------------------------------------------
# rnnoise library

add_library(las-rnnoise STATIC)

set_target_properties(las-rnnoise
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(las-rnnoise
  PRIVATE
    DISABLE_DEBUG_FLOAT
    FLOAT_APPROX
    RNNOISE_EXPORT=
)

target_include_directories(las-rnnoise
  PRIVATE
    rnnoise/src
    rnnoise-model-data
  PUBLIC
    rnnoise/include
)

target_sources(las-rnnoise
  PRIVATE
    rnnoise/src/celt_lpc.c
    rnnoise/src/denoise.c
    rnnoise/src/kiss_fft.c
    rnnoise/src/nnet.c
    rnnoise/src/nnet_default.c
    rnnoise/src/parse_lpcnet_weights.c
    rnnoise/src/pitch.c
    rnnoise/src/rnn.c
    rnnoise/src/rnnoise_tables.c
    rnnoise-model-data/rnnoise_data.c
)

if(LIBREAUDIO_USE_SSE)
  target_compile_definitions(las-rnnoise
    PRIVATE
      CPU_INFO_BY_ASM
      RNN_ENABLE_X86_RTCD
  )

  target_sources(las-rnnoise
    PRIVATE
      rnnoise/src/x86/nnet_avx2.c
      rnnoise/src/x86/nnet_sse4_1.c
      rnnoise/src/x86/x86cpu.c
      rnnoise/src/x86/x86_dnn_map.c
  )

  set_source_files_properties(rnnoise/src/x86/nnet_avx2.c
    PROPERTIES
      COMPILE_FLAGS
        "-mavx -mfma -mavx2"
  )

  set_source_files_properties(rnnoise/src/x86/nnet_sse4_1.c
    PROPERTIES
      COMPILE_FLAGS
        "-msse4.1"
  )
endif()

# ---------------------------------------------------------------------------------------------------------------------
# speexdsp-resample library

add_library(las-speexdsp-resample STATIC)

set_target_properties(las-speexdsp-resample
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(las-speexdsp-resample
  PRIVATE
    FLOATING_POINT
    $<$<BOOL:${LIBREAUDIO_USE_NEON}>:USE_NEON>
    $<$<BOOL:${LIBREAUDIO_USE_SSE}>:USE_SSE>
    $<$<BOOL:${LIBREAUDIO_USE_SSE}>:USE_SSE2>
  PUBLIC
    OUTSIDE_SPEEX
    RANDOM_PREFIX=las
)

target_include_directories(las-speexdsp-resample
  PRIVATE
    speexdsp/libspeexdsp
  PUBLIC
    speexdsp/include/speex
)

target_sources(las-speexdsp-resample
  PRIVATE
    speexdsp/libspeexdsp/resample.c
)

# ---------------------------------------------------------------------------------------------------------------------
