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
class UI : public LibreAudioBaseUI
{
    const std::unique_ptr<ShaderBaseWidget> fShaderBackground {
        new BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    static constexpr const std::string_view label = DISTRHO_PLUGIN_LABEL;
    using AnalyzerShaderW = std::conditional_t<label == "dualGain",
                                               BackgroundShaderWidget<shaderSrc, shaderSrcSize>,
                                               BotShaderWidget<shaderSrc, shaderSrcSize>>;

    const std::unique_ptr<ShaderBaseWidget> fShaderAnalyser {
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
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
