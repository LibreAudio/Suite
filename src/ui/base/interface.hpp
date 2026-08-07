// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUIWidgetInterface
{
public:
    enum SnapshotButton {
        kSnapshotButtonCopy,
        kSnapshotButtonA,
        kSnapshotButtonB,
        kSnapshotButtonC,
        kSnapshotButtonD,
    };

    virtual ~LibreAudioUIWidgetInterface() = default;

    virtual float getParameterValue(const uint32_t index) const noexcept = 0;

    virtual bool canUndo() const noexcept = 0;
    virtual bool canRedo() const noexcept = 0;
    virtual bool isCopyingSnapshot() const noexcept = 0;
    virtual uint8_t getCurrentSnapshot() const noexcept = 0;

    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void parameterControlPressed(uint32_t index) = 0;
    virtual void parameterControlReleased(uint32_t index) = 0;
    virtual void parameterControlModified(uint32_t index, float value) = 0;

    virtual void snapshotButtonClicked(SnapshotButton button) = 0;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
