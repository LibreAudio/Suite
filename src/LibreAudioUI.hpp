// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUI.hpp"

#include "LibreAudioParameters.hpp"

#include <vector>

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public UI
{
    const std::vector<FaustParameter>& kFaustParameters;
    const uint32_t kParameterCount;
    float* const fParameterValues;

public:
    LibreAudioUI(const std::vector<FaustParameter>& parameters)
        : UI(),
          kFaustParameters(parameters),
          kParameterCount(kParametersMainStart  + parameters.size()),
          fParameterValues(new float[kParameterCount])
    {
        // set minimum size
        const double scaleFactor = getScaleFactor();
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);
    }

    ~LibreAudioUI() override
    {
        delete[] fParameterValues;
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(const uint32_t index, const float value) override
    {
        fParameterValues[index] = value;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
   /**
    * @brief ${...}
    *
    */
   void onImGuiDisplay() override
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

        constexpr int flags = ImGuiWindowFlags_NoSavedSettings
                            | ImGuiWindowFlags_NoTitleBar
                            | ImGuiWindowFlags_NoResize
                            | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("LibreAudio", nullptr, flags);

        if (ImGui::Button("Undo"))
        {
        }

        if (ImGui::Button("Redo"))
        {
        }

        for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParameters[i];

            if (param.isOutput)
                continue;

            float* const valueptr = &fParameterValues[kParametersMainStart + i];

            // if (param.isBoolean && 0)
            // {
            //     if (ImGui::Checkbox(param.label, valueptr))
            //     {
            //         if (ImGui::IsItemActivated())
            //             editParameter(kParametersMainStart + i, true);
            //
            //         valueptr = ui->parameters[i].bvalue ? ui->parameters[i].max : ui->parameters[i].min;
            //         setParameterValue(kParametersMainStart + i, valueptr);
            //     }
            // }
            // else
            {
                if (ImGui::SliderFloat(param.label,
                                       valueptr,
                                       param.min,
                                       param.max,
                                       "%.3f",
                                       param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0))
                {
                    if (ImGui::IsItemActivated())
                        editParameter(kParametersMainStart + i, true);

                    setParameterValue(kParametersMainStart + i, *valueptr);
                }
            }

            if (ImGui::IsItemDeactivated())
                editParameter(kParametersMainStart + i, false);
        }

        ImGui::BeginDisabled();

        for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParameters[i];

            if (! param.isOutput)
                continue;

            ImGui::SliderFloat(param.label,
                               &fParameterValues[kParametersMainStart + i],
                               param.min,
                               param.max,
                               "%.3f",
                               ImGuiSliderFlags_NoInput | (param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0));
        }

        ImGui::EndDisabled();

        ImGui::End();
    }

    // ----------------------------------------------------------------------------------------------------------------

    void setState(const char* const key, const char* const value)
    {
        if (std::strcmp(key, kStateKeys[kStateUndoRedo]) == 0)
        {
            return;
        }

        if (std::strncmp(key, LIBREAUDIO_STATE_KEY_SNAPSHOT_PREFIX, std::strlen(LIBREAUDIO_STATE_KEY_SNAPSHOT_PREFIX)) == 0)
        {
            return;
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
