// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "knob.hpp"
#include "Layout.hpp"

#include <memory>
#include <type_traits>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

template<class KnobWidget = LibreAudioSmallKnobWidget>
class LibreAudioKnobGroupWidget : public LibreAudioContainerSubWidget<LibreAudioReference::Widgets::KnobGroup>
{
    using R = LibreAudioReference::Widgets::KnobGroup;

    static constexpr const uint kMaxNumParameters = 10;

    std::vector<std::unique_ptr<LibreAudioSmallKnobWidget>> fKnobs;
    std::vector<std::unique_ptr<LibreAudioWidget>> fSpacers;

public:
    explicit LibreAudioKnobGroupWidget(LibreAudioWidget* const parent,
                                       const std::vector<FaustParameter>& parameters,
                                       const uint32_t idOffset = 0)
        : LibreAudioContainerSubWidget(parent)
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

        LibreAudioWidget::setHeight((border + margin) * 2 + knobHeight);
    }

private:
    void addSpacer()
    {
        std::unique_ptr<LibreAudioWidget> spacer { new LibreAudioEmptyWidget(this) };
        widgets.push_back({ spacer.get(), Expanding });
        fSpacers.emplace_back(std::move(spacer));
    }

    void addWidget() = delete;

    // void onNanoDisplay() final
    // {
    //     const float w = getWidth();
    //     const float h = getHeight();
    //     d_stdout("knob group size %f %f", w, h);
    //
    //     // TODO divider??
    // }
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
