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
constexpr std::array<Param, 8> globals {{ kFaustParameterKnee, kFaustParameterRange, kFaustParameterRms_time,
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
constexpr const char* globalLabels[] = { "KNEE", "RANGE", "RMS TIME", "RELEASE SHAPE", "MID-SIDE LINK", "BAND LINK", "LOOKAHEAD", "DRY / WET" };
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
    enum class Kind { Value, Toggle, Crossover, Meter, Amount, Slope, SlopeItem, Presets, Preset, Stars, Spectrum, Back };
    struct Hit { Box box; Kind kind; int parameter = -1, band = -1, field = -1; };
    struct Gesture { int parameter; float start; };
    std::vector<Hit> fHits;
    std::vector<Gesture> fGesture;
    Hit fDrag {};
    bool fDragging = false, fMoved = false, fSlopeMenu = false, fPresetMenu = false;
    bool fStars = true, fSpectrum = true, fSelectText = false;
    float fDragX = 0, fDragY = 0, fLastY = 0, fDelta = 0;
    float fMouseX = -1, fMouseY = -1;
    int fEdit = -1, fLastClick = -1;
    double fClickTime = 0, fHoverTime = 0;
    std::string fText;
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
    Box graph() const { return {0,48,width(),std::max(70.f,height()-(expert() ? 382.f : 165.f))}; }
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
            for (const auto& b : bands) add(b[h.kind == Kind::Amount ? 0 : h.field]);
        else add(h.parameter);
    }
    void panel(Box b, Color color, float radius=7) {
        beginPath(); roundedRect(b.x,b.y,b.w,b.h,radius); fillColor(color); fill();
    }
    void line(float x1,float y1,float x2,float y2,Color color,float thickness=1) {
        beginPath(); moveTo(x1,y1); lineTo(x2,y2); strokeColor(color); strokeWidth(thickness); stroke();
    }
    void label(float x,float y,const char* s,float size,Color color,int align=ALIGN_LEFT|ALIGN_MIDDLE,bool mono=false) {
        fontFace(mono?"mono":"regular"); fontSize(size); textAlign(align); fillColor(color); text(x,y,s);
    }
    void frame(Box b, Color color) {
        panel(b,Color(255,255,255,.045f),10);
        beginPath(); roundedRect(b.x+.5f,b.y+.5f,b.w-1,b.h-1,10);
        strokeColor(Color(color,.15f)); strokeWidth(1); stroke();
    }
    void format(int p, char* s, size_t n, bool units=true) const {
        const auto& a=kFaustParameters[p]; const float v=value(p);
        if (p==kFaustParameterShape) {
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
    void number(Box b,int p,const char* title,Color color,int band=-1,int field=-1) {
        panel(b,track,5);
        label(b.x+7,b.y+9,title,10,muted);
        char text[48]; format(p,text,sizeof(text));
        if (field==1) std::snprintf(text,sizeof(text),"%.1f :1",value(p));
        const bool editing = fEdit==p;
        const bool enabled = band<0 || value(bands[band][6])<.5f;
        save(); scissor(b.x+4,b.y,b.w-8,b.h);
        if (editing && fSelectText) panel({b.x+5,b.y+15,b.w-10,b.h-19},Color(accent,.2f),2);
        label(b.x+7,b.y+b.h*.66f,editing?fText.c_str():text,16,enabled?(editing?ink:color):muted,ALIGN_LEFT|ALIGN_MIDDLE,true);
        restore();
        line(b.x+1,b.y+b.h-1,b.x+1+(b.w-2)*std::clamp(normalized(p,value(p)),0.f,1.f),b.y+b.h-1,enabled?color:muted,2);
        if (enabled) fHits.push_back({b,Kind::Value,p,band,field});
    }
    void button(Box b,const char* text,Kind kind,int id=-1,Color color=accent,bool on=false) {
        panel(b,on?Color(color,.22f):Color(255,255,255,.055f),5);
        label(b.x+b.w/2-(kind==Kind::Slope||kind==Kind::Presets?5:0),b.y+b.h/2,text,11,on?color:muted,ALIGN_CENTER|ALIGN_MIDDLE);
        if(kind==Kind::Slope||kind==Kind::Presets) {
            const float x=b.x+b.w-10,y=b.y+b.h/2;
            line(x-3,y-2,x,y+1,muted);line(x,y+1,x+3,y-2,muted);
        }
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
            panel({x1,0,std::max(0.f,x2-x1),h},Color(colors[b],live(b)?.045f:.012f),0);
        }
        // Five measured detector bands, not an FFT or a simulated spectrum.
        if(fSpectrum && !bypassed()) {
            std::array<float,5> db {}, centers {};
            for(int b=0;b<5;++b) {
                db[b]=std::max(value(levels[b][0]),value(levels[b][1]));
                centers[b]=MbCompResponse::position(std::sqrt(edges[b]*edges[b+1]));
            }
            beginPath();moveTo(0,h);
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
                lineTo(t*width(),g.y+(1-std::clamp((level+60)/60,0.f,1.f))*(h-g.y));
            }
            lineTo(width(),h);closePath();
            fillPaint(linearGradient(0,g.y,0,h,Color(143,163,190,.45f),Color(45,49,59,.05f)));fill();
        }
        for(float f : {50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f}) {
            const float x=MbCompResponse::position(f)*width();
            line(x,g.y,x,h,Color(59,59,68,.4f));
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
        save(); scissor(0,g.y,width(),g.h);
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
            for(int section=0;section<4;++section) {
                const float left=section*width()/4,right=(section+1)*width()/4;
                save();intersectScissor(left,g.y,right-left,g.h);
                if(c==0) {
                    path();lineTo(width(),yDb(0));lineTo(0,yDb(0));closePath();
                    fillPaint(linearGradient(0,yDb(0),0,yDb(-18),rainbow((section+.5f)/4,0),
                        rainbow((section+.5f)/4,.35f)));fill();
                    path();strokeColor(rainbow((section+.5f)/4,bypassed()?.04f:.13f));strokeWidth(6);stroke();
                }
                path();const float opacity=bypassed()?.25f:c==0?.95f:.5f;
                strokePaint(linearGradient(left,0,right,0,Color(colors[section],opacity),Color(colors[section+1],opacity)));
                strokeWidth(c==0?2:1);stroke();restore();
            }
        }
        restore();
        label(width()-12,g.y+g.h-13,"M —  S ·   WET GAIN",9,muted,ALIGN_RIGHT|ALIGN_MIDDLE);
        if(fSpectrum) label(9,g.y+g.h-13,"5-BAND INPUT LEVEL",9,muted);
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
            line(x,42,x,g.y+g.h,Color(colors[i+1],.5f),1.2f);
            const Box chip {chips[i]-34,g.y+g.h+7,68,22};
            panel(chip,Color(26,27,32,.94f),5);
            char s[40]; const double f=freq[i];
            if(f>=1000) std::snprintf(s,sizeof(s),"%.2fk",f/1000); else std::snprintf(s,sizeof(s),"%.0f Hz",f);
            label(chips[i],chip.y+11,s,11,colors[i+1],ALIGN_CENTER|ALIGN_MIDDLE,true);
            fHits.push_back({{x-8,42,16,g.y+g.h-42},Kind::Crossover,crossovers[i],i});
            fHits.push_back({chip,Kind::Crossover,crossovers[i],i});
        }
    }
    void bandColumns() {
        for(int b=0;b<5;++b) {
            const Box c=column(b); const Color color=live(b)?colors[b]:Color(100,100,110);
            frame(c,value(bands[b][5])>.5f?colors[b]:muted);
            label(c.x+8,c.y+14,names[b],10,color);
            const Box listen {c.x+c.w-44,c.y+6,17,17}, power {c.x+c.w-24,c.y+6,17,17};
            // Speaker and power icons from the prototype.
            panel(listen,value(bands[b][5])>.5f?colors[1]:Color(255,255,255,.04f),4);
            const Color speaker=value(bands[b][5])>.5f?track:ink;
            beginPath(); moveTo(listen.x+3,listen.y+6); lineTo(listen.x+6,listen.y+6);
            lineTo(listen.x+9,listen.y+3); lineTo(listen.x+9,listen.y+14);
            lineTo(listen.x+6,listen.y+11); lineTo(listen.x+3,listen.y+11); closePath(); fillColor(speaker); fill();
            line(listen.x+12,listen.y+6,listen.x+12,listen.y+11,speaker);
            panel(power,value(bands[b][6])>.5f?Color(122,47,51):Color(255,255,255,.04f),4);
            beginPath(); arc(power.x+8.5f,power.y+9,5,-.8f,3.94f,CW); strokeColor(ink); strokeWidth(1.2); stroke();
            line(power.x+8.5f,power.y+2,power.x+8.5f,power.y+9,ink,1.3f);
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
            label(m.x+14,m.y+m.h+7,"M S",7,muted,ALIGN_CENTER|ALIGN_MIDDLE);
            if(value(bands[b][6])<.5f) fHits.push_back({m,Kind::Meter,bands[b][0],b,0});
        }
        const Box common {12,height()-64,width()-24,52}; frame(common,accent);
        const float cell=(common.w-16-7*6)/8;
        for(int i=0;i<8;++i) number({common.x+8+i*(cell+6),common.y+8,cell,36},globals[i],globalLabels[i],accent);
    }
    float amount() const { float sum=0;for(const auto& b:bands) sum+=normalized(b[0],value(b[0]));return 1-sum/5; }
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
    void easyControls() {
        const float cx=width()/2,cy=height()*.55f,r=76;
        label(cx,cy-r-27,"AMOUNT",18,ink,ALIGN_CENTER|ALIGN_MIDDLE);
        beginPath(); circle(cx,cy,r); fillPaint(radialGradient(cx-20,cy-30,8,r+30,Color(57,58,66,.98f),Color(23,23,27,.98f))); fill();
        beginPath(); arc(cx,cy,r-8,.75f*M_PI,2.25f*M_PI,CW); strokeColor(track);strokeWidth(4);stroke();
        const float a=.75f*M_PI+amount()*1.5f*M_PI;
        beginPath();arc(cx,cy,r-8,.75f*M_PI,a,CW);strokeColor(bypassed()?muted:accent);strokeWidth(4);stroke();
        line(cx+std::cos(a)*(r-25),cy+std::sin(a)*(r-25),cx+std::cos(a)*(r-11),cy+std::sin(a)*(r-11),ink,3);
        label(cx,cy,"LA",28,accent,ALIGN_CENTER|ALIGN_MIDDLE);
        char s[32];std::snprintf(s,sizeof(s),"%.0f%%",amount()*100);
        label(cx,cy+r+27,bypassed()?"BYPASS":s,18,ink,ALIGN_CENTER|ALIGN_MIDDLE,true);
        fHits.push_back({{cx-r,cy-r,r*2,r*2},Kind::Amount});
        const float w=(width()-24-3*10)/4;
        const int active=activePreset();
        for(int p=0;p<4;++p) {
            const Box b {12+p*(w+10),height()-88,w,70}; frame(b,colors[p]);
            if(active==p)panel(b,Color(colors[p],.12f),10);
            label(b.x+12,b.y+22,presets[p].name,12,colors[p]);
            label(b.x+12,b.y+44,presets[p].description,10,muted);
            fHits.push_back({b,Kind::Preset,p});
        }
    }
    void menus() {
        if(fSlopeMenu) {
            frame({34,41,116,86},accent);panel({34,41,116,86},Color(25,26,31,.99f));
            for(int i=0;i<3;++i) {
                char s[24];std::snprintf(s,sizeof(s),"%d dB/oct",6*(1<<i));
                button({38,45+i*26.f,108,24},s,Kind::SlopeItem,i,accent,static_cast<int>(value(kFaustParameterSlope))==i);
            }
        }
        if(fPresetMenu) {
            const float x=width()-204;
            panel({x,41,192,8*29.f+8},Color(25,26,31,.99f));
            const int active=activePreset();
            for(int i=0;i<8;++i)button({x+4,45+i*29.f,184,26},presets[i].name,Kind::Preset,i,colors[i%5],active==i);
        }
    }
    void onNanoDisplay() final {
        save();scale(fScaleFactor,fScaleFactor);fHits.clear();
        panel({0,0,width(),height()},Color(20,21,26,.32f));
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
            if(expert()) {
                bandColumns();char s[24];std::snprintf(s,sizeof(s),"%d dB/oct",6*(1<<std::clamp(static_cast<int>(value(kFaustParameterSlope)),0,2)));
                button({34,12,100,24},s,Kind::Slope);
            } else easyControls();
            const int preset=activePreset();
            button({width()-204,12,192,24},preset<0?"PRESETS · CUSTOM":presets[preset].name,Kind::Presets);
            menus();
        }
        // A short interaction hint appears after hovering a value or threshold meter.
        if(!fDragging && fEdit<0 && getTime()-fHoverTime>.7 && !fPresetMenu && !fSlopeMenu) {
            const Hit* h=hit(fMouseX,fMouseY);
            if(h && (h->kind==Kind::Value || h->kind==Kind::Meter || h->kind==Kind::Crossover)) {
                const auto& p=kFaustParameters[h->parameter];
                const float x=std::clamp(fMouseX-150,8.f,width()-308),y=std::max(42.f,h->box.y-65);
                panel({x,y,300,54},Color(35,36,42,.98f));
                label(x+10,y+15,p.name,12,ink);
                label(x+10,y+34,h->kind==Kind::Value?"Drag · click to type · Shift: fine · Ctrl/⌘: all bands":"Drag to adjust · double-click to reset",10,muted);
            }
        }
        restore();
    }
    const Hit* hit(float x,float y) const {
        for(auto i=fHits.rbegin();i!=fHits.rend();++i) if(i->box.contains(x,y))return &*i;
        return nullptr;
    }
    void idleCallback() final {
        const Page page=getCurrentPage(fInterface);
        if(fPage!=page) { endGesture();commitText();fSlopeMenu=fPresetMenu=false;fPage=page; }
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
    void crossoverDrag(const Hit& h,float x) {
        const int i=h.band; const auto freq=xo();
        const float low=i==0?20:freq[i-1]*1.02;
        const float high=i==3?std::min(20000.,sampleRate()*.45):freq[i+1]/1.02;
        // If host automation has collapsed crossovers at Nyquist, std::clamp
        // still receives an ordered interval.
        write(h.parameter,std::clamp(static_cast<float>(MbCompResponse::frequency(std::clamp(x/width(),0.f,1.f))),std::min(low,high),high));
    }
    bool onMouse(const MouseEvent& ev) final {
        if(ev.button!=kMouseButtonLeft)return false;
        const float x=ev.pos.getX()/fScaleFactor,y=ev.pos.getY()/fScaleFactor;
        if(!ev.press) {
            if(!fDragging)return false;
            const Hit d=fDrag;const bool moved=fMoved;endGesture();
            if(!moved && d.kind==Kind::Value)textEntry(d.parameter);
            return true;
        }
        commitText();
        const Hit* hp=hit(x,y);
        if(!hp) {fSlopeMenu=fPresetMenu=false;return false;}
        const Hit h=*hp;
        if(fSlopeMenu && h.kind!=Kind::SlopeItem && h.kind!=Kind::Slope) {fSlopeMenu=false;return true;}
        if(fPresetMenu && h.kind!=Kind::Preset && h.kind!=Kind::Presets) {fPresetMenu=false;return true;}
        switch(h.kind) {
        case Kind::Toggle:set(h.parameter,value(h.parameter)>.5f?0:1);return true;
        case Kind::Slope:fSlopeMenu=!fSlopeMenu;fPresetMenu=false;return true;
        case Kind::SlopeItem:set(kFaustParameterSlope,h.parameter);fSlopeMenu=false;return true;
        case Kind::Presets:fPresetMenu=!fPresetMenu;fSlopeMenu=false;return true;
        case Kind::Preset:applyPreset(h.parameter);fPresetMenu=false;return true;
        case Kind::Stars:fStars=!fStars;return true;
        case Kind::Spectrum:fSpectrum=!fSpectrum;return true;
        case Kind::Back:fInterface->buttonClicked(kWidgetExpert);return true;
        default:break;
        }
        const double now=getTime();
        if(h.parameter>=0 && fLastClick==h.parameter && now-fClickTime<.35) {
            set(h.parameter,kFaustParameters[h.parameter].init);fLastClick=-1;return true;
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
            if(std::abs(x-fMouseX)+std::abs(y-fMouseY)>1)fHoverTime=getTime();
            fMouseX=x;fMouseY=y;
            const Hit* h=hit(x,y);
            getWindow().setCursor(h?(h->kind==Kind::Crossover?kMouseCursorLeftRight:
                h->kind==Kind::Value||h->kind==Kind::Meter||h->kind==Kind::Amount?kMouseCursorUpDown:kMouseCursorHand):kMouseCursorArrow);
            return h!=nullptr;
        }
        if(!fMoved && std::abs(y-fDragY)+std::abs(x-fDragX)<2)return true;
        fMoved=true;
        if(fDrag.kind==Kind::Crossover)crossoverDrag(fDrag,x);
        else if(fDrag.kind==Kind::Meter)write(fDrag.parameter,-60*std::clamp((y-fDrag.box.y)/fDrag.box.h,0.f,1.f));
        else {
            fDelta+=(fLastY-y)/((ev.mod&kModifierShift)?760.f:190.f);
            for(const auto& g:fGesture)write(g.parameter,denormalized(g.parameter,g.start+(fDrag.kind==Kind::Amount?-fDelta:fDelta)));
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
        if(h.kind==Kind::Crossover)crossoverDrag(h,(MbCompResponse::position(value(h.parameter))+step)*width());
        else for(const auto& g:fGesture)write(g.parameter,denormalized(g.parameter,g.start+(h.kind==Kind::Amount?-step:step)));
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
