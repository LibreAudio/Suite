// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/base.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace LibreAudio::MbCompResponse {
    constexpr double pi = 3.14159265358979323846;
    constexpr double crossoverRatio = 2.0; // one octave between adjacent crossovers
    constexpr double crossoverMin = 40.0;
    constexpr double crossoverMax = 12000.0;
    inline double crossoverCeiling(double sr) { return std::min(crossoverMax, sr * 0.45); }
    inline
    double crossoverFloor(double sr) {
        return std::min(crossoverMin, crossoverCeiling(sr) / std::pow(crossoverRatio, 3));
    }

    inline std::array<double, 4> effectiveCrossovers(std::array<double, 4> xo, const double sr)
    {
        const double ceiling = crossoverCeiling(sr);
        const double floor = crossoverFloor(sr);
        for (size_t i = 0; i < xo.size(); ++i)
        {
            // Reserve space for every remaining split, including automated or
            // recalled values which arrive crossed or above Nyquist.
            const double low = i == 0 ? floor : xo[i-1] * crossoverRatio;
            const double high = ceiling / std::pow(crossoverRatio, 3-i);
            xo[i] = std::clamp(xo[i], low, high);
        }
        return xo;
    }

    // Move one crossover, pushing a chain of neighbours only as far as needed.
    // Quantize before propagating: the host parameters have a 1 Hz step, and
    // rounding a pushed lower neighbour upward could violate the octave gap.
    inline std::array<double, 4> pushCrossover(std::array<double, 4> xo, const int index,
                                               const double requested, const double sr)
    {
        const double floor = std::ceil(crossoverFloor(sr));
        const double ceiling = std::floor(crossoverCeiling(sr));
        for (int i = 0; i < 4; ++i)
        {
            const double low = i == 0 ? floor : xo[i-1] * crossoverRatio;
            const double high = std::floor(ceiling / std::pow(crossoverRatio, 3-i));
            xo[i] = std::clamp(std::round(xo[i]), low, high);
        }
        const double low = floor * std::pow(crossoverRatio, index);
        const double high = std::floor(ceiling / std::pow(crossoverRatio, 3-index));
        xo[index] = std::clamp(std::round(requested), low, high);
        for (int i = index-1; i >= 0; --i)
            xo[i] = std::min(xo[i], std::floor(xo[i+1] / crossoverRatio));
        for (int i = index+1; i < 4; ++i)
            xo[i] = std::max(xo[i], xo[i-1] * crossoverRatio);
        return xo;
    }

    // Magnitude of the same bilinear-transform shelves as mbComp5.dsp. This is
    // the instantaneous wet response, before the DSP's dry/wet blend.
    inline double shelf(const double f, const double fc, const double gain, const int slope, const double sr)
    {
        const double r = std::tan(pi * std::min(f, sr * 0.499) / sr) / std::tan(pi * fc / sr);
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

    inline double response(const double f, const std::array<double, 4>& xo,
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
constexpr std::array<FaustParameterIndex, 4> crossovers {
    { kFaustParameterXover1,
        kFaustParameterXover2,
        kFaustParameterXover3, kFaustParameterXover4 }

};

constexpr std::array<std::array<FaustParameterIndex, 2>, 5> reductions {{
#define GR(n) {{ kFaustParameterGr_band##n##_mid, kFaustParameterGr_band##n##_side }}
    GR(1), GR(2), GR(3), GR(4), GR(5)
#undef GR
}};

constexpr std::array<Color, 5> colors {{ {255,191,203}, {255,223,173}, {210,253,211}, {190,241,255}, {218,193,243} }};

static constexpr float kCorner = 7;

constexpr std::array<std::array<FaustParameterIndex, 2>, 5> levels {{
#define LEVEL(n) {{ kFaustParameterLevel_band##n##_ch0, kFaustParameterLevel_band##n##_ch1 }}
    LEVEL(1), LEVEL(2), LEVEL(3), LEVEL(4), LEVEL(5)
#undef LEVEL
}};

const Color ink {246,246,248}, muted {140,142,150}, track {19,19,22}, accent {195,217,255};

struct Box {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && px <= x+w && py >= y && py <= y+h; }
};

class MbComp5GraphWidget : public LabWidget
{
    Box _box;

public:
    explicit MbComp5GraphWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
    }

    void setBox(float margin, float width, float height)
    {
        _box.x = 0;
        _box.y = margin / fScaleFactor;
        _box.w = width / fScaleFactor;
        _box.h = height / fScaleFactor;
    }

private:
    float width() const { return getWidth() / fScaleFactor; }
    float height() const { return getHeight() / fScaleFactor; }
    bool expert() const { return getCurrentPage(fInterface) == kPageExpert; }
    bool bypassed() const { return fInterface->getParameterValue(kCommonParameterBypass) > .5f; }
    float value(int p) const { return fInterface->getParameterValue(kParametersMainStart + p); }
    double sampleRate() const { return dynamic_cast<DISTRHO_NAMESPACE::UI*>(fInterface)->getSampleRate(); }
    bool anyListen() const { for (const auto& b : bands) if (value(b[5]) > .5f) return true; return false; }
    bool live(int b) const { return !bypassed() && value(bands[b][6]) < .5f && (!anyListen() || value(bands[b][5]) > .5f); }

    // Box graph() const { return {0,16,width(),std::max(70.f,height()-(expert() ? 380.f : 133.f))}; }
    Box graph() const { return _box; }
    float yDb(float db) const { const Box g = graph(); return g.y+(6-db)/24*g.h; }
    std::array<double,4> xo() const {
        return MbCompResponse::effectiveCrossovers({value(crossovers[0]),value(crossovers[1]),value(crossovers[2]),value(crossovers[3])}, sampleRate());
    }

    // Rectangle path with individual corner radii.
    void cornerPath(float x,float y,float w,float h,float tl,float tr,float br,float bl) {
        beginPath();moveTo(x+tl,y);
        lineTo(x+w-tr,y);arcTo(x+w,y,x+w,y+tr,tr);
        lineTo(x+w,y+h-br);arcTo(x+w,y+h,x+w-br,y+h,br);
        lineTo(x+bl,y+h);arcTo(x,y+h,x,y+h-bl,bl);
        lineTo(x,y+tl);arcTo(x,y,x+tl,y,tl);
        closePath();
    }
    void line(float x1,float y1,float x2,float y2,Color color,float thickness=1) {
        beginPath(); moveTo(x1,y1); lineTo(x2,y2); strokeColor(color); strokeWidth(thickness); stroke();
    }
    void label(float x,float y,const char* s,float size,Color color,int align=ALIGN_LEFT|ALIGN_MIDDLE,bool mono=false) {
        fontFace(mono?"mono":"regular"); fontSize(size); textAlign(align); fillColor(color); text(x,y,s);
    }

    Color rainbow(float t,float alpha=1) const
    {
        const float p=std::clamp(t,0.f,1.f)*4;
        int i=std::min(3,static_cast<int>(p)); const float f=p-i;
        Color c=colors[i];
        for(int k=0;k<3;++k) c.rgba[k]=colors[i].rgba[k]*(1-f)+colors[i+1].rgba[k]*f;
        c.alpha=alpha; return c;
    }

    void onNanoDisplay() final
    {
        // return;
#if 0
        std::array<double,6> edges {{20,freq[0],freq[1],freq[2],freq[3],20000}};
        for(int b=0;b<5;++b) {
            const float x1=freq2norm(edges[b])*width(), x2=freq2norm(edges[b+1])*width();
            // The outer bands follow the display's rounded corners.
            const float left=b==0?kCorner:0,right=b==4?kCorner:0;
            cornerPath(x1,0,std::max(0.f,x2-x1),h,left,right,right,left);
            fillColor(Color(colors[b],live(b)?.045f:.012f));
            fill();
        }

        // Five measured detector bands, not an FFT or a simulated spectrum.
        if(!bypassed()) {
            std::array<float,5> db {}, centers {};
            for(int b=0;b<5;++b) {
                db[b]=std::max(value(levels[b][0]),value(levels[b][1]));
                centers[b]=freq2norm(std::sqrt(edges[b]*edges[b+1]));
            }
            beginPath();moveTo(0,h-kCorner);
            for(int i=0;i<=160;++i) {
                const float t=i/160.f;int b=0;
                while(b<4 && centers[b+1]<t)++b;
                float level=db[b];
                if(t<centers[0])level=db[0];
                else if(b<4) {
                    const float f=std::clamp((t-centers[b])/std::max(.001f,centers[b+1]-centers[b]),0.f,1.f);
                    const float blend=f*f*(3-2*f);
                    level=db[b]+(db[b+1]-db[b])*blend;
                }
                const float y=g.y+(1-std::clamp((level+60)/60,0.f,1.f))*(h-g.y);
                lineTo(t*width(),i==0||i==160?std::min(y,h-kCorner):y);
            }
            arcTo(width(),h,width()-kCorner,h,kCorner);
            lineTo(kCorner,h);arcTo(0,h,0,h-kCorner,kCorner);
            closePath();
            fillPaint(linearGradient(0,g.y,0,h,Color(143,163,190,.45f),Color(45,49,59,.05f)));fill();
        }

        // Logarithmic grid: 1-9 times each decade, so the lines crowd together toward
        // the top of every decade. Decades are strongest, then 5, then the rest.
        for (float decade : {10.f,100.f,1000.f,10000.f})
        {
            for(int m=1;m<=9;++m)
            {
                const float f=m*decade;
                if (f<=20 || f>=20000)
                    continue;
                const float x=freq2norm(f)*width();
                line(x,g.y,x,h,Color(59,59,68,m==1?.7f:m==5?.45f:.22f));
                if ((m==1 || m==2 || m==5) && f>=50) {
                    char s[16];
                    if(f>=1000)
                        std::snprintf(s,sizeof(s),"%.0fk",f/1000);
                    else
                        std::snprintf(s,sizeof(s),"%.0f",f);
                    label(x,g.y/2,s,10,muted,ALIGN_CENTER|ALIGN_MIDDLE);
                }
            }
        }

        for(int db : {6,0,-6,-12,-18}) {
            const float y=yDb(db);
            for(float x=0;x<width();x+=7)
                line(x,y,x+3,y,Color(100,100,112,db==0?.5f:.22f));
            char s[24];
            std::snprintf(s,sizeof(s),db==-18?"%d dB":db>0?"+%d":"%d",db);
            label(9,y,s,10,muted);
            label(width()-9,y,s,10,muted,ALIGN_RIGHT|ALIGN_MIDDLE);
        }
#endif

        std::array<std::array<double,5>,2> gains {};
        for(int c=0;c<2;++c) {
            for(int b=0;b<5;++b) {
                gains[c][b] = bypassed() ? 0 :
                    anyListen() && value(bands[b][5]) < .5f ? -18 :
                        value(bands[b][6]) > .5f ? 0 :
                            value(reductions[b][c]) + value(bands[b][2]);
            }
        }

        save();
        scale(fScaleFactor, fScaleFactor);

        const Box g=graph();
        const auto freq=xo();
        const float h=height();

        // The solid mid and finer side curves use the real per-channel GR.
        // Match the DSP shelf cascade, including sample-rate frequency warping.
        // scissor(0,0,width(),h);

        constexpr int steps=256;
        const int slope=std::clamp(static_cast<int>(std::round(value(kFaustParameterSlope))),0,2);
        for(int c=1;c>=0;--c) {
            std::array<float,steps+1> ys {};
            for(int i=0;i<=steps;++i)
                ys[i]=yDb(MbCompResponse::response(
                    norm2freq(i/static_cast<double>(steps)),freq,gains[c],slope,sampleRate()));

            const auto path = [&]() {
                beginPath();
                moveTo(0,ys[0]);
                for(int i=1;i<=steps;++i)
                    lineTo(i*width()/steps,ys[i]);
            };

            if(c==0) {
                const float zero=yDb(0);
                const auto fillSegment = [&](float left,float y1,float right,float y2) {
                    const float edge=std::abs(y1-zero)>std::abs(y2-zero)?y1:y2;
                    if(std::abs(edge-zero)<.01f) return;
                    const float t=(left+right)/(2*width());
                    beginPath();
                    moveTo(left,zero);
                    lineTo(left,y1);
                    lineTo(right,y2);
                    lineTo(right,zero);
                    closePath();
                    fillPaint(linearGradient(0,zero,0,edge,rainbow(t,.015f),rainbow(t,.24f)));
                    fill();
                };
                // Fade toward 0 dB for both gain and reduction, following the curve's colors.
                // save();
                // nvgShapeAntiAlias(getContext(),0); // FIXME
                for(int i=0;i<steps;++i) {
                    const float left=i*width()/steps,right=(i+1)*width()/steps;
                    if((ys[i]-zero)*(ys[i+1]-zero)<0) {
                        const float crossing=left+(right-left)*(zero-ys[i])/(ys[i+1]-ys[i]);
                        fillSegment(left,ys[i],crossing,zero);
                        fillSegment(crossing,zero,right,ys[i+1]);
                    } else {
                        fillSegment(left,ys[i],right,ys[i+1]);
                    }
                }
                // restore();
            }
            for(int section=0;section<4;++section) {
                const float left=section*width()/4,right=(section+1)*width()/4;
                save();
                intersectScissor(left,0,right-left,h);
                if(c==0) {
                    path();
                    strokeColor(rainbow((section+.5f)/4,bypassed()?.04f:.13f));
                    strokeWidth(6);
                    stroke();
                }
                path();const float opacity=bypassed()?.25f:c==0?.95f:.5f;
                strokePaint(linearGradient(left,0,right,0,Color(colors[section],opacity),Color(colors[section+1],opacity)));
                strokeWidth(c==0?2:1);
                stroke();
                restore();
            }
        }

        if(expert())
        {
            std::array<float,4> chips {};
            for(int i=0;i<4;++i) chips[i]=std::max(112.f,static_cast<float>(freq2norm(freq[i])*width()));
            for(int i=1;i<4;++i) chips[i]=std::max(chips[i],chips[i-1]+76);
            for(int i=3;i>=0;--i) {
                chips[i]=std::min(chips[i],width()-44);
                if(i>0)
                    chips[i-1]=std::min(chips[i-1],chips[i]-76);
            }
            for(int i=0;i<4;++i) {
                const float x=freq2norm(freq[i])*width();
                const float top=g.y, bottom=g.y+g.h, fade=g.h*.18f;
                const Color color(colors[i+1],.5f), transparent(colors[i+1],0.f);
                beginPath();
                moveTo(x,top);
                lineTo(x,top+fade);
                strokePaint(linearGradient(x,top,x,top+fade,transparent,color));
                strokeWidth(1.2f); stroke();
                line(x,top+fade,x,bottom-fade,color,1.2f);
                beginPath();
                moveTo(x,bottom-fade);
                lineTo(x,bottom);
                strokePaint(linearGradient(x,bottom-fade,x,bottom,color,transparent));
                strokeWidth(1.2f);
                stroke();

                const Box chip {chips[i]-34,g.y+g.h-1,68,22};
                char s[40]; const double f=freq[i];
                if (f>=1000)
                    std::snprintf(s,sizeof(s),"%.2fk",f/1000);
                else
                    std::snprintf(s,sizeof(s),"%.0f Hz",f);
                label(chips[i],chip.y+11,s,11,colors[i+1],ALIGN_CENTER|ALIGN_MIDDLE,true);

                // fHits.push_back({{x-8,top,16,g.h},Kind::Crossover,crossovers[i],i});
                // fHits.push_back({chip,Kind::Crossover,crossovers[i],i});
            }
        }
        restore();
    }
};

}
