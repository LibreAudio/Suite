// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUI.hpp"

#include "FaustDSP.hpp"

#include "nlohmann/json_fwd.hpp"

#include <string>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public UI
{
public:
    LibreAudioUI();
    ~LibreAudioUI() override;

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

    bool onMouse(const MouseEvent& ev) final;
    void uiIdle() final;

private:
    static const std::vector<FaustParameter>& kFaustParameters;
    static const std::vector<FaustParameter>& kFaustParametersIn;
    static const std::vector<FaustParameter>& kFaustParametersOut;

    static bool isParameterOutputOrTrigger(uint32_t index);
    static void serializeParameterValues(nlohmann::json& j, const float* parameterValues);
    static void unserializeParameterValues(const nlohmann::json& j, float* parameterValues);

    const uint32_t kParameterCount;
    float* const fParameterValues;
    float* const fParameterValuesABCD[4];
    std::vector<std::string> fParameterLabels;
    std::vector<std::string> fParameterRenders;

    uint8_t fCurrentSnapshot = 0;
    uint8_t fPreviousSnapshot = 0;

    uint32_t fLastMouseReleaseTime = 0;

    void displayMeter(const FaustParameter &param, uint32_t index);
    void displaySlider(const FaustParameter &param, uint32_t index);
    void saveCurrentSnapshot();
    void snapshotButtonClicked(uint8_t snapshot);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
