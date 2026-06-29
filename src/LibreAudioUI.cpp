// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioUI.hpp"

#include "LibreAudioParameters.hpp"
#include "common_input-parameters.hpp"
#include "common_output-parameters.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

const std::vector<FaustParameter>& LibreAudioUI::kFaustParameters = getFaustParameters();

// TODO convert common IO to C++
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersIn = common_input::getFaustParameters();
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersOut = common_output::getFaustParameters();

// --------------------------------------------------------------------------------------------------------------------

LibreAudioUI::LibreAudioUI()
    : UI(),
      kParameterCount(kParametersMainStart  + kFaustParameters.size()),
      fParameterValues(new float[kParameterCount])
{
    // set minimum size
    const double scaleFactor = getScaleFactor();
    setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);

    // caching strings for display
    fParameterLabels.resize(kParameterCount);
    fParameterRenders.resize(kParameterCount);

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter &param = kFaustParametersIn[i];

        std::string& label = fParameterLabels[kParametersInputStart + i];
        label = param.label;
        label += "##";
        label += param.symbol;

        std::string& render = fParameterRenders[kParametersInputStart + i];
        render = param.isInteger ? "%d" : "%.2f";
        if (param.unit != nullptr)
        {
            render += " ";
            render += param.unit;
        }
    }

    for (uint32_t i = kCommonIOParameters, size = kFaustParametersOut.size(); i < size; ++i)
    {
        const FaustParameter &param = kFaustParametersOut[i];

        std::string& label = fParameterLabels[kParametersOutputStart + i - kCommonIOParameters];
        label = param.label;
        label += "##";
        label += param.symbol;

        std::string& render = fParameterRenders[kParametersOutputStart + i - kCommonIOParameters];
        render = param.isInteger ? "%d" : "%.2f";
        if (param.unit != nullptr)
        {
            render += " ";
            render += param.unit;
        }
    }

    for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
    {
        const FaustParameter &param = kFaustParameters[i];

        std::string& label = fParameterLabels[kParametersMainStart + i];
        label = param.label;
        label += "##";
        label += param.symbol;

        std::string& render = fParameterRenders[kParametersMainStart + i];
        render = param.isInteger ? "%d" : "%.2f";
        if (param.unit != nullptr)
        {
            render += " ";
            render += param.unit;
        }
    }
}

LibreAudioUI::~LibreAudioUI()
{
    delete[] fParameterValues;
}

// --------------------------------------------------------------------------------------------------------------------
// DSP/Plugin Callbacks

void LibreAudioUI::parameterChanged(const uint32_t index, const float value)
{
    fParameterValues[index] = value;
}

void LibreAudioUI::stateChanged(const char* const key, const char* const value)
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

// --------------------------------------------------------------------------------------------------------------------
// Widget Callbacks

void LibreAudioUI::onImGuiDisplay()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

    constexpr int flags = ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoTitleBar
                        | ImGuiWindowFlags_NoResize
                        | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("LibreAudio", nullptr, flags);

    {
        ImGui::SeparatorText("Undo / Redo");
        ImGui::BeginGroup();

        if (ImGui::Button("Undo"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button("Redo"))
        {
        }

        ImGui::EndGroup();
    }

    {
        ImGui::SeparatorText("Snapshots");
        ImGui::BeginGroup();

        if (ImGui::Button("X"))
        {
        }

        ImGui::SameLine();

        ImGui::Spacing();

        ImGui::SameLine();

        if (ImGui::Button("A"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button("B"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button("C"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button("D"))
        {
        }

        ImGui::EndGroup();
    }

    {
        ImGui::SeparatorText("Input");
        ImGui::BeginGroup();

        for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        {
            const FaustParameter &param = kFaustParametersIn[i];

            if (param.isOutput)
                continue;

            displaySlider(param, kParametersInputStart + i);
        }

        ImGui::BeginDisabled();

        for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        {
            const FaustParameter &param = kFaustParametersIn[i];

            if (! param.isOutput)
                continue;

            displayMeter(param, kParametersInputStart + i);
        }

        ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    {
        ImGui::SeparatorText("Output");
        ImGui::BeginGroup();

        for (uint32_t i = kCommonIOParameters, size = kFaustParametersOut.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParametersOut[i];

            if (param.isOutput)
                continue;

            displaySlider(param, kParametersOutputStart + i - kCommonIOParameters);
        }

        ImGui::BeginDisabled();

        for (uint32_t i = kCommonIOParameters, size = kFaustParametersOut.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParametersOut[i];

            if (! param.isOutput)
                continue;

            displayMeter(param, kParametersOutputStart + i - kCommonIOParameters);
        }

        ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    bool hasOutputs = false;

    {
        ImGui::SeparatorText("Parameters");
        ImGui::BeginGroup();

        for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParameters[i];

            if (param.isOutput)
            {
                hasOutputs = true;
                continue;
            }

            displaySlider(param, kParametersMainStart + i);
        }

        ImGui::EndGroup();
    }

    if (hasOutputs)
    {
        ImGui::SeparatorText("Meters / Outputs");
        ImGui::BeginGroup();
        ImGui::BeginDisabled();

        for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
        {
            const FaustParameter &param = kFaustParameters[i];

            if (! param.isOutput)
                continue;

            displayMeter(param, kParametersMainStart + i);
        }

        ImGui::EndGroup();
        ImGui::EndDisabled();
    }

    ImGui::End();
}

// --------------------------------------------------------------------------------------------------------------------

void LibreAudioUI::displaySlider(const FaustParameter &param, const uint32_t index)
{
    float* const valueptr = &fParameterValues[index];

    if (param.isBoolean)
    {
        bool bvalue = *valueptr > (param.max - param.min) * 0.5f;

        if (ImGui::Checkbox(fParameterLabels[index].c_str(), &bvalue))
        {
            if (ImGui::IsItemActivated())
                editParameter(index, true);

            *valueptr = bvalue ? param.max : param.min;
            setParameterValue(index, *valueptr);
        }
    }
    else
    {
        if (ImGui::SliderFloat(fParameterLabels[index].c_str(),
                                valueptr,
                                param.min,
                                param.max,
                                fParameterRenders[index].c_str(),
                                param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0))
        {
            if (ImGui::IsItemActivated())
                editParameter(index, true);

            setParameterValue(index, *valueptr);
        }
    }

    if (ImGui::IsItemDeactivated())
        editParameter(index, false);
}

void LibreAudioUI::displayMeter(const FaustParameter &param, const uint32_t index)
{
    ImGui::SliderFloat(fParameterLabels[index].c_str(),
                       &fParameterValues[index],
                       param.min,
                       param.max,
                       fParameterRenders[index].c_str(),
                       ImGuiSliderFlags_NoInput | (param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0));
}

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
