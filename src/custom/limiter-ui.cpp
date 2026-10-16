// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/toggle-switch.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <array>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display, with the starfield and the output waveform behind it, and the control bar floats at its
// bottom.
// The bar is styled like the mbComp5 band groups.

struct LimiterBarReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + margin * 2;
};

// accent colour of a parameter, as set by [accentcolor] in limiter.dsp

template<FaustParameterIndex id>
constexpr const Color& accentColor() noexcept
{
    constexpr uint accent = kFaustParameters[id].accentcolor;
    static_assert(accent >= 1 && accent <= 6, "accentcolor must be 1..6");

    if constexpr (accent == 1) return Reference::Colors::acc1;
    else if constexpr (accent == 2) return Reference::Colors::acc2;
    else if constexpr (accent == 3) return Reference::Colors::acc3;
    else if constexpr (accent == 4) return Reference::Colors::acc4;
    else if constexpr (accent == 5) return Reference::Colors::acc5;
    else return Reference::Colors::acc6;
}

template<FaustParameterIndex id>
using LimiterNumberBoxWidget = NumberBoxWidget<accentColor<id>()>;

// An on / off parameter shown as its name only: grey when off, accent colour when on, 70% accent when hovered.
// Two of them stack in the height of a number box.

struct LimiterToggleReference : Reference::Zero {
    static constexpr const Color color = Reference::Colors::ink3;
    static constexpr const float fontSize = 13.f;
    static constexpr const float letterSpacing = fontSize * 0.06f;
    static constexpr const uint margin = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height / 2;
};

template<const Color& kAccentColor>
class LimiterToggleWidget final : public ToggleSwitchBaseWidget
{
    using R = LimiterToggleReference;
    using BaseWidget = ToggleSwitchBaseWidget;

public:
    explicit LimiterToggleWidget(LabWidget* const parent, const FaustParameterIndex id)
        : BaseWidget(parent, kParametersMainStart + id, kFaustParameters[id].name)
    {
        setChecked(d_isNotZero(kFaustParameters[id].init), false);
        updateSize(false);
    }

private:
    void setupFont()
    {
        fontFace("regular");
        fontSize(R::fontSize * fScaleFactor);
        textLetterSpacing(R::letterSpacing * fScaleFactor);
    }

    void onNanoDisplay() final
    {
        setupFont();
        fillColor(isChecked() ? kAccentColor : isHovered() ? kAccentColor.withAlpha(0.7f) : R::color);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(R::margin * fScaleFactor, getHeight() * 0.5f, getName(), nullptr);
    }

    void updateSize(const bool updateChildren) final
    {
        Rectangle<float> bounds;
        setupFont();
        textAlign(0);
        textBounds(0, 0, getName(), nullptr, bounds);

        setSize(d_roundToUnsignedInt(bounds.getWidth() + R::margin * 2 * fScaleFactor),
                d_roundToUnsignedInt(R::height * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

class LimiterTogglesWidget final : public ReferenceContainerWidget<Reference::Zero, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Zero, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<LimiterToggleWidget<accentColor<kFaustParameterTrue_peak>()>>(kFaustParameterTrue_peak),
        addWidget<LimiterToggleWidget<accentColor<kFaustParameterUnity_gain>()>>(kFaustParameterUnity_gain),
    };

public:
    explicit LimiterTogglesWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        uint width = 0;
        for (const std::shared_ptr<LabWidget>& widget : widgets)
            width = std::max(width, widget->getWidth());

        LabWidget::setSize(width, d_roundToUnsignedInt(Reference::Widgets::NumberBox<>::height * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

// fixed space between the groups of number boxes, so the boxes get the width left over

struct LimiterGapReference : Reference::Zero {
    static constexpr const uint width = 18;
    static constexpr const uint height = 1;
};

class LimiterBarWidget final : public ReferenceContainerWidget<LimiterBarReference>
{
    using BaseWidget = ReferenceContainerWidget<LimiterBarReference>;
    using GapWidget = LabReferenceWidget<LimiterGapReference>;

    const std::array<std::shared_ptr<LabWidget>, 10> widgets {
        addWidget<LimiterTogglesWidget>(),
        addSpacer(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterThreshold>, Expanding>(kFaustParameterThreshold),
        addWidget<LimiterNumberBoxWidget<kFaustParameterCeiling>, Expanding>(kFaustParameterCeiling),
        addWidget<GapWidget>(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterLookahead>, Expanding>(kFaustParameterLookahead),
        addWidget<GapWidget>(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<LimiterNumberBoxWidget<kFaustParameterAuto_release>, Expanding>(kFaustParameterAuto_release),
        addSpacer(),
    };

public:
    explicit LimiterBarWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    // faint rim all around, lit along the top edge
    void onNanoDisplay() final
    {
        drawReferenceBackground<LimiterBarReference>();

        const float w = getWidth();
        const float h = getHeight();
        const float r = LimiterBarReference::borderRadius * fScaleFactor;

        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokeColor(Color(1.f, 1.f, 1.f, 0.05f));
        strokeWidth(1.f);
        stroke();

        save();
        intersectScissor(0, 0, w, r);
        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokePaint(linearGradient(0, 0, 0, r, Color(1.f, 1.f, 1.f, 0.12f), Color(1.f, 1.f, 1.f, 0.f)));
        strokeWidth(1.f);
        stroke();
        restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LimiterStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Stage, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addSpacer(),
        addWidget<LimiterBarWidget>(),
    };

public:
    explicit LimiterStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

using LimiterRootWidget = RootWidget<TopBar, MainAreaContainerWidget<LimiterStageWidget>>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class LimiterUI final : public LibreAudioBaseUI
{
    using WaveformShaderWidget = LibreAudio::BotShaderWidget<SHADERS_FFT_WAVEFORM_FRAG_DATA,
                                                             SHADERS_FFT_WAVEFORM_FRAG_LEN,
                                                             kNumSamplePointsForWaveform>;

    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    // scrolling output peaks, above the starfield
    const std::unique_ptr<WaveformShaderWidget> fShaderWaveform {
        new WaveformShaderWidget(this, this)
    };

public:
    LimiterUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::LimiterRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get(), fShaderWaveform.get() });
    }

private:
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
        fShaderWaveform->push(std::max(value.ptr[0], value.ptr[1]));
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterUI)
};

UI* createUI()
{
    return new LimiterUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
