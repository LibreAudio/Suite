# Libre Audio Suite
# Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.8...3.31)

project(LibreAudioSuite VERSION 0.0.1)

option(LIBREAUDIO_BLOCK_SIZE "Vectorizable block size")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
  set(RELEASE_FLAGS "-O3")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -DNDEBUG=1")
  set(RELEASE_FLAGS "${RELEASE_FLAGS} -ffast-math -fdata-sections -ffunction-sections")
  # set(RELEASE_FLAGS "${RELEASE_FLAGS} -fsingle-precision-constant")
  if("${C_COMPILER_ID}" STREQUAL "AppleClang")
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -Wl,-dead_strip,-dead_strip_dylibs,-x")
  elseif("${C_COMPILER_ID}" STREQUAL "GNU")
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -fno-gnu-unique")
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -Wl,-O1,--as-needed,--gc-sections,--no-undefined,--strip-all")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RELEASE_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RELEASE_FLAGS}")
endif()

find_program(FAUST faust REQUIRED)
find_program(FAUSTDR faustdr REQUIRED)

add_subdirectory(deps/DPF)
# add_subdirectory(deps/DPF-Widgets)
add_subdirectory(deps/json)
add_subdirectory(src)
