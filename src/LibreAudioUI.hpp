// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoUI.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

template<int numFaustParameters>
class LibreAudioUI : public UI
{
    static constexpr const uint32_t kParameterCount = kParametersMainStart  + numFaustParameters;

    float fParameterValues[kParameterCount + 2] = {};

    const FaustParameters<numFaustParameters>& kFaustParameters;

public:
    LibreAudioUI(const FaustParameters<numFaustParameters>& parameters)
        : UI(),
          kFaustParameters(parameters)
    {
        // set minimum size
        const double scaleFactor = getScaleFactor();
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);
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

        for (int i = 0; i < numFaustParameters; ++i)
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

        for (int i = 0; i < numFaustParameters; ++i)
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

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
