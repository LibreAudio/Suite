// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/_lab/color.hpp"

#include "ui/reference.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/toggle-switch.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

struct ReferenceTopBar : Reference::TopBar {
    // static constexpr const Color backgroundColor = Reference::Colors::ink3;
    static constexpr const uint height = Reference::Widgets::ToggleSwitch<1, true>::height;
};

class DualGainTopBarWidget : public ReferenceContainerWidget<ReferenceTopBar>
{
    using BaseWidget = ReferenceContainerWidget<ReferenceTopBar>;

    static constexpr const float kColor1[] = { 0.3f, 0.1f, 0.05f, 1.f };
    static constexpr const float kColor2[] = { 0.1f, 0.3f, 0.05f, 1.f };
    std::shared_ptr<LabWidget> w1 = addWidget<LabColorWidget<kColor1>, Expanding>();
    std::shared_ptr<LabWidget> w2 = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTriml);
    std::shared_ptr<LabWidget> w2b = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTriml);
    std::shared_ptr<LabWidget> w2c = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTriml);
    std::shared_ptr<LabWidget> w2e = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTriml);
    std::shared_ptr<LabWidget> w3 = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTrimr);
    std::shared_ptr<LabWidget> w3b = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTrimr);
    std::shared_ptr<LabWidget> w3c = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTrimr);
    std::shared_ptr<LabWidget> w3d = addWidget<NumberBoxWidget, Expanding>(kFaustParameterTrimr);
    std::shared_ptr<LabWidget> w4 = addWidget<ToggleSwitchWidget<1, true>, Expanding>(kCommonParameterBypass, "Bypass");

public:
    explicit DualGainTopBarWidget(LabTopLevelWidget* const parent)
        : BaseWidget(parent) {}
};

struct ReferenceMainArea : Reference::MainArea {
    // static constexpr const Color backgroundColor = Reference::Colors::bg0;
    static constexpr const Color borderColor = Reference::Colors::ink;
    static constexpr const uint border = 4;
    static constexpr const uint borderRadius = 20;
};

using DualGainMainAreaWidget = MainAreaContainerWidget<LabReferenceWidget<ReferenceMainArea>>;
using DualGainRootWidget = RootWidget<DualGainTopBarWidget, DualGainMainAreaWidget>;

}

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DualGainRootWidget, SHADERS_FFT_WAVEFORM_FRAG_DATA, SHADERS_FFT_WAVEFORM_FRAG_LEN>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
