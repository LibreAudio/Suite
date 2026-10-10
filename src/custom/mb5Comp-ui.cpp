// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
// Native port of mbComp5_v2.html. Audio values always come from the DSP;
// only hover, text entry, menus and display preferences belong to the UI.

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "LibreAudioBaseUI.hpp"
#include "ui/widgets/shader.hpp"
#include "LibreAudioParameters.hpp"
#include "mbComp5/response.hpp"
#include "src/nanovg/nanovg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace LibreAudio {
namespace {
using Param = FaustParameterIndex;
constexpr std::array<std::array<Param, 7>, 5> bands {{
#define BAND(n) {{ kFaustParameterBand##n##_threshold, kFaustParameterBand##n##_ratio, \
    kFaustParameterBand##n##_makeup, kFaustParameterBand##n##_attack, kFaustParameterBand##n##_release, \
    kFaustParameterBand##n##_listen, kFaustParameterBand##n##_bypass }}
    BAND(1), BAND(2), BAND(3), BAND(4), BAND(5)
#undef BAND
}};
constexpr std::array<Param, 4> crossovers {{ kFaustParameterXover1, kFaustParameterXover2,
                                          kFaustParameterXover3, kFaustParameterXover4 }};
constexpr std::array<Param, 9> globals {{ kFaustParameterSlope, kFaustParameterKnee, kFaustParameterRange, kFaustParameterRms_time,
    kFaustParameterShape, kFaustParameterChan_link, kFaustParameterBand_link,
    kFaustParameterLookahead, kFaustParameterDrywet }};
constexpr std::array<std::array<Param, 2>, 5> reductions {{
#define GR(n) {{ kFaustParameterGr_band##n##_mid, kFaustParameterGr_band##n##_side }}
    GR(1), GR(2), GR(3), GR(4), GR(5)
#undef GR
}};
constexpr std::array<std::array<Param, 2>, 5> levels {{
#define LEVEL(n) {{ kFaustParameterLevel_band##n##_ch0, kFaustParameterLevel_band##n##_ch1 }}
    LEVEL(1), LEVEL(2), LEVEL(3), LEVEL(4), LEVEL(5)
#undef LEVEL
}};
constexpr std::array<Color, 5> colors {{ {255,191,203}, {255,223,173}, {210,253,211}, {190,241,255}, {218,193,243} }};
constexpr const char* names[] = { "LOW", "LOW MID", "MID", "HIGH MID", "HIGH" };
constexpr const char* labels[] = { "THRESHOLD", "RATIO", "GAIN", "ATTACK", "RELEASE" };
constexpr const char* globalLabels[] = { "SLOPE", "KNEE", "RANGE", "RMS TIME", "RELEASE SHAPE", "MID-SIDE LINK", "BAND LINK", "LOOKAHEAD", "DRY / WET" };
const Color ink {246,246,248}, muted {140,142,150}, track {19,19,22}, accent {195,217,255};

struct Box {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && px <= x+w && py >= y && py <= y+h; }
};
struct Preset {
    const char* name;
    const char* description;
    std::array<float, 4> xo;
    std::array<std::array<float, 5>, 5> values;
};
constexpr std::array<Preset, 8> presets {{
    {"GENTLE GLUE", "Even, transparent hold", {110,480,2000,6500}, {{{-20,1.8,.5,30,260},{-22,2,0,22,200},{-24,2,.5,14,160},{-24,2.2,.5,8,120},{-26,2,1,5,90}}}},
    {"PUNCH BUS", "Fast mids, loose lows", {95,420,1800,7000}, {{{-16,3,1.5,55,340},{-20,3.5,.5,12,150},{-22,4,1,5,90},{-24,3.5,1,3,70},{-26,3,1.5,2,60}}}},
    {"TAME LOWS", "Control under 260 Hz", {70,260,1600,6000}, {{{-30,5,-1,12,180},{-26,4,-.5,18,200},{-14,1.5,0,20,220},{-12,1.2,0,15,200},{-12,1.2,0,10,160}}}},
    {"BROADCAST", "Dense, level, forward", {130,550,2400,8000}, {{{-28,4.5,2,20,200},{-30,5,1.5,10,130},{-32,6,2,5,80},{-32,5,2.5,3,60},{-30,4,3,2,50}}}},
    {"DE-ESS TOP", "Sibilance only", {120,500,2200,5200}, {{{-8,1.2,0,30,250},{-8,1.2,0,20,200},{-10,1.5,0,12,150},{-24,4,0,1,45},{-28,6,-1,.6,40}}}},
    {"WARM TAPE", "Soft top, thick low", {90,400,1900,5800}, {{{-18,2.2,1.5,45,300},{-20,2,.5,30,240},{-22,1.8,-.5,20,200},{-24,2.2,-1,10,140},{-26,2.5,-1.5,6,110}}}},
    {"DRUM SMASH", "Aggressive, pumping", {100,350,1500,6800}, {{{-24,6,3,8,120},{-26,8,2,4,80},{-28,8,2.5,2,55},{-28,6,2.5,1.5,45},{-30,5,3,1,40}}}},
    {"VOCAL LEVEL", "Even vocal density", {140,600,2600,7500}, {{{-26,4,0,25,220},{-22,3,.5,15,170},{-20,3,1,8,120},{-22,3.5,1,4,90},{-24,3,.5,2,70}}}}
}};
}

class MbCompWidget final : public LabWidget, private IdleCallback
{
    enum class Kind { Value, Toggle, Crossover, Meter, Amount, Slope, Preset, Stars, Spectrum, Back };
    struct Hit { Box box; Kind kind; int parameter = -1, band = -1, field = -1; };
    struct Gesture { int parameter; float start; };
    std::vector<Hit> fHits;
    std::vector<Gesture> fGesture;
    Hit fDrag {};
    bool fDragging = false, fMoved = false;
    bool fStars = true, fSpectrum = true, fSelectText = false;
    float fDragX = 0, fDragY = 0, fLastY = 0, fDelta = 0;
    std::array<float,2> fTurn {};
    int fEdit = -1, fLastClick = -1;
    double fClickTime = 0;
    std::string fText;
    static constexpr float kCorner = 7;
    Page fPage = kPageInit;

public:
    explicit MbCompWidget(LabWidget* parent) : LabWidget(parent) { addIdleCallback(this); }
    void finishInteraction() { endGesture(); commitText(true); }
    float getBorderRadius() const noexcept { return 7 * fScaleFactor; }
    bool starsVisible() const noexcept { return fStars; }

private:
    float width() const { return getWidth() / fScaleFactor; }
    float height() const { return getHeight() / fScaleFactor; }
    bool expert() const { return getCurrentPage(fInterface) == kPageExpert; }
    bool bypassed() const { return fInterface->getParameterValue(kCommonParameterBypass) > .5f; }
    float value(int p) const { return fInterface->getParameterValue(kParametersMainStart + p); }
    double sampleRate() const { return std::max(1.0, dynamic_cast<DISTRHO_NAMESPACE::UI*>(fInterface)->getSampleRate()); }
    bool anyListen() const { for (const auto& b : bands) if (value(b[5]) > .5f) return true; return false; }
    bool live(int b) const { return !bypassed() && value(bands[b][6]) < .5f && (!anyListen() || value(bands[b][5]) > .5f); }
    Box column(int b) const { float w = (width()-24-32)/5; return {12+b*(w+8),height()-308,w,236}; }
    Box meter(int b) const { const Box c = column(b); return {c.x+c.w-28,c.y+30,20,c.h-40}; }
    Box graph() const { return {0,16,width(),std::max(70.f,height()-(expert() ? 350.f : 133.f))}; }
    float yDb(float db) const { const Box g = graph(); return g.y+(6-db)/24*g.h; }
    std::array<double,4> xo() const {
        return MbCompResponse::effectiveCrossovers({value(crossovers[0]),value(crossovers[1]),value(crossovers[2]),value(crossovers[3])}, sampleRate());
    }
    static float normalized(int p, float v) {
        const auto& a = kFaustParameters[p];
        return a.isLogarithmic && a.min > 0 ? std::log(v/a.min)/std::log(a.max/a.min) : (v-a.min)/(a.max-a.min);
    }
    static float denormalized(int p, float n) {
        const auto& a = kFaustParameters[p]; n = std::clamp(n,0.f,1.f);
        return a.isLogarithmic && a.min > 0 ? a.min*std::pow(a.max/a.min,n) : a.min+n*(a.max-a.min);
    }
    void write(int p, float v) {
        const auto& a = kFaustParameters[p];
        if (!std::isfinite(v)) return;
        v = std::clamp(v,a.min,a.max);
        if (a.step > 0) v = a.min+std::round((v-a.min)/a.step)*a.step;
        fInterface->parameterControlModified(kParametersMainStart+p,std::clamp(v,a.min,a.max));
    }
    void set(int p, float v) {
        fInterface->parameterControlPressed(kParametersMainStart+p);
        write(p,v);
        fInterface->parameterControlReleased(kParametersMainStart+p);
    }
    void endGesture() {
        for (const auto& g : fGesture) fInterface->parameterControlReleased(kParametersMainStart+g.parameter);
        fGesture.clear(); fDragging = false;
    }
    void beginGesture(const Hit& h, bool all) {
        endGesture(); fDrag=h; fDragging=true; fMoved=false; fDelta=0;
        const auto add = [this](int p) {
            fGesture.push_back({p,normalized(p,value(p))});
            fInterface->parameterControlPressed(kParametersMainStart+p);
        };
        if (h.kind == Kind::Amount || (all && h.kind == Kind::Value && h.band >= 0 && h.field >= 0))
            for (const auto& b : bands) add(b[h.field]);
        else add(h.parameter);
    }
    void panel(Box b, Color color, float radius=7) {
        beginPath(); roundedRect(b.x,b.y,b.w,b.h,radius); fillColor(color); fill();
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
    void frame(Box b, Color color, float backingOpacity=0) {
        if(backingOpacity>0) panel(b,Color(24,25,30,backingOpacity),10);
        panel(b,Color(255,255,255,.045f),10);
        beginPath(); roundedRect(b.x+.5f,b.y+.5f,b.w-1,b.h-1,10);
        strokeColor(Color(color,.15f)); strokeWidth(1); stroke();
    }
    void format(int p, char* s, size_t n, bool units=true) const {
        const auto& a=kFaustParameters[p]; const float v=value(p);
        if (p==kFaustParameterSlope) {
            std::snprintf(s,n,"%d dB/oct",6*(1<<std::clamp(static_cast<int>(std::round(v)),0,2)));
        } else if (p==kFaustParameterShape) {
            if (v <= 0) std::snprintf(s,n,"Linear");
            else if (v >= 100) std::snprintf(s,n,"Analog");
            else std::snprintf(s,n,"%.0f%%",v);
        } else if (p==kFaustParameterRms_time && v==0) std::snprintf(s,n,"PEAK");
        else if (std::string(a.unit)=="Hz") {
            if (v>=1000) std::snprintf(s,n,"%.2fk",v/1000);
            else std::snprintf(s,n,"%.0f%s",v,units?" Hz":"");
        } else if (std::string(a.unit)=="ms" && v>=1000 && units) std::snprintf(s,n,"%.2f s",v/1000);
        else std::snprintf(s,n,a.step>=1?"%.0f%s%s":v!=0 && std::abs(v)<10?"%.2f%s%s":"%.1f%s%s",
            v,units && *a.unit?" ":"",units?a.unit:"");
    }
    // Number box in the style of the EQ: inset track, small spaced caption, mono
    // value with its unit, and a fill bar along the bottom edge.
    void number(Box b,int p,const char* title,Color color,int band=-1,int field=-1) {
        constexpr float radius=5;
        const auto& a=kFaustParameters[p]; const float v=value(p);
        const bool editing = fEdit==p;
        const bool enabled = band<0 || value(bands[band][6])<.5f;
        beginPath(); roundedRect(b.x,b.y,b.w,b.h,radius); fillColor(track); fill();
        beginPath(); roundedRect(b.x,b.y,b.w,b.h,radius);
        fillPaint(linearGradient(0,b.y,0,b.y+4,Color(0,0,0,.55f),Color(0,0,0,0))); fill();
        strokeColor(Color(0,0,0,.45f)); strokeWidth(1); stroke();
        const float px=b.x+7;
        fontFace("regular"); fontSize(9);
        textAlign(ALIGN_LEFT|ALIGN_TOP); fillColor(muted); text(px,b.y+4,title,nullptr);
        char valueText[48]; const char* unit=nullptr; const std::string unitName=a.unit;
        if (p==kFaustParameterSlope) {
            std::snprintf(valueText,sizeof(valueText),"%d",6*(1<<std::clamp(static_cast<int>(std::round(v)),0,2)));
            unit="dB/oct";
        } else if (field==1) { std::snprintf(valueText,sizeof(valueText),"%.1f",v); unit=":1"; }
        else if (unitName=="Hz") {
            if (v>=1000) { std::snprintf(valueText,sizeof(valueText),"%.2f",v/1000); unit="kHz"; }
            else { std::snprintf(valueText,sizeof(valueText),"%.0f",v); unit="Hz"; }
        } else if (unitName=="ms" && v>=1000) { std::snprintf(valueText,sizeof(valueText),"%.2f",v/1000); unit="s"; }
        else {
            format(p,valueText,sizeof(valueText),false);
            if (!unitName.empty() && p!=kFaustParameterShape && !(p==kFaustParameterRms_time && v==0)) unit=a.unit;
        }
        const char* shown=editing?fText.c_str():valueText;
        if (editing) unit=nullptr;
        // Shrink the value and unit together when the box is narrow.
        Rectangle<float> vb,ub;
        fontFace("mono"); fontSize(15); textBounds(0,0,shown,nullptr,vb);
        float uw=0;
        if (unit) { fontFace("regular"); fontSize(9.5f); textBounds(0,0,unit,nullptr,ub); uw=ub.getWidth()+3; }
        const float k=std::min(1.f,(b.w-14)/std::max(1.f,vb.getWidth()+uw));
        const float base=b.y+b.h-8;
        fontFace("mono"); fontSize(15*k); textAlign(ALIGN_LEFT|ALIGN_BASELINE);
        if (editing && fSelectText) panel({px-2,base-13,vb.getWidth()*k+4,16},Color(accent,.2f),2);
        fillColor(enabled?(editing?ink:color):Color(0x5d,0x5d,0x66));
        text(px,base,shown,nullptr);
        if (unit) {
            fontFace("regular"); fontSize(9.5f*k); fillColor(muted);
            textAlign(ALIGN_RIGHT|ALIGN_BASELINE); text(b.x+b.w-7,base,unit,nullptr);
        }
        save(); scissor(b.x,b.y+b.h-2,b.w,2);
        beginPath(); roundedRect(b.x,b.y,b.w,b.h,radius); fillColor(Color(0,0,0,.5f)); fill();
        beginPath(); roundedRect(b.x,b.y,std::max(1.f,std::clamp(normalized(p,v),0.f,1.f)*b.w),b.h,radius);
        fillColor(enabled?color:Color(0x45,0x45,0x4d)); fill();
        restore();
        if (enabled) fHits.push_back({b,p==kFaustParameterSlope?Kind::Slope:Kind::Value,p,band,field});
    }
    void button(Box b,const char* text,Kind kind,int id=-1,Color color=accent,bool on=false) {
        panel(b,on?Color(color,.22f):Color(255,255,255,.055f),5);
        label(b.x+b.w/2,b.y+b.h/2,text,11,on?color:muted,ALIGN_CENTER|ALIGN_MIDDLE);
        fHits.push_back({b,kind,id});
    }
    Color rainbow(float t,float alpha=1) const {
        const float p=std::clamp(t,0.f,1.f)*4;
        int i=std::min(3,static_cast<int>(p)); const float f=p-i;
        Color c=colors[i];
        for(int k=0;k<3;++k) c.rgba[k]=colors[i].rgba[k]*(1-f)+colors[i+1].rgba[k]*f;
        c.alpha=alpha; return c;
    }
    void scope() {
        const Box g=graph(); const auto freq=xo();
        const float h=height();
        std::array<double,6> edges {{20,freq[0],freq[1],freq[2],freq[3],20000}};
        for(int b=0;b<5;++b) {
            const float x1=MbCompResponse::position(edges[b])*width(), x2=MbCompResponse::position(edges[b+1])*width();
            // The outer bands follow the display's rounded corners.
            const float left=b==0?kCorner:0,right=b==4?kCorner:0;
            cornerPath(x1,0,std::max(0.f,x2-x1),h,left,right,right,left);
            fillColor(Color(colors[b],live(b)?.045f:.012f));fill();
        }
        // Five measured detector bands, not an FFT or a simulated spectrum.
        if(fSpectrum && !bypassed()) {
            std::array<float,5> db {}, centers {};
            for(int b=0;b<5;++b) {
                db[b]=std::max(value(levels[b][0]),value(levels[b][1]));
                centers[b]=MbCompResponse::position(std::sqrt(edges[b]*edges[b+1]));
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
        for(float decade : {10.f,100.f,1000.f,10000.f}) for(int m=1;m<=9;++m) {
            const float f=m*decade;
            if(f<=20 || f>=20000) continue;
            const float x=MbCompResponse::position(f)*width();
            line(x,g.y,x,h,Color(59,59,68,m==1?.7f:m==5?.45f:.22f));
            if((m==1 || m==2 || m==5) && f>=50) {
                char s[16]; if(f>=1000) std::snprintf(s,sizeof(s),"%.0fk",f/1000); else std::snprintf(s,sizeof(s),"%.0f",f);
                label(x,g.y/2,s,10,muted,ALIGN_CENTER|ALIGN_MIDDLE);
            }
        }
        for(int db : {6,0,-6,-12,-18}) {
            const float y=yDb(db);
            for(float x=0;x<width();x+=7) line(x,y,x+3,y,Color(100,100,112,db==0?.5f:.22f));
            char s[24]; std::snprintf(s,sizeof(s),db==-18?"%d dB":db>0?"+%d":"%d",db);
            label(9,y,s,10,muted); label(width()-9,y,s,10,muted,ALIGN_RIGHT|ALIGN_MIDDLE);
        }
        std::array<std::array<double,5>,2> gains {};
        for(int c=0;c<2;++c) for(int b=0;b<5;++b) {
            gains[c][b] = bypassed()?0: anyListen() && value(bands[b][5])<.5f ? -18 :
                value(bands[b][6])>.5f?0:value(reductions[b][c])+value(bands[b][2]);
        }
        // The solid mid and finer side curves use the real per-channel GR.
        // Match the DSP shelf cascade, including sample-rate frequency warping.
        save(); scissor(0,0,width(),h);
        constexpr int steps=256;
        const int slope=std::clamp(static_cast<int>(std::round(value(kFaustParameterSlope))),0,2);
        for(int c=1;c>=0;--c) {
            std::array<float,steps+1> ys {};
            for(int i=0;i<=steps;++i) ys[i]=yDb(MbCompResponse::response(
                MbCompResponse::frequency(i/static_cast<double>(steps)),freq,gains[c],slope,sampleRate()));
            const auto path = [&]() {
                beginPath();moveTo(0,ys[0]);
                for(int i=1;i<=steps;++i)lineTo(i*width()/steps,ys[i]);
            };
            if(c==0) {
                const float zero=yDb(0);
                const auto fillSegment = [&](float left,float y1,float right,float y2) {
                    const float edge=std::abs(y1-zero)>std::abs(y2-zero)?y1:y2;
                    if(std::abs(edge-zero)<.01f) return;
                    const float t=(left+right)/(2*width());
                    beginPath();moveTo(left,zero);lineTo(left,y1);
                    lineTo(right,y2);lineTo(right,zero);closePath();
                    fillPaint(linearGradient(0,zero,0,edge,rainbow(t,.015f),rainbow(t,.24f)));fill();
                };
                // Fade toward 0 dB for both gain and reduction, following the curve's colors.
                save();nvgShapeAntiAlias(getContext(),0);
                for(int i=0;i<steps;++i) {
                    const float left=i*width()/steps,right=(i+1)*width()/steps;
                    if((ys[i]-zero)*(ys[i+1]-zero)<0) {
                        const float crossing=left+(right-left)*(zero-ys[i])/(ys[i+1]-ys[i]);
                        fillSegment(left,ys[i],crossing,zero);
                        fillSegment(crossing,zero,right,ys[i+1]);
                    } else fillSegment(left,ys[i],right,ys[i+1]);
                }
                restore();
            }
            for(int section=0;section<4;++section) {
                const float left=section*width()/4,right=(section+1)*width()/4;
                save();intersectScissor(left,0,right-left,h);
                if(c==0) {
                    path();strokeColor(rainbow((section+.5f)/4,bypassed()?.04f:.13f));strokeWidth(6);stroke();
                }
                path();const float opacity=bypassed()?.25f:c==0?.95f:.5f;
                strokePaint(linearGradient(left,0,right,0,Color(colors[section],opacity),Color(colors[section+1],opacity)));
                strokeWidth(c==0?2:1);stroke();restore();
            }
        }
        restore();
        if(!expert()) return;
        std::array<float,4> chips {};
        for(int i=0;i<4;++i) chips[i]=std::max(112.f,static_cast<float>(MbCompResponse::position(freq[i])*width()));
        for(int i=1;i<4;++i) chips[i]=std::max(chips[i],chips[i-1]+76);
        for(int i=3;i>=0;--i) {
            chips[i]=std::min(chips[i],width()-44);
            if(i>0) chips[i-1]=std::min(chips[i-1],chips[i]-76);
        }
        for(int i=0;i<4;++i) {
            const float x=MbCompResponse::position(freq[i])*width();
            const float top=g.y, bottom=g.y+g.h, fade=g.h*.18f;
            const Color color(colors[i+1],.5f), transparent(colors[i+1],0.f);
            beginPath(); moveTo(x,top); lineTo(x,top+fade);
            strokePaint(linearGradient(x,top,x,top+fade,transparent,color));
            strokeWidth(1.2f); stroke();
            line(x,top+fade,x,bottom-fade,color,1.2f);
            beginPath(); moveTo(x,bottom-fade); lineTo(x,bottom);
            strokePaint(linearGradient(x,bottom-fade,x,bottom,color,transparent));
            strokeWidth(1.2f); stroke();
            const Box chip {chips[i]-34,g.y+g.h-1,68,22};
            char s[40]; const double f=freq[i];
            if(f>=1000) std::snprintf(s,sizeof(s),"%.2fk",f/1000); else std::snprintf(s,sizeof(s),"%.0f Hz",f);
            label(chips[i],chip.y+11,s,11,colors[i+1],ALIGN_CENTER|ALIGN_MIDDLE,true);
            fHits.push_back({{x-8,top,16,g.h},Kind::Crossover,crossovers[i],i});
            fHits.push_back({chip,Kind::Crossover,crossovers[i],i});
        }
    }
    void bandColumns() {
        for(int b=0;b<5;++b) {
            const Box c=column(b); const Color color=live(b)?colors[b]:Color(100,100,110);
            frame(c,value(bands[b][5])>.5f?colors[b]:muted,.9f);
            label(c.x+8,c.y+14,names[b],10,color);
            const Box listen {c.x+c.w-44,c.y+6,17,17}, power {c.x+c.w-24,c.y+6,17,17};
            // Speaker and power icons from the prototype.
            panel(listen,value(bands[b][5])>.5f?colors[b]:Color(colors[b],.08f),4);
            const Color speaker=value(bands[b][5])>.5f?track:colors[b];
            beginPath(); moveTo(listen.x+3,listen.y+6); lineTo(listen.x+6,listen.y+6);
            lineTo(listen.x+9,listen.y+3); lineTo(listen.x+9,listen.y+14);
            lineTo(listen.x+6,listen.y+11); lineTo(listen.x+3,listen.y+11); closePath(); fillColor(speaker); fill();
            line(listen.x+12,listen.y+6,listen.x+12,listen.y+11,speaker);
            panel(power,value(bands[b][6])>.5f?colors[b]:Color(colors[b],.08f),4);
            const Color powerIcon=value(bands[b][6])>.5f?track:colors[b];
            beginPath(); arc(power.x+8.5f,power.y+9,5,-.8f,3.94f,CW); strokeColor(powerIcon); strokeWidth(1.2); stroke();
            line(power.x+8.5f,power.y+2,power.x+8.5f,power.y+9,powerIcon,1.3f);
            fHits.push_back({listen,Kind::Toggle,bands[b][5]}); fHits.push_back({power,Kind::Toggle,bands[b][6]});
            const float rowH=(c.h-40-4*5)/5;
            for(int j=0;j<5;++j) number({c.x+8,c.y+30+j*(rowH+5),c.w-43,rowH},bands[b][j],labels[j],color,b,j);
            const Box m=meter(b); panel({m.x+8,m.y,12,m.h},track,4);
            for(int channel=0;channel<2;++channel) {
                const float level=bypassed()?-60:value(levels[b][channel]);
                const float mh=std::clamp((level+60)/60,0.f,1.f)*m.h;
                panel({m.x+9+channel*5,m.y+m.h-mh,4,mh},Color(color,.7f),1);
            }
            for(int db : {0,-12,-24,-36,-48}) {
                const float y=m.y-db/60.f*m.h; line(m.x+8,y,m.x+20,y,Color(muted,.45f));
            }
            const float ty=m.y-value(bands[b][0])/60*m.h;
            beginPath(); moveTo(m.x,ty-4); lineTo(m.x+6,ty); lineTo(m.x,ty+4); closePath(); fillColor(color); fill();
            line(m.x+8,ty,m.x+20,ty,color);
            if(value(bands[b][6])<.5f) fHits.push_back({m,Kind::Meter,bands[b][0],b,0});
        }
        const Box common {12,height()-64,width()-24,52}; frame(common,accent,.9f);
        const float cell=(common.w-16-(globals.size()-1)*6)/globals.size();
        for(size_t i=0;i<globals.size();++i) number({common.x+8+i*(cell+6),common.y+8,cell,36},globals[i],globalLabels[i],accent);
    }
    int activePreset() const {
        for(int p=0;p<8;++p) {
            bool match=true;
            for(int i=0;i<4;++i) match &= std::abs(value(crossovers[i])-presets[p].xo[i])<1;
            for(int b=0;b<5;++b) {
                for(int j=0;j<5;++j) match &= std::abs(value(bands[b][j])-presets[p].values[b][j])<.02f;
                match &= value(bands[b][5])<.5f && value(bands[b][6])<.5f;
            }
            if(match)return p;
        }
        return -1;
    }
    void applyPreset(int p) {
        for(int i=0;i<4;++i) set(crossovers[i],presets[p].xo[i]);
        for(int b=0;b<5;++b) {
            for(int j=0;j<5;++j)set(bands[b][j],presets[p].values[b][j]);
            set(bands[b][5],0);set(bands[b][6],0);
        }
    }
    // Moves the gestured parameter of every band by the same dB. The shift stops
    // where the first one reaches its limit, so the offsets between bands are kept.
    float shiftAll(float db) {
        for(const auto& g:fGesture) {
            const auto& a=kFaustParameters[g.parameter];
            const float start=denormalized(g.parameter,g.start);
            db=std::clamp(db,a.min-start,a.max-start);
        }
        for(const auto& g:fGesture)write(g.parameter,denormalized(g.parameter,g.start)+db);
        return db;
    }
    // One large endless knob that moves a parameter of all bands together;
    // the ring of values around it is read-only reference.
    void bigKnob(float cx,float cy,int knob,int field,const char* title) {
        constexpr float S=2.4f;
        const float r=30*S;
        // The large knob of the other plugins (ui/widgets/knob.hpp), drawn in its 100-unit box.
        // Endless: pointers in rainbow colors turn with the drag.
        const Color tint=bypassed()?Color(0x5d,0x5d,0x66):accent;
        save();translate(cx,cy);scale(S,S);translate(-50,-50);
        beginPath();circle(50,53.5f,38);
        fillPaint(radialGradient(50,53.5f,28,36,Color(0,0,0,50.f/255),Color(0,0,0,0)));fill();
        beginPath();circle(50,50,30);
        fillPaint(linearGradient(50,20,50,80,Color(0x4a,0x4a,0x51),Color(0x3a,0x3a,0x41)));fill();
        strokeColor(Color(0x0d,0x0d,0x0f));strokeWidth(1);stroke();
        beginPath();circle(50,49.4f,29.1f);
        strokePaint(linearGradient(50,20,50,50,Color(255,255,255,90.f/255),Color(255,255,255,0)));strokeWidth(1);stroke();
        // The light stays fixed while the pointers turn: dome shading, two soft
        // anisotropic sheens and a rim light underneath, a specular spot on top.
        beginPath();circle(50,50,30);
        fillPaint(radialGradient(50,50,16,30,Color(0,0,0,0),Color(0,0,0,45.f/255)));fill();
        for(const float mid:{2.356f,5.498f}) {
            beginPath();moveTo(50,50);arc(50,50,29.2f,mid-.26f,mid+.26f,CW);closePath();
            fillPaint(radialGradient(50,50,4,29,Color(255,255,255,13.f/255),Color(255,255,255,0)));fill();
        }
        beginPath();circle(50,50.6f,29.1f);
        strokePaint(linearGradient(50,62,50,80,Color(255,255,255,0),Color(255,255,255,20.f/255)));strokeWidth(1);stroke();
        for(int i=0;i<12;++i) {
            const float a=fTurn[knob]+i*M_PI/6;
            const float x0=50+std::sin(a)*22,y0=50-std::cos(a)*22,x1=50+std::sin(a)*27,y1=50-std::cos(a)*27;
            // Engraved: the lower lip catches the light, the groove is dark, the paint sits inside.
            lineCap(ROUND);
            beginPath();moveTo(x0,y0+.7f);lineTo(x1,y1+.7f);strokeColor(Color(255,255,255,.2f));strokeWidth(1.8f);stroke();
            beginPath();moveTo(x0,y0);lineTo(x1,y1);strokeColor(Color(0,0,0,.5f));strokeWidth(1.8f);stroke();
            beginPath();moveTo(x0,y0+.15f);lineTo(x1,y1+.15f);
            strokeColor(bypassed()?tint:rainbow(i/11.f,.9f));strokeWidth(1.1f);stroke();
        }
        beginPath();circle(50,50,30);
        fillPaint(radialGradient(41,36,1,17,Color(255,255,255,28.f/255),Color(255,255,255,0)));fill();
        restore();
        label(cx,cy,bypassed()?"BYPASS":title,14,Color(172,174,182),ALIGN_CENTER|ALIGN_MIDDLE);
        fHits.push_back({{cx-r,cy-r,r*2,r*2},Kind::Amount,-1,-1,field});
        // Each band's value on a 270° arc around the knob, low to high.
        for(int b=0;b<5;++b) {
            char s[32];format(bands[b][field],s,sizeof(s));
            const float a=.75f*M_PI+b*.375f*M_PI,x=cx+std::cos(a)*(r+42),y=cy+std::sin(a)*(r+42);
            label(x,y-9,names[b],10,Color(colors[b],.65f),ALIGN_CENTER|ALIGN_MIDDLE);
            label(x,y+7,s,13,value(bands[b][6])<.5f?colors[b]:muted,ALIGN_CENTER|ALIGN_MIDDLE,true);
        }
    }
    void easyControls() {
        bigKnob(width()*.27f,height()*.56f,0,0,"THRESHOLD");
        bigKnob(width()*.73f,height()*.56f,1,2,"GAIN");
        const float w=(width()-24-3*10)/4;
        const int active=activePreset();
        for(int p=0;p<4;++p) {
            const Box b {12+p*(w+10),height()-88,w,70}; frame(b,colors[p],.9f);
            if(active==p)panel(b,Color(colors[p],.12f),10);
            label(b.x+12,b.y+22,presets[p].name,12,colors[p]);
            label(b.x+12,b.y+44,presets[p].description,10,muted);
            fHits.push_back({b,Kind::Preset,p});
        }
    }
    void onNanoDisplay() final {
        save();scale(fScaleFactor,fScaleFactor);fHits.clear();
        panel({0,0,width(),height()},Color(20,21,26,.32f),kCorner);
        scope();
        const Page page=getCurrentPage(fInterface);
        if(page==kPageSettings || page==kPageAbout) {
            panel({20,60,width()-40,height()-80},Color(25,26,31,.98f),10);
            label(44,92,page==kPageAbout?"5-BAND M-S COMPRESSOR":"DISPLAY",20,ink);
            button({width()-100,75,56,26},"BACK",Kind::Back);
            if(page==kPageSettings) {
                button({44,130,180,34},"STARFIELD",Kind::Stars,-1,accent,fStars);
                button({44,180,180,34},"BAND LEVELS",Kind::Spectrum,-1,accent,fSpectrum);
            } else {
                label(44,145,"Libre Audio Suite · Klaus Scheuermann",15,accent);
                label(44,177,"Five-band mid/side dynamics · GPL-3.0-or-later",13,muted);
            }
        } else {
            if(expert()) bandColumns();
            else easyControls();
        }
        restore();
    }
    const Hit* hit(float x,float y) const {
        for(auto i=fHits.rbegin();i!=fHits.rend();++i) if(i->box.contains(x,y))return &*i;
        return nullptr;
    }
    void idleCallback() final {
        const Page page=getCurrentPage(fInterface);
        if(fPage!=page) { endGesture();commitText();fPage=page; }
        repaint();
    }
    void commitText(bool cancel=false) {
        if(fEdit<0)return;
        if(!cancel) {
            std::replace(fText.begin(),fText.end(),',','.');
            char* tail=nullptr;float v=std::strtof(fText.c_str(),&tail);
            if(tail!=fText.c_str()) {
                while(*tail==' ')++tail;
                const std::string unit=kFaustParameters[fEdit].unit;
                if((*tail=='k'||*tail=='K') && unit=="Hz") {v*=1000;++tail;}
                else if(*tail=='s' && unit=="ms") {v*=1000;++tail;}
                if(*tail=='\0' || unit==tail) set(fEdit,v);
            }
        }
        fEdit=-1;fText.clear();
    }
    void textEntry(int p) {
        fEdit=p;char s[32];std::snprintf(s,sizeof(s),"%.4g",value(p));fText=s;fSelectText=true;
    }
    void adjustCrossover(const Hit& h,double requested) {
        const auto before=xo();
        const auto next=MbCompResponse::pushCrossover(before,h.band,requested,sampleRate());
        const bool rising=next[h.band]>before[h.band];
        // Move the neighbours out of the way first. Every affected parameter
        // participates in the same mouse gesture for host automation/undo.
        for(int step=0;step<4;++step) {
            const int i=rising?3-step:step;
            const int p=crossovers[i];
            if(value(p)==next[i])continue;
            if(std::none_of(fGesture.begin(),fGesture.end(),[p](const Gesture& g){return g.parameter==p;})) {
                fGesture.push_back({p,normalized(p,value(p))});
                fInterface->parameterControlPressed(kParametersMainStart+p);
            }
            write(p,next[i]);
        }
    }
    void crossoverDrag(const Hit& h,float x) {
        adjustCrossover(h,MbCompResponse::frequency(std::clamp(x/width(),0.f,1.f)));
    }
    bool onMouse(const MouseEvent& ev) final {
        if(ev.button!=kMouseButtonLeft)return false;
        const float x=ev.pos.getX()/fScaleFactor,y=ev.pos.getY()/fScaleFactor;
        if(!ev.press) {
            if(!fDragging)return false;
            const Hit d=fDrag;const bool moved=fMoved;
            if(!moved && d.kind==Kind::Slope)write(d.parameter,(static_cast<int>(value(d.parameter))+1)%3);
            endGesture();
            if(!moved && d.kind==Kind::Value)textEntry(d.parameter);
            return true;
        }
        commitText();
        const Hit* hp=hit(x,y);
        if(!hp)return false;
        const Hit h=*hp;
        switch(h.kind) {
        case Kind::Toggle:set(h.parameter,value(h.parameter)>.5f?0:1);return true;
        case Kind::Preset:applyPreset(h.parameter);return true;
        case Kind::Stars:fStars=!fStars;return true;
        case Kind::Spectrum:fSpectrum=!fSpectrum;return true;
        case Kind::Back:fInterface->buttonClicked(kWidgetExpert);return true;
        default:break;
        }
        const double now=getTime();
        if(h.parameter>=0 && fLastClick==h.parameter && now-fClickTime<.35) {
            if(h.kind==Kind::Crossover) {
                beginGesture(h,false);
                adjustCrossover(h,kFaustParameters[h.parameter].init);
                endGesture();
            } else set(h.parameter,kFaustParameters[h.parameter].init);
            fLastClick=-1;return true;
        }
        fLastClick=h.parameter;fClickTime=now;
        beginGesture(h,(ev.mod&(kModifierControl|kModifierSuper))!=0);
        fDragX=x;fDragY=fLastY=y;
        if(h.kind==Kind::Meter) {write(h.parameter,-60*std::clamp((y-h.box.y)/h.box.h,0.f,1.f));fMoved=true;}
        return true;
    }
    bool onMotion(const MotionEvent& ev) final {
        const float x=ev.pos.getX()/fScaleFactor,y=ev.pos.getY()/fScaleFactor;
        if(!fDragging) {
            const Hit* h=hit(x,y);
            getWindow().setCursor(h?(h->kind==Kind::Crossover?kMouseCursorLeftRight:
                h->kind==Kind::Value||h->kind==Kind::Slope||h->kind==Kind::Meter||h->kind==Kind::Amount?kMouseCursorUpDown:kMouseCursorHand):kMouseCursorArrow);
            return h!=nullptr;
        }
        if(!fMoved && std::abs(y-fDragY)+std::abs(x-fDragX)<2)return true;
        fMoved=true;
        if(fDrag.kind==Kind::Crossover)crossoverDrag(fDrag,x);
        else if(fDrag.kind==Kind::Meter)write(fDrag.parameter,-60*std::clamp((y-fDrag.box.y)/fDrag.box.h,0.f,1.f));
        else if(fDrag.kind==Kind::Slope) {
            fDelta+=(fLastY-y)/((ev.mod&kModifierShift)?88.f:22.f);
            write(fDrag.parameter,denormalized(fDrag.parameter,fGesture.front().start)+std::round(fDelta));
        }
        else if(fDrag.kind==Kind::Amount) {
            const float applied=shiftAll(fDelta+(fLastY-y)*((ev.mod&kModifierShift)?.08f:.32f));
            fTurn[fDrag.field>0]+=(applied-fDelta)*.1f;fDelta=applied;
        }
        else {
            fDelta+=(fLastY-y)/((ev.mod&kModifierShift)?760.f:190.f);
            for(const auto& g:fGesture)write(g.parameter,denormalized(g.parameter,g.start+fDelta));
        }
        fLastY=y;return true;
    }
    bool onScroll(const ScrollEvent& ev) final {
        if(fDragging)return true;
        commitText();
        const Hit* hp=hit(ev.pos.getX()/fScaleFactor,ev.pos.getY()/fScaleFactor);
        if(!hp || ev.delta.getY()==0)return false;
        const Hit h=*hp;
        const float step=(ev.delta.getY()>0?1:-1)*((ev.mod&kModifierShift)?.004f:.02f);
        if(h.kind==Kind::Slope) {set(kFaustParameterSlope,value(kFaustParameterSlope)+(step>0?1:-1));return true;}
        if(h.kind!=Kind::Value && h.kind!=Kind::Meter && h.kind!=Kind::Amount && h.kind!=Kind::Crossover)return false;
        beginGesture(h,(ev.mod&(kModifierControl|kModifierSuper))!=0);
        if(h.kind==Kind::Crossover)crossoverDrag(h,(MbCompResponse::position(xo()[h.band])+step)*width());
        else if(h.kind==Kind::Amount) fTurn[h.field>0]+=shiftAll(step*60)*.1f;
        else for(const auto& g:fGesture)write(g.parameter,denormalized(g.parameter,g.start+step));
        endGesture();return true;
    }
    bool onKeyboard(const KeyboardEvent& ev) final {
        if(fEdit<0 || !ev.press)return false;
        if(ev.key==kKeyEnter)commitText();
        else if(ev.key==kKeyEscape)commitText(true);
        else if(ev.key==kKeyBackspace) {if(fSelectText)fText.clear();else if(!fText.empty())fText.pop_back();fSelectText=false;}
        else if((ev.mod&(kModifierControl|kModifierSuper)) && (ev.key=='a'||ev.key=='A'))fSelectText=true;
        return true;
    }
    bool onCharacterInput(const CharacterInputEvent& ev) final {
        if(fEdit<0)return false;
        const char c=static_cast<char>(ev.character);
        if((c>='0'&&c<='9')||c=='.'||c==','||c=='-'||c=='+'||c=='k'||c=='K'||c=='s') {
            if(fSelectText) {fText.clear();fSelectText=false;}
            if(fText.size()<24)fText+=c;
        }
        return true;
    }
};

class MbCompMainArea final : public MainAreaContainerWidget<MbCompWidget>
{
public:
    explicit MbCompMainArea(LabTopLevelWidget* parent) : MainAreaContainerWidget(parent) {}
    bool starsVisible() const { return fStage->starsVisible(); }
    void finishInteraction() { fStage->finishInteraction(); }
};

// Like the EQ, a starfield shader sits underneath the transparent NanoVG display.
// The response and measured band-level overlay are drawn entirely in NanoVG.
class MbCompRoot final : public RootBaseWidget, private IdleCallback
{
    std::shared_ptr<TopBar> fTopBar=addWidget<TopBar>();
    std::shared_ptr<MbCompMainArea> fMain=addWidget<MbCompMainArea,Expanding>();
public:
    MbCompRoot(Window& window,LabUIWidgetInterface* iface) : RootBaseWidget(window,iface) {addIdleCallback(this);}
    void finishInteraction() { fMain->finishInteraction(); }
private:
    void idleCallback() final {
        if(!fShaders.empty()) {
            fShaders.front()->setVisible(fMain->starsVisible());
        }
    }
    void updateSize(bool children) final {
        RootBaseWidget::updateSize(children);
        for(auto* shader:fShaders) {
            shader->setAbsolutePos(fMain->getMainAreaAbsolutePos());
            shader->setSize(fMain->getMainAreaSize());
            shader->setBorderRadius(fMain->getMainAreaBorderRadius());
        }
    }
};

class MbCompUI final : public DISTRHO_NAMESPACE::LibreAudioBaseUI
{
    std::unique_ptr<BotShaderBaseWidget> fStarfield {
        new BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                            SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this,this)
    };
public:
    MbCompUI()
    {
        createRootWidget<MbCompRoot>();
        fRootWidget->enableShaders({fStarfield.get()});
    }
    ~MbCompUI() override
    {
        // Close automation gestures before LibreAudioBaseUI frees its parameter state.
        static_cast<MbCompRoot*>(fRootWidget.get())->finishInteraction();
    }
};
} // namespace LibreAudio

START_NAMESPACE_DISTRHO
UI* createUI() { return new LibreAudio::MbCompUI(); }
END_NAMESPACE_DISTRHO
