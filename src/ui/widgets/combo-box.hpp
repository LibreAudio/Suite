// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "button.hpp"
#include "../_lab/metrics.hpp"
#include "../reference.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R>
struct ComboBoxMetrics : LabMetrics<R> {
    explicit ComboBoxMetrics(LabWidget* const widget)
        : LabMetrics<R>(widget),
          box(widget, *this) {}

    const struct Box : LabMetrics<typename R::Box> {
        const float x;
        const float y;
        const float width;
        const float height;

        explicit Box(LabWidget* const widget, const ComboBoxMetrics& m)
            : LabMetrics<typename R::Box>(widget),
              x((m.width + m.padding) * 0.5f),
              y(m.border + m.margin),
              width(m.width - x - m.margin - m.border),
              height(m.height - (m.border + m.margin) * 2.f) {}
    } box;
};

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1, uint height = Reference::Widgets::ComboBox<>::height>
class ComboBoxWidget final : public ParameterReferenceButtonWidget<Reference::Widgets::ComboBox<kAccentColor, height>>,
                             private ButtonEventHandler::Callback
{
    using R = Reference::Widgets::ComboBox<kAccentColor, height>;
    using BaseWidget = ParameterReferenceButtonWidget<R>;

public:
    explicit ComboBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(this->fParameter.isEnumerator,);

        BaseWidget::setCallback(this);
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        std::vector<const char*> items;
        items.reserve(this->fParameter.scalePointCount);

        for (uint i = 0; i < this->fParameter.scalePointCount; ++i)
            items.push_back(this->fParameter.scalePoints[i].label);

        const ComboBoxMetrics<R> m { this };

        const Rectangle<uint> area {
            d_roundToUnsignedInt(BaseWidget::getAbsoluteX() + m.box.x),
            d_roundToUnsignedInt(BaseWidget::getAbsoluteY() + m.box.y),
            d_roundToUnsignedInt(m.box.width),
            d_roundToUnsignedInt(m.box.height),
        };
        this->fInterface->openMenu(area, R::color, BaseWidget::getId(), items);
    }

    void onNanoDisplay() final
    {
        const ComboBoxMetrics<R> m { this };

        BaseWidget::template drawReferenceBackground<R, R::backgroundColor>();
        BaseWidget::template drawReferenceBorder<R, R::borderColor>();

        // ------------------------------------------------------------------------------------------------------------
        // draw box background

        if constexpr (d_isNotZero(R::Box::backgroundColor.alpha))
        {
            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::roundedRect(m.box.x, m.box.y, m.box.width, m.box.height, m.box.borderRadius);
            else
                BaseWidget::rect(m.box.x, m.box.y, m.box.width, m.box.height);

            BaseWidget::fillColor(R::Box::backgroundColor);
            BaseWidget::fill();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw box border

        if constexpr (R::Box::border != 0)
        {
            BaseWidget::beginPath();

            if constexpr (R::Box::borderRadius != 0)
                BaseWidget::borderRoundedRect(m.box.x, m.box.y, m.box.width, m.box.height, m.box.border, m.box.borderRadius);
            else
                BaseWidget::borderRect(m.box.x, m.box.y, m.box.width, m.box.height, m.box.border);

            BaseWidget::strokeColor(BaseWidget::isHovered() ? R::Box::borderColor〡hovered : R::Box::borderColor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw labels

        BaseWidget::fillColor(R::color);
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.fontSize);
        BaseWidget::textLetterSpacing(m.letterSpacing);

        BaseWidget::textAlign(BaseWidget::ALIGN_RIGHT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text((m.width - m.padding) * 0.5f, m.height * 0.5f, BaseWidget::getName());

        const uint pos = d_roundToUnsignedInt(this->fInterface->getParameterValue(BaseWidget::getId()));
        DISTRHO_SAFE_ASSERT_INT_RETURN(pos >= 0 && pos < this->fParameter.scalePointCount, pos,);

        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text((m.width + m.padding) * 0.5f + m.box.margin, m.height * 0.5f, this->fParameter.scalePoints[pos].label);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
