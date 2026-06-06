// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoPlugin.hpp"
#include "FaustMetadata.hpp"

#include "dsp-input.hpp"
#include "dsp-output.hpp"

#include "metadata-input.hpp"
#include "metadata-output.hpp"

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

    enum Groups {
        kGroupInput,
        kGroupOutput,
        kGroupMain,
    };

    DSP* const dsp = new DSP;
    input* const dspInput = new input;
    output* const dspOutput = new output;

    const FaustParameters<numParameters>& parametersMeta;

public:
    LibreAudioPlugin(const FaustParameters<numParameters>& parameters)
        : Plugin(kCommonParameterCount + ::input::kNumParameters + ::output::kNumParameters + numParameters, 0, 0),
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

        const int sampleRate = d_roundToIntPositive(getSampleRate());
        dsp->init(sampleRate);
        dspInput->init(sampleRate);
        dspOutput->init(sampleRate);
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
        if (index < kCommonParameterCount)
        {
            switch (static_cast<CommonParameters>(index))
            {
            case kCommonParameterBypass:
                parameter.initDesignation(kParameterDesignationBypass);
                break;
            case kCommonParameterReset:
                parameter.initDesignation(kParameterDesignationReset);
                break;
            case kCommonParameterCount:
                __builtin_unreachable();
                break;
            }
            return;
        }

        index -= kCommonParameterCount;

        if (index < ::input::kNumParameters)
        {
            parameter.groupId = kGroupInput;
            initParameterFromFaust(parameter, ::input::kParameters[index]);
            return;
        }

        index -= ::input::kNumParameters;

        if (index < ::output::kNumParameters)
        {
            parameter.groupId = kGroupOutput;
            initParameterFromFaust(parameter, ::output::kParameters[index]);
            return;
        }

        index -= ::output::kNumParameters;

        parameter.groupId = kGroupMain;
        initParameterFromFaust(parameter, parametersMeta[index]);
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
    float getParameterValue(uint32_t index) const final
    {
        if (index < kCommonParameterCount)
            return fCommonParameters[index];

        index -= kCommonParameterCount;

        if (index < ::input::kNumParameters)
            return dspInput->get(index);

        index -= ::input::kNumParameters;

        if (index < ::output::kNumParameters)
            return dspOutput->get(index);

        index -= ::output::kNumParameters;

        return dsp->get(index);
    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automatable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, const float value) final
    {
        if (index < kCommonParameterCount)
        {
            fCommonParameters[index] = value;

            // TODO special handling
            return;
        }

        index -= kCommonParameterCount;

        if (index < ::input::kNumParameters)
        {
            dspInput->set(index, value);
            return;
        }

        index -= ::input::kNumParameters;

        if (index < ::output::kNumParameters)
        {
            dspOutput->set(index, value);
            return;
        }

        index -= ::output::kNumParameters;

        dsp->set(index, value);
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
            dsp->instanceClear();
            dspInput->instanceClear();
            dspOutput->instanceClear();
        }

        const float** routputs = const_cast<const float**>(outputs);

        dspInput->compute(frames, inputs, outputs);
        dsp->compute(frames, routputs, outputs);
        dspOutput->compute(frames, routputs, outputs);
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(const double newSampleRate) final
    {
        const int sampleRate = d_roundToIntPositive(newSampleRate);
        dsp->instanceConstants(sampleRate);
        dspInput->instanceConstants(sampleRate);
        dspOutput->instanceConstants(sampleRate);
    }

// protected:
    float fCommonParameters[kCommonParameterCount];
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
