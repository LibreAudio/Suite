// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/parameter-dump-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/shader.hpp"

#include <list>

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#define SHADER_ANALYSER_FFT SHADERS_ANALYSER_FFT_FRAG_DATA, SHADERS_ANALYSER_FFT_FRAG_LEN
#define SHADER_CLOUDSTARFIELD SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA, SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN

#if defined(LIBREAUDIO_PLUGIN__chorus)
#define SHADER_PLUGIN SHADERS_CURVE_CHORUS_FRAG_DATA, SHADERS_CURVE_CHORUS_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__djFilter)
#define SHADER_PLUGIN SHADERS_CURVE_DJ_FILTER_FRAG_DATA, SHADERS_CURVE_DJ_FILTER_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__springReverb)
#define SHADER_PLUGIN SHADERS_CURVE_SPRING_REVERB_FRAG_DATA, SHADERS_CURVE_SPRING_REVERB_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__tiltEQ)
#define SHADER_PLUGIN SHADERS_CURVE_TILT_EQ_FRAG_DATA, SHADERS_CURVE_TILT_EQ_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__vocalDoubler)
#define SHADER_PLUGIN SHADERS_CURVE_VOCAL_DOUBLER_FRAG_DATA, SHADERS_CURVE_VOCAL_DOUBLER_FRAG_LEN
#endif

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI
{
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 0;
   #endif

    using ShaderAnalyzer = LibreAudio::BotShaderWidget<SHADER_ANALYSER_FFT, kTextureSize>;
    std::shared_ptr<ShaderAnalyzer> fShaderAnalyser;

public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        const std::list<std::shared_ptr<LibreAudio::ShaderWidgetInterface>> shaders {
            std::make_shared<LibreAudio::ShaderBackgroundWidget>(this, this),
            std::make_shared<LibreAudio::BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
           #ifdef SHADER_PLUGIN
            makeAnalyzer(),
            std::make_shared<LibreAudio::BotShaderWidget<SHADER_PLUGIN>>(this, this)
           #endif
        };

       #ifdef SHADER_PLUGIN
        createRootWidget<LibreAudio::TopBar, LibreAudio::MainAreaContainerWidget<>>();
       #else
        createRootWidget<LibreAudio::TopBar, LibreAudio::ParameterDumpArea>();
       #endif
        fRootWidget->enableShaders(shaders);
    }

private:
    std::shared_ptr<ShaderAnalyzer> makeAnalyzer()
    {
        fShaderAnalyser.reset(new ShaderAnalyzer(this, this));
        return fShaderAnalyser;
    }

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        fShaderAnalyser->replace(values);
    }
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
       #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 1
        fShaderAnalyser->push(value);
       #else
        fShaderAnalyser->push(std::max(value.ptr[0], value.ptr[1]));
       #endif
    }
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
