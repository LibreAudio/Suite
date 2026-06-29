// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioUI.hpp"

#include "LibreAudioParameters.hpp"
#include "common_input-parameters.hpp"
#include "common_output-parameters.hpp"

#include "nlohmann/json.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#define constexprstr constexpr
#else
#define constexprstr
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

const std::vector<FaustParameter>& LibreAudioUI::kFaustParameters = getFaustParameters();

static constexpr bool isFaustParameterOutputOrTrigger(const FaustParameter& param)
{
    return param.isOutput || param.isTrigger;
}

bool LibreAudioUI::isParameterOutput(const uint32_t i)
{
    return
        i >= kParametersMainStart ? isFaustParameterOutputOrTrigger(kFaustParameters[i - kParametersMainStart]) :
        i >= kParametersOutputStart ? isFaustParameterOutputOrTrigger(kFaustParametersOut[i - kParametersOutputStart + kCommonIOParameters]) :
        i >= kParametersInputStart ? isFaustParameterOutputOrTrigger(kFaustParametersIn[i - kParametersInputStart]) :
        false;
}

void LibreAudioUI::serializeParameterValues(nlohmann::json& j, const float* const parameterValues)
{
   #if LIBREAUDIO_WANT_DRYWET
    j["dry_wet"] = parameterValues[kCommonParameterDryWet];
   #endif

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter &param = kFaustParametersIn[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        j[param.symbol] = parameterValues[kParametersInputStart + i];
    }

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
    {
        const FaustParameter &param = kFaustParametersOut[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        j[param.symbol] = parameterValues[kParametersOutputStart + i - kCommonIOParameters];
    }

    for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
    {
        const FaustParameter &param = kFaustParameters[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        j[param.symbol] = parameterValues[kParametersMainStart + i];
    }
}

void LibreAudioUI::unserializeParameterValues(const nlohmann::json& j, float* const parameterValues)
{
   #if LIBREAUDIO_WANT_DRYWET
    parameterValues[kCommonParameterDryWet] = j["dry_wet"];
   #endif

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter &param = kFaustParametersIn[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersInputStart + i] = j.at(param.symbol).get<float>();
        } DISTRHO_SAFE_EXCEPTION("Missing input property");
    }

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
    {
        const FaustParameter &param = kFaustParametersOut[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersOutputStart + i - kCommonIOParameters] = j.at(param.symbol).get<float>();
        } DISTRHO_SAFE_EXCEPTION("Missing output property");
    }

    for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
    {
        const FaustParameter &param = kFaustParameters[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersMainStart + i] = j.at(param.symbol).get<float>();
        } DISTRHO_SAFE_EXCEPTION("Missing main property");
    }
}

// TODO convert common IO to C++
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersIn = common_input::getFaustParameters();
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersOut = common_output::getFaustParameters();

// --------------------------------------------------------------------------------------------------------------------

LibreAudioUI::LibreAudioUI()
    : UI(),
      kParameterCount(kParametersMainStart  + kFaustParameters.size()),
      fParameterValuesABCD{
          new float[kParameterCount],
          new float[kParameterCount],
          new float[kParameterCount],
          new float[kParameterCount]
      }
{
    initCommonParameterValuesToDefault(fParameterValues);

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

        fParameterValues[kParametersInputStart + i] = param.init;
    }

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
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

        fParameterValues[kParametersOutputStart + i - kCommonIOParameters] = param.init;
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

        fParameterValues[kParametersMainStart + i] = param.init;
    }

    std::memcpy(fParameterValuesABCD[1], fParameterValues, sizeof(float) * kParameterCount);
    std::memcpy(fParameterValuesABCD[2], fParameterValues, sizeof(float) * kParameterCount);
    std::memcpy(fParameterValuesABCD[3], fParameterValues, sizeof(float) * kParameterCount);
}

LibreAudioUI::~LibreAudioUI()
{
    delete[] fParameterValuesABCD[0];
    delete[] fParameterValuesABCD[1];
    delete[] fParameterValuesABCD[2];
    delete[] fParameterValuesABCD[3];
}

// --------------------------------------------------------------------------------------------------------------------
// DSP/Plugin Callbacks

void LibreAudioUI::parameterChanged(const uint32_t index, const float value)
{
    if (isParameterOutput(index))
    {
        fParameterValuesABCD[0][index] = value;
        fParameterValuesABCD[1][index] = value;
        fParameterValuesABCD[2][index] = value;
        fParameterValuesABCD[3][index] = value;
    }
    else
    {
        fParameterValues[index] = value;
    }
}

void LibreAudioUI::stateChanged(const char* const key, const char* const value)
{
    if (std::strcmp(key, kStateKeys[kStateUndoRedo]) == 0)
    {
        return;
    }

    if (std::strcmp(key, kStateKeys[kStateSnapshot]) == 0)
    {
        DISTRHO_SAFE_ASSERT_RETURN(value[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value[1] == '\0',);

        const uint8_t snapshot = value[0];
        DISTRHO_SAFE_ASSERT_INT_RETURN(isValidSnapshot(snapshot), snapshot,);

        if (fCurrentSnapshot != snapshot)
        {
            const float* const previousValues = fParameterValues;
            fCurrentSnapshot = snapshot;
            fParameterValues = fParameterValuesABCD[snapshot - 'A'];

            snapshotValuesChanged(previousValues);
        }
        return;
    }

    constexprstr const size_t LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX_len =
        std::strlen(LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX);

    if (std::strncmp(key,
                     LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX,
                     LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX_len) == 0)
    {
        const char* const snapshotKey = key + LIBREAUDIO_STATE_KEY_SNAPSHOT_VALUES_PREFIX_len;
        DISTRHO_SAFE_ASSERT_RETURN(snapshotKey[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(snapshotKey[1] == '\0',);

        const uint8_t snapshot = snapshotKey[0];
        DISTRHO_SAFE_ASSERT_INT_RETURN(isValidSnapshot(snapshot), snapshot,);

        float* const previousValues = new float[kParameterCount];

        if (fCurrentSnapshot == snapshot)
            std::memcpy(previousValues, fParameterValues, sizeof(float) * kParameterCount);

        // nlohmann::json j;
        // TODO json decode from string
        // j << std::string(value);
        // unserializeParameterValues(j, fParameterValuesABCD[snapshot - 'A']);

        if (fCurrentSnapshot == snapshot)
            snapshotValuesChanged(previousValues);

        delete[] previousValues;
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
        ImGui::BeginDisabled();

        if (ImGui::Button("Undo"))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button("Redo"))
        {
        }

        ImGui::EndDisabled();
        ImGui::EndGroup();
    }

    {
        ImGui::SeparatorText("Snapshots");
        ImGui::BeginGroup();

        ImGui::BeginDisabled();

        if (ImGui::Button("X"))
        {
        }

        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::Spacing();

        ImGui::SameLine();

        if (fCurrentSnapshot == 'A' ? ImGui::SmallButton("A") :  ImGui::Button("A"))
        {
            setState(kStateKeys[kStateSnapshot], "A");
            stateChanged(kStateKeys[kStateSnapshot], "A");
        }

        ImGui::SameLine();

        if (fCurrentSnapshot == 'B' ? ImGui::SmallButton("B") :  ImGui::Button("B"))
        {
            setState(kStateKeys[kStateSnapshot], "B");
            stateChanged(kStateKeys[kStateSnapshot], "B");
        }

        ImGui::SameLine();

        if (fCurrentSnapshot == 'C' ? ImGui::SmallButton("C") :  ImGui::Button("C"))
        {
            setState(kStateKeys[kStateSnapshot], "C");
            stateChanged(kStateKeys[kStateSnapshot], "C");
        }

        ImGui::SameLine();

        if (fCurrentSnapshot == 'D' ? ImGui::SmallButton("D") :  ImGui::Button("D"))
        {
            setState(kStateKeys[kStateSnapshot], "D");
            stateChanged(kStateKeys[kStateSnapshot], "D");
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

void LibreAudioUI::snapshotValuesChanged(const float* const previousValues)
{
    for (uint32_t i = 0; i < kParameterCount; ++i)
    {
        if (d_isEqual(previousValues[i], fParameterValues[i]))
            continue;

        if (isParameterOutput(i))
            continue;

        editParameter(i, true);
        setParameterValue(i, fParameterValues[i]);
        editParameter(i, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
