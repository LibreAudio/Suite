// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUI.hpp"
#include "LibreAudioSnapshots.hpp"
#include "LibreAudioUndoRedo.hpp"
#include "FaustParameters.hpp"
#include "nlohmann/json_fwd.hpp"

#include <string>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public UI,
                     private LibreAudioSnapshots::Callback
                     // private LibreAudioUndoRedo::Callback
{
public:
    LibreAudioUI();
    ~LibreAudioUI() override;

    static const std::vector<FaustParameter>& kFaustParameters;
    static const std::vector<FaustParameter>& kFaustParametersIn;
    static const std::vector<FaustParameter>& kFaustParametersOut;
    static const std::vector<const char*>& kParameterSymbols;

protected:
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

   /**
      ImGui specific onDisplay function.
    */
    void onImGuiDisplay() final;

    // bool onMouse(const MouseEvent& ev) final;
    void uiIdle() final;

    // ----------------------------------------------------------------------------------------------------------------
    // Other Callbacks

    void snapshotDataToSave(uint32_t snapshot,
                            const float* parameterValues,
                            const LibreAudioUndoRedo::Actions& undoRedoActions) final;
    void snapshotParameterChanged(uint32_t parameterIndex, float parameterValue) final;
    void snapshotParametersChanged(const float* parameterValues) final;

private:
    static bool isParameterOutputOrTrigger(uint32_t index);

    const uint32_t kParameterCount;
    float* const fParameterValues;
    float* const fParameterValuesWhenActivated;

    LibreAudioSnapshots fSnapshots;

    std::vector<std::string> fParameterLabels;
    std::vector<std::string> fParameterRenders;

    bool fCopyingSnapshot = false;

    // uint32_t fLastMouseReleaseTime = 0;

    void displayMeter(const FaustParameter &param, uint32_t index);
    void displaySlider(const FaustParameter &param, uint32_t index);
    void snapshotButtonClicked(uint8_t snapshot);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
