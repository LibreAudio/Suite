// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoPlugin.hpp"
#include "extra/ScopedDenormalDisable.hpp"
#include "extra/ValueSmoother.hpp"

#include "common_input.hpp"
#include "common_output.hpp"

// --------------------------------------------------------------------------------------------------------------------

static_assert(DISTRHO_PLUGIN_NUM_INPUTS == 2, "has 2 audio inputs");
static_assert(DISTRHO_PLUGIN_NUM_OUTPUTS == 2, "has 2 audio outputs");

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

template<class DSP, int numParameters>
class LibreAudioPlugin : public Plugin
{
    enum CommonParameters {
        kCommonParameterBypass,
        kCommonParameterReset,
        kCommonParameterCount
    };

    enum Parameters {
        kParametersCommonStart,
        kParametersCommonEnd = kParametersCommonStart + kCommonParameterCount - 1,
        kParametersInputStart,
        kParametersInputEnd = kParametersInputStart + common_input::kNumParameters - 1,
        kParametersOutputStart,
        kParametersOutputEnd = kParametersOutputStart + common_output::kNumParameters - 1,
        kParametersMainStart,
    };

    enum Groups {
        kGroupInput,
        kGroupOutput,
        kGroupMain,
    };

    DSP* const dsp = new DSP;
    common_input::common_input* const dspInput = new common_input::common_input;
    common_output::common_output* const dspOutput = new common_output::common_output;

    const FaustParameters<numParameters>& parametersMeta;

public:
    LibreAudioPlugin(const FaustParameters<numParameters>& parameters)
        : Plugin(kParametersMainStart + numParameters, 0, 0),
          parametersMeta(parameters)
    {
        for (uint32_t i = 0; i < kCommonParameterCount; ++i)
        {
            switch (static_cast<CommonParameters>(i))
            {
            case kCommonParameterBypass:
            case kCommonParameterReset:
                fCommonParameters[i] = 0.f;
                break;
            case kCommonParameterCount:
                break;
            }
        }

        const double sampleRate = getSampleRate();
        const int iSampleRate = d_roundToIntPositive(sampleRate);

        globalDryValue.setSampleRate(sampleRate);
        globalDryValue.setTimeConstant(0.02f);
        globalDryValue.setTargetValue(1.f);

        dsp->init(iSampleRate);
        dspInput->init(iSampleRate);
        dspOutput->init(iSampleRate);
    }

    ~LibreAudioPlugin() override
    {
        delete dsp;
        delete dspInput;
        delete dspOutput;
    }

private:
   /* -----------------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const noexcept final
    {
        return 0;
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
        case kParametersInputStart ... kParametersInputEnd:
            parameter.groupId = kGroupInput;
            initParameterFromFaust(parameter, common_input::kParameters[index - kParametersInputStart]);
            break;
        case kParametersOutputStart ... kParametersOutputEnd:
            parameter.groupId = kGroupOutput;
            initParameterFromFaust(parameter, common_output::kParameters[index - kParametersOutputStart]);
            break;
        default:
            parameter.groupId = kGroupMain;
            initParameterFromFaust(parameter, parametersMeta[index - kParametersMainStart]);
            break;
        }
    }

    static void initParameterFromFaust(Parameter& parameter, const FaustParameter& faustParameter)
    {
        parameter.hints = kParameterIsAutomatable;
        if (faustParameter.isBoolean)
            parameter.hints = kParameterIsBoolean;
        if (faustParameter.isInteger)
            parameter.hints = kParameterIsInteger;
        if (faustParameter.isLogarithmic)
            parameter.hints = kParameterIsLogarithmic;
        if (faustParameter.isOutput)
            parameter.hints = kParameterIsOutput;
        if (faustParameter.isTrigger)
            parameter.hints = kParameterIsTrigger;

        parameter.name = faustParameter.label;
        parameter.symbol = faustParameter.symbol;
        parameter.description = faustParameter.tooltip;
        parameter.unit = faustParameter.unit;
        parameter.ranges.def = faustParameter.init;
        parameter.ranges.min = faustParameter.min;
        parameter.ranges.max = faustParameter.max;
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
            return fCommonParameters[index - kParametersCommonStart];
        case kParametersInputStart ... kParametersInputEnd:
            return dspInput->get(index - kParametersInputStart);
        case kParametersOutputStart ... kParametersOutputEnd:
            return dspInput->get(index - kParametersOutputStart);
        default:
            return dsp->get(index - kParametersMainStart);
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
        switch (index)
        {
        case kParametersCommonStart ... kParametersCommonEnd:
            if (index == kCommonParameterBypass)
                globalDryValue.setTargetValue(value);
            fCommonParameters[index - kParametersCommonStart] = value;
            break;
        case kParametersInputStart ... kParametersInputEnd:
            dspInput->set(index - kParametersInputStart, value);
            break;
        case kParametersOutputStart ... kParametersOutputEnd:
            dspOutput->set(index - kParametersOutputStart, value);
            break;
        default:
            dsp->set(index - kParametersMainStart, value);
            break;
        }
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Activate this plugin.
    */
    void activate() final
    {
        fCommonParameters[kCommonParameterReset] = 1.f;
    }

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames) final
    {
        if (d_isNotZero(fCommonParameters[kCommonParameterReset]))
        {
            fCommonParameters[kCommonParameterReset] = 0.f;
            globalDryValue.clearToTargetValue();
            dsp->instanceClear();
            dspInput->instanceClear();
            dspOutput->instanceClear();
        }

        float cycledInputs[DISTRHO_PLUGIN_NUM_INPUTS][32];
        float cycledOutputs[DISTRHO_PLUGIN_NUM_OUTPUTS][32];
        float* cycledInputsPtr[2] = { cycledInputs[0], cycledInputs[1] };
        float* cycledOutputsPtr[2] = { cycledOutputs[0], cycledOutputs[1] };
        float dry, wet;

        for (uint32_t i = 0, cycleFrames; i < frames; i += 32)
        {
            cycleFrames = std::min<uint32_t>(32, frames - i);

            for (uint32_t c = 0; c < DISTRHO_PLUGIN_NUM_OUTPUTS; ++c)
                std::memcpy(cycledInputs[c], inputs[c], sizeof(float) * cycleFrames);

            dspInput->compute(cycleFrames, cycledInputsPtr, cycledOutputsPtr);
            dsp->compute(cycleFrames, cycledOutputsPtr, cycledOutputsPtr);
            dspOutput->compute(cycleFrames, cycledOutputsPtr, cycledOutputsPtr);

            for (uint32_t j = 0; j < cycleFrames; ++j)
            {
                dry = globalDryValue.next();
                wet = 1.f - dry;

                for (uint32_t c = 0; c < DISTRHO_PLUGIN_NUM_OUTPUTS; ++c)
                    outputs[c][j] = cycledOutputs[c][j] * wet + inputs[c][j] * dry;
            }
        }
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(const double newSampleRate) final
    {
        globalDryValue.setSampleRate(newSampleRate);

        const int sampleRate = d_roundToIntPositive(newSampleRate);
        dsp->instanceConstants(sampleRate);
        dspInput->instanceConstants(sampleRate);
        dspOutput->instanceConstants(sampleRate);
    }

// protected:
    float fCommonParameters[kCommonParameterCount];
    LinearValueSmoother globalDryValue;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
