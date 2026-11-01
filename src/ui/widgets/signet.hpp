// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"

#include "../reference.hpp"

#include <algorithm>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

// Libre Audio signet, scaled to fit and centered within the widget size
class SignetWidget final : public LabReferenceWidget<Reference::Signet>
{
    using R = Reference::Signet;
    using BaseWidget = LabReferenceWidget<R>;

    static constexpr const float kShapeWidth = 32.f;
    static constexpr const float kShapeHeight = 16.576567f;

    static constexpr const float kStopOffsets[] = { 0.f, 0.17f, 0.34f, 0.5f, 0.671099f, 0.84f, 1.f };
    static constexpr const Color kStopColors[] = {
        { 0xff, 0xbf, 0xcb },
        { 0xff, 0xcf, 0xbc },
        { 0xe1, 0xd4, 0xdd },
        { 0xdd, 0xe8, 0xdb },
        { 0xdf, 0xf2, 0xde },
        { 0xdc, 0xd9, 0xe8 },
        { 0xff, 0xdc, 0xf5 },
    };
    static constexpr const uint kNumSections = sizeof(kStopOffsets) / sizeof(kStopOffsets[0]) - 1;

public:
    explicit SignetWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        const float w = getWidth();
        const float h = getHeight();
        const float shapeScale = std::min(w / kShapeWidth, h / kShapeHeight);

        // scissor edges are soft across 1 pixel, overlap them so the shape has no seams or faded outer edges
        const float overlap = 1.f / shapeScale;

        save();
        translate((w - kShapeWidth * shapeScale) * 0.5f, (h - kShapeHeight * shapeScale) * 0.5f);
        scale(shapeScale, shapeScale);

        // nanovg gradients have 2 colors, so draw one gradient per section between color stops
        for (uint i = 0; i < kNumSections; ++i)
        {
            const float left = kStopOffsets[i] * kShapeWidth;
            const float right = kStopOffsets[i + 1] * kShapeWidth;
            const float scissorLeft = i == 0 ? -overlap : left;

            save();
            intersectScissor(scissorLeft, -overlap, right + overlap - scissorLeft, kShapeHeight + overlap * 2);
            drawShape();
            fillPaint(linearGradient(left, 0, right, 0, kStopColors[i], kStopColors[i + 1]));
            fill();
            restore();
        }

        restore();
    }

    void drawShape()
    {
        beginPath();
        moveTo(2.900574f, 11.102262f);
        bezierTo(2.900574f, 12.609458f, 4.118099f, 13.833548f, 5.625272f, 13.841654f);
        lineTo(10.491557f, 13.867824f);
        bezierTo(11.503215f, 13.873224f, 12.435521f, 13.229267f, 12.916425f, 12.339204f);
        lineTo(14.543723f, 9.32738f);
        bezierTo(15.9405f, 6.299428f, 17.268992f, 3.810145f, 18.534819f, 1.853919f);
        bezierTo(20.168489f, -0.670776f, 23.891974f, -0.581155f, 25.401199f, 2.019945f);
        bezierTo(25.660458f, 2.466771f, 25.907063f, 2.935364f, 26.171729f, 3.477818f);
        bezierTo(26.328058f, 3.798225f, 26.488827f, 4.140098f, 26.689011f, 4.575357f);
        bezierTo(26.799364f, 4.815299f, 27.19644f, 5.685996f, 27.238447f, 5.777785f);
        bezierTo(27.744701f, 6.883988f, 28.1465f, 7.732981f, 28.63027f, 8.701584f);
        bezierTo(29.546877f, 10.536814f, 31.101265f, 13.657045f, 31.322422f, 14.096932f);
        lineTo(31.331622f, 14.115292f);
        bezierTo(31.401802f, 14.254855f, 31.590893f, 14.627932f, 31.898882f, 15.234522f);
        bezierTo(32.208415f, 15.844153f, 31.770127f, 16.567544f, 31.086432f, 16.572226f);
        lineTo(30.468894f, 16.576526f);
        bezierTo(29.769384f, 16.581226f, 29.128118f, 16.188351f, 28.813344f, 15.563647f);
        bezierTo(28.788794f, 15.514917f, 28.764504f, 15.466667f, 28.740274f, 15.418478f);
        lineTo(28.730874f, 15.399858f);
        bezierTo(28.506892f, 14.954354f, 26.949697f, 11.828488f, 26.035284f, 9.997651f);
        bezierTo(25.534664f, 8.99531f, 25.119697f, 8.118493f, 24.600892f, 6.984864f);
        bezierTo(24.557172f, 6.889334f, 24.161755f, 6.022274f, 24.053722f, 5.787377f);
        bezierTo(23.861996f, 5.370507f, 23.709972f, 5.047231f, 23.564826f, 4.749743f);
        bezierTo(23.328728f, 4.265844f, 23.113732f, 3.857312f, 22.892294f, 3.47567f);
        bezierTo(22.469885f, 2.747663f, 21.42755f, 2.722575f, 20.969972f, 3.42972f);
        bezierTo(19.780474f, 5.267986f, 18.507396f, 7.655509f, 17.156317f, 10.587613f);
        lineTo(17.137017f, 10.629503f);
        lineTo(17.115097f, 10.670083f);
        lineTo(15.468274f, 13.718048f);
        bezierTo(14.478177f, 15.550532f, 12.558724f, 16.573288f, 10.475898f, 16.562086f);
        lineTo(5.609673f, 16.535876f);
        bezierTo(2.506668f, 16.519187f, 0.f, 14.113881f, 0.f, 11.010831f);
        lineTo(0.f, 1.289144f);
        bezierTo(0.f, 0.577169f, 0.577169f, 0.f, 1.289144f, 0.f);
        lineTo(1.61143f, 0.f);
        bezierTo(2.323405f, 0.f, 2.900574f, 0.577169f, 2.900574f, 1.289144f);
        closePath();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
