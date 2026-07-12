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

bool LibreAudioUI::isParameterOutputOrTrigger(const uint32_t i)
{
    return
        i >= kParametersMainStart ? isFaustParameterOutputOrTrigger(kFaustParameters[i - kParametersMainStart]) :
        i >= kParametersOutputStart ? isFaustParameterOutputOrTrigger(kFaustParametersOut[i - kParametersOutputStart + kCommonIOParameters]) :
        i >= kParametersInputStart ? isFaustParameterOutputOrTrigger(kFaustParametersIn[i - kParametersInputStart]) :
        false;
}

// TODO convert common IO to C++
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersIn = common_input::getFaustParameters();
const std::vector<FaustParameter>& LibreAudioUI::kFaustParametersOut = common_output::getFaustParameters();

// --------------------------------------------------------------------------------------------------------------------

static std::vector<const char*> createParameterSymbols()
{
    static std::vector<const char*> symbols;
    symbols.reserve(kParametersMainStart + getFaustParameters().size());

    for (uint32_t i = 0; i < kCommonParameterCount; ++i)
    {
        switch (static_cast<CommonParameters>(i))
        {
        case kCommonParameterBypass:
        case kCommonParameterReset:
            symbols.push_back(nullptr);
            break;
       #if LIBREAUDIO_WANT_DRYWET
        case kCommonParameterDryWet:
            symbols.push_back("dry_wet");
            break;
       #endif
        case kCommonParameterCount:
            break;
        }
    }

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        symbols.push_back(LibreAudioUI::kFaustParametersIn[i].symbol);

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
        symbols.push_back(LibreAudioUI::kFaustParametersOut[i].symbol);

    for (uint32_t i = 0, size = LibreAudioUI::kFaustParameters.size(); i < size; ++i)
        symbols.push_back(LibreAudioUI::kFaustParameters[i].symbol);

    return symbols;
}

const std::vector<const char*>& LibreAudioUI::kParameterSymbols = createParameterSymbols();

// --------------------------------------------------------------------------------------------------------------------

static float* createParameterValues(const uint32_t paramCount)
{
    float* const values = new float[paramCount];

    initCommonParameterValuesToDefault(values);

    for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        values[kParametersInputStart + i] = LibreAudioUI::kFaustParametersIn[i].init;

    for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
        values[kParametersOutputStart + i - kCommonIOParameters] = LibreAudioUI::kFaustParametersOut[i].init;

    for (uint32_t i = 0, size = LibreAudioUI::kFaustParameters.size(); i < size; ++i)
        values[kParametersMainStart + i] = LibreAudioUI::kFaustParameters[i].init;

    return values;
}

// --------------------------------------------------------------------------------------------------------------------

LibreAudioUI::LibreAudioUI()
    : UI(),
      kParameterCount(kParametersMainStart  + kFaustParameters.size()),
      fParameterValues(createParameterValues(kParameterCount)),
      fParameterValuesWhenActivated(new float[kParameterCount]),
      fSnapshots(kNumSnapshots, kParameterCount, fParameterValues, this)
{
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

    std::memcpy(fParameterValuesWhenActivated, fParameterValues, sizeof(float) * kParameterCount);
}

LibreAudioUI::~LibreAudioUI()
{
    delete[] fParameterValues;
    delete[] fParameterValuesWhenActivated;
}

// --------------------------------------------------------------------------------------------------------------------
// DSP/Plugin Callbacks

void LibreAudioUI::parameterChanged(const uint32_t index, const float value)
{
    fParameterValues[index] = value;

    if (! isParameterOutputOrTrigger(index))
        fSnapshots.updateParameterValue(index, value, value);
}

void LibreAudioUI::stateChanged(const char* const key, const char* const value)
{
    if (std::strcmp(key, kStateKeys[kStateCurrentSnapshot]) == 0)
    {
        DISTRHO_SAFE_ASSERT_RETURN(value[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value[1] == '\0',);

        const uint8_t snapshot = value[0] - 'A';
        DISTRHO_SAFE_ASSERT_INT_RETURN(snapshot < kNumSnapshots, snapshot,);

        fSnapshots.restoreCurrentAndPrevious(snapshot);
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

        // d_stdout("loading snapshot %u | %s", snapshot, value);

        float* const parameterValues = new float[kParameterCount];
        std::memcpy(parameterValues, fParameterValues, sizeof(float) * kParameterCount);

        nlohmann::json j;

        try {
            j = nlohmann::json::parse(value);
        } DISTRHO_SAFE_EXCEPTION("failed to unserialize snapshot");

        try {
            const nlohmann::json& jparameters = j.at("parameters");

           #if LIBREAUDIO_WANT_DRYWET
            try {
                parameterValues[kCommonParameterDryWet] = jparameters.at("dry_wet").get<float>();
            } catch(...) {
                parameterValues[kCommonParameterDryWet] = fParameterValues[kCommonParameterDryWet];
            }
           #endif

            for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
            {
                const FaustParameter& param = kFaustParametersIn[i];

                if (isFaustParameterOutputOrTrigger(param))
                    continue;

                try {
                    parameterValues[kParametersInputStart + i] = jparameters.at(param.symbol).get<float>();
                } catch(...) {
                    parameterValues[kParametersInputStart + i] = fParameterValues[kParametersInputStart + i];
                }
            }

            for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
            {
                const FaustParameter& param = kFaustParametersOut[i];

                if (isFaustParameterOutputOrTrigger(param))
                    continue;

                try {
                    parameterValues[kParametersOutputStart + i - kCommonIOParameters] = jparameters.at(param.symbol).get<float>();
                } catch(...) {
                    parameterValues[kParametersOutputStart + i - kCommonIOParameters] = fParameterValues[kParametersOutputStart + i - kCommonIOParameters];
                }
            }

            for (uint32_t i = 0, size = kFaustParameters.size(); i < size; ++i)
            {
                const FaustParameter& param = kFaustParameters[i];

                if (isFaustParameterOutputOrTrigger(param))
                    continue;

                try {
                    parameterValues[kParametersMainStart + i] = jparameters.at(param.symbol).get<float>();
                } catch(...) {
                    parameterValues[kParametersMainStart + i] = fParameterValues[kParametersMainStart + i];
                }
            }
        } DISTRHO_SAFE_EXCEPTION("failed to unserialize snapshot parameters");

        LibreAudioUndoRedo::Actions actions;
        try {
            const nlohmann::json& jundoredo = j.at("undo/redo");

            const nlohmann::json& jactions = jundoredo.at("actions");

            const auto findSymbol = [](const std::string& symbol){
                for (uint32_t i = 0, size = kParameterSymbols.size(); i < size; ++i)
                    if (kParameterSymbols[i] != nullptr && symbol == kParameterSymbols[i])
                        return i;
                d_stderr2("invalid symbol %s", symbol.c_str());
                return UINT32_MAX;
            };

            for (const auto& jaction : jactions)
            {
                LibreAudioUndoRedo::Action action;
                for (const auto& jparameter : jaction)
                {
                    if (jparameter.contains("symbol") && jparameter.contains("value"))
                    {
                        const uint32_t index = findSymbol(jparameter.at("symbol").get<std::string>());

                        if (index != UINT32_MAX)
                            action.push_back({
                                index,
                                static_cast<float>(jparameter.at("value").get<double>()),
                            });
                    }
                }

                actions.data.push_back(action);
            }

            actions.position = jundoredo.at("position").get<int>();
        } catch(...) {
            d_stderr2("exception when restoring actions");
            actions = {};
        }

        fSnapshots.restoreSnapshotData(snapshot, parameterValues, std::move(actions));

        delete[] parameterValues;
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
        const bool canUndo = fSnapshots.canUndo();
        const bool canRedo = fSnapshots.canRedo();

        ImGui::SeparatorText("Undo / Redo");
        ImGui::BeginGroup();

        if (! canUndo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Undo"))
            fSnapshots.undo();

        if (! canUndo)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (! canRedo)
            ImGui::BeginDisabled();

        if (ImGui::Button("Redo"))
            fSnapshots.redo();

        if (! canRedo)
            ImGui::EndDisabled();

        ImGui::EndGroup();
    }

    {
        const bool copyingSnapshot = fCopyingSnapshot;
        const uint8_t currentSnapshot = fSnapshots.getCurrent();

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

// bool LibreAudioUI::onMouse(const MouseEvent& ev)
// {
//     if (ev.press)
//         fLastMouseReleaseTime = 0;
//     else
//         fLastMouseReleaseTime = d_gettime_ms() ?: 1;
//
//     return UI::onMouse(ev);
// }

void LibreAudioUI::uiIdle()
{
    fSnapshots.idle();
    // if (fLastMouseReleaseTime != 0 && fLastMouseReleaseTime - d_gettime_ms() >= 250)
    // {
    //     fLastMouseReleaseTime = 0;
    //     saveCurrentSnapshot();
    // }
}

// ----------------------------------------------------------------------------------------------------------------
// Other Callbacks

void LibreAudioUI::snapshotDataToSave(const uint32_t snapshot,
                                      const float* const parameterValues,
                                      const LibreAudioUndoRedo::Actions& undoRedoActions)
{
    std::string value = "{}";

    try {
        nlohmann::json j;

        // parameter value
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

        // undo/redo actions
        {
            nlohmann::json& jundoredo = j["undo/redo"] = nlohmann::json::object();

            jundoredo["position"] = static_cast<int>(undoRedoActions.position);

            nlohmann::json& jactions = jundoredo["actions"] = nlohmann::json::array();
            for (const LibreAudioUndoRedo::Action& action : undoRedoActions.data)
            {
                nlohmann::json jaction = nlohmann::json::array();

                for (const LibreAudioUndoRedo::Parameter& parameter : action)
                {
                    if (const char* const symbol = kParameterSymbols[parameter.index])
                    {
                        jaction.push_back(nlohmann::json::object({
                            { "symbol", symbol },
                            { "value", parameter.value },
                        }));
                    }
                }

                jactions.push_back(jaction);
            }
        }

        value = j.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    } DISTRHO_SAFE_EXCEPTION("failed to serialize snapshot");

    // d_stdout("saving snapshot %u | %s", snapshot, value.c_str());

    setState(kStateKeys[kStateSnapshotValuesA + snapshot], value.c_str());
}

void LibreAudioUI::snapshotParametersChanged(const float* const parameterValues)
{
    for (uint32_t i = 0; i < kParameterCount; ++i)
    {
        if (isParameterOutputOrTrigger(i))
            continue;
        if (d_isEqual(fParameterValues[i], parameterValues[i]))
            continue;

        fParameterValues[i] = parameterValues[i];

        editParameter(i, true);
        setParameterValue(i, fParameterValues[i]);
        editParameter(i, false);
    }
}

void LibreAudioUI::snapshotParameterChanged(const uint32_t parameterIndex, const float parameterValue)
{
    fParameterValues[parameterIndex] = parameterValue;

    editParameter(parameterIndex, true);
    setParameterValue(parameterIndex, parameterValue);
    editParameter(parameterIndex, false);
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
        fParameterValuesWhenActivated[index] = oldvalue;
        editParameter(index, true);
    }

    if (modified)
    {
        setParameterValue(index, *valueptr);
    }

    if (ImGui::IsItemDeactivated())
    {
        fSnapshots.updateParameterValue(index, fParameterValues[index], fParameterValuesWhenActivated[index]);
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

void LibreAudioUI::snapshotButtonClicked(uint8_t snapshot)
{
    // nothing to do if current snapshot clicked and there is no previous
    if (fSnapshots.getCurrent() == snapshot && fSnapshots.getPrevious() == snapshot)
        return;

    // special case for copy & pasting snapshot
    if (fCopyingSnapshot)
    {
        fCopyingSnapshot = false;
        fSnapshots.copyTo(snapshot);
        return;
    }

    // clicked new snapshot, load it
    if (fSnapshots.getCurrent() != snapshot)
        fSnapshots.load(snapshot);

    // clicked current snapshot, load previous one
    else
        fSnapshots.load(fSnapshots.getPrevious());

    // fUndoRedo.clear();

    // set state of active/current snapshot (index)
    const char snapshotStr[] = { static_cast<char>('A' + snapshot), '\0' };
    setState(kStateKeys[kStateCurrentSnapshot], snapshotStr);
}

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
