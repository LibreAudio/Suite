// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class EqWidget final : public LabReferenceWidget<Reference::Stage>,
                       private IdleCallback
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

using EqMainArea = MainAreaContainerWidget<EqWidget>;
using EqRootWidget = RootWidget<TopBar, EqMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::EqRootWidget>();
}

END_NAMESPACE_DISTRHO
