// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "button.hpp"
#include "../reference.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1>
class ComboBoxWidget final : public ParameterReferenceButtonWidget<Reference::Widgets::ComboBox<kAccentColor>>,
                             private ButtonEventHandler::Callback,
                             private IdleCallback
{
    using R = Reference::Widgets::ComboBox<kAccentColor>;
    using BaseWidget = ParameterReferenceButtonWidget<R>;

public:
    explicit ComboBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(this->fParameter.isEnumerator,);

        BaseWidget::addIdleCallback(this);
        BaseWidget::setCallback(this);
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        // TODO open menu
    }

    void idleCallback() final
    {
        // TODO change to new item position if needed
    }

    void onNanoDisplay() final
    {
        BaseWidget::template drawReferenceBackground<R, R::backgroundColor>();
        BaseWidget::template drawReferenceBorder<R, R::borderColor>();

        const float fScaleFactor = this->fScaleFactor;
        const float h = BaseWidget::getHeight();
        const float w = BaseWidget::getWidth();
        const float border = d_roundToIntPositive(R::border * fScaleFactor);
        const float margin = d_roundToIntPositive(R::margin * fScaleFactor);
        const float padding = d_roundToIntPositive(R::padding * fScaleFactor);

        const float bx = (w + padding) * 0.5f;
        const float by = border + margin;
        const float bwidth = w - bx - margin - border;
        const float bheight = h - (border + margin) * 2.f;
        const float bmargin = d_roundToIntPositive(R::Box::margin * this->fScaleFactor);

        // ------------------------------------------------------------------------------------------------------------
        // draw box background

        if constexpr (d_isNotZero(R::Box::backgroundColor.alpha))
        {
            const float boxr = d_roundToIntPositive(R::Box::borderRadius * fScaleFactor);

            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::roundedRect(bx, by, bwidth, bheight, boxr);
            else
                BaseWidget::rect(bx, by, bwidth, bheight);

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
                BaseWidget::borderRoundedRect(bx, by, bwidth, bheight, boxborder, boxr);
            else
                BaseWidget::borderRect(bx, by, bwidth, bheight, boxborder);

            BaseWidget::strokeColor(BaseWidget::isHovered() ? R::Box::borderColor〡hovered : R::Box::borderColor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw labels

        BaseWidget::fillColor(R::color);
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(R::fontSize * fScaleFactor);
        BaseWidget::textLetterSpacing(R::letterSpacing * fScaleFactor);

        BaseWidget::textAlign(BaseWidget::ALIGN_RIGHT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text((w - padding) * 0.5f, h * 0.5f, BaseWidget::getName());

        const uint pos = d_roundToUnsignedInt(this->fInterface->getParameterValue(BaseWidget::getId()));
        DISTRHO_SAFE_ASSERT_INT_RETURN(pos >= 0 && pos < this->fParameter.scalePointCount, pos,);

        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text((w + padding) * 0.5f + bmargin, h * 0.5f, this->fParameter.scalePoints[pos].label);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
