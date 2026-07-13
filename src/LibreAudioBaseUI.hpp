// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUI.hpp"
#include "FaustParameters.hpp"
#include "LibreAudioSnapshots.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBaseUI : public UI,
                         private LibreAudioSnapshots::Callback
{
public:
    LibreAudioBaseUI();
    ~LibreAudioBaseUI() override;

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // static metadata

    static const std::vector<FaustParameter>& kFaustParameters;
    static const std::vector<FaustParameter>& kFaustParametersIn;
    static const std::vector<FaustParameter>& kFaustParametersOut;
    static const std::vector<const char*>& kParameterSymbols;

    static bool isParameterOutputOrTrigger(uint32_t index);

    // ----------------------------------------------------------------------------------------------------------------
    // protected data

    const uint32_t kParameterCount;

    bool canUndo() const { return fSnapshots.canUndo(); }
    bool canRedo() const { return fSnapshots.canRedo(); }
    bool isCopyingSnapshot() const { return fCopyingSnapshot; }
    uint8_t getCurrentSnapshot() const { return fSnapshots.getCurrent(); }

    void undo() { fSnapshots.undo(); }
    void redo() { fSnapshots.redo(); }

    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(const uint32_t index, const float value) final;

    void stateChanged(const char* key, const char* value) final;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void parameterControlPressed(uint32_t index);
    void parameterControlReleased(uint32_t index);
    void parameterControlModified(uint32_t index, float value);

    enum SnapshotButton {
        kSnapshotButtonCopy,
        kSnapshotButtonA,
        kSnapshotButtonB,
        kSnapshotButtonC,
        kSnapshotButtonD,
    };

    void snapshotButtonClicked(SnapshotButton button);

    void uiIdle() override;

private:
    // ----------------------------------------------------------------------------------------------------------------
    // Other Callbacks

    void snapshotDataToSave(uint32_t snapshot,
                            const float* parameterValues,
                            const LibreAudioUndoRedo::Actions& undoRedoActions) final;
    void snapshotParameterChanged(uint32_t parameterIndex, float parameterValue) final;
    void snapshotParametersChanged(const float* parameterValues) final;

    // ----------------------------------------------------------------------------------------------------------------
    // private data

    float* const fParameterValues;
    float* const fParameterValuesWhenActivated;

    LibreAudioSnapshots fSnapshots;
    bool fCopyingSnapshot = false;

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // shared data

    const float* const fParameterValuesRef;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioBaseUI)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
