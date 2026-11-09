// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"
#include "../_lab/metrics.hpp"

#include "../reference/parameter-button.hpp"
#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R, typename RType = float>
struct CheckBoxMetrics : LabMetrics<R, RType> {
    explicit CheckBoxMetrics(LabWidget* const widget)
        : LabMetrics<R, RType>(widget),
          box(widget, *this) {}

    const struct Box : LabMetrics<typename R::Box> {
        const RType checkmark;
        const RType xy;
        const RType size;

        explicit Box(LabWidget* const widget, const CheckBoxMetrics& m)
            : LabMetrics<typename R::Box>(widget),
              checkmark(2 * m.s),
              xy(m.border + m.margin),
              size(m.height - xy * 2) {}
    } box;
};

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex index>
using CheckBoxReference = Reference::Widgets::CheckBox<Reference::AccentColor<index>::color>;

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex>
class CheckBoxWidget : public ReferenceParameterButtonWidget<CheckBoxReference<kParameterIndex>>,
                       private ButtonEventHandler::Callback,
                       private IdleCallback
{
    using R = CheckBoxReference<kParameterIndex>;
    using BaseWidget = ReferenceParameterButtonWidget<R, kCornerAuto>;
    using State = typename BaseWidget::State;

    static constexpr const FaustParameter& kParameter = kFaustParameters[kParameterIndex];
    static_assert(kParameter.isBoolean, "check-box parameter must be boolean");

public:
    explicit CheckBoxWidget(LabWidget* const parent)
        : BaseWidget(parent, kParameterIndex)
    {
        BaseWidget::addIdleCallback(this);
        BaseWidget::setCallback(this);
        BaseWidget::updateSize(false);
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        this->fInterface->parameterControlModified(
            BaseWidget::getId(), BaseWidget::isChecked() ? kParameter.max : kParameter.min);
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

            BaseWidget::strokeColor((BaseWidget::getState() & BaseWidget::kButtonStateActiveHover) != 0
                ? R::Box::borderColor〡hovered
                : R::Box::borderColor);
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
        BaseWidget::stateChanged(state, oldState);

        if ((state & BaseWidget::kButtonStateActive) != 0)
        {
            if ((oldState & BaseWidget::kButtonStateActive) == 0)
                this->fInterface->parameterControlPressed(BaseWidget::getId());
        }
        else
        {
            if ((oldState & BaseWidget::kButtonStateActive) != 0)
                this->fInterface->parameterControlReleased(BaseWidget::getId());
        }
    }

    void updateSize(const bool updateChildren) final
    {
        const CheckBoxMetrics<R, uint> m { this };

        // NOTE inner box depends on height, so we can't always rely on it here
        uint boxsize;
        if constexpr (R::height != 0)
            boxsize = m.box.size;
        else
            boxsize = d_roundToUnsignedInt(m.fontSize);

        Rectangle<float> bounds;
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.fontSize);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_TOP);
        BaseWidget::textLetterSpacing(m.letterSpacing);
        BaseWidget::textBounds(0, 0, BaseWidget::getName(), nullptr, bounds);

        const uint basewidth = (m.border + m.margin + m.box.border + m.box.margin) * 2 + m.padding;
        BaseWidget::setWidth(basewidth + boxsize + d_roundToUnsignedInt(bounds.getWidth()));
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
