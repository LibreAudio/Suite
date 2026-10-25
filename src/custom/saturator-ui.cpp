// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

struct ReferenceTopBar : Reference::TopBar {
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height;
};

class SaturatorAlwaysOnButton : public TextButtonWidget<kCornerBoth>,
                                private IdleCallback
{
    using BaseWidget = TextButtonWidget<kCornerBoth>;

public:
    explicit SaturatorAlwaysOnButton(LabWidget* const parent, const uint id)
        : BaseWidget(parent, id, "Always On")
    {
        addIdleCallback(this);
        setCheckable(true);
        setName("always");
    }

private:
    void idleCallback() final
    {
        if (isChecked())
        {
            const float db = fInterface->getParameterValue(kParametersMainStart + kFaustParameterAuto_makeup_db);
            fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterMakeup);
            fInterface->parameterControlModified(kParametersMainStart + kFaustParameterMakeup, db);
            fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterMakeup);
        }
    }
};

class SaturatorAutoButton : public TextButtonWidget<kCornerBoth>,
                            private ButtonEventHandler::Callback
{
    using BaseWidget = TextButtonWidget<kCornerBoth>;

public:
    explicit SaturatorAutoButton(LabWidget* const parent, const uint id)
        : BaseWidget(parent, id, "Apply Auto")
    {
        setCallback(this);
        setName("auto");
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        const float db = fInterface->getParameterValue(kParametersMainStart + kFaustParameterAuto_makeup_db);
        fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterMakeup);
        fInterface->parameterControlModified(kParametersMainStart + kFaustParameterMakeup, db);
        fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterMakeup);
    }
};

class SaturatorTopBarWidget : public ReferenceContainerWidget<ReferenceTopBar>
{
    using BaseWidget = ReferenceContainerWidget<ReferenceTopBar>;

    std::array<std::shared_ptr<LabWidget>, 11> widgets {
        addSpacer(),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterAttack_time),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterDecay_time),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterPush),
        addSpacer(),
        addWidget<SaturatorAutoButton>(kParametersMainStart + kFaustParameterAuto_makeup_db),
        addWidget<SaturatorAlwaysOnButton>(kParametersMainStart + kFaustParameterAuto_makeup_db),
        addSpacer(),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterDrywet),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterMakeup),
        addSpacer(),
    };

public:
    explicit SaturatorTopBarWidget(LabTopLevelWidget* const parent)
        : BaseWidget(parent) {}
};

using SaturatorRootWidget = RootWidget<SaturatorTopBarWidget, MainAreaContainerWidget<>>;

}

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::SaturatorRootWidget, SHADERS_FFT_WAVEFORM_FRAG_DATA, SHADERS_FFT_WAVEFORM_FRAG_LEN>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
