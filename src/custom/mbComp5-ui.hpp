// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/base.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace LibreAudio::MbCompResponse {
    // Magnitude of the same bilinear-transform shelves as mbComp5.dsp. This is
    // the instantaneous wet response, before the DSP's dry/wet blend.
    inline double shelf(const double f, const double fc, const double gain, const int slope, const double sr)
    {
        const double r = std::tan(M_PI * std::min(f, sr * 0.499) / sr) / std::tan(M_PI * fc / sr);
        if (slope == 0)
        {
            const double g = std::pow(10.0, gain / 20.0);
            return 10.0 * std::log10((g*g + r*r) / (1.0 + r*r));
        }
        const auto svf = [r](const double db, const double q) {
            const double a = std::pow(10.0, db / 40.0);
            const double u = r * std::sqrt(a), u2 = u*u;
            const double num = (a*a-u2)*(a*a-u2) + a*a*u2/(q*q);
            const double den = (1.0-u2)*(1.0-u2) + u2/(q*q);
            return 10.0 * std::log10(num / den);
        };
        return slope == 1 ? svf(gain, 0.70710678)
        : svf(gain * 0.5, 0.54119610) + svf(gain * 0.5, 1.30656296);
    }

    inline double response(const double f, const std::array<float, 4>& xo,
                           const std::array<double, 5>& gains, const int slope, const double sr)
    {
        double db = gains[4];
        for (size_t i = 0; i < xo.size(); ++i)
            db += shelf(f, xo[i], gains[i] - gains[i+1], slope, sr);
        return db;
    }
} // namespace LibreAudio::MbCompResponse

namespace LibreAudio {

// using Param = FaustParameterIndex;

constexpr std::array<std::array<FaustParameterIndex, 7>, 5> bands {{
#define BAND(n) {{ kFaustParameterBand##n##_threshold, kFaustParameterBand##n##_ratio, \
    kFaustParameterBand##n##_makeup, kFaustParameterBand##n##_attack, kFaustParameterBand##n##_release, \
    kFaustParameterBand##n##_listen, kFaustParameterBand##n##_bypass }}
    BAND(1), BAND(2), BAND(3), BAND(4), BAND(5)
#undef BAND
}};

constexpr std::array<std::array<FaustParameterIndex, 2>, 5> reductions {{
#define GR(n) {{ kFaustParameterGr_band##n##_mid, kFaustParameterGr_band##n##_side }}
    GR(1), GR(2), GR(3), GR(4), GR(5)
#undef GR
}};

constexpr std::array<Color, 5> colors {{ {255,191,203}, {255,223,173}, {210,253,211}, {190,241,255}, {218,193,243} }};

struct Box {
    float x, y, w, h;
};

class MbComp5GraphWidget : public LabWidget
{
    Box _box;
    std::array<float, 4> _xo;

public:
    explicit MbComp5GraphWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
    }

    void setFreqs(const std::array<float, 4>& xo)
    {
        _xo = xo;
    }

    void setBox(float margin, float width, float height)
    {
        _box.x = 0;
        _box.y = margin / fScaleFactor;
        _box.w = width / fScaleFactor;
        _box.h = height / fScaleFactor;
    }

private:
    bool bypassed() const { return fInterface->getParameterValue(kCommonParameterBypass) > .5f; }
    float value(int p) const { return fInterface->getParameterValue(kParametersMainStart + p); }
    double sampleRate() const { return dynamic_cast<DISTRHO_NAMESPACE::UI*>(fInterface)->getSampleRate(); }
    bool anyListen() const { for (const auto& b : bands) if (value(b[5]) > .5f) return true; return false; }
    bool live(int b) const { return !bypassed() && value(bands[b][6]) < .5f && (!anyListen() || value(bands[b][5]) > .5f); }

    float yDb(float db) const { return _box.y+(6-db) / 24 * _box.h; }

    Color rainbow(float t, float alpha = 1) const
    {
        const float p = std::clamp(t, 0.f, 1.f) * 4;
        int i = std::min(3, static_cast<int>(p));
        const float f = p - i;
        Color c = colors[i];
        for (int k = 0; k < 3; ++k)
            c.rgba[k] = colors[i].rgba[k] * (1 - f) + colors[i + 1].rgba[k] * f;
        c.alpha = alpha;
        return c;
    }

    void onNanoDisplay() final
    {
        // return;

        std::array<std::array<double, 5>, 2> gains {};

        for (int c = 0; c < 2; ++c)
        {
            for(int b = 0; b < 5; ++b)
            {
                gains[c][b] = bypassed() ? 0 :
                    anyListen() && value(bands[b][5]) < .5f ? -18 :
                        value(bands[b][6]) > .5f ? 0 :
                            value(reductions[b][c]) + value(bands[b][2]);
            }
        }

        save();
        scale(fScaleFactor, fScaleFactor);

#if 1
        // The solid mid and finer side curves use the real per-channel GR.
        // Match the DSP shelf cascade, including sample-rate frequency warping.
        // scissor(0,0,_box.w,h);

        constexpr int steps = 256;
        const int slope=std::clamp(static_cast<int>(std::round(value(kFaustParameterSlope))), 0, 2);

        for (int c = 1; c >= 0; --c)
        {
            std::array<float, steps + 1> ys {};

            for(int i = 0; i <= steps; ++i)
                ys[i] = yDb(MbCompResponse::response(
                    norm2freq(i / static_cast<double>(steps)), _xo, gains[c], slope, sampleRate()));

            const auto path = [&]() {
                beginPath();
                moveTo(0, ys[0]);
                for(int i = 1; i <= steps;++i)
                    lineTo(i * _box.w / steps, ys[i]);
            };

            if (c == 0)
            {
                const float zero = yDb(0);
                const auto fillSegment = [&](float left, float y1, float right, float y2)
                {
                    const float edge = std::abs(y1 - zero) > std::abs(y2 - zero) ? y1 : y2;
                    if (std::abs(edge - zero) < .01f)
                        return;
                    const float t = (left + right) / (2 * _box.w);
                    beginPath();
                    moveTo(left, zero);
                    lineTo(left, y1);
                    lineTo(right, y2);
                    lineTo(right, zero);
                    closePath();
                    fillPaint(linearGradient(0, zero, 0, edge, rainbow(t, .015f), rainbow(t, .24f)));
                    fill();
                };
                // Fade toward 0 dB for both gain and reduction, following the curve's colors.
                // save();
                // nvgShapeAntiAlias(getContext(),0); // FIXME
                for(int i = 0; i < steps; ++i)
                {
                    const float left = i * _box.w / steps;
                    const float right = (i + 1) * _box.w / steps;
                    if((ys[i] - zero) * (ys[i + 1] - zero)<0) {
                        const float crossing=left+(right-left)*(zero-ys[i])/(ys[i+1]-ys[i]);
                        fillSegment(left,ys[i],crossing,zero);
                        fillSegment(crossing,zero,right,ys[i+1]);
                    } else {
                        fillSegment(left,ys[i],right,ys[i+1]);
                    }
                }
                // restore();
            }

            for (int section = 0; section < 4; ++section)
            {
                const float left = section * _box.w / 4;
                const float right = (section + 1) * _box.w / 4;
                save();
                intersectScissor(left, 0, right - left, _box.h);
                if (c == 0)
                {
                    path();
                    strokeColor(rainbow((section + .5f) / 4, bypassed() ? .04f : .13f));
                    strokeWidth(6);
                    stroke();
                }
                path();
                const float opacity = bypassed() ? .25f : c == 0 ? .95f : .5f;
                strokePaint(linearGradient(left, 0, right, 0, Color(colors[section], opacity), Color(colors[section + 1], opacity)));
                strokeWidth(c == 0 ? 2 : 1);
                stroke();
                restore();
            }
        }
#endif

        restore();
    }
};

}
