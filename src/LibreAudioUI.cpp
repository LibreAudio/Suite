// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioUI.hpp"

#include "extra/Time.hpp"

#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"
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

bool LibreAudioUI::isParameterOutputOrTrigger(const uint32_t i)
{
    return
        i >= kParametersMainStart ? isFaustParameterOutputOrTrigger(kFaustParameters[i - kParametersMainStart]) :
        i >= kParametersOutputStart ? isFaustParameterOutputOrTrigger(kFaustParametersOut[i - kParametersOutputStart + kCommonIOParameters]) :
        i >= kParametersInputStart ? isFaustParameterOutputOrTrigger(kFaustParametersIn[i - kParametersInputStart]) :
        false;
}

void LibreAudioUI::serializeParameterValues(nlohmann::json& j, const float* const parameterValues)
{
    nlohmann::json& jparameters = j["parameters"] = nlohmann::json::object();

   #if LIBREAUDIO_WANT_DRYWET
    jparameters["dry_wet"] = parameterValues[kCommonParameterDryWet];
   #endif

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersIn[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        jparameters[param.symbol] = parameterValues[kParametersInputStart + i];
    }

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersOut[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        jparameters[param.symbol] = parameterValues[kParametersOutputStart + i - kCommonIOParameters];
    }

    for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
    {
        const FaustParameter& param = kFaustParameters[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        jparameters[param.symbol] = parameterValues[kParametersMainStart + i];
    }
}

void LibreAudioUI::unserializeParameterValues(const nlohmann::json& j, float* const parameterValues)
{
    const nlohmann::json& jparameters = j.at("parameters");

   #if LIBREAUDIO_WANT_DRYWET
    parameterValues[kCommonParameterDryWet] = jparameters.at("dry_wet").get<float>();
   #endif

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersIn[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersInputStart + i] = jparameters.at(param.symbol).get<float>();
        } DISTRHO_SAFE_EXCEPTION("Missing input property");
    }

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
    {
        const FaustParameter& param = kFaustParametersOut[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersOutputStart + i - kCommonIOParameters] = jparameters.at(param.symbol).get<float>();
        } DISTRHO_SAFE_EXCEPTION("Missing output property");
    }

    for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
    {
        const FaustParameter& param = kFaustParameters[i];

        if (isFaustParameterOutputOrTrigger(param))
            continue;

        try {
            parameterValues[kParametersMainStart + i] = jparameters.at(param.symbol).get<float>();
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
      fParameterValues(new float[kParameterCount]),
      fParameterValuesABCD(new float*[kNumSnapshots]),
      fUndoRedo(this)
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

        fParameterValues[kParametersInputStart + i] = param.init;
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

        fParameterValues[kParametersOutputStart + i - kCommonIOParameters] = param.init;
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

        fParameterValues[kParametersMainStart + i] = param.init;
    }

    for (uint8_t i = 0; i < kNumSnapshots; ++i)
    {
        fParameterValuesABCD[i] = new float[kParameterCount];
        std::memcpy(fParameterValuesABCD[i], fParameterValues, sizeof(float) * kParameterCount);
    }
}

LibreAudioUI::~LibreAudioUI()
{
    for (uint8_t i = 0; i < kNumSnapshots; ++i)
        delete[] fParameterValuesABCD[i];

    delete[] fParameterValues;
    delete[] fParameterValuesABCD;
}

// --------------------------------------------------------------------------------------------------------------------
// DSP/Plugin Callbacks

void LibreAudioUI::parameterChanged(const uint32_t index, const float value)
{
    fParameterValues[index] = value;
}

void LibreAudioUI::stateChanged(const char* const key, const char* const value)
{
    if (std::strcmp(key, kStateKeys[kStateCurrentSnapshot]) == 0)
    {
        DISTRHO_SAFE_ASSERT_RETURN(value[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value[1] == '\0',);

        const uint8_t snapshot = value[0] - 'A';
        DISTRHO_SAFE_ASSERT_INT_RETURN(snapshot < kNumSnapshots, snapshot,);

        fCurrentSnapshot = fPreviousSnapshot = snapshot;
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

        const uint8_t snapshot = snapshotKey[0] -'a';
        DISTRHO_SAFE_ASSERT_INT_RETURN(snapshot < kNumSnapshots, snapshot,);

        try {
            unserializeParameterValues(nlohmann::json::parse(value), fParameterValuesABCD[snapshot]);
        } DISTRHO_SAFE_EXCEPTION("failed to unserialize snapshot");
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
        const bool canUndo = fUndoRedo.canUndo();
        const bool canRedo = fUndoRedo.canRedo();

        ImGui::SeparatorText("Undo / Redo");
        ImGui::BeginGroup();

        if (! canUndo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Undo"))
            fUndoRedo.undo();

        if (! canUndo)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (! canRedo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Redo"))
            fUndoRedo.redo();

        if (! canRedo)
            ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    {
        const bool copyingSnapshot = fCopyingSnapshot;
        const uint8_t currentSnapshot = fCurrentSnapshot;

        ImGui::SeparatorText("Snapshots");
        ImGui::BeginGroup();

        ImGui::Spacing();

        if (copyingSnapshot ? ImGui::SmallButton("(COPY)") : ImGui::Button("(COPY)"))
        {
            fCopyingSnapshot = !copyingSnapshot;
        }

        ImGui::SameLine();

        ImGui::Spacing();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 0)
            ImGui::BeginDisabled();

        if (currentSnapshot == 0 ? ImGui::SmallButton("A") :  ImGui::Button("A"))
            snapshotButtonClicked(0);

        if (copyingSnapshot && currentSnapshot == 0)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 1)
            ImGui::BeginDisabled();

        if (currentSnapshot == 1 ? ImGui::SmallButton("B") :  ImGui::Button("B"))
            snapshotButtonClicked(1);

        if (copyingSnapshot && currentSnapshot == 1)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 2)
            ImGui::BeginDisabled();

        if (currentSnapshot == 2 ? ImGui::SmallButton("C") :  ImGui::Button("C"))
            snapshotButtonClicked(2);

        if (copyingSnapshot && currentSnapshot == 2)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (copyingSnapshot && currentSnapshot == 3)
            ImGui::BeginDisabled();

        if (currentSnapshot == 3 ? ImGui::SmallButton("D") :  ImGui::Button("D"))
            snapshotButtonClicked(3);

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

bool LibreAudioUI::onMouse(const MouseEvent& ev)
{
    if (ev.press)
        fLastMouseReleaseTime = 0;
    else
        fLastMouseReleaseTime = d_gettime_ms() ?: 1;

    return UI::onMouse(ev);
}

void LibreAudioUI::uiIdle()
{
    if (fLastMouseReleaseTime != 0 && fLastMouseReleaseTime - d_gettime_ms() >= 250)
    {
        fLastMouseReleaseTime = 0;
        saveCurrentSnapshot();
    }
}

// ----------------------------------------------------------------------------------------------------------------
// Other Callbacks

void LibreAudioUI::undoRedoParameterChanged(const uint32_t index, const float value)
{
    fParameterValues[index] = value;

    editParameter(index, true);
    setParameterValue(index, value);
    editParameter(index, false);
}

// --------------------------------------------------------------------------------------------------------------------

void LibreAudioUI::displaySlider(const FaustParameter& param, const uint32_t index)
{
    float* const valueptr = fParameterValues + index;
    float const oldvalue = *valueptr;
    bool modified;

    if (param.isBoolean)
    {
        bool bvalue = *valueptr > (param.max - param.min) * 0.5f;
        modified = ImGui::Checkbox(fParameterLabels[index].c_str(), &bvalue);

        if (modified)
            *valueptr = bvalue ? param.max : param.min;
    }
    else
    {
        modified = ImGui::SliderFloat(fParameterLabels[index].c_str(),
                                      valueptr,
                                      param.min,
                                      param.max,
                                      fParameterRenders[index].c_str(),
                                      param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0);
    }

    if (ImGui::IsItemActivated())
    {
        fUndoRedo.pushIfFirst({ index, oldvalue });

        editParameter(index, true);
    }

    if (modified)
    {
        setParameterValue(index, *valueptr);
    }

    if (ImGui::IsItemDeactivated())
    {
        if (d_isNotEqual(fParameterValues[index], fParameterValuesABCD[fCurrentSnapshot][index]))
            fUndoRedo.push({ index, fParameterValues[index] });

        editParameter(index, false);
    }
}

void LibreAudioUI::displayMeter(const FaustParameter& param, const uint32_t index)
{
    ImGui::SliderFloat(fParameterLabels[index].c_str(),
                       fParameterValues + index,
                       param.min,
                       param.max,
                       fParameterRenders[index].c_str(),
                       ImGuiSliderFlags_NoInput | (param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0));
}

void LibreAudioUI::saveCurrentSnapshot()
{
    std::memcpy(fParameterValuesABCD[fCurrentSnapshot], fParameterValues, sizeof(float) * kParameterCount);

    std::string value = "{}";
    try {
        nlohmann::json j;
        serializeParameterValues(j, fParameterValues);
        value = j.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    } DISTRHO_SAFE_EXCEPTION("failed to serialize snapshot");

    setState(kStateKeys[kStateSnapshotValuesA + fCurrentSnapshot], value.c_str());
}

void LibreAudioUI::snapshotButtonClicked(uint8_t snapshot)
{
    // nothing to do if current snapshot clicked and there is no previous
    if (fCurrentSnapshot == snapshot && fPreviousSnapshot == snapshot)
        return;

    // special case for copy & pasting snapshot
    if (fCopyingSnapshot)
    {
        fCopyingSnapshot = false;
        DISTRHO_SAFE_ASSERT_RETURN(fCurrentSnapshot != snapshot,);

        std::memcpy(fParameterValuesABCD[fCurrentSnapshot], fParameterValues, sizeof(float) * kParameterCount);
        std::memcpy(fParameterValuesABCD[snapshot], fParameterValues, sizeof(float) * kParameterCount);

        // set state of current snapshot (values) before changing to new one
        saveCurrentSnapshot();

        // set new snapshot (index)
        fPreviousSnapshot = fCurrentSnapshot;
        fCurrentSnapshot = snapshot;

        // set state of new snapshot (values) too
        saveCurrentSnapshot();
        return;
    }

    // set state of current snapshot (values) before changing to new one
    saveCurrentSnapshot();

    // clicked new snapshot, load it
    if (fCurrentSnapshot != snapshot)
    {
        fPreviousSnapshot = fCurrentSnapshot;
        fCurrentSnapshot = snapshot;
    }
    // clicked current snapshot, load previous one
    else
    {
        std::swap(fPreviousSnapshot, fCurrentSnapshot);
        snapshot = fCurrentSnapshot;
    }

    // set state of active/current snapshot (index)
    const char snapshotStr[] = { static_cast<char>('A' + snapshot), '\0' };
    setState(kStateKeys[kStateCurrentSnapshot], snapshotStr);

    const float* const snapshotValues = fParameterValuesABCD[snapshot];
    for (uint32_t i = 0; i < kParameterCount; ++i)
    {
        if (isParameterOutputOrTrigger(i))
            continue;
        if (d_isEqual(fParameterValues[i], snapshotValues[i]))
            continue;

        fParameterValues[i] = snapshotValues[i];

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
