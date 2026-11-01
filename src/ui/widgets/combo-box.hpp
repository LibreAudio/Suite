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
    const uint labelWidth;
    const uint valueWidth;

    explicit ComboBoxMetrics(LabWidget* const widget, const uint wlabel, const uint wvalue)
        : LabMetrics<R>(widget),
          labelWidth(wlabel),
          valueWidth(wvalue),
          box(widget, *this) {}

    const struct Box : LabMetrics<typename R::Box> {
        const float x;
        const float y;
        const float width;
        const float height;

        explicit Box(LabWidget* const widget, const ComboBoxMetrics& m)
            : LabMetrics<typename R::Box>(widget),
              x(m.labelWidth != 0 ? m.border + m.margin + m.padding + m.labelWidth : (m.width + m.padding) * 0.5f),
              y(m.border + m.margin),
              width(m.valueWidth != 0 ? (this->border + this->margin) * 2.f + m.valueWidth : m.width - m.border - m.margin - x),
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

    void maxBounds(uint& label, uint& value)
    {
        const ComboBoxMetrics<R> m { this, 0, 0 };

        Rectangle<float> bounds;
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.box.fontSize);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_TOP);
        BaseWidget::textLetterSpacing(m.box.letterSpacing);

        BaseWidget::textBounds(0, 0, BaseWidget::getName(), nullptr, bounds);
        label = std::max(label, d_roundToUnsignedInt(bounds.getWidth()));

        for (uint i = 0; i < this->fParameter.scalePointCount; ++i)
        {
            BaseWidget::textBounds(0, 0, this->fParameter.scalePoints[i].label, nullptr, bounds);
            value = std::max(value, d_roundToUnsignedInt(bounds.getWidth()));
        }
    }

    void setBounds(const uint label, const uint value)
    {
        fBounds.label = label;
        fBounds.value = value;
    }

private:
    struct Bounds{
        uint label;
        uint value;
    } fBounds;

    void buttonClicked(SubWidget*, int) final
    {
        std::vector<const char*> items;
        items.reserve(this->fParameter.scalePointCount);

        for (uint i = 0; i < this->fParameter.scalePointCount; ++i)
            items.push_back(this->fParameter.scalePoints[i].label);

        const ComboBoxMetrics<R> m { this, fBounds.label, fBounds.value };

        const Rectangle<uint> area {
            d_roundToUnsignedInt(BaseWidget::getAbsoluteX() + m.box.x),
            d_roundToUnsignedInt(BaseWidget::getAbsoluteY() + m.box.y),
            d_roundToUnsignedInt(m.box.width),
            d_roundToUnsignedInt(m.box.height),
        };
        this->fInterface->openMenu(area, R::color, BaseWidget::getId(), items);

        BaseWidget::resetCursor();
    }

    void onNanoDisplay() final
    {
        const ComboBoxMetrics<R> m { this, fBounds.label, fBounds.value };

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

            BaseWidget::strokeColor((BaseWidget::getState() & BaseWidget::kButtonStateActiveHover) != 0
                ? R::Box::borderColor〡hovered
                : R::Box::borderColor);
            BaseWidget::stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw labels

        const float xlabel = fBounds.label != 0
                            ? m.border + m.margin + fBounds.label
                            : (m.width - m.padding) * 0.5f;

        BaseWidget::fillColor(R::color);
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.fontSize);
        BaseWidget::textLetterSpacing(m.letterSpacing);

        BaseWidget::textAlign(BaseWidget::ALIGN_RIGHT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text(xlabel, m.height * 0.5f, BaseWidget::getName());

        const uint pos = d_roundToUnsignedInt(this->fInterface->getParameterValue(BaseWidget::getId()));
        DISTRHO_SAFE_ASSERT_INT_RETURN(pos >= 0 && pos < this->fParameter.scalePointCount, pos,);

        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text(m.box.x + m.box.border + m.box.margin,
                         m.height * 0.5f,
                         this->fParameter.scalePoints[pos].label);
    }

    void updateSize(const bool updateChildren) override
    {
        BaseWidget::template updateReferenceSize<R>();

        if (fBounds.label == 0)
            return;

        const ComboBoxMetrics<R> m { this, fBounds.label, fBounds.value };

        const uint basewidth = (m.border + m.margin + m.box.border + m.box.margin) * 2 + m.padding;
        BaseWidget::setWidth(basewidth + fBounds.label + fBounds.value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, class CB, FaustParameterIndex... Indexes>
class ComboBoxGroupWidget : public ReferenceContainerWidget<R, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    const std::array<std::shared_ptr<CB>, sizeof...(Indexes)> comboBoxes = {
        this->template addWidget<CB, Expanding>(Indexes)...
    };

public:
    ComboBoxGroupWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(true);
    }

protected:
    void updateSize(const bool updateChildren) override
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        // update children size first
        BaseWidget::updateSize(true);

        // find width for label and value fields
        uint wlabel = 0;
        uint wvalue = 0;
        for (const std::shared_ptr<CB>& cb : comboBoxes)
            cb->maxBounds(wlabel, wvalue);

        // set the same width for all fields
        for (const std::shared_ptr<CB>& cb : comboBoxes)
            cb->setBounds(wlabel, wvalue);

        // update children size again
        BaseWidget::updateSize(true);

        // set parent width the same as the children (plus margins)
        const LabMetrics<R> m { this };
        LabWidget::setWidth((m.border + m.margin) * 2 + comboBoxes[0]->getWidth());
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
