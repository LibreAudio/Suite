// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoDetails.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct LibreAudioSnapshots {
    struct Callback {
        virtual ~Callback() = default;
        virtual void snapshotDataToSave(uint32_t snapshot, const float* parameterValues) = 0;
        virtual void snapshotParametersChanged(const float* parameterValues) = 0;
    };

    LibreAudioSnapshots(uint32_t snapshotCount,
                        uint32_t parameterCount,
                        const float* parameterValues,
                        Callback* const callback);
    ~LibreAudioSnapshots();

    inline uint32_t getCurrent() const noexcept
    {
        return fCurrent;
    }

    inline uint32_t getPrevious() const noexcept
    {
        return fPrevious;
    }

    void idle();

    void clearCurrentAndPrevious(uint32_t snapshot);
    void clearParameterValues(uint32_t snapshot, const float* parameterValues);

    void copyTo(uint32_t snapshot);
    void load(uint32_t snapshot);

    void updateParameterValue(uint32_t parameterIndex, float parameterValue) noexcept;

private:
    Callback* const fCallback;
    const uint32_t fParameterCount;
    const uint32_t fSnapshotCount;
    uint32_t fCurrent = 0;
    uint32_t fPrevious = fCurrent;
    float** const fParameterValues;
    bool* const fUpdated;

    void triggerSave(uint32_t snapshot);
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
