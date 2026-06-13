import("stdfaust.lib");

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Flanger";
declare unique_id "LAfl";

process = _,_ : flanger : _,_;


flanger = flanger_stereo
//flanger = ef.dryWetMixerConstantPower(drywet, flanger_stereo)
with{
    flanger_group(x) = vgroup("FLANGER", x);
    meter_group(x) = flanger_group(hgroup("[0]", x));
    ctl_group(x) = flanger_group(hgroup("[1]", x));
    del_group(x) = flanger_group(hgroup("[2] Delay Controls", x));
    lvl_group(x) = flanger_group(hgroup("[3]", x));

    invert = meter_group(checkbox("[1] Invert Flange Sum"));

    // FIXME: This should be an amplitude-response display:
    flangeview = lfor(freq) + lfol(freq) : meter_group(hbargraph("[2] Flange LFO ", -1.5,+1.5));

    flanger_stereo(x,y) = attach(x,flangeview),y :
        //*(level),*(level) : 
        pf.flanger_stereo(dmax,curdel1,curdel2,depth,fb,invert);

    lfol = os.oscrs;
    lfor = os.oscrc;

    dmax = 2048;
    dflange = 0.001 * ma.SR * del_group(hslider("[1] Flange Delay [unit:ms] [style:knob] [symbol:delay]", 10, 0, 20, 0.001));
    odflange = 0.001 * ma.SR * del_group(hslider("[2] Delay Offset [unit:ms] [style:knob] [symbol:delay_offset]", 1, 0, 20, 0.001));
    freq   = ctl_group(hslider("[1] Speed [unit:Hz] [style:knob] [symbol:speed]", 0.5, 0, 10, 0.01));
    depth  = ctl_group(hslider("[2] Depth [style:knob] [symbol:depth]", 1, 0, 1, 0.001));
    fb     = ctl_group(hslider("[3] Feedback [style:knob] [symbol:feedback]", 0, -0.999, 0.999, 0.001));
    level  = lvl_group(hslider("Flanger Output Level [unit:dB] [symbol:level]", 0, -60, 10, 0.1)) : ba.db2linear;
    drywet  = lvl_group(hslider("dry/wet [unit:%] [symbol:drywet]", 50, 0, 100, 1)) / 100;
    curdel1 = odflange+dflange*(1 + lfol(freq))/2;
    curdel2 = odflange+dflange*(1 + lfor(freq))/2;
};
