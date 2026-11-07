// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/container.hpp"

#include "../widgets/pill-toggle.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<uint kMaxNumToggles = 2>
class PillAreaWidget : public LabReferenceContainerWidget<Reference::PillTogglesArea, Horizontal>
{
    using R = Reference::PillTogglesArea;
    using BaseWidget = LabReferenceContainerWidget<R, Horizontal>;

    std::list<std::unique_ptr<LabWidget>> fToggles;
    std::list<std::unique_ptr<LabWidget>> fSpacers;

public:
    explicit PillAreaWidget(LabWidget* const parent, const uint32_t parameterStart = 0)
        : BaseWidget(parent)
    {
        uint8_t numPills = 0;
        for (uint32_t i = parameterStart, count = std::size(kFaustParameters); i < count && numPills < kMaxNumToggles; ++i)
        {
            const FaustParameter& parameter = kFaustParameters[i];
            if (! parameter.isEnumerator || parameter.isOutput) {
                d_stdout("pill area skipped parameter %s", parameter.label);
                continue;
            }
            ++numPills;
        }

        if constexpr (kMaxNumToggles != 1)
        {
            if (numPills == 1)
                addSpacer();
        }

        static constexpr const SizeHint sizeHint = kMaxNumToggles == 1 ? Expanding : Fixed;

        for (uint32_t i = parameterStart, count = std::size(kFaustParameters); i < count && fToggles.size() < kMaxNumToggles; ++i)
        {
            const FaustParameter& parameter = kFaustParameters[i];
            if (! parameter.isEnumerator || parameter.isOutput) {
                d_stdout("pill area skipped parameter %s", parameter.label);
                continue;
            }
            d_stdout("using pill for parameter %s", parameter.label);
            std::unique_ptr<LabWidget> widget { new PillToggleWidget<sizeHint>(this, parameter, kParametersMainStart + i) };
            widgets.push_back({ widget.get(), sizeHint });
            fToggles.emplace_back(std::move(widget));

            if (numPills == 2)
            {
                numPills = 0;
                addSpacer();
            }
        }

        if constexpr (kMaxNumToggles != 1)
        {
            if (numPills == 1)
                addSpacer();
        }

        updateSize(true);
    }

private:
    void addSpacer()
    {
        std::unique_ptr<LabWidget> spacer { new LabEmptyWidget(this) };
        widgets.push_back({ spacer.get(), Expanding });
        fSpacers.emplace_back(std::move(spacer));
    }

    void addWidget() = delete;

    void updateSize(const bool updateChildren) final
    {
        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        uint pillHeight;

        if constexpr (R::height != 0)
            pillHeight = d_roundToUnsignedInt(R::height * fScaleFactor);
        else if (! fToggles.empty())
            pillHeight = fToggles.front()->getHeight();
        else
            pillHeight = d_roundToUnsignedInt(fScaleFactor);

        LabWidget::setHeight((border + margin) * 2 + pillHeight);

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

