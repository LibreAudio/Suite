# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# function to generate dsp-$TARGET.hpp and metadata-$TARGET.hpp
# used for common code and DPF plugins

function(las_generate_faust_cpp TARGET)
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/dsp-${TARGET}.hpp
    COMMAND
      faustdr
      -a ${CMAKE_SOURCE_DIR}/src/templates/dsp.hpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/dsp-${TARGET}.hpp
      -Did=${TARGET}
      -X-vec
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/dsp.hpp.in
    VERBATIM
  )

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/metadata-${TARGET}.hpp
    COMMAND
      faustdr
      -a ${CMAKE_SOURCE_DIR}/src/templates/metadata.hpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/metadata-${TARGET}.hpp
      -Did=${TARGET}
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/metadata.hpp.in
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# function to generate $TARGET/config.h
# used for DPF plugins

function(las_generate_faust_config TARGET)
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/config.h
    COMMAND
      faustdr
      -a ${CMAKE_SOURCE_DIR}/src/templates/config.h.in
      -c ${TARGET}
      -l c
      -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/config.h
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/config.h.in
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# common definitions between all plugins

las_generate_faust_cpp(input)
las_generate_faust_cpp(output)

add_library(las-definitions INTERFACE)

set_target_properties(las-definitions
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(las-definitions
  INTERFACE
    DISTRHO_PLUGIN_HOMEPAGE="https://libreaudio.org/"
    DISTRHO_PLUGIN_LICENSE="GPL-3.0-or-later"
    DISTRHO_PLUGIN_MAKER="Libre Audio"
)

target_include_directories(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    "."
)

target_sources(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/dsp-input.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/dsp-output.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/metadata-input.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/metadata-output.hpp
)

# ---------------------------------------------------------------------------------------------------------------------
# function to setup a DPF plugin

function(las_add_plugin LABEL)
  set(TARGET "la-${LABEL}")

  dpf_add_plugin(${TARGET}
    TARGETS
      clap lv2 vst3 jack
    FILES_DSP
      plugins/${LABEL}.cpp
  )

  las_generate_faust_cpp(${LABEL})
  las_generate_faust_config(${LABEL})

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

  target_sources(${TARGET}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/dsp-${LABEL}.hpp
      ${CMAKE_CURRENT_BINARY_DIR}/metadata-${LABEL}.hpp
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}/config.h
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# setup all plugins

las_add_plugin(chorus)
las_add_plugin(djFilter)

# ---------------------------------------------------------------------------------------------------------------------
