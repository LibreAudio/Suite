// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoPlugin.hpp"
#include "extra/ScopedDenormalDisable.hpp"
#include "extra/ValueSmoother.hpp"

#include "LibreAudioParameters.hpp"

#include "common_input.hpp"
#include "common_output.hpp"

#include <atomic>
#include <cassert>
#include <vector>

/* TODO
 * - convert common IO to C++
 */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioPlugin : public Plugin
{
   #ifdef LIBREAUDIO_BLOCK_SIZE
    static constexpr const uint32_t kInternalBlockSize = LIBREAUDIO_BLOCK_SIZE;
   #else
    static constexpr const uint32_t kInternalBlockSize = 32;
   #endif
    static constexpr const float kParameterSmoothingTime = 0.05f; // in seconds

    const std::vector<FaustParameter>& kFaustParameters;
    const uint32_t kParameterCount;

    float fCommonParameterValues[kCommonParameterCount];
    LinearValueSmoother fGlobalDryValue;
    LinearValueSmoother fGlobalWetValue;

    float* const fInternalBuffer = new float[kInternalBlockSize * 4];
    float* fCycleBuffer1[2] = {
        fInternalBuffer + kInternalBlockSize * 0,
        fInternalBuffer + kInternalBlockSize * 1,
    };
    float* fCycleBuffer2[2] = {
        fInternalBuffer + kInternalBlockSize * 2,
        fInternalBuffer + kInternalBlockSize * 3,
    };

   #if DISTRHO_PLUGIN_WANT_LATENCY
    float* fLatencyBuffer[2] = {
        new float[LIBREAUDIO_MAX_LATENCY_SAMPLES],
        new float[LIBREAUDIO_MAX_LATENCY_SAMPLES],
    };
    int32_t fLatencyReadPos = 0;
    int32_t fLatencyWritePos = 0;
    uint32_t fLastKnownLatency = 0;
   #endif

    // click-free mute/unmute status
    std::atomic<bool> fMuting { false };

    FaustDSP* const fMainDSP;
    FaustDSP* const fInputDSP = new common_input::common_input;
    FaustDSP* const fOutputDSP = new common_output::common_output;

public:
    LibreAudioPlugin(const std::vector<FaustParameter>& faustParameters, FaustDSP* const dsp)
        : Plugin(kParametersMainStart  + faustParameters.size(), 0, kStateCount),
          kFaustParameters(faustParameters),
          kParameterCount(kParametersMainStart  + faustParameters.size()),
          fMainDSP(dsp)
    {
        for (uint32_t i = 0; i < kCommonParameterCount; ++i)
        {
            switch (static_cast<CommonParameters>(i))
            {
            case kCommonParameterBypass:
            case kCommonParameterReset:
                fCommonParameterValues[i] = 0.f;
                break;
           #if LIBREAUDIO_WANT_DRYWET
            case kCommonParameterDryWet:
                fCommonParameterValues[i] = 50.f;
           #endif
                break;
            case kCommonParameterCount:
                break;
            }
        }

        const double sampleRate = getSampleRate();
        const int iSampleRate = d_roundToIntPositive(sampleRate);

        fGlobalDryValue.setSampleRate(sampleRate);
        fGlobalDryValue.setTimeConstant(kParameterSmoothingTime);
        fGlobalDryValue.setTargetValue(0.f);

        fGlobalWetValue.setSampleRate(sampleRate);
        fGlobalWetValue.setTimeConstant(kParameterSmoothingTime);
        fGlobalWetValue.setTargetValue(1.f);

        fMainDSP->init(iSampleRate);
        fInputDSP->init(iSampleRate);
        fOutputDSP->init(iSampleRate);

       #if DISTRHO_PLUGIN_WANT_LATENCY
        fMainDSP->compute(0, fCycleBuffer1, fCycleBuffer2);
        updateLatencyIfNeeded();
       #endif
    }

    ~LibreAudioPlugin() override
    {
        delete fMainDSP;
        delete fInputDSP;
        delete fOutputDSP;
        delete[] fInternalBuffer;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        delete[] fLatencyBuffer[0];
        delete[] fLatencyBuffer[1];
       #endif
    }

private:
   #if DISTRHO_PLUGIN_WANT_LATENCY
    // called when deactivated or during run()
    bool updateLatencyIfNeeded()
    {
        const uint32_t latency = fMainDSP->latency();
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(latency < LIBREAUDIO_MAX_LATENCY_SAMPLES,
                                         latency,
                                         LIBREAUDIO_MAX_LATENCY_SAMPLES, true);

        if (fLastKnownLatency == latency)
            return false;

        for (uint32_t c = 0; c < ARRAY_SIZE(fLatencyBuffer); ++c)
            std::memset(fLatencyBuffer[c], 0, sizeof(float) * latency);

        fLastKnownLatency = latency;
        fLatencyReadPos = -latency;
        fLatencyWritePos = 0;
        setLatency(latency);

        return true;
    }
   #endif
   /* -----------------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const noexcept final
    {
        return d_version(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) final
    {
        switch (index)
        {
        case kCommonParameterBypass:
            parameter.initDesignation(kParameterDesignationBypass);
            break;
        case kCommonParameterReset:
            parameter.initDesignation(kParameterDesignationReset);
            break;
       #if LIBREAUDIO_WANT_DRYWET
        case kCommonParameterDryWet:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Dry / Wet";
            parameter.symbol = "dry_wet";
            parameter.unit = "%";
            parameter.ranges.def = 50.f;
            parameter.ranges.min = 0.f;
            parameter.ranges.max = 100.f;
            break;
       #endif
        case kParametersInputStart ... kParametersInputEnd:
            parameter.groupId = kGroupInput;
            initParameterFromFaust(parameter, common_input::kFaustParameters[index - kParametersInputStart]);
            break;
        case kParametersOutputStart ... kParametersOutputEnd:
            parameter.groupId = kGroupOutput;
            initParameterFromFaust(parameter,
                                   common_output::kFaustParameters[index - kParametersOutputStart + kCommonIOParameters]);
            break;
        default:
            DISTRHO_SAFE_ASSERT_RETURN(index < kParameterCount,);
            parameter.groupId = kGroupMain;
            initParameterFromFaust(parameter, kFaustParameters[index - kParametersMainStart]);
            break;
        }
    }

    static void initParameterFromFaust(Parameter& parameter, const FaustParameter& faustParameter)
    {
        parameter.hints = kParameterIsAutomatable;
        if (faustParameter.isBoolean)
            parameter.hints |= kParameterIsBoolean;
        if (faustParameter.isInteger)
            parameter.hints |= kParameterIsInteger;
        if (faustParameter.isLogarithmic)
            parameter.hints |= kParameterIsLogarithmic;
        if (faustParameter.isOutput)
            parameter.hints |= kParameterIsOutput;
        if (faustParameter.isTrigger)
            parameter.hints |= kParameterIsTrigger;

        parameter.name = faustParameter.label;
        parameter.symbol = faustParameter.symbol;
        parameter.description = faustParameter.tooltip;
        parameter.unit = faustParameter.unit;
        parameter.ranges.def = faustParameter.init;
        parameter.ranges.min = faustParameter.min;
        parameter.ranges.max = faustParameter.max;

        if (std::strcmp(faustParameter.symbol, "input_ms_on") == 0)
        {
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
            values[0].label = "L/R";
            values[0].value = 0.f;
            values[1].label = "Mid/Side";
            values[1].value = 1.f;
            parameter.enumValues.restrictedMode = true;
            parameter.enumValues.count = 2;
            parameter.enumValues.values = values;
        }
        else if (std::strncmp(faustParameter.symbol, "input_phase_", 12) == 0)
        {
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
            values[0].label = "Normal";
            values[0].value = 0.f;
            values[1].label = "Inverted";
            values[1].value = 1.f;
            parameter.enumValues.restrictedMode = true;
            parameter.enumValues.count = 2;
            parameter.enumValues.values = values;
        }
    }

   /**
      Initialize the state @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(const uint32_t index, State& state) final
    {
        state.hints = kStateIsBase64Blob | kStateIsOnlyForUI;
        state.key = kStateKeys[index];

        switch (static_cast<States>(index))
        {
        case kStateUndoRedo:
            state.key = "undo_redo";
            state.label = "Undo/Redo";
        case kStateSnapshotA:
            state.key = "snapshot_a";
            state.label = "Snapshot A";
            break;
        case kStateSnapshotB:
            state.key = "snapshot_b";
            state.label = "Snapshot B";
            break;
        case kStateSnapshotC:
            state.key = "snapshot_c";
            state.label = "Snapshot C";
            break;
        case kStateSnapshotD:
            state.key = "snapshot_d";
            state.label = "Snapshot D";
            break;
        case kStateCount:
            break;
        }

    }
   /**
      Initialize the port group @a groupId.
      This function will be called once,
      shortly after the plugin is created and all audio ports and parameters have been enumerated.
    */
    void initPortGroup(const uint32_t groupId, PortGroup& portGroup) final
    {
        switch (static_cast<Groups>(groupId))
        {
        case kGroupInput:
            portGroup.name = "Input";
            portGroup.symbol = "input";
            break;
        case kGroupOutput:
            portGroup.name = "Output";
            portGroup.symbol = "output";
            break;
        case kGroupMain:
            portGroup.name = DISTRHO_PLUGIN_NAME;
            portGroup.symbol = DISTRHO_PLUGIN_LABEL;
            portGroup.symbol.toBasic();
            break;
        }
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(const uint32_t index) const final
    {
        switch (index)
        {
        case kParametersCommonStart ... kParametersCommonEnd:
            return fCommonParameterValues[index - kParametersCommonStart];
        case kParametersInputStart ... kParametersInputEnd:
            return fInputDSP->get(index - kParametersInputStart);
        case kParametersOutputStart ... kParametersOutputEnd:
            return fOutputDSP->get(index - kParametersOutputStart + kCommonIOParameters);
        default:
            DISTRHO_SAFE_ASSERT_RETURN(index < kParameterCount, 0.f);
            return fMainDSP->get(index - kParametersMainStart);
        }
    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, const float value) final
    {
        // common handling first
        switch (index)
        {
        case kParametersCommonStart ... kParametersCommonEnd:
            fCommonParameterValues[index - kParametersCommonStart] = value;
            break;
        case kParametersInputStart ... kParametersInputEnd:
            fInputDSP->set(index - kParametersInputStart, value);
            break;
        case kParametersOutputStart ... kParametersOutputEnd:
            fOutputDSP->set(index - kParametersOutputStart + kCommonIOParameters, value);
            break;
        default:
            DISTRHO_SAFE_ASSERT_RETURN(index < kParameterCount,);
            fMainDSP->set(index - kParametersMainStart, value);
            break;
        }

        // custom behaviour
        switch (index)
        {
        case kCommonParameterBypass:
       #if LIBREAUDIO_WANT_DRYWET
        case kCommonParameterDryWet:
       #endif
            if (fMuting.load() == false)
                doUnmute();
            break;
        case kParametersInputStart + common_input::kFaustParameterInput_ms_on:
            fOutputDSP->set(common_output::kFaustParameterInput_ms_on, value);
            break;
        }
    }

   /**
      Change an internal state @a key to @a value.
    */
    void setState(const char*, const char*) final
    {
        // all states in LA plugins are UI-only
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

    void mute()
    {
        fMuting.store(true);
        doMute();
    }

    inline void doMute()
    {
        assert(fMuting.load());

        // do not mute if bypassed
        if (d_isZero(fCommonParameterValues[kCommonParameterBypass]))
        {
            fGlobalDryValue.setTargetValue(0.f);
            fGlobalWetValue.setTargetValue(0.f);
        }
    }

    void unmute()
    {
        fMuting.store(false);
        doUnmute();
    }

    inline void doUnmute()
    {
        assert(fMuting.load() == false);

        // NOTE can trigger clicky operation here

        if (d_isZero(fCommonParameterValues[kCommonParameterBypass]))
        {
            // enabled, use full wet
           #if LIBREAUDIO_WANT_DRYWET
            const float wet = fCommonParameterValues[kCommonParameterDryWet] * 0.01f;
            fGlobalDryValue.setTargetValue(1.f - wet);
            fGlobalWetValue.setTargetValue(wet);
           #else
            fGlobalDryValue.setTargetValue(0.f);
            fGlobalWetValue.setTargetValue(1.f);
           #endif
        }
        else
        {
            // bypassed, use full dry
            fGlobalDryValue.setTargetValue(1.f);
            fGlobalWetValue.setTargetValue(0.f);
        }
    }

   /**
      Activate this plugin.
    */
    void activate() final
    {
        fCommonParameterValues[kCommonParameterReset] = 1.f;
    }

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames) final
    {
        if (d_isNotZero(fCommonParameterValues[kCommonParameterReset]))
        {
            if (fMuting.exchange(false))
                doUnmute();

            fCommonParameterValues[kCommonParameterReset] = 0.f;
            fGlobalDryValue.clearToTargetValue();
            fGlobalWetValue.clearToTargetValue();
            fMainDSP->instanceClear();
            fInputDSP->instanceClear();
            fOutputDSP->instanceClear();

           #if DISTRHO_PLUGIN_WANT_LATENCY
            fLatencyReadPos = -fLastKnownLatency;
            fLatencyWritePos = 0;
           #endif
        }

        float dry, wet;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        float input;
        int32_t latencyReadPos = fLatencyReadPos;
        int32_t latencyWritePos = fLatencyWritePos;
       #endif

        for (uint32_t i = 0, cycleFrames; i < frames; i += kInternalBlockSize)
        {
            cycleFrames = std::min<uint32_t>(kInternalBlockSize, frames - i);
           #ifdef __GNUC__
            #pragma GCC poison frames
           #endif

            for (uint32_t c = 0; c < DISTRHO_PLUGIN_NUM_OUTPUTS; ++c)
                std::memcpy(fCycleBuffer1[c], inputs[c] + i, sizeof(float) * cycleFrames);

           #if DISTRHO_PLUGIN_WANT_LATENCY
            for (uint32_t j = 0; j < cycleFrames; ++j)
            {
                for (uint32_t c = 0; c < DISTRHO_PLUGIN_NUM_OUTPUTS; ++c)
                    fLatencyBuffer[c][latencyWritePos] = inputs[c][i + j];
                // fCycleBuffer1[c][j];

                if (++latencyWritePos == LIBREAUDIO_MAX_LATENCY_SAMPLES)
                    latencyWritePos = 0;
            }
           #endif

            fInputDSP->compute(cycleFrames, fCycleBuffer1, fCycleBuffer2);
            fMainDSP->compute(cycleFrames, fCycleBuffer2, fCycleBuffer1);
            fOutputDSP->compute(cycleFrames, fCycleBuffer1, fCycleBuffer2);

            for (uint32_t j = 0; j < cycleFrames; ++j)
            {
                dry = fGlobalDryValue.next();
                wet = fGlobalWetValue.next();

                for (uint32_t c = 0; c < DISTRHO_PLUGIN_NUM_OUTPUTS; ++c)
                {
                   #if DISTRHO_PLUGIN_WANT_LATENCY
                    input = latencyReadPos >= 0 ? fLatencyBuffer[c][latencyReadPos] * dry : 0.f;
                    outputs[c][i + j] = fCycleBuffer2[c][j] * wet + input;
                   #else
                    outputs[c][i + j] = fCycleBuffer2[c][j] * wet + inputs[c][i + j] * dry;
                   #endif
                }

               #if DISTRHO_PLUGIN_WANT_LATENCY
                if (++latencyReadPos == LIBREAUDIO_MAX_LATENCY_SAMPLES)
                    latencyReadPos = 0;
               #endif
            }

            if (fMuting.load() && d_isZero(fGlobalDryValue.peek()) && d_isZero(fGlobalWetValue.peek()))
                unmute();
        }

       #if DISTRHO_PLUGIN_WANT_LATENCY
        if (! updateLatencyIfNeeded())
        {
            fLatencyReadPos = latencyReadPos;
            fLatencyWritePos = latencyWritePos;
        }
       #endif
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(const double newSampleRate) final
    {
        fGlobalDryValue.setSampleRate(newSampleRate);
        fGlobalWetValue.setSampleRate(newSampleRate);

        const int sampleRate = d_roundToIntPositive(newSampleRate);
        fMainDSP->instanceConstants(sampleRate);
        fInputDSP->instanceConstants(sampleRate);
        fOutputDSP->instanceConstants(sampleRate);

       #if DISTRHO_PLUGIN_WANT_LATENCY
        fMainDSP->compute(0, fCycleBuffer1, fCycleBuffer2);
        updateLatencyIfNeeded();
       #endif
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
