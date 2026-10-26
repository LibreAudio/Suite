// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"
#include "../_lab/metrics.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R>
struct CheckBoxMetrics : LabMetrics<R> {
    explicit CheckBoxMetrics(LabWidget* const widget)
        : LabMetrics<R>(widget),
          box(widget, *this) {}

    const struct Box : LabMetrics<typename R::Box> {
        const float checkmark;
        const float xy;
        const float size;

        explicit Box(LabWidget* const widget, const CheckBoxMetrics& m)
            : LabMetrics<typename R::Box>(widget),
              checkmark(2 * m.s),
              xy(m.border + m.margin),
              size(m.height - xy * 2) {}
    } box;
};

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1, uint height = Reference::Widgets::CheckBox<>::height>
class CheckBoxWidget final : public ParameterReferenceButtonWidget<Reference::Widgets::CheckBox<kAccentColor, height>>,
                             private ButtonEventHandler::Callback,
                             private IdleCallback
{
    using R = Reference::Widgets::CheckBox<kAccentColor, height>;
    using BaseWidget = ParameterReferenceButtonWidget<R, kCornerAuto>;
    using State = typename BaseWidget::State;

public:
    explicit CheckBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, index)
    {
        DISTRHO_SAFE_ASSERT(this->fParameter.isBoolean);

        BaseWidget::addIdleCallback(this);
        BaseWidget::setCallback(this);
        BaseWidget::updateSize(false);
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        this->fInterface->parameterControlModified(
            BaseWidget::getId(), BaseWidget::isChecked() ? this->fParameter.max : this->fParameter.min);
    }

    void idleCallback() final
    {
        BaseWidget::setChecked(d_isNotZero(this->fInterface->getParameterValue(BaseWidget::getId())), false);
    }

    void onNanoDisplay() final
    {
        const CheckBoxMetrics<R> m { this };

        BaseWidget::template drawReferenceBackground<R, R::backgroundColor>();
        BaseWidget::template drawReferenceBorder<R, R::borderColor>();

        // ------------------------------------------------------------------------------------------------------------
        // draw box background

        if constexpr (d_isNotZero(R::Box::backgroundColor.alpha))
        {
            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::roundedRect(m.box.xy, m.box.xy, m.box.size, m.box.size, m.box.borderRadius);
            else
                BaseWidget::rect(m.box.xy, m.box.xy, m.box.size, m.box.size);

            BaseWidget::fillColor(R::Box::backgroundColor);
            BaseWidget::fill();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw box border

        if constexpr (R::Box::border != 0)
        {
            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::borderRoundedRect(m.box.xy, m.box.xy, m.box.size, m.box.size, m.box.border, m.box.borderRadius);
            else
                BaseWidget::borderRect(m.box.xy, m.box.xy, m.box.size, m.box.size, m.box.border);

            BaseWidget::strokeColor(BaseWidget::isHovered() ? R::Box::borderColor〡hovered : R::Box::borderColor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw checkmark

        if (BaseWidget::isChecked())
        {
            BaseWidget::beginPath();
            BaseWidget::moveTo(m.box.xy + m.box.size * 0.3f, m.box.xy + m.box.size * 0.5f);
            BaseWidget::lineTo(m.box.xy + m.box.size * 0.44f, m.box.xy + m.box.size * 0.666f);
            BaseWidget::lineTo(m.box.xy + m.box.size * 0.75f, m.box.xy + m.box.size * 0.35f);
            BaseWidget::lineCap(BaseWidget::ROUND);
            BaseWidget::lineJoin(BaseWidget::ROUND);
            BaseWidget::strokeColor(R::color〡checked);
            BaseWidget::strokeWidth(m.box.checkmark);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw label

        BaseWidget::fillColor(R::color〡checked);
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.fontSize);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::textLetterSpacing(m.letterSpacing);
        BaseWidget::text(m.box.xy + m.box.size + m.padding, m.height * 0.5f, BaseWidget::getName());
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & BaseWidget::kButtonStateActive) != 0 && (oldState & BaseWidget::kButtonStateActive) == 0)
        {
            this->fInterface->parameterControlPressed(BaseWidget::getId());
            return;
        }

        if ((state & BaseWidget::kButtonStateActive) == 0 && (oldState & BaseWidget::kButtonStateActive) != 0)
        {
            this->fInterface->parameterControlReleased(BaseWidget::getId());
            return;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
