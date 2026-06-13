# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# function to generate $TARGET.hpp
# used for common code and DPF plugins

function(las_generate_faust_cpp TARGET)
  string(REPLACE "/" "_" OUTPUT ${TARGET})

  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}.hpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/dsp.hpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}.hpp
      -Did=${OUTPUT}
      -X-vec
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      dsp-faust/${TARGET}.dsp
      templates/dsp.hpp.in
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------------------------------------------------
# function to generate $TARGET/config.h and $TARGET/plugin.cpp
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
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/plugin.cpp
    COMMAND
      ${FAUSTDR}
      -a ${CMAKE_SOURCE_DIR}/src/templates/plugin.cpp.in
      -c ${TARGET}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/plugin.cpp
      -Did=${TARGET}
      ${CMAKE_SOURCE_DIR}/src/dsp-faust/${TARGET}.dsp
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
      dsp-faust/${TARGET}.dsp
      templates/plugin.cpp.in
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
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(las-definitions
  INTERFACE
    VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
    VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
    VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
)

target_include_directories(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    "."
)

target_sources(las-definitions
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/common_input.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/common_output.hpp
)

# ---------------------------------------------------------------------------------------------------------------------
# function to setup a DPF plugin

function(las_add_plugin LABEL)
  set(TARGET "la-${LABEL}")

  dpf_add_plugin(${TARGET}
    TARGETS
      clap lv2 vst3 jack
    FILES_DSP
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}/plugin.cpp
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

  target_sources(${TARGET}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${LABEL}.hpp
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
