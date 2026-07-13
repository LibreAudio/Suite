// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

// #include "extra/Time.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

LibreAudioUI::LibreAudioUI()
    : LibreAudioBaseUI()
{
    // caching strings for display
    fParameterLabels.resize(kParameterCount);
    fParameterRenders.resize(kParameterCount);

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersIn[i];

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

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersOut[i];

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
        const FaustParameter& param = kFaustParameters[i];

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
        const bool canUndo = LibreAudioBaseUI::canUndo();
        const bool canRedo = LibreAudioBaseUI::canRedo();

        ImGui::SeparatorText("Undo / Redo");
        ImGui::BeginGroup();

        if (! canUndo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Undo"))
            undo();

        if (! canUndo)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (! canRedo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Redo"))
            redo();

        if (! canRedo)
            ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    {
        const bool copyingSnapshot = isCopyingSnapshot();
        const uint8_t currentSnapshot = getCurrentSnapshot();

        ImGui::SeparatorText("Snapshots");
        ImGui::BeginGroup();

        ImGui::Spacing();

        if (copyingSnapshot ? ImGui::SmallButton("(COPY)") : ImGui::Button("(COPY)"))
            snapshotButtonClicked(kSnapshotButtonCopy);

        ImGui::SameLine();

        ImGui::Spacing();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 0)
            ImGui::BeginDisabled();

        if (currentSnapshot == 0 ? ImGui::SmallButton("A") :  ImGui::Button("A"))
            snapshotButtonClicked(kSnapshotButtonA);

        if (copyingSnapshot && currentSnapshot == 0)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 1)
            ImGui::BeginDisabled();

        if (currentSnapshot == 1 ? ImGui::SmallButton("B") :  ImGui::Button("B"))
            snapshotButtonClicked(kSnapshotButtonB);

        if (copyingSnapshot && currentSnapshot == 1)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 2)
            ImGui::BeginDisabled();

        if (currentSnapshot == 2 ? ImGui::SmallButton("C") :  ImGui::Button("C"))
            snapshotButtonClicked(kSnapshotButtonC);

        if (copyingSnapshot && currentSnapshot == 2)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 3)
            ImGui::BeginDisabled();

        if (currentSnapshot == 3 ? ImGui::SmallButton("D") :  ImGui::Button("D"))
            snapshotButtonClicked(kSnapshotButtonD);

        if (copyingSnapshot && currentSnapshot == 3)
            ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    {
        ImGui::SeparatorText("Input");
        ImGui::BeginGroup();

        for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        {
            const FaustParameter& param = kFaustParametersIn[i];

            if (param.isOutput)
                continue;

            displaySlider(param, kParametersInputStart + i);
        }

        ImGui::BeginDisabled();

        for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        {
            const FaustParameter& param = kFaustParametersIn[i];

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
            const FaustParameter& param = kFaustParametersOut[i];

            if (param.isOutput)
                continue;

            displaySlider(param, kParametersOutputStart + i - kCommonIOParameters);
        }

        ImGui::BeginDisabled();

        for (uint32_t i = kCommonIOParameters, size = kFaustParametersOut.size(); i < size; ++i)
        {
            const FaustParameter& param = kFaustParametersOut[i];

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
            const FaustParameter& param = kFaustParameters[i];

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
            const FaustParameter& param = kFaustParameters[i];

            if (! param.isOutput)
                continue;

            displayMeter(param, kParametersMainStart + i);
        }

        ImGui::EndGroup();
        ImGui::EndDisabled();
    }

    ImGui::End();
}

// bool LibreAudioUI::onMouse(const MouseEvent& ev)
// {
//     if (ev.press)
//         fLastMouseReleaseTime = 0;
//     else
//         fLastMouseReleaseTime = d_gettime_ms() ?: 1;
//
//     return UI::onMouse(ev);
// }

// void LibreAudioUI::uiIdle()
// {
//     LibreAudioBaseUI::uiIdle();
//
//     if (fLastMouseReleaseTime != 0 && fLastMouseReleaseTime - d_gettime_ms() >= 250)
//     {
//         fLastMouseReleaseTime = 0;
//         saveCurrentSnapshot();
//     }
// }

// --------------------------------------------------------------------------------------------------------------------

void LibreAudioUI::displaySlider(const FaustParameter& param, const uint32_t index)
{
    float value = fParameterValuesRef[index];
    bool modified;

    if (param.isBoolean)
    {
        bool bvalue = value > (param.max - param.min) * 0.5f;
        modified = ImGui::Checkbox(fParameterLabels[index].c_str(), &bvalue);

        if (modified)
            value = bvalue ? param.max : param.min;
    }
    else
    {
        modified = ImGui::SliderFloat(fParameterLabels[index].c_str(),
                                      &value,
                                      param.min,
                                      param.max,
                                      fParameterRenders[index].c_str(),
                                      param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0);
    }

    if (ImGui::IsItemActivated())
        parameterControlPressed(index);

    if (modified)
        parameterControlModified(index, value);

    if (ImGui::IsItemDeactivated())
        parameterControlReleased(index);
}

void LibreAudioUI::displayMeter(const FaustParameter& param, const uint32_t index)
{
    float value = fParameterValuesRef[index];
    ImGui::SliderFloat(fParameterLabels[index].c_str(),
                       &value,
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
