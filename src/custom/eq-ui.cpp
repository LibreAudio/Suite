// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/top-bar.hpp"
#include "ui/widgets/gain-meter.hpp"
#include "ui/widgets/shader.hpp"

#include "eq-parameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class EqWidget final : public LabReferenceWidget<Reference::Stage>,
                       protected IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = LabReferenceWidget<R>;

public:
    explicit EqWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        for (uint32_t i = 0; i < kFaustParameterCount; ++i)
            fParamValues[i] = fInterface->getParameterValue(kParametersMainStart + i);
    }

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:
    float fParamValues[kFaustParameterCount];

    void idleCallback() override
    {
        for (uint32_t i = 0; i < kFaustParameterCount; ++i)
        {
            if (const float value = fInterface->getParameterValue(kParametersMainStart + i); d_isNotEqual(fParamValues[i], value))
            {
                fParamValues[i] = value;

                // NOTE perhaps recalculate coeffs here?

                repaint();
            }
        }
    }

    void onNanoDisplay() final
    {
        // draw background first
        drawReferenceBackground<R, kCornerBoth>();

        // TODO draw eq stuff here

        // draw border last
        drawReferenceBorder<R>();
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        // TODO insert custom mouse click handling here

        return BaseWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        // TODO insert custom mouse motion/move handling here

        return BaseWidget::onMotion(ev);
    }

    bool onScroll(const Widget::ScrollEvent& ev) final
    {
        // TODO insert custom mouse scroll handling here

        return BaseWidget::onScroll(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class EqMainArea : public ReferenceContainerWidget<Reference::MainArea>
{
    using R = Reference::MainArea;
    using BaseWidget = ReferenceContainerWidget<R>;

    std::shared_ptr<LabWidget> fMetersIn = addWidget<GainMeterWidget<Input>>();
    std::shared_ptr<EqWidget> fEq = addWidget<EqWidget, Expanding>();
    std::shared_ptr<LabWidget> fMetersOut = addWidget<GainMeterWidget<Output>>();

public:
    EqMainArea(LabTopLevelWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] Point<int> getMiddleAreaAbsolutePos() const noexcept
    {
        return fEq->getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getMiddleAreaSize() const noexcept
    {
        return fEq->getSize();
    }

    [[nodiscard]] float getMiddleAreaBorderRadius() const noexcept
    {
        return fEq->getBorderRadius();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class EqRootWidget final : public RootWidget<TopBar, EqMainArea>
{
    using BaseWidget = RootWidget<TopBar, EqMainArea>;

    ShaderBaseWidget* fShaderBackground = nullptr;
    BotShaderBaseWidget* fShaderAnalyser = nullptr;

public:
    EqRootWidget(Window& window, LabUIWidgetInterface* const iface)
        : BaseWidget(window, iface) {}

    void setup(ShaderBaseWidget* const background, BotShaderBaseWidget* const analyser)
    {
        fShaderBackground = background;
        fShaderAnalyser = analyser;
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        BaseWidget::updateSize(updateChildren);

        const Point<int> pos = fMainArea->getMiddleAreaAbsolutePos();
        const Size<uint> size = fMainArea->getMiddleAreaSize();
        const float borderRadius = fMainArea->getMiddleAreaBorderRadius();

        if (fShaderBackground != nullptr)
        {
            fShaderBackground->setAbsolutePos(pos);
            fShaderBackground->setSize(size);
            fShaderBackground->setBorderRadius(borderRadius);
        }

        if (fShaderAnalyser != nullptr)
        {
            fShaderAnalyser->setAbsolutePos(pos);
            fShaderAnalyser->setSize(size);
            fShaderAnalyser->setBorderRadius(borderRadius);
        }
    }
};

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI
{
    std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground;
    std::unique_ptr<LibreAudio::BotShaderBaseWidget> fShaderAnalyser;

public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        fShaderBackground.reset(new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                                                SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this));

        fShaderAnalyser.reset(new LibreAudio::BotShaderWidget<SHADERS_ANALYSER_FFT_FRAG_DATA,
                                                              SHADERS_ANALYSER_FFT_FRAG_LEN>(this, this));

        createRootWidget<LibreAudio::EqRootWidget>();
        static_cast<LibreAudio::EqRootWidget*>(fRootWidget.get())->setup(fShaderBackground.get(),
                                                                         fShaderAnalyser.get());
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
