// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioBaseUI.hpp"

#include "ui/widgets/fft-waveform.hpp"
#include "ui/widgets/shader.hpp"

#include "las-resources.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

#if defined(LIBREAUDIO_PLUGIN__chorus)
#define SHADER_PLUGIN SHADERS_CURVE_CHORUS_FRAG_DATA, SHADERS_CURVE_CHORUS_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__delay)
#define SHADER_PLUGIN SHADERS_CURVE_DELAY_FRAG_DATA, SHADERS_CURVE_DELAY_FRAG_LEN
// #elif defined(LIBREAUDIO_PLUGIN__djFilter)
// #define SHADER_PLUGIN SHADERS_CURVE_DJ_FILTER_FRAG_DATA, SHADERS_CURVE_DJ_FILTER_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__springReverb)
#define SHADER_PLUGIN SHADERS_CURVE_SPRING_REVERB_FRAG_DATA, SHADERS_CURVE_SPRING_REVERB_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__tiltEQ)
#define SHADER_PLUGIN SHADERS_CURVE_TILT_EQ_FRAG_DATA, SHADERS_CURVE_TILT_EQ_FRAG_LEN
#elif defined(LIBREAUDIO_PLUGIN__vocalDoubler)
#define SHADER_PLUGIN SHADERS_CURVE_VOCAL_DOUBLER_FRAG_DATA, SHADERS_CURVE_VOCAL_DOUBLER_FRAG_LEN
#endif

#define SHADER_CLOUDSTARFIELD SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA, SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN

template<class RootWidget>
class UI : public LibreAudioBaseUI
{
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 0;
   #endif

    using MainAreaWidget = typename RootWidget::MainAreaWidget;
    using CentralAreaWidget = typename MainAreaWidget::CentralArea;

public:
   #ifdef SHADER_PLUGIN
    using BotShaderPluginWidget = BotShaderWidget<SHADER_PLUGIN, kTextureSize>;
   #endif

    UI() : LibreAudioBaseUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
           #ifdef SHADER_PLUGIN
            std::make_shared<BotShaderPluginWidget>(this, this),
           #endif
        };

        createRootWidget<RootWidget>();

        RootWidget* const root = static_cast<RootWidget*>(fRootWidget.get());

        root->enableShaders(shaders);
        const std::shared_ptr<MainAreaWidget> mainArea = root->getMainAreaWidget();
        const std::shared_ptr<CentralAreaWidget> centralArea = mainArea->getCentralAreaWidget();
        fWaveform = centralArea->getWaveformWidget();
    }

private:
    std::shared_ptr<WaveformWidget> fWaveform;

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        if (WaveformWidget* const waveform = fWaveform.get())
            waveform->replace(values);
    }
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
        if (WaveformWidget* const waveform = fWaveform.get())
        {
           #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 2
            waveform->push(value.ptr[0], value.ptr[1]);
           #else
            waveform->push(value);
           #endif
        }
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
