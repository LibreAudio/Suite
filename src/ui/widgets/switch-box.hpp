// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"
#include "../_lab/interface.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

#include <vector>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// A parameter of a few values, drawn like a number box: its name at the top left, and under it, centred, a row of
// cells styled like mbComp5's listen / bypass buttons, one per value. A checkbox shows Off / On, an enumerator its
// scale points.

template<const Color& kAccentColor = Reference::Colors::acc1>
struct SwitchBoxReference : Reference::Widgets::NumberBox<kAccentColor> {
    struct Group {
        static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.07f };
        static constexpr const uint borderRadius = 6;
        static constexpr const uint margin = 2;
        static constexpr const uint padding = 2;
    };
    struct Cell {
        static constexpr const Color backgroundColor〡checked = kAccentColor;
        static constexpr const Color color = Reference::Colors::ink2;
        static constexpr const Color color〡checked = Color(0x17, 0x17, 0x1b);
        static constexpr const Color color〡hover = kAccentColor;
        static constexpr const float fontSize = 11.f;
        static constexpr const float letterSpacing = 0.f;
        static constexpr const uint borderRadius = 4;
        static constexpr const uint height = 16;
        static constexpr const uint margin = 5;
        static constexpr const uint minWidth = 18;
    };
};

template<const Color& kAccentColor = Reference::Colors::acc1>
class SwitchBoxWidget final : public LabWidget,
                              private IdleCallback
{
    using R = SwitchBoxReference<kAccentColor>;
    using BaseWidget = LabWidget;

    struct Option {
        const char* label;
        float value;
    };

public:
    // name, if given, replaces the parameter's name, such as a shorter one that fits the box
    explicit SwitchBoxWidget(LabWidget* const parent, const FaustParameterIndex id, const char* const name = nullptr)
        : BaseWidget(parent),
          fParameter(kFaustParameters[id])
    {
        setId(kParametersMainStart + id);
        setName(name != nullptr ? name : fParameter.name);

        if (fParameter.scalePointCount != 0)
        {
            for (uint i = 0; i < fParameter.scalePointCount; ++i)
                fOptions.push_back({ fParameter.scalePoints[i].label,
                                     static_cast<float>(fParameter.scalePoints[i].value) });
        }
        else
        {
            fOptions.push_back({ "Off", fParameter.min });
            fOptions.push_back({ "On", fParameter.max });
        }

        fCells.resize(fOptions.size());
        fValue = fParameter.init;

        addIdleCallback(this);
        updateSize(false);
    }

private:
    const FaustParameter& fParameter;
    std::vector<Option> fOptions;
    // cell areas as last drawn, for the mouse
    std::vector<Rectangle<float>> fCells;
    float fValue;
    int fHovered = -1;

    [[nodiscard]] int cellAt(const Point<double>& pos) const noexcept
    {
        for (size_t i = 0; i < fCells.size(); ++i)
            if (fCells[i].contains(pos.getX(), pos.getY()))
                return static_cast<int>(i);
        return -1;
    }

    // the option nearest the value
    [[nodiscard]] size_t selected() const noexcept
    {
        size_t index = 0;
        for (size_t i = 1; i < fOptions.size(); ++i)
            if (std::abs(fOptions[i].value - fValue) < std::abs(fOptions[index].value - fValue))
                index = i;
        return index;
    }

    void idleCallback() final
    {
        const float value = fInterface->getParameterValue(getId());

        if (d_isNotEqual(value, fValue))
        {
            fValue = value;
            repaint();
        }
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (ev.press && ev.button == 1)
        {
            const int cell = cellAt(ev.pos);

            if (cell >= 0)
            {
                fValue = fOptions[cell].value;
                fInterface->parameterControlPressed(getId());
                fInterface->parameterControlModified(getId(), fValue);
                fInterface->parameterControlReleased(getId());
                repaint();
                return true;
            }
        }

        return BaseWidget::onMouse(ev);
    }

    bool onMotion(const MotionEvent& ev) final
    {
        const int hovered = contains(ev.pos) ? cellAt(ev.pos) : -1;

        if (hovered != fHovered)
        {
            fHovered = hovered;
            repaint();
        }

        return BaseWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();

        const float s = fScaleFactor;
        const float border = R::border * s;
        const float border2x = border * 2;
        const float borderRadius = R::borderRadius * s;
        const float margin = R::margin * s;
        const float w = getWidth();
        const float h = getHeight();

        // inset shading
        beginPath();
        roundedRect(0, 0, w, h, borderRadius);
        fillPaint(linearGradient(0, 0, 0, margin, R::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::backgroundColor);
        strokeWidth(border * 0.5f);
        stroke();

        fillColor(R::Name::color);
        fontFace("regular");
        fontSize(R::Name::fontSize * s);
        textLetterSpacing(R::Name::letterSpacing * s);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(border2x + margin, border + margin, getName(), nullptr);

        // the cells share the widest label's width; the group is centred under the name
        fontSize(R::Cell::fontSize * s);
        textLetterSpacing(R::Cell::letterSpacing * s);
        textAlign(0);

        float cellWidth = R::Cell::minWidth * s;
        for (const Option& option : fOptions)
        {
            Rectangle<float> bounds;
            textBounds(0, 0, option.label, nullptr, bounds);
            cellWidth = std::max(cellWidth, bounds.getWidth() + R::Cell::margin * 2 * s);
        }

        const size_t count = fOptions.size();
        const float groupMargin = R::Group::margin * s;
        const float groupPadding = R::Group::padding * s;
        const float cellHeight = R::Cell::height * s;
        const float groupWidth = groupMargin * 2 + cellWidth * count + groupPadding * (count - 1);
        const float groupHeight = groupMargin * 2 + cellHeight;
        const float nameBottom = border + margin + R::Name::fontSize * s;
        const float gx = std::round((w - groupWidth) * 0.5f);
        const float gy = std::round(nameBottom + (h - border - margin - nameBottom - groupHeight) * 0.5f);

        beginPath();
        roundedRect(gx, gy, groupWidth, groupHeight, R::Group::borderRadius * s);
        fillColor(R::Group::backgroundColor);
        fill();

        const size_t checked = selected();
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        for (size_t i = 0; i < count; ++i)
        {
            const float x = gx + groupMargin + (cellWidth + groupPadding) * i;
            const float y = gy + groupMargin;

            fCells[i] = Rectangle<float>(x, y, cellWidth, cellHeight);

            if (i == checked)
            {
                beginPath();
                roundedRect(x, y, cellWidth, cellHeight, R::Cell::borderRadius * s);
                fillColor(R::Cell::backgroundColor〡checked);
                fill();
                fillColor(R::Cell::color〡checked);
            }
            else
            {
                fillColor(static_cast<int>(i) == fHovered ? R::Cell::color〡hover : R::Cell::color);
            }

            text(x + cellWidth * 0.5f, y + cellHeight * 0.5f, fOptions[i].label, nullptr);
        }
    }

    void updateSize(const bool updateChildren) final
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
