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
    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    const std::unique_ptr<LibreAudio::BotShaderBaseWidget> fShaderAnalyser {
        new LibreAudio::BotShaderWidget<shaderSrc, shaderSrcSize>(this, this)
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
