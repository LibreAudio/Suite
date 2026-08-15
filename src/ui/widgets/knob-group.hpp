// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../reference.hpp"
#include "../base/container.hpp"
#include "knob.hpp"

#include "LibreAudioParameters.hpp"

#include "Layout.hpp"

#include <memory>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

template<class KnobWidget = LibreAudioSmallKnobWidget>
class LibreAudioKnobGroupWidget : public LibreAudioContainerWidget<LibreAudioReference::Widgets::KnobGroup>
{
    using R = LibreAudioReference::Widgets::KnobGroup;

    static constexpr const uint kMaxNumParameters = 10;

    std::vector<std::unique_ptr<LibreAudioSmallKnobWidget>> fKnobs;
    std::vector<std::unique_ptr<LibreAudioWidget>> fSpacers;

    struct Bracket {
        uint start;
        uint end;
        const char* label;
    };
    std::vector<Bracket> fBrackets;

public:
    explicit LibreAudioKnobGroupWidget(LibreAudioWidget* const parent,
                                       const std::vector<FaustParameter>& parameters,
                                       const uint32_t idOffset = 0)
        : LibreAudioContainerWidget(parent),
          fParameters(parameters),
          fParametersOffset(idOffset)
    {
        DISTRHO_SAFE_ASSERT_RETURN(!parameters.empty(),);

        fKnobs.reserve(kMaxNumParameters);
        fSpacers.reserve(kMaxNumParameters + 1);

        addSpacer();

        for (uint32_t i = 0, count = parameters.size(); i < count && widgets.size() < kMaxNumParameters * 2; ++i)
        {
            const FaustParameter& parameter = parameters[i];
            if (parameter.isEnumerator || parameter.isOutput) {
                d_stdout("knob-group skipped parameter %s", parameter.label);
                continue;
            }
            std::unique_ptr<KnobWidget> widget { new KnobWidget(this, parameter, idOffset + i) };
            widgets.push_back({ widget.get(), Fixed });
            fKnobs.emplace_back(std::move(widget));
            addSpacer();
        }

        // middle spacer
        {
            std::unique_ptr<LibreAudioWidget> spacer { new LibreAudioEmptyWidget<LibreAudioReference::Widgets::Knob>(this) };
            auto it = widgets.begin();
            for (uint i = 0, middle = widgets.size() / 2; i < middle; ++i)
                ++it;
            widgets.insert(it, { spacer.get(), Fixed });

            fSpacers.emplace_back(std::move(spacer));
        }

        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        uint knobHeight;

        if constexpr (R::height != 0)
            knobHeight = R::height * fScaleFactor;
        else if (! fKnobs.empty())
            knobHeight = fKnobs.front()->getHeight();
        else
            knobHeight = d_roundToUnsignedInt(fScaleFactor);

        const char* lastBracket = "";
        for (uint i = 0, size = fKnobs.size(); i < size; ++i)
        {
            const std::unique_ptr<LibreAudioSmallKnobWidget>& knob = fKnobs[i];

            const FaustParameter& parameter = fParameters.at(knob->getId() - fParametersOffset);

            if (std::strcmp(parameter.bracket, lastBracket) != 0)
            {
                if (fBrackets.empty())
                {
                    fBrackets.push_back({ i, i, parameter.bracket });
                }
                else
                {
                    if (*lastBracket != '\0')
                        fBrackets.back().end = i - 1;

                    if (*parameter.bracket != '\0')
                        fBrackets.push_back({ i, i, parameter.bracket });
                }
            }

            lastBracket = parameter.bracket;
        }

        if (*lastBracket != '\0')
            fBrackets.back().end = fKnobs.size() - 1;

        LibreAudioWidget::setHeight((border + margin) * 2 + knobHeight);
    }

private:
    const std::vector<FaustParameter>& fParameters;
    const uint32_t fParametersOffset;

    void addSpacer()
    {
        std::unique_ptr<LibreAudioEmptyWidget<>> spacer { new LibreAudioEmptyWidget(this) };
        widgets.push_back({ spacer.get(), Expanding });
        fSpacers.emplace_back(std::move(spacer));
    }

    void addWidget() = delete;

    void onNanoDisplay() final
    {
        for (const Bracket& bracket : fBrackets)
        {
            const LibreAudioSmallKnobWidget* const knobS = fKnobs[bracket.start].get();
            const LibreAudioSmallKnobWidget* const knobE = fKnobs[bracket.end - 1].get();

            const float lw = 2;
            const float sx = knobS->getAbsoluteX() - knobS->getWidth();
            const float ex = knobE->getAbsoluteX() + knobE->getWidth();
            const float y = 20;

            beginPath();
            fontSize(LibreAudioReference::Common::fontSize * fScaleFactor);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

            fillColor(Color(LibreAudioReference::Colors::ink.invert(), 0.5f));
            text(sx + (ex - sx) * 0.5f, 1 * fScaleFactor, bracket.label);

            fillColor(LibreAudioReference::Colors::ink3);
            text(sx + (ex - sx) * 0.5f, 0, bracket.label);

            Rectangle<float> bounds;
            textBounds(sx + (ex - sx) * 0.5f, 0, bracket.label, nullptr, bounds);

            strokeColor(LibreAudioReference::Colors::ink3);
            strokeWidth(lw);

            beginPath();
            moveTo(sx, y);
            lineTo(sx, 0);
            lineTo(bounds.getX() - 2, 0);
            stroke();

            beginPath();
            moveTo(bounds.getX() + bounds.getWidth() + 2, 0);
            lineTo(ex, 0);
            lineTo(ex, y);
            stroke();
        }

        // const float w = getWidth();
        // const float h = getHeight();
        //
        // beginPath();
        // roundedRect(0, 0, w, h, 4 * this->fScaleFactor);
        // fillColor(Color(1.f, 0.f, 0.f));
        // fill();

        // const float border = 18 * this->fScaleFactor;
        // const float radius = 4 * this->fScaleFactor;
        // const float feather = 28 * this->fScaleFactor;

        // fillPaint(boxGradient(0, 0, w, h, radius, feather, Color(0.f, 0.f, 0.f, 0.f), Color(0.f, 0.f, 0.f, 1.0f)));
        // fill();
        //
        // beginPath();
        // roundedRect(border * 0.5f, border * 0.5f, w - border, h - border, radius);
        // strokeColor(Color(0.f, 1.f, 0.f, 0.5f));
        // strokeWidth(border);
        // stroke();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioExpertKnobsGroupWidget : public LibreAudioKnobGroupWidget<>
{
public:
    explicit LibreAudioExpertKnobsGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioKnobGroupWidget(parent, getFaustParameters(), kParametersMainStart) {}
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
