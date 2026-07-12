# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.8...3.31)

project(LibreAudioSuite VERSION 0.0.1)

option(LIBREAUDIO_BLOCK_SIZE "Vectorizable block size")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

#######################################################################################################################
# check for dependencies

find_program(FAUST faust REQUIRED)
find_program(FAUSTDR faustdr REQUIRED)

#######################################################################################################################
# common flags

# enable fast-math for all builds
set(COMMON_FLAGS "-ffast-math")

# invalidate possible -fno-finite-math-only from user environment
set(COMMON_FLAGS "${COMMON_FLAGS} -ffinite-math-only")

# use a custom namespace for all DPF things
set(COMMON_FLAGS "${COMMON_FLAGS} -DDISTRHO_NAMESPACE=LibreAudioSuiteDISTRHO")
set(COMMON_FLAGS "${COMMON_FLAGS} -DDGL_NAMESPACE=LibreAudioSuiteDGL")

# base tuning, SSE2 or NEON required
if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(x86_64|amd64|AMD64|x64|X64|i.86|x86|X86)$")
  set(COMMON_FLAGS "${COMMON_FLAGS} -mtune=generic -msse -msse2 -mfpmath=sse")
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(ARM|armv[3-9][a-z]*)$")
  set(COMMON_FLAGS "${COMMON_FLAGS} -mfpu=neon-vfpv4 -mfloat-abi=hard")
endif()

# GCC specific flags
if("${C_COMPILER_ID}" STREQUAL "GNU")
  # do not keep GNU-specific symbols around, preventing dlclose() from working properly
  set(COMMON_FLAGS "${COMMON_FLAGS} -fno-gnu-unique")

  # always report undefined symbols
  set(COMMON_FLAGS "${COMMON_FLAGS} -Wl,--no-undefined")
endif()

#######################################################################################################################
# debug flags

set(DEBUG_FLAGS "-O0")
set(DEBUG_FLAGS "${DEBUG_FLAGS} -g")
set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG=1")

#######################################################################################################################
# release flags

set(RELEASE_FLAGS "-O3")
set(RELEASE_FLAGS "${RELEASE_FLAGS} -DNDEBUG=1")
set(RELEASE_FLAGS "${RELEASE_FLAGS}  -fdata-sections -ffunction-sections")
# set(RELEASE_FLAGS "${RELEASE_FLAGS} -fsingle-precision-constant")
if("${C_COMPILER_ID}" STREQUAL "AppleClang")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -Wl,-dead_strip,-dead_strip_dylibs,-x")
elseif("${C_COMPILER_ID}" STREQUAL "GNU")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -Wl,-O1,--as-needed,--gc-sections,--strip-all")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RELEASE_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RELEASE_FLAGS} ${COMMON_FLAGS}")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAGS} ${COMMON_FLAGS}")
endif()

#######################################################################################################################
# setup dependencies

add_subdirectory(deps/DPF)
# add_subdirectory(deps/DPF-Widgets)
add_subdirectory(deps/json)

# main source folder
add_subdirectory(src)
