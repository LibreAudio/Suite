// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"
#include "../_lab/metrics.hpp"

#include "../reference/parameter-button.hpp"
#include "../reference.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R, typename RType = float>
struct ComboBoxMetrics : LabMetrics<R, RType> {
    const uint labelWidth;
    const uint valueWidth;

    explicit ComboBoxMetrics(LabWidget* const widget, const uint wlabel, const uint wvalue)
        : LabMetrics<R, RType>(widget),
          labelWidth(wlabel),
          valueWidth(wvalue),
          box(widget, *this) {}

    const struct Box : LabMetrics<typename R::Box> {
        const RType x;
        const RType y;
        const RType width;
        const RType height;

        explicit Box(LabWidget* const widget, const ComboBoxMetrics& m)
            : LabMetrics<typename R::Box>(widget),
              x(m.labelWidth != 0 ? m.border + m.margin + m.padding + m.labelWidth : (m.width + m.padding) * 0.5f),
              y(m.border + m.margin),
              width(m.valueWidth != 0 ? (this->border + this->margin) * 2.f + m.valueWidth : m.width - m.border - m.margin - x),
              height(m.height - (m.border + m.margin) * 2.f) {}
    } box;
};

// --------------------------------------------------------------------------------------------------------------------

namespace Reference {
    // template<auto index>
    // struct AccentColor {
    // // //     static_assert(index >= 0 && index < 6, "accent index must be >= 0 && < 6");
    //     static constexpr const Color color = {}; // = kAccentColors[index];
    // };

    template<FaustParameterIndex index>
    struct AccentColor {
        static constexpr const Color color = kAccentColors[kFaustParameters[index].accentcolor];
    };
}

template<FaustParameterIndex index>
using ComboBoxReference = Reference::Widgets::ComboBox<Reference::AccentColor<index>::color>;

// --------------------------------------------------------------------------------------------------------------------

class ComboBoxInterface
{
public:
    virtual ~ComboBoxInterface() = default;
    virtual void maxBounds(uint& label, uint& value) = 0;
    virtual void setBounds(uint label, uint value) = 0;
};

template<FaustParameterIndex kParameterIndex>
class ComboBoxWidget final : public ReferenceParameterButtonWidget<ComboBoxReference<kParameterIndex>>,
                             public ComboBoxInterface,
                             private ButtonEventHandler::Callback
{
    using R = ComboBoxReference<kParameterIndex>;
    using BaseWidget = ReferenceParameterButtonWidget<R>;

    static constexpr const FaustParameter& kParameter = kFaustParameters[kParameterIndex];
    static_assert(kParameter.isEnumerator, "combo-box parameter must be enumerator");

public:
    explicit ComboBoxWidget(LabWidget* const parent)
        : BaseWidget(parent, kParameterIndex)
    {
        BaseWidget::setCallback(this);
    }

    void maxBounds(uint& label, uint& value) override
    {
        const ComboBoxMetrics<R> m { this, 0, 0 };

        Rectangle<float> bounds;
        BaseWidget::fontFace("regular");
        BaseWidget::fontSize(m.box.fontSize);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_TOP);
        BaseWidget::textLetterSpacing(m.box.letterSpacing);

        BaseWidget::textBounds(0, 0, BaseWidget::getName(), nullptr, bounds);
        label = std::max(label, d_roundToUnsignedInt(bounds.getWidth()));

        for (uint i = 0; i < kParameter.scalePointCount; ++i)
        {
            BaseWidget::textBounds(0, 0, kParameter.scalePoints[i].label, nullptr, bounds);
            value = std::max(value, d_roundToUnsignedInt(bounds.getWidth()));
        }
    }

    void setBounds(const uint label, const uint value) override
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
        items.reserve(kParameter.scalePointCount);

        for (uint i = 0; i < kParameter.scalePointCount; ++i)
            items.push_back(kParameter.scalePoints[i].label);

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
        DISTRHO_SAFE_ASSERT_INT_RETURN(pos >= 0 && pos < kParameter.scalePointCount, pos,);

        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::text(m.box.x + m.box.border + m.box.margin,
                         m.height * 0.5f,
                         kParameter.scalePoints[pos].label);
    }

    void updateSize(const bool updateChildren) override
    {
        BaseWidget::template updateReferenceSize<R>();

        if (fBounds.label == 0)
            return;

        const ComboBoxMetrics<R, uint> m { this, fBounds.label, fBounds.value };

        const uint basewidth = (m.border + m.margin + m.box.border + m.box.margin) * 2 + m.padding;
        BaseWidget::setWidth(basewidth + fBounds.label + fBounds.value);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
