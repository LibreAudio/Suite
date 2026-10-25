// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

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
        BaseWidget::template drawReferenceBackground<R, R::backgroundColor>();
        BaseWidget::template drawReferenceBorder<R, R::borderColor>();

        const float fScaleFactor = this->fScaleFactor;
        const float h = BaseWidget::getHeight();
        const float border = d_roundToIntPositive(R::border * fScaleFactor);
        const float margin = d_roundToIntPositive(R::margin * fScaleFactor);
        const float padding = d_roundToIntPositive(R::padding * fScaleFactor);

        const float bxy = border + margin;
        const float bsize = h - bxy * 2.f;

        // ------------------------------------------------------------------------------------------------------------
        // draw box background

        if constexpr (d_isNotZero(R::Box::backgroundColor.alpha))
        {
            const float boxr = d_roundToIntPositive(R::Box::borderRadius * fScaleFactor);

            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::roundedRect(bxy, bxy, bsize, bsize, boxr);
            else
                BaseWidget::rect(bxy, bxy, bsize, bsize);

            BaseWidget::fillColor(R::Box::backgroundColor);
            BaseWidget::fill();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw box border

        if constexpr (R::Box::border != 0)
        {
            const float boxborder = d_roundToIntPositive(R::Box::border * fScaleFactor);
            const float boxr = d_roundToIntPositive(R::Box::borderRadius * fScaleFactor);

            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::borderRoundedRect(bxy, bxy, bsize, bsize, boxborder, boxr);
            else
                BaseWidget::borderRect(bxy, bxy, bsize, bsize, boxborder);

            BaseWidget::strokeColor(BaseWidget::isHovered() ? R::Box::borderColor〡hovered : R::Box::borderColor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw checkmark

        if (BaseWidget::isChecked())
        {
            BaseWidget::beginPath();
            BaseWidget::moveTo(bxy + bsize * 0.3f, bxy + bsize * 0.5f);
            BaseWidget::lineTo(bxy + bsize * 0.44f, bxy + bsize * 0.666f);
            BaseWidget::lineTo(bxy + bsize * 0.75f, bxy + bsize * 0.35f);
            BaseWidget::lineCap(BaseWidget::ROUND);
            BaseWidget::lineJoin(BaseWidget::ROUND);
            BaseWidget::strokeColor(R::color〡checked);
            BaseWidget::strokeWidth(2.f * this->fScaleFactor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw label

        BaseWidget::fillColor(R::color〡checked);
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(R::fontSize * fScaleFactor);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::textLetterSpacing(R::letterSpacing * fScaleFactor);
        BaseWidget::text(bxy + bsize + padding, h * 0.5f, BaseWidget::getName());
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
