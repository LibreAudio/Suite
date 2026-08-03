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
class LibreAudioKnobGroupWidget : public LibreAudioContainer<LibreAudioReference::Widgets::KnobGroup>
{
    using R = LibreAudioReference::Widgets::KnobGroup;

    std::vector<std::unique_ptr<LibreAudioSmallKnobWidget>> fKnobs;
    std::vector<std::unique_ptr<LibreAudioWidget>> fSpacers;

public:
    explicit LibreAudioKnobGroupWidget(LibreAudioWidget* const parent,
                                       const std::vector<FaustParameter>& parameters,
                                       const uint32_t idOffset = 0)
        : LibreAudioContainer(parent)
    {
        DISTRHO_SAFE_ASSERT_RETURN(!parameters.empty(),);

        static constexpr const uint kMaxNumParameters = 11;

        fKnobs.reserve(kMaxNumParameters);
        fSpacers.reserve(kMaxNumParameters + 1);

        addSpacer();

        for (uint32_t i = 0, count = parameters.size(); i < count && widgets.size() < kMaxNumParameters; ++i)
        {
            const FaustParameter& parameter = parameters[i];
            if (parameter.isOutput) {
                d_stdout("skipped parameter %s", parameter.label);
                continue;
            }
            std::unique_ptr<KnobWidget> widget { new KnobWidget(this, parameter, idOffset + i) };
            widgets.push_back({ widget.get(), Fixed });
            fKnobs.emplace_back(std::move(widget));
            addSpacer();
        }

        const uint border = R::border * fScaleFactor;
        const uint margin = R::margin * fScaleFactor;
        uint knobHeight;

        if constexpr (R::height != 0)
            knobHeight = R::height * fScaleFactor;
        else
            knobHeight = fKnobs.front()->getHeight();

        LibreAudioWidget::setHeight((border + margin) * 2 + knobHeight);
    }

private:
    void addSpacer()
    {
        std::unique_ptr<LibreAudioWidget> spacer { new LibreAudioEmptyWidget(this) };
        widgets.push_back({ spacer.get(), Expanding });
        fSpacers.emplace_back(std::move(spacer));
    }

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
