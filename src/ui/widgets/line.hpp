// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/base.hpp"
#include "../reference.hpp"

#include <array>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioLineWidget final : public LibreAudioWidget,
                                   private IdleCallback
{
    using R = LibreAudioReference::Line;

public:
    LibreAudioLineWidget(LibreAudioWidget* const parent)
        : LibreAudioWidget(parent)
    {
        colors[0] = Color::fromHTML("#ffbfcb");
        colors[1] = Color::fromHTML("#ffdfad");
        colors[2] = Color::fromHTML("#d2fdd3");
        colors[3] = Color::fromHTML("#bef1ff");
        colors[4] = Color::fromHTML("#c3d9ff");
        colors[5] = Color::fromHTML("#dac1f3");
        colors[6] = Color::fromHTML("#ffdcf5");

        addIdleCallback(this);
    }

private:
    struct Pt { float x; float y; };
    static constexpr int kStopWidth = 50;
    static constexpr int kNumStops = 6;
    std::array<Pt, kStopWidth * kNumStops + 1> pts;
    std::array<Color, kNumStops + 1> colors;

    static constexpr int WIN = 2;
    static constexpr int dbMax = 18;
    static constexpr int dbMin = -42;
    // static constexpr int rw = 868;
    // static constexpr int rh = 200;

    float ra() const
    {
        return 1.0 - (double)std::rand() / RAND_MAX * std::fmod(fInterface->getParameterValue(kParametersMainStart + 1), 1.01f) * 0.1f;
    }

    void tracePts(float hz, float amp, float ph)
    {
        for (int i = 0; i < pts.size(); i++) {
            float t = (float)i / (pts.size() - 1);
            float y = /*baseY(t)*/ getHeight() * 0.4f - (amp * std::sin(2.f * M_PI * (t * hz * WIN - ph))) * ra() * fScaleFactor;
            // y = std::max(-40.f, std::min(h + 40, y));
            pts[i] = { t * getWidth() + (float)fScaleFactor * 100.f * ra() - 100.f * (float)fScaleFactor, y };
        }
    }

    float h1 = 0.f;
    float ph1 = 0.f;
    float last = 0.f;

    void idleCallback() final
    {
        int t = getTime() * 1000;
        float phase = t * 0.0001f;
        float dt = phase - last;
        if (!(dt >= 0) || dt > 0.1) dt = 0.016;
        last = phase;

        const float k = std::min(1.f, dt * 7.f);

        h1 += (3.0f - h1) * k;
        ph1 += h1 * dt;
        repaint();
    }

    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

        tracePts(h1, fInterface->getParameterValue(kParametersMainStart) * 20, ph1);

        strokeWidth(3.f * fScaleFactor);

        scissor(0, 0, w, h);
        beginPath();
        moveTo(pts[0].x, pts[0].y);

        for (int i = 1, step = 0; i < kStopWidth * kNumStops; ++i)
        {
            if ((i % kStopWidth) == 0)
            {
                lineTo(pts[i].x, pts[i].y);
                strokePaint(linearGradient(pts[step * kStopWidth].x,
                                            pts[step * kStopWidth].y,
                                            pts[(step + 1) * kStopWidth].x,
                                            pts[(step + 1) * kStopWidth].y,
                                            colors[step],
                                            colors[step + 1]));
                stroke();
                ++step;

                beginPath();
                moveTo(pts[i].x, pts[i].y);
            }
            else
            {
                lineTo(pts[i].x, pts[i].y);
            }
        }

        lineTo(pts[kStopWidth * kNumStops].x, pts[kStopWidth * kNumStops].y);
        strokePaint(linearGradient(pts[kStopWidth * (kNumStops - 1)].x,
                                   pts[kStopWidth * (kNumStops - 1)].y,
                                   pts[kStopWidth * kNumStops].x,
                                   pts[kStopWidth * kNumStops].y,
                                   colors[(kNumStops - 1)],
                                   colors[kNumStops]));
        stroke();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
