// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/image.hpp"
#include "../_lab/interface.hpp"

#include "../containers/button-group.hpp"
#include "../containers/container.hpp"
#include "../reference/button.hpp"
#include "../widgets/plugin-name.hpp"

#include <array>

#include "las-resources.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class TopBar : public LabReferenceContainerWidget<Reference::TopBar, Horizontal>,
               private IdleCallback
{
    using BaseWidget = LabReferenceContainerWidget<Reference::TopBar, Horizontal>;

    // ----------------------------------------------------------------------------------------------------------------

    using LogoWidget = LabImageWidget<IMAGES_LA_68_PNG_DATA, IMAGES_LA_68_PNG_LEN>;

    template<Corner corner, const uchar* imageData, uint imageDataSize>
    using Button = ImageButtonWidget<Reference::Widgets::TopBarButton, corner, imageData, imageDataSize>;

    template<Corner corner, const uchar* imageData, uint imageDataSize>
    using BypassButton = ImageButtonWidget<Reference::Widgets::TopBarButton〡Bypass, corner, imageData, imageDataSize>;

    template<Corner corner, const uchar* imageData, uint imageDataSize, const uchar* imageData2, uint imageDataSize2>
    using DualButton = DualImageButtonWidget<Reference::Widgets::TopBarButton, corner, imageData, imageDataSize, imageData2, imageDataSize2>;

    template<Corner corner, const char* text>
    using TextButton = StaticTextButtonWidget<Reference::Widgets::TopBarButton, corner, text>;

    template<typename... PB>
    using ButtonGroup = StaticButtonGroupWidget<Reference::Widgets::TopBarButtonGroup, PB...>;

    // ----------------------------------------------------------------------------------------------------------------

    using UndoRedoGroupWidget = ButtonGroup<
        WidgetPair<kWidgetUndo, Button<kCornerLeft, IMAGES_UNDO_28_PNG_DATA, IMAGES_UNDO_28_PNG_LEN>>,
        WidgetPair<kWidgetRedo, Button<kCornerRight, IMAGES_REDO_28_PNG_DATA, IMAGES_REDO_28_PNG_LEN>>
    >;

    static constexpr const char kA[] = "A";
    static constexpr const char kB[] = "B";
    static constexpr const char kC[] = "C";
    static constexpr const char kD[] = "D";
    using SnapshotsGroupWidget = ButtonGroup<
        WidgetPair<kWidgetSnapshotCopy, DualButton<kCornerLeft, IMAGES_X_28_PNG_DATA, IMAGES_X_28_PNG_LEN, IMAGES_COPY_28_PNG_DATA, IMAGES_COPY_28_PNG_LEN>>,
        WidgetPair<kWidgetSnapshotSlotA, TextButton<kCornerNone, kA>>,
        WidgetPair<kWidgetSnapshotSlotB, TextButton<kCornerNone, kB>>,
        WidgetPair<kWidgetSnapshotSlotC, TextButton<kCornerNone, kC>>,
        WidgetPair<kWidgetSnapshotSlotD, TextButton<kCornerRight, kD>>
    >;

    static constexpr const char kEasy[] = "Easy";
    static constexpr const char kExpert[] = "Expert";
    using EasyExpertGroupWidget = ButtonGroup<
        WidgetPair<kWidgetEasy, TextButton<kCornerLeft, kEasy>>,
        WidgetPair<kWidgetExpert, TextButton<kCornerRight, kExpert>>
    >;

    using MenuPowerGroupWidget = ButtonGroup<
        WidgetPair<kWidgetMenu, Button<kCornerLeft, IMAGES_MENU_28_PNG_DATA, IMAGES_MENU_28_PNG_LEN>>,
        WidgetPair<kWidgetPower, BypassButton<kCornerRight, IMAGES_POWER_28_PNG_DATA, IMAGES_POWER_28_PNG_LEN>>
    >;

    // ----------------------------------------------------------------------------------------------------------------

    const std::array<std::shared_ptr<LabWidget>, 3> fWidgetsLeft = {
        addWidget<LogoWidget>(),
        addWidget<PluginNameWidget>(),
        addSpacer(),
    };
    const std::array<std::shared_ptr<LabWidget>, 2> fWidgetsExpert = {
        addWidget<UndoRedoGroupWidget>(),
        addWidget<SnapshotsGroupWidget>(),
    };
    const std::array<std::shared_ptr<LabWidget>, 2> fWidgetsRight = {
        addWidget<EasyExpertGroupWidget>(),
        addWidget<MenuPowerGroupWidget>()
    };

    // ----------------------------------------------------------------------------------------------------------------

public:
    TopBar(LabTopLevelWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(true);
        idleCallback();

        addIdleCallback(this);
    }

private:
    Page fLastPage = kPageInit;

    void idleCallback() final
    {
        if (const Page page = getCurrentPage(fInterface); fLastPage != page)
        {
            switch (page)
            {
            case kPageAbout:
            case kPageSettings:
                if (fLastPage != kPageInit)
                    break;
                [[fallthrough]];

            case kPageEasy:
            case kPageExpert:
                for (const std::shared_ptr<LabWidget>& widget : fWidgetsExpert)
                    widget->setVisible(page == kPageExpert);

                if (fLastPage != kPageInit)
                    updateSize(true);

                fLastPage = page;
                break;
            }
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

}
