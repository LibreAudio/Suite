// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "../_lab/base.hpp"
#include "../reference.hpp"

namespace LibreAudio {

// Paths from resources/images/top-bar-logo.svg (LibreAudioLogoDraft01).
class TopBarLogoWidget final : public LabWidget
{
    using BaseWidget = LabWidget;
public:
    explicit TopBarLogoWidget(LabWidget* parent) : LabWidget(parent) { updateSize(false); }
private:
    void onNanoDisplay() final
    {
        const float size = Reference::TopBar::height * fScaleFactor;
        save();
        translate((getWidth() - size) * .5f, (getHeight() - size) * .5f);
        const float pathScale = size / 34.f;
        scale(pathScale, pathScale);
        beginPath();
        moveTo(17.0f, 0.0f);
        bezierTo(26.3888407f, 0.0f, 34.0f, 7.61115925f, 34.0f, 17.0f);
        bezierTo(34.0f, 26.3888407f, 26.3888407f, 34.0f, 17.0f, 34.0f);
        bezierTo(7.61115925f, 34.0f, 0.0f, 26.3888407f, 0.0f, 17.0f);
        bezierTo(0.0f, 7.61115925f, 7.61115925f, 0.0f, 17.0f, 0.0f);
        closePath();
        pathWinding(CCW);
        moveTo(17.0f, 1.0f);
        bezierTo(8.163444f, 1.0f, 1.0f, 8.163444f, 1.0f, 17.0f);
        bezierTo(1.0f, 25.836556f, 8.163444f, 33.0f, 17.0f, 33.0f);
        bezierTo(25.836556f, 33.0f, 33.0f, 25.836556f, 33.0f, 17.0f);
        bezierTo(33.0f, 8.163444f, 25.836556f, 1.0f, 17.0f, 1.0f);
        closePath();
        pathWinding(CW);
        moveTo(12.2055198f, 11.7900544f);
        lineTo(9.19621309f, 21.5123818f);
        bezierTo(9.04083263f, 22.0143778f, 9.4160684f, 22.5233224f, 9.9415615f, 22.5233224f);
        lineTo(14.1082736f, 22.5225187f);
        lineTo(17.8565589f, 10.3377331f);
        bezierTo(18.826139f, 7.18564418f, 23.1974703f, 7.06497279f, 24.3829556f, 10.0629537f);
        lineTo(24.4514997f, 10.250401f);
        lineTo(27.9817928f, 20.7312439f);
        bezierTo(28.6668551f, 22.7650782f, 27.1539482f, 24.8711011f, 25.0078373f, 24.8711011f);
        lineTo(23.1072736f, 24.8705187f);
        lineTo(23.8292736f, 22.5125187f);
        lineTo(25.0078373f, 22.5132064f);
        bezierTo(25.5414268f, 22.5132064f, 25.9175821f, 21.989584f, 25.7472545f, 21.48391f);
        lineTo(22.2169614f, 11.0030671f);
        bezierTo(21.8738171f, 9.9843297f, 20.4262921f, 10.0034982f, 20.1102442f, 11.0309649f);
        lineTo(16.5742736f, 22.5225187f);
        lineTo(20.3232736f, 22.5215187f);
        lineTo(19.6339123f, 24.7788379f);
        lineTo(19.6012736f, 24.8795187f);
        lineTo(15.8492736f, 24.8805187f);
        lineTo(15.6250246f, 25.6123411f);
        bezierTo(15.4809846f, 26.080613f, 15.7981901f, 26.5542767f, 16.267903f, 26.6153507f);
        lineTo(16.3707776f, 26.6219705f);
        lineTo(19.1262736f, 26.6215187f);
        lineTo(18.4042736f, 28.9795187f);
        lineTo(16.3707776f, 28.9798652f);
        bezierTo(14.259539f, 28.9798652f, 12.7506256f, 26.9370397f, 13.3713393f, 24.9191094f);
        lineTo(13.3832736f, 24.8805187f);
        lineTo(9.9415615f, 24.8812171f);
        bezierTo(7.88840142f, 24.8812171f, 6.40553998f, 22.9495283f, 6.89537465f, 20.9882683f);
        lineTo(6.94375049f, 20.8151876f);
        lineTo(9.95305721f, 11.0928601f);
        bezierTo(10.2864584f, 10.015723f, 9.22897151f, 9.0295834f, 8.17770812f, 9.43729153f);
        lineTo(7.32512646f, 7.23893484f);
        bezierTo(10.2150251f, 6.11815468f, 13.122032f, 8.82902961f, 12.2055198f, 11.7900544f);
        closePath();
        pathWinding(CCW);
        // SVG Display-P3 fill converted to sRGB for NanoVG.
        fillColor(Color(1.0000000f, 0.8733699f, 0.6738767f));
        fill();
        restore();
    }
    void updateSize(bool updateChildren) final
    {
        const float size = Reference::TopBar::height * fScaleFactor;
        if (getWidth() == getHeight()) setSize(size, size);
        else if (getWidth() > getHeight()) setHeight(size);
        else setWidth(size);
        BaseWidget::updateSize(updateChildren);
    }
};
}
