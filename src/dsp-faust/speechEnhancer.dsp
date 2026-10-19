// -*-Faust-*-

declare author "Klaus Scheuermann";
declare description "Speech enhancer: RNNoise voice isolation, spectral balancing, multiband dynamics and loudness leveling";
declare license "GPL-3.0-or-later";
declare name "Speech Enhancer";
declare unique_id "LAse";
declare voice_isolation "true";

import("stdfaust.lib");

// The suite's leveler, used as is.
lv = library("leveler.dsp");

// The suite's multiband compressor, with fixed settings in place of its controls.
// The values are the multiband compressor of PodcastPluginTrack.dsp
// (PodcastPlugins) at its default style, halfway between its two parameter sets;
// its expander is left out. Every control mbComp5.dsp would declare is
// redefined here, so the only widgets it brings are its gain reduction meters.
mb = library("mbComp5.dsp")[
    knee         = mb_knee;
    range        = 60;        // mbComp5's widest: PodcastPlugins has no ceiling
    rmsTime      = 0;         // peak detection, as co.peak_compression_gain_N_chan_db
    shape        = 1;         // exponential release, Release reads as a time constant
    chanLink     = 1;         // fully linked, as PodcastPlugins
    bandLink     = 0;
    slopeSel     = 1;         // 12 dB/oct: svf shelves at Q .707, as PodcastPlugins'
    lookaheadMs  = 0;
    xf(n)        = ba.take(n, mb_xovers);
    thresh_array = par(b, 5, target + mbParam(mb_thresh1, mb_thresh2, b));
    // the Compression knob scales the slope above the threshold
    ratio_array  = par(b, 5, 1 / (1 - mbParam(mb_strength1, mb_strength2, b) * mb_strength));
    attack_array = par(b, 5, mbParam(mb_att1, mb_att2, b) * 0.001);
    release_array = par(b, 5, mbParam(mb_rel1, mb_rel2, b) * 0.001);
    bypassOf(n)  = 0;
    listenOf(n)  = 0;
    makeupOf(n)  = mbParam(mb_makeup1, mb_makeup2, n - 1) * mb_strength;
    // its lookahead is off, and the limiter reports this plugin's latency
    latency_meter = _;
    detectorMeter(c, n, x) = x;
    meter_group(x) = uiMeters(hgroup("[22]Compressor Reduction", x));
    // named apart from the RNNoise band meters, symbols as in mbComp5.dsp
    bandMeterM(n) = mb.meterBallistics : mb.mid_meters(hbargraph("[%n]Comp Band %n Mid[unit:dB][symbol:gr_band%{n}_mid][label:Comp %n M]",
                                                           mb.maxMeter, 0));
    bandMeterS(n) = mb.meterBallistics : mb.side_meters(hbargraph("[%n]Comp Band %n Side[unit:dB][symbol:gr_band%{n}_side][label:Comp %n S]",
                                                            mb.maxMeter, 0));
];

// Based on bbba_gui.dsp 0.33 from BigBlueBetterAudio, with the spectral
// balancer's band centres and target curve and the multiband compressor's
// settings from PodcastPluginTrack.dsp (PodcastPlugins).
//
// Signal flow:
//
//   common/input.dsp
//   RNNoise voice isolation, in C++ ahead of this DSP (src/LibreAudioVoiceIsolation.hpp)
//   low cut
//   leveler (leveler.dsp)
//   spectral balancer, 20 bands
//   multiband compressor, 5 bands (mbComp5.dsp)
//   lookahead limiter
//   common/output.dsp
//
// The voice isolation stage reads its controls from here and writes its meters
// back. Its voice activity also drives dpf_vad, which the leveler's speech mode
// and the spectral balancer read.
//
// The leveler applies its gain ahead of the correction stage, so the compressor
// thresholds, set relative to the leveler target, meet a signal that is already
// at the target loudness. It measures the output of the correction stage, fed
// back, so that output is what lands on the target.
//
// No Bypass control here: the host wrapper provides one, and input/output trim,
// phase and M/S come from common/input.dsp and common/output.dsp.

Nbands = 20;    // bands of the spectral balancer
numVIBands = 32; // LAS_RNNOISE_NB_BANDS in deps/rnnoise-stereo.h

meters_minimum = -70;

process = attach(_, viControls + viMeters), _
        : par(c, 2, preFilter)
        : levelAndCorrect
        : limiter_lookahead
        : lufs_out_meter;


//======================================================================
// UI
//======================================================================

uiVoice(x)    = hgroup("[1]Voice Isolation", x);
uiDenoiser(x) = uiVoice(hgroup("[1]Denoiser", x));
uiGate(x)     = uiVoice(hgroup("[2]Gate", x));
uiShaping(x)  = hgroup("[2]Sound Shaping", x);
// leveler.dsp's own groups nest inside this one
uiLeveler(x)  = hgroup("[4]Leveler", x);
uiMeters(x)   = hgroup("[9]Meters", x);

// --- Voice isolation, read by the C++ side ---

maxReduction = uiDenoiser(hslider("[01]Max Reduction[style:knob][unit:dB][symbol:max_reduction][label:Max Red.][accentcolor:01][bracket:DENOISER]
      [tooltip: Limits how far any band is turned down. 100 dB is no limit]",
                                  100, 0, 100, 1));

release = uiDenoiser(hslider("[02]Release[style:knob][unit:ms][scale:log][symbol:release][label:Release][accentcolor:01][bracket:DENOISER]
      [tooltip: Fastest a band may fall by 60 dB. RNNoise's own is 135 ms]",
                             135, 10, 1000, 1));

pitchEnhancement = uiDenoiser(checkbox("[03]Pitch Enhancement[symbol:pitch_enhancement][label:Pitch][accentcolor:01][bracket:DENOISER]
      [tooltip: Restores voice harmonics between the bands, estimated per channel, which can blur the stereo image]"));

threshold = uiGate(hslider("[04]Threshold[style:knob][unit:%][symbol:threshold][label:Threshold][accentcolor:04][bracket:GATE]
      [tooltip: Voice activity needed to open the gate. 0% keeps it open]",
                           50, 0, 100, 1));

gracePeriod = uiGate(hslider("[05]Grace Period[style:knob][unit:ms][symbol:grace_period][label:Grace][accentcolor:04][bracket:GATE]
      [tooltip: How long the gate stays open after voice activity drops]",
                             1000, 0, 2000, 1));

viControls = maxReduction + release + pitchEnhancement + threshold + gracePeriod;

// --- Sound shaping ---

preFilter_hp_freq = uiShaping(hslider("[01]Low Cut[style:knob][unit:Hz][scale:log][symbol:pre_lowcut][label:Low Cut][accentcolor:02][bracket:FILTER]
      [tooltip: First-order high pass ahead of the leveler]",
                                      42, 1, 400, 1));

sbmb_strength = uiShaping(hslider("[02]Sound Shaping[style:knob][unit:%][symbol:sbmb_strength][label:Shaping][accentcolor:03][bracket:SHAPING][easy]
      [tooltip: Overall amount of spectral balancing and multiband compression]",
                                  100, 0, 100, 1)) / 100;

sb_strength = uiShaping(hslider("[03]Spectral Balance[style:knob][unit:%][symbol:sb_strength][label:Balance][accentcolor:03][bracket:SHAPING]
      [tooltip: How far the spectrum is pulled towards the target curve]",
                                50, 0, 100, 1)) / 100;

mb_strength = uiShaping(hslider("[04]Compression[style:knob][unit:%][symbol:mb_strength][label:Comp][accentcolor:03][bracket:SHAPING]
      [tooltip: Strength of the multiband compressor]",
                                100, 0, 100, 1)) / 100 * sbmb_strength;

// Target spectrum from PodcastPlugins, band level relative to the full-range
// level in dB. Two curves, blended by its timbre control, here fixed at its
// default. The balancer is level neutral, so only the curve's shape counts.
sb_spectrum1 = -22,-19,-18,-16,  -17,-18,-18,-18,  -18,-19,-20,-22,  -24,-27,-26,-28,  -29,-29,-29,-29;
sb_spectrum2 = -24,-22,-20,-19,  -18,-18,-18,-18,  -20,-22,-24,-24,  -23,-24,-25,-25,  -24,-23,-20,-16;
sb_timbre = 0.5;

// Analysis layer. Not exposed: these shape how the balancer decides, not what the user reaches for.
sb_levelneutral = 1;    // corrections sum to zero: tonal balance only
sb_bandrange = 18;      // dB below the average band before a band stops counting
sb_tolerance = 1;       // dB deadband
sb_threshold = -50;     // dBFS fullrange gate, applied after the ballistics
sb_gateknee = 6;        // dB, soft knee of both gates

// --- Leveler ---

// the same widget as inside the leveler, so the correction stage follows its target
target = uiLeveler(lv.target);
// voice activity 0..1, written by the C++ side from the voice isolation stage
vad_ext = uiLeveler(lv.dpf_vad);


//======================================================================
// Meters
//======================================================================

// --- Voice isolation, written by the C++ side ---

viVadMeters = 0 <: (
    uiMeters(hbargraph("[01]VAD[unit:%][symbol:vad][label:VAD]", 0, 100)),
    uiMeters(hbargraph("[02]VAD Average[unit:%][symbol:vad_average][label:VAD Avg]", 0, 100)),
    uiMeters(hbargraph("[03]VAD Minimum[unit:%][symbol:vad_minimum][label:VAD Min]", 0, 100)),
    uiMeters(hbargraph("[04]VAD Maximum[unit:%][symbol:vad_maximum][label:VAD Max]", 0, 100))
);

// one per RNNoise band, from about 0 Hz up to 20 kHz
viBandMeter(n) = 0 : uiMeters(hgroup("[05]Band Reduction",
    hbargraph("[%o]Band %b[unit:dB][symbol:band_%b][label:Band %b]", -100, 0)))
with {
    o = n + 10;  // keeps the order key two digits wide
    b = n + 1;
};

viMeters = viVadMeters :> _ + (par(i, numVIBands, viBandMeter(i)) :> _);

// --- Sound shaping ---

// Named apart from the RNNoise band meters. The order keys are two digits wide,
// so band 10 does not sort between 1 and 2.
sb_meter(i) = _ <: attach(_, uiMeters(hgroup("[20]Normalized Spectrum",
    vbargraph("[%o]Balance Level %i[unit:dB][symbol:sb_meter_%i][label:Bal Lvl %i]", -40, 40))))
with { o = i + 10; };
// gains arrive in dB, so no linear2db here
sb_gainmeter(i) = _ <: attach(_, uiMeters(hgroup("[21]Spectral Balance Gain",
    vbargraph("[%o]Balance Gain %i[unit:dB][symbol:sb_gain_%i][label:Bal Gain %i]", -12, 12))))
with { o = i + 10; };

// --- Output ---

limiter_meter = _ <: attach(_, abs : ba.linear2db : uiMeters(vbargraph("[30]Limiter[unit:dB][symbol:limiter_gain]", -12, 0)));

// short-term loudness of the plugin output, ahead of the output trim
lufs_out_meter(l, r) = attach(l, lufsS), r
with {
    lufsS = lv.sq(l : lv.kfilter) + lv.sq(r : lv.kfilter)
          : lv.meanSquare(lv.shortTermWindow) : lv.lufs
          : uiMeters(vbargraph("[31]LUFS-S Final[unit:dB][symbol:lufs_out_meter]", meters_minimum, 0));
};


//======================================================================
// Leveler and correction
//======================================================================
//
// The leveler gain goes ahead of the correction stage, and the leveler
// measures the corrected signal, one sample old. Its detectors want power
// referred to the leveler input, so the fed-back power is divided by the
// leveler's own previous gain; the LUFS-I trim and the output meters take it
// as it is.
//
// Feedback bus: leveler gain (dB), LUFS-I trim (dB), corrected left, right.

levelAndCorrect = loop ~ si.bus(4) : (!,!,!,!,_,_)
with {
    loop(gPrev, trimPrev, fl, fr, l, r) =
        (gPrev, trimPrev, l, r : uiLeveler(lv.levelerCore(pDetect, pBrake, outPower)))
        : _, _, (spectralBalancer : mb.wetChain <: si.bus(4))
    with {
        unGain = pow(10.0, 0.0 - gPrev / 10.0);
        pOut = lv.kPower(fl, fr);
        pDetect = pOut * unGain;
        pBrake = lv.brakePower(fl, fr) * unGain;
        outPower(a, b) = pOut;
    };
};


//======================================================================
// Utilities
//======================================================================

ratio2strength(ratio) = 1 - (1/ratio);

preFilter = fi.highpass(1, preFilter_hp_freq);

// 10^(x/20) == e^(x*ln10/20) and 20*log10(x) == ln(x)*20/ln10
lin2db(x) = log(max(ma.MIN, x)) * 8.685889638065035;   // 20/ln(10)
db2lin(x) = exp(x * 0.11512925464970229);              // ln(10)/20

// Interleave two n-buses into n pairs.
pairUp(n) = route(2*n, 2*n, par(i, n, ((i+1, 2*i+1), (n+i+1, 2*i+2))));
// Lay a copy of one scalar beside every signal on an n-bus.
withScalar(n) = route(n+1, 2*n, (par(i, n, (i+1, 2*i+1)),
                                 par(i, n, (n+1, 2*i+2))));

//======================================================================
// Analysis / correction bank
//======================================================================

// Log spaced from 60 Hz to 12 kHz, as PodcastPlugins.
fcOf(i) = 60 * pow(12000 / 60, i / (Nbands - 1));
bandSpacing = log(12000 / 60) / log(2.0) / (Nbands - 1);   // octaves

// Q of a filter whose -3 dB bandwidth is bw octaves: Q = 1/(2 sinh(ln2/2 * bw)).
qOct(bw) = 1.0/(2.0*shx(0.5*log(2.0)*bw)) with { shx(x) = 0.5*(exp(x) - exp(0.0-x)); };
bellWidth = 2.0;                        // correction bells wider than the spacing, so the
qBell = qOct(bandSpacing*bellWidth);    // de-overlapped curve does not scallop between centres

qAnalysis = qOct(bandSpacing);          // one band spacing wide, to tell bands apart

// Analysis bandpass, unity at centre. Detector only, never in the audio path.
bandpass(i) = fi.svf.bp(fcOf(i), qAnalysis) : /(qAnalysis);

// De-overlap. The bells overlap, so what the cascade produces at a band centre
// is the sum of every bell's response there: applied = W . g. W is measured on
// this exact bank at 48 kHz with 6 dB bells; fi.svf.bell sets k = 1/(Q*A), so
// its bandwidth moves with gain and the analytic shape does not describe it.
// Nearest-neighbour leakage 0.5, cond(W) = 9.5. Rows below are inv(W).
// Regenerate if fcOf, qBell or Nbands change.
deconvolve = si.bus(Nbands) <: par(i, Nbands, krow(i))
with {
    krow(i) = par(j, Nbands, *(kmat(i,j))) :> _;
    kmat(i,j) = Krow(i) : ba.selector(j, Nbands);
    Krow(0) = 1.34138, -0.72205, 0.11677, -0.03558, 0.00113, -0.00296, -0.00076, -0.00051, -0.00023, -0.00012, -0.00006, -0.00003, -0.00001, -0.00001, 0, 0, 0, 0, 0, 0;
    Krow(1) = -0.72205, 1.73004, -0.78489, 0.13592, -0.03619, 0.00272, -0.00255, -0.00049, -0.00039, -0.00016, -0.00009, -0.00004, -0.00002, -0.00001, -0.00001, 0, 0, 0, 0, 0;
    Krow(2) = 0.11677, -0.78489, 1.74018, -0.78796, 0.13600, -0.03644, 0.00266, -0.00260, -0.00051, -0.00040, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001, -0.00001, 0, 0, 0, 0;
    Krow(3) = -0.03558, 0.13592, -0.78796, 1.74106, -0.78793, 0.13605, -0.03641, 0.00267, -0.00259, -0.00050, -0.00040, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001, -0.00001, 0, 0, 0;
    Krow(4) = 0.00113, -0.03619, 0.13600, -0.78793, 1.74096, -0.78783, 0.13600, -0.03639, 0.00266, -0.00259, -0.00050, -0.00040, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001, -0.00001, 0, 0;
    Krow(5) = -0.00296, 0.00272, -0.03644, 0.13605, -0.78783, 1.74080, -0.78765, 0.13592, -0.03636, 0.00265, -0.00258, -0.00051, -0.00040, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001, -0.00001, 0;
    Krow(6) = -0.00076, -0.00255, 0.00266, -0.03641, 0.13600, -0.78765, 1.74050, -0.78734, 0.13577, -0.03630, 0.00263, -0.00258, -0.00051, -0.00040, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001, 0;
    Krow(7) = -0.00051, -0.00049, -0.00260, 0.00267, -0.03639, 0.13592, -0.78734, 1.73997, -0.78680, 0.13551, -0.03621, 0.00260, -0.00256, -0.00051, -0.00039, -0.00017, -0.00009, -0.00004, -0.00002, -0.00001;
    Krow(8) = -0.00023, -0.00039, -0.00051, -0.00259, 0.00266, -0.03636, 0.13577, -0.78680, 1.73906, -0.78585, 0.13506, -0.03604, 0.00254, -0.00254, -0.00052, -0.00039, -0.00017, -0.00009, -0.00004, -0.00002;
    Krow(9) = -0.00012, -0.00016, -0.00040, -0.00050, -0.00259, 0.00265, -0.03630, 0.13551, -0.78585, 1.73746, -0.78421, 0.13427, -0.03574, 0.00244, -0.00250, -0.00052, -0.00038, -0.00017, -0.00008, -0.00004;
    Krow(10) = -0.00006, -0.00009, -0.00017, -0.00040, -0.00050, -0.00258, 0.00263, -0.03621, 0.13506, -0.78421, 1.73467, -0.78135, 0.13291, -0.03524, 0.00227, -0.00244, -0.00054, -0.00037, -0.00016, -0.00008;
    Krow(11) = -0.00003, -0.00004, -0.00009, -0.00017, -0.00040, -0.00051, -0.00258, 0.00260, -0.03604, 0.13427, -0.78135, 1.72983, -0.77638, 0.13057, -0.03438, 0.00198, -0.00233, -0.00055, -0.00035, -0.00016;
    Krow(12) = -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00040, -0.00051, -0.00256, 0.00254, -0.03574, 0.13291, -0.77638, 1.72145, -0.76779, 0.12655, -0.03293, 0.00152, -0.00217, -0.00057, -0.00033;
    Krow(13) = -0.00001, -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00040, -0.00051, -0.00254, 0.00244, -0.03524, 0.13057, -0.76779, 1.70703, -0.75309, 0.11979, -0.03057, 0.00082, -0.00192, -0.00060;
    Krow(14) = 0, -0.00001, -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00039, -0.00052, -0.00250, 0.00227, -0.03438, 0.12655, -0.75309, 1.68251, -0.72824, 0.10871, -0.02690, -0.00013, -0.00166;
    Krow(15) = 0, 0, -0.00001, -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00039, -0.00052, -0.00244, 0.00198, -0.03293, 0.11979, -0.72824, 1.64163, -0.68723, 0.09135, -0.02161, -0.00138;
    Krow(16) = 0, 0, 0, -0.00001, -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00038, -0.00054, -0.00233, 0.00152, -0.03057, 0.10871, -0.68723, 1.57572, -0.62211, 0.06624, -0.01527;
    Krow(17) = 0, 0, 0, 0, -0.00001, -0.00001, -0.00002, -0.00004, -0.00009, -0.00017, -0.00037, -0.00055, -0.00217, 0.00082, -0.02690, 0.09135, -0.62211, 1.47531, -0.52474, 0.03290;
    Krow(18) = 0, 0, 0, 0, 0, -0.00001, -0.00001, -0.00002, -0.00004, -0.00008, -0.00016, -0.00035, -0.00057, -0.00192, -0.00013, -0.02161, 0.06624, -0.52474, 1.33679, -0.39036;
    Krow(19) = 0, 0, 0, 0, 0, 0, 0, -0.00001, -0.00002, -0.00004, -0.00008, -0.00016, -0.00033, -0.00060, -0.00166, -0.00138, -0.01527, 0.03290, -0.39036, 1.12825;
};

// fi.svf.bell with the gain in dB. With k = 1/(q*a), scaling the tick divisor
// 1 + g*(g + k) by a turns it into a*(1 + g*g) + g/q, and a then cancels out of
// the mix term k*(a*a - 1)*v1: one division per sample instead of three. The
// min(ma.MAX) is a no-op that stops Faust's normaliser pushing a and
// (a*a - 1)/q back into the quotient, which would turn the one shared division
// into two. Matches the k/d form to float rounding (-105 dB worst case, gain
// swept +-20 dB).
bellv(f, q, gDb) = tick ~ (_,_) : !,!,_
with {
    a   = db2lin(gDb*0.5);              // 10^(g/40)
    g   = tan(ma.PI*f/ma.SR);           // f is constant, so this is init-time
    c1  = 1.0 + g*g;                    // init-time
    c2  = g/q;                          // init-time
    tick(ic1eq, ic2eq, v0) = 2.0*v1 - ic1eq, 2.0*v2 - ic2eq, v0 + (a*a - 1.0)/q*r
    with {
        r  = (ic1eq + g*(v0 - ic2eq))/(a*c1 + c2) : min(ma.MAX);
        v1 = a*r;
        v2 = ic2eq + g*v1;
    };
};

// (gains bus, audio) -> audio. Peeled from the top band down, each stage taking
// the gain sitting next to the signal.
bells = bc(Nbands)
with {
    bc(0) = _;
    bc(n) = (si.bus(n-1), bellAt(n-1)) : bc(n-1);
};
bellAt(i, g, x) = x : bellv(fcOf(i), qBell, g);


//======================================================================
// Spectral balancer
//======================================================================
//
// Each band's level relative to the full-range level is compared with the
// target curve, and the difference applied by one cascade of 20 SVF bells per
// channel.
//
// Detection is stereo-linked: both channels get the same gains. Detected
// levels are linked, never signed audio, so anti-phase signals survive.

spectralBalancer = si.bus(2) <: (sbGains : deconvolve), si.bus(2) : stereoBells
with {
    // Duplicate only the gain bus; each bell cascade has its own audio state.
    stereoBells = route(Nbands+2, 2*(Nbands+1),
        (par(i,Nbands, ((i+1,i+1),(i+1,Nbands+2+i))),
         (Nbands+1,Nbands+1), (Nbands+2,2*(Nbands+1)))) : bells,bells;

    // ---- analysis -----------------------------------------------------------
    measure_full = par(c,2,fi.itu_r_bs_1770_4_kfilter : detect) : max;

    // bbba's 8 values, interpolated over log frequency
    kw_offset = 0.00, -0.22, -0.76, -1.29, -1.13, -0.92, -0.72, -0.55, -0.37, 0.09,
                0.55, 1.11, 1.77, 2.44, 2.73, 3.01, 3.18, 3.24, 3.30, 3.30;

    measure_bp(0) = _ * ba.db2linear(12)
                    : fi.itu_r_bs_1770_4_kfilter
                    : detect;
    measure_bp(i) = _ * ba.db2linear(12 + (kw_offset : ba.selector(i,Nbands)))
                    : detect;

    detect = an.amp_follower_ud(0.01,0.1) : max(-90:ba.db2linear) : lin2db;

    curveBus = si.bus(2) <: ((si.bus(2) <: par(i,Nbands, par(c,2,bandpass(i) : measure_bp(i)) : max)),
                     (measure_full <: si.bus(Nbands)))
             : pairUp(Nbands)
             : par(i,Nbands, (_-_) : sb_meter(i));

    // ---- gains --------------------------------------------------------------
    // bbba's 8 values, interpolated over log frequency
    sb_limitUP = 6, 6.5, 7.7, 8.9, 10.1, 11.3, 12, 12, 12, 12,
                 12, 12, 12, 12, 10.8, 9.6, 8.4, 7.2, 6, 6;
    sb_limitDOWN = 12;
    sb_limit(i) = max(ma.neg(sb_limitDOWN)) : min(sb_limitUP : ba.selector(i,Nbands));

    sb_envelope(i) = si.smooth(ba.tau2pole(tau)) with {
        tau = 0.2 * ((Nbands-i) / Nbands);
    };

    sb_target(i) = (sb_spectrum1 : ba.selector(i,Nbands)) * (1 - sb_timbre)
                 + (sb_spectrum2 : ba.selector(i,Nbands)) * sb_timbre;

    weights = si.bus(Nbands) <: (si.bus(Nbands),
                                 (si.bus(Nbands) :> _ : /(Nbands)))
            : withScalar(Nbands) : par(i,Nbands, weightOf)
    with {
        weightOf(c, mean) = (c - mean + sb_bandrange)/sb_gateknee : clamp01;
    };

    devMean = si.bus(2*Nbands) : pairUp(Nbands) <: (wSum, dSum) : ratio
    with {
        wSum = par(i,Nbands, (!,_)) :> _ : max(ma.EPSILON);
        dSum = par(i,Nbands, devOf(i)) :> _;
        devOf(i, c, w) = (sb_target(i) - c) * w;
        ratio(w, d) = d / w;
    };

    levelGate = measure_full : _-sb_threshold : _/sb_gateknee : clamp01 : si.smoo;

    toWant = si.bus(2*Nbands) <: (si.bus(2*Nbands), devMean)
           : spread : par(i,Nbands, wantOf(i))
    with {
        spread = route(2*Nbands+1, 3*Nbands,
                   par(i, Nbands, ((i+1,        3*i+1),
                                   (Nbands+i+1, 3*i+2),
                                   (2*Nbands+1, 3*i+3))));
        wantOf(i, c, w, dMean) =
              ((sb_target(i) - c) - dMean*sb_levelneutral)
            : deadband(sb_tolerance)
            : sb_limit(i)
            : _*sb_strength
            : _*sbmb_strength
            : _*vad_ext
            : _*w;
    };

    sbGains = si.bus(2) <: (curveBus <: (si.bus(Nbands), weights) : toWant), levelGate
            : withScalar(Nbands)
            : par(i,Nbands, applyGain(i))
    with {
        applyGain(i, want, gate) = want : sb_envelope(i) : _*gate : sb_gainmeter(i);
    };

    clamp01 = max(0.0) : min(1.0);
    deadband(t, x) = ma.signum(x) * max(0.0, abs(x) - t);
};


//======================================================================
// Multiband compressor settings
//======================================================================
//
// PodcastPluginTrack.dsp's two parameter sets per band, low band first,
// blended by its style control, here fixed at its default. Thresholds are
// relative to the leveler target.

mb_style = 0.5;
mbParam(set1, set2, b) = ba.take(b+1, set1) * (1 - mb_style) + ba.take(b+1, set2) * mb_style;

mb_xovers    = par(i, 4, 120 * pow(6000 / 120, i / 3));   // Hz, log spaced
mb_strength1 = par(b, 5, ratio2strength(1.1));
mb_strength2 = par(b, 5, ratio2strength(2));
mb_thresh1   = -13, -12, -12, -16, -18;                 // dB below target
mb_thresh2   = -23, -16, -18, -23, -22;
mb_att1      = 5, 5, 5, 5, 5;                           // ms
mb_att2      = 2, 2, 2, 2, 2;
mb_rel1      = 200, 160, 120, 80, 40;                   // ms
mb_rel2      = 15, 20, 30, 35, 40;
mb_makeup1   = 0, 0, 0, 0, 0;                           // dB
mb_makeup2   = 6, 6, 6, 6, 6;
mb_knee      = 6 * (1 - mb_style) + 0 * mb_style;       // dB, shared by all bands


//======================================================================
// Lookahead limiter
//======================================================================

limiter_lookahead_time = 0.01;  // in s
limiter_thresh = -1 : ba.db2linear;

// reported to the host on top of the voice isolation latency
limiter_latency = ba.sec2samp(limiter_lookahead_time);
latency_meter = limiter_latency : hbargraph("[symbol:latency_samples]Latency", 0, 4096);

limiter_lookahead = limiter_lad_N(2, limiter_latency, limiter_thresh,
                                  limiter_lookahead_time/twopi, .01, 1/twopi)
                  : attach(_, latency_meter), _
with {
    twopi = 2 * ma.PI;
};

limiter_lad_N(N, LD, ceiling, attack, hold, release) =
      si.bus(N) <: par(i, N, @(LD)),
                   (scaling <: si.bus(N)) : ro.interleave(N, 2) : par(i, N, *)
      with {
           scaling = ceiling / max(amp_profile, ma.EPSILON) : min(1) : limiter_meter;
           amp_profile = par(i, N, abs) : maxN(N) : ba.peakholder(hold * ma.SR) :
               att_smooth(attack) : rel_smooth(release);
           att_smooth(time, in) = si.smooth(ba.tau2pole(time), in);
           rel_smooth(time, in) = an.peak_envelope(time, in);
           maxN(1) = _;
           maxN(2) = max;
           maxN(N) = max(maxN(N - 1));
      };
