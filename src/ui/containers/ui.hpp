// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioBaseUI.hpp"

#include "ui/widgets/shader.hpp"

#include "las-resources.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

#define SHADER_CLOUDSTARFIELD SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA, SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN

template<class RootWidget,
         const char shaderSrc[] = SHADERS_ANALYSER_FFT_FRAG_DATA,
         uint shaderSrcSize = SHADERS_ANALYSER_FFT_FRAG_LEN>
class UI : public LibreAudioBaseUI
{
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 0;
   #endif

    static constexpr const std::string_view label = DISTRHO_PLUGIN_LABEL;
    using AnalyzerShaderW = std::conditional_t<label == "dualGain",
                                               CleanShaderWidget<shaderSrc, shaderSrcSize, kTextureSize>,
                                               BotShaderWidget<shaderSrc, shaderSrcSize, kTextureSize>>;

public:
    UI() : LibreAudioBaseUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
            makeAnalyzer()
        };

        createRootWidget<RootWidget>();

        static_cast<RootWidget*>(fRootWidget.get())->enableShaders(shaders);
    }

protected:
    std::shared_ptr<AnalyzerShaderW> fShaderAnalyser;

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) override
    {
        fShaderAnalyser->replace(values);
    }
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) override
    {
       #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 1
        fShaderAnalyser->push(value);
       #else
        fShaderAnalyser->push(std::max(value.ptr[0], value.ptr[1]));
       #endif
    }
   #endif

private:
    std::shared_ptr<AnalyzerShaderW> makeAnalyzer()
    {
        fShaderAnalyser.reset(new AnalyzerShaderW(this, this));
        return fShaderAnalyser;
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
