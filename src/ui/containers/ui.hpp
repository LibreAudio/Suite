// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioBaseUI.hpp"

#include "ui/widgets/shader.hpp"

#include "las-resources.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class RootWidget,
         const char shaderSrc[] = SHADERS_ANALYSER_FFT_FRAG_DATA,
         uint shaderSrcSize = SHADERS_ANALYSER_FFT_FRAG_LEN>
class UI final : public LibreAudioBaseUI
{
    const std::unique_ptr<ShaderBaseWidget> fShaderBackground {
        new BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 0;
   #endif

    static constexpr const std::string_view label = DISTRHO_PLUGIN_LABEL;
    using AnalyzerShaderW = std::conditional_t<label == "dualGain",
                                               BackgroundShaderWidget<shaderSrc, shaderSrcSize, kTextureSize>,
                                               BotShaderWidget<shaderSrc, shaderSrcSize>>;

    const std::unique_ptr<AnalyzerShaderW> fShaderAnalyser {
        new AnalyzerShaderW(this, this)
    };

public:
    UI() : LibreAudioBaseUI()
    {
        createRootWidget<RootWidget>();

        static_cast<RootWidget*>(fRootWidget.get())->enableShaders({
            fShaderBackground.get(), fShaderAnalyser.get()
        });
    }

private:
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        if constexpr (label == "dualGain")
        {
            fShaderAnalyser->replace(values);
        }
    }
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
        if constexpr (label == "dualGain")
        {
           #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 1
            fShaderAnalyser->push(value);
           #else
            fShaderAnalyser->push(std::max(value.ptr[0], value.ptr[1]));
           #endif
        }
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
