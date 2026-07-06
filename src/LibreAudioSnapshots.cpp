// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioSnapshots.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

LibreAudioSnapshots::LibreAudioSnapshots(const uint32_t snapshotCount,
                                         const uint32_t parameterCount,
                                         const float* const parameterValues,
                                         Callback* const callback)
    : fCallback(callback),
      fParameterCount(parameterCount),
      fSnapshotCount(snapshotCount),
      fCurrent(0),
      fParameterValues(new float*[snapshotCount]),
      fUpdated(new bool[snapshotCount])
{
    for (uint32_t i = 0; i < fSnapshotCount; ++i)
    {
        fParameterValues[i] = new float[parameterCount];
        std::memcpy(fParameterValues[i], parameterValues, sizeof(float) * parameterCount);
    }
}

LibreAudioSnapshots::~LibreAudioSnapshots()
{
    for (uint32_t i = 0; i < fSnapshotCount; ++i)
        delete[] fParameterValues[i];
    delete[] fParameterValues;
    delete[] fUpdated;
}

void LibreAudioSnapshots::idle()
{
    for (uint32_t i = 0; i < fSnapshotCount; ++i)
    {
        if (fUpdated[i])
            triggerSave(i);
    }
}

void LibreAudioSnapshots::clearCurrentAndPrevious(const uint32_t snapshot)
{
    fCurrent = fPrevious = snapshot;
}

void LibreAudioSnapshots::clearParameterValues(const uint32_t snapshot, const float* const parameterValues)
{
    std::memcpy(fParameterValues[snapshot], parameterValues, sizeof(float) * fParameterCount);
}

void LibreAudioSnapshots::copyTo(const uint32_t snapshot)
{
    DISTRHO_SAFE_ASSERT_RETURN(fCurrent != snapshot,);

    std::memcpy(fParameterValues[snapshot], fParameterValues[fCurrent], sizeof(float) * fParameterCount);

    // set new snapshot (index)
    fPrevious = fCurrent;
    fCurrent = snapshot;

    // set state of previous and current snapshot (values)
    triggerSave(fPrevious);
    triggerSave(fCurrent);
}

void LibreAudioSnapshots::load(const uint32_t snapshot)
{
    DISTRHO_SAFE_ASSERT_RETURN(fCurrent != snapshot,);

    fPrevious = fCurrent;
    fCurrent = snapshot;
    fCallback->snapshotParametersChanged(fParameterValues[snapshot]);
}

void LibreAudioSnapshots::updateParameterValue(const uint32_t parameterIndex, const float parameterValue) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(parameterIndex < fParameterCount,);

    if (d_isEqual(fParameterValues[fCurrent][parameterIndex], parameterValue))
        return;

    fParameterValues[fCurrent][parameterIndex] = parameterValue;
    fUpdated[fCurrent] = true;
}

void LibreAudioSnapshots::triggerSave(const uint32_t snapshot)
{
    fUpdated[snapshot] = false;
    fCallback->snapshotDataToSave(snapshot, fParameterValues[snapshot]);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
