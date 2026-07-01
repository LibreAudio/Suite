// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoDetails.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

enum States {
    kStateCurrentSnapshot,
    kStateSnapshotValuesA,
    kStateSnapshotValuesB,
    kStateSnapshotValuesC,
    kStateSnapshotValuesD,
    kStateCount,
};

#define LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX "snapshot_values_"

inline constexpr const char* kStateKeys[kStateCount] = {
    "snapshot",
    LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX "a",
    LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX "b",
    LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX "c",
    LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX "d",
};

inline constexpr const uint8_t kNumSnapshots = 4;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
