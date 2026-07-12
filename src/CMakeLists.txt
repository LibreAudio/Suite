# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# function to generate $TARGET-dsp.{c,h}pp and $TARGET-parameters.{c,h}pp
# used for common code and DPF plugins

function(las_generate_faust_cpp TARGET)
  string(REPLACE "/" "_" OUTPUT ${TARGET})

  if(${LIBREAUDIO_BLOCK_SIZE})
    set(VEC_ARGS "-X-vec -X-vs -X${LIBREAUDIO_BLOCK_SIZE}")
  endif()

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-dsp.cpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/dsp.cpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-dsp.cpp
      -Did=${OUTPUT}
      ${VEC_ARGS}
      -X--class-name -X${OUTPUT}
      -X--super-class-name -XFaustDSP
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/dsp.cpp.in
    VERBATIM
  )

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-dsp.hpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/dsp.hpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-dsp.hpp
      -Did=${OUTPUT}
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/dsp.hpp.in
    VERBATIM
  )

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-parameters.cpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/parameters.cpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-parameters.cpp
      -Did=${OUTPUT}
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/parameters.cpp.in
    VERBATIM
  )

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-parameters.hpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/parameters.hpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}-parameters.hpp
      -Did=${OUTPUT}
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/parameters.hpp.in
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# function to generate $TARGET/config.h
# used for DPF plugins

function(las_generate_faust_plugin TARGET)
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    COMMAND
      ${CMAKE_COMMAND}
      -E
      make_directory
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    VERBATIM
  )
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/config.h
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/config.h.in
      -c ${TARGET}
      -l c
      -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/config.h
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
      dsp-faust/${TARGET}.dsp
      templates/config.h.in
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# common definitions between all plugins

las_generate_faust_cpp(common/input)
las_generate_faust_cpp(common/output)

add_library(las-definitions INTERFACE)

set_target_properties(las-definitions
  PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
    POSITION_INDEPENDENT_CODE ON
)

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_64|amd64|AMD64|x64|X64|i.86|x86|X86)$")
  target_compile_options(las-definitions
    INTERFACE
      -mtune=generic -msse -msse2 -mfpmath=sse
  )
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(ARM|armv[3-9][a-z]*)$")
  target_compile_options(las-definitions
    INTERFACE
      -mfpu=neon-vfpv4 -mfloat-abi=hard
  )
endif()

target_compile_definitions(las-definitions
  INTERFACE
    IMGUI_DISABLE_DEBUG_TOOLS
    IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
    IMGUI_DISABLE_DEMO_WINDOWS
    VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
    VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
    VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
    $<$<BOOL:${LIBREAUDIO_BLOCK_SIZE}>:LIBREAUDIO_BLOCK_SIZE=${LIBREAUDIO_BLOCK_SIZE}>
)

target_include_directories(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    .
)

target_sources(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/common_input-dsp.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_input-parameters.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_output-dsp.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_output-parameters.hpp
)

# ---------------------------------------------------------------------------------------------------------------------

add_library(las-definitions-dsp INTERFACE)

target_compile_definitions(las-definitions-dsp
  INTERFACE
    $<$<BOOL:${LIBREAUDIO_BLOCK_SIZE}>:LIBREAUDIO_BLOCK_SIZE=${LIBREAUDIO_BLOCK_SIZE}>
)

target_sources(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/common_input-dsp.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_input-parameters.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_output-dsp.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_output-parameters.cpp
)

# ---------------------------------------------------------------------------------------------------------------------

add_library(las-definitions-ui INTERFACE)

target_compile_definitions(las-definitions-ui
  INTERFACE
    IMGUI_DISABLE_DEBUG_TOOLS
    IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
    IMGUI_DISABLE_DEMO_WINDOWS
)

target_include_directories(las-definitions-ui
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    ../deps/DPF-Widgets/generic
    ../deps/DPF-Widgets/opengl
    .
)

target_sources(las-definitions-ui
  INTERFACE
    LibreAudioSnapshots.cpp
    LibreAudioUndoRedo.cpp
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
    ../deps/rnnoise/src
    ../deps/rnnoise-model-data
  PUBLIC
    ../deps/rnnoise/include
)

target_sources(las-rnnoise
  PRIVATE
    ../deps/rnnoise/src/celt_lpc.c
    ../deps/rnnoise/src/denoise.c
    ../deps/rnnoise/src/kiss_fft.c
    ../deps/rnnoise/src/nnet.c
    ../deps/rnnoise/src/nnet_default.c
    ../deps/rnnoise/src/parse_lpcnet_weights.c
    ../deps/rnnoise/src/pitch.c
    ../deps/rnnoise/src/rnn.c
    ../deps/rnnoise/src/rnnoise_tables.c
    ../deps/rnnoise-model-data/rnnoise_data.c
)

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_64|amd64|AMD64|x64|X64|i.86|x86|X86)$")
  target_compile_definitions(las-rnnoise
    PRIVATE
      CPU_INFO_BY_ASM
      RNN_ENABLE_X86_RTCD
  )

  target_sources(las-rnnoise
    PRIVATE
      ../deps/rnnoise/src/x86/nnet_avx2.c
      ../deps/rnnoise/src/x86/nnet_sse4_1.c
      ../deps/rnnoise/src/x86/x86cpu.c
      ../deps/rnnoise/src/x86/x86_dnn_map.c
  )

  set_source_files_properties(../deps/rnnoise/src/x86/nnet_avx2.c
    PROPERTIES
      COMPILE_FLAGS
        "-mavx -mfma -mavx2"
  )

  set_source_files_properties(../deps/rnnoise/src/x86/nnet_sse4_1.c
    PROPERTIES
      COMPILE_FLAGS
        -msse4.1
  )
endif()

# ---------------------------------------------------------------------------------------------------------------------
# function to setup a DPF plugin

function(las_add_plugin LABEL)
  set(TARGET "la-${LABEL}")

  set(EXTRA_DSP_LIBS)
  set(EXTRA_UI_LIBS)
  if("${LABEL}" STREQUAL "leveler")
    list(APPEND EXTRA_DSP_LIBS las-rnnoise)
  endif()

  dpf_add_plugin(${TARGET}
    TARGETS
      clap lv2 vst3 jack
    FILES_COMMON
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}-parameters.cpp
    FILES_DSP
      LibreAudioPlugin.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}-dsp.cpp
    FILES_UI
      LibreAudioUI.cpp
      ../deps/DPF-Widgets/opengl/DearImGui.cpp
    RUNTIME_TESTING
  )

  las_generate_faust_cpp(${LABEL})
  las_generate_faust_plugin(${LABEL})

  target_compile_definitions(${TARGET}
    PUBLIC
      DISTRHO_NAMESPACE=${LABEL}
      DISTRHO_PLUGIN_CLAP_ID="org.libreaudio.${LABEL}"
      DISTRHO_PLUGIN_LABEL="${LABEL}"
      DISTRHO_PLUGIN_URI="https://libreaudio.org/plugins/${LABEL}"
  )

  target_include_directories(${TARGET}
    PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}
  )

  target_link_libraries(${TARGET}
    PUBLIC
      las-definitions
  )

  target_link_libraries(${TARGET}-dsp
    PUBLIC
      las-definitions-dsp
      ${EXTRA_DSP_LIBS}
  )

  target_link_libraries(${TARGET}-ui
    PUBLIC
      las-definitions-ui
      nlohmann_json::nlohmann_json
      ${EXTRA_UI_LIBS}
  )

  target_sources(${TARGET}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}-dsp.hpp
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}-parameters.hpp
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}/config.h
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# setup all plugins

file(GLOB
  LIBREAUDIO_PLUGINS
    CONFIGURE_DEPENDS
      dsp-faust/*.dsp
)

foreach(PLUGIN ${LIBREAUDIO_PLUGINS})
  string(REGEX REPLACE "^(.*)/(.*)\.dsp$" "\\2" PLUGIN "${PLUGIN}")
  las_add_plugin(${PLUGIN})
endforeach()

# ---------------------------------------------------------------------------------------------------------------------
