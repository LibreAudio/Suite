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

// Based on bbba_gui.dsp 0.33 from BigBlueBetterAudio.
//
// Signal flow:
//
//   common/input.dsp
//   RNNoise voice isolation, in C++ ahead of this DSP (src/LibreAudioVoiceIsolation.hpp)
//   low cut
//   leveler (leveler.dsp)
//   spectral balancer + multiband compressor, one analyser and one bell cascade
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

Nbands = 8;     // bands of the spectral balancer and the multiband dynamics
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
                                50, 0, 100, 1)) / 100 * sbmb_strength;

sb_target_spectrum = -10, -5, -5, -8, -9, -10, -7, -4;

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

sb_meter(i) = _ <: attach(_, uiMeters(hgroup("[20]Normalized Spectrum",
    vbargraph("[%i]Band %i[unit:dB][symbol:sb_meter_%i]", -40, 40))));
// gains arrive in dB, so no linear2db here
sb_gainmeter(i) = _ <: attach(_, uiMeters(hgroup("[21]Spectral Balance Gain",
    vbargraph("[%i]Band %i[unit:dB][symbol:sb_gain_%i]", -12, 12))));
compressor_meter(i) = _ <: attach(_, uiMeters(hgroup("[22]Compressor Gain",
    vbargraph("[%i]Band %i[unit:dB][symbol:mb_comp_gain_%i]", -12, 12))));

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
        : _, _, (correct <: si.bus(4))
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

// Compressor gain with the level supplied in dB. The detector chain is
// abs : lin2db : gain_computer : onePoleSwitching, so scaling the input by a
// gain is exactly adding that gain to the level in dB, which is how the spectral
// balancer's correction reaches this compressor without a trip through linear.
comp_gain_from_db(strength, thresh, att, rel, knee) =
    gain_computer(strength, thresh, knee) : si.onePoleSwitching(rel, att)
with {
    gain_computer(strength, thresh, knee, level) =
        select3((level>(thresh-(knee/2)))+(level>(thresh+(knee/2))),
                0,
                ((level-thresh+(knee/2)) : pow(2)/(2*max(ma.EPSILON,knee))),
                (level-thresh))
        : max(0)*-strength;
};


//======================================================================
// Analysis / correction bank
//======================================================================

// Geometric centres of one-octave bands.
fcL = 70.7, 141.4, 282.8, 565.7, 1131.4, 2262.7, 4525.5, 9051.0;
fcOf(i) = fcL : ba.selector(i, Nbands);

// Q of a filter whose -3 dB bandwidth is bw octaves: Q = 1/(2 sinh(ln2/2 * bw)).
qOct(bw) = 1.0/(2.0*shx(0.5*log(2.0)*bw)) with { shx(x) = 0.5*(exp(x) - exp(0.0-x)); };
qAnalysis = qOct(1.0);          // one band spacing wide, to tell bands apart
bellWidth = 2.0;                // correction bells wider than the spacing, so the
qBell = qOct(1.0*bellWidth);    // de-overlapped curve does not scallop between centres

// Analysis bandpass, unity at centre. Detector only, never in the audio path.
bandpass(i) = fi.svf.bp(fcOf(i), qAnalysis) : /(qAnalysis);

// De-overlap. The bells overlap, so what the cascade produces at a band centre
// is the sum of every bell's response there: applied = W . g. W was measured on
// this exact bank; fi.svf.bell sets k = 1/(Q*A), so its bandwidth moves with gain
// and the analytic shape does not describe it. Nearest-neighbour leakage measures
// 0.19, cond(W) = 2.03. Rows below are inv(W).
// Regenerate if fcL, qBell or Nbands change.
deconvolve = si.bus(Nbands) <: par(i, Nbands, krow(i))
with {
    krow(i) = par(j, Nbands, *(kmat(i,j))) :> _;
    kmat(i,j) = Krow(i) : ba.selector(j, Nbands);
    Krow(0) = 1.35422, -0.77469, 0.22165, -0.05398, 0.01302, -0.00309, 0.00069, -0.00013;
    Krow(1) = -0.77301, 1.80799, -0.91783, 0.25175, -0.06104, 0.01449, -0.00325, 0.00059;
    Krow(2) = 0.21506, -0.90890, 1.85152, -0.91688, 0.25042, -0.05976, 0.01340, -0.00243;
    Krow(3) = -0.05244, 0.25035, -0.91860, 1.84578, -0.91077, 0.24485, -0.05519, 0.00999;
    Krow(4) = 0.01270, -0.06095, 0.25175, -0.91263, 1.83204, -0.89004, 0.22587, -0.04111;
    Krow(5) = -0.00302, 0.01451, -0.06024, 0.24587, -0.89053, 1.78298, -0.81733, 0.16725;
    Krow(6) = 0.00068, -0.00325, 0.01350, -0.05536, 0.22574, -0.81715, 1.62165, -0.60264;
    Krow(7) = -0.00012, 0.00059, -0.00245, 0.01003, -0.04112, 0.16735, -0.60280, 1.23852;
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
// Merged correction
//======================================================================
//
// One analyser drives both stages. The spectral balancer and the multiband
// compressor each produce a per-band gain in dB; they are summed,
// de-overlapped, and realised by one cascade of 8 SVF bells per channel.
//
// The stages stay in series, because a gain in dB simply adds to a level in dB:
// the compressor is fed the band level plus the balancer's gain.
//
// Detection is stereo-linked: both channels get the same gains.

correct = si.bus(2) <: (gainsTotal, si.bus(2)) : stereoBells
with {
    // Duplicate only the gain bus; each bell cascade has its own audio state.
    stereoBells = route(Nbands+2, 2*(Nbands+1),
        (par(i,Nbands, ((i+1,i+1),(i+1,Nbands+2+i))),
         (Nbands+1,Nbands+1), (Nbands+2,2*(Nbands+1)))) : bells,bells;

    // ---- analysis -----------------------------------------------------------
    // Link detected levels, never signed audio, so anti-phase signals survive.
    measure_full = par(c,2,fi.itu_r_bs_1770_4_kfilter : detect) : max;

    kw_offset = 0, -1.33, -0.80, -0.36, 0.80, 2.45, 3.15, 3.30;

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

    // ---- spectral balancer --------------------------------------------------
    sb_limitUP = 6, 9, 12, 12, 12, 12, 9, 6;
    sb_limitDOWN = 12;
    sb_limit(i) = max(ma.neg(sb_limitDOWN)) : min(sb_limitUP : ba.selector(i,Nbands));

    sb_envelope(i) = si.smooth(ba.tau2pole(tau)) with {
        tau = 0.2 * ((Nbands-i) / Nbands);
    };

    sb_target(i) = sb_target_spectrum : ba.selector(i,Nbands);

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

    // ---- multiband compressor ----------------------------------------------
    mb_makeup = 1.5;
    ratio = 4,4,4,4,4,4,4,4;
    thresh = -6,-6,-7,-8,-11,-12,-12,-13;
    att = 30,25,20,15,10,5,3,2;
    rel = 100,80,60,40,20,15,15,15;
    knee = 1;

    mbGain(i) = comp_gain_from_db(
                    ratio2strength(ratio : ba.selector(i,Nbands)),
                    target + (thresh : ba.selector(i,Nbands)),
                    att : ba.selector(i,Nbands) : _*0.001,
                    rel : ba.selector(i,Nbands) : _*0.001,
                    knee)
                : +(mb_makeup) : *(mb_strength) : compressor_meter(i);

    // ---- band levels --------------------------------------------------------
    // the compressor detects instantaneously
    bandAbs(i) = par(c,2,bandpass(i) : abs) : max;
    mbLevels = si.bus(2) <: par(i,Nbands, bandAbs(i) : lin2db);

    // ---- sum both decisions, undo the overlap, correct ----------------------
    gainsTotal = si.bus(2) <: (sbGains, mbLevels)
               : pairUp(Nbands)
               : par(i,Nbands, combine(i))
               : deconvolve
    with {
        combine(i, sbg, mlvl) = sbg + (mlvl + sbg : mbGain(i));
    };

    clamp01 = max(0.0) : min(1.0);
    deadband(t, x) = ma.signum(x) * max(0.0, abs(x) - t);
};


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
