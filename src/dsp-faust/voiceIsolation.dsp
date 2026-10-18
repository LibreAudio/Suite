// -*-Faust-*-

declare author "Klaus Scheuermann";
declare description "Voice isolation with RNNoise";
declare license "GPL-3.0-or-later";
declare name "Voice Isolation";
declare unique_id "LAvi";
declare voice_isolation "true";
declare drywet "true";
declare drywet_default "100";
declare graph "analyzer";

import("stdfaust.lib");

// The denoising itself is RNNoise, run in C++ ahead of this DSP
// (src/LibreAudioVoiceIsolation.hpp), stereo-linked: both channels get the
// same spectral mask, the less attenuated estimate of either side per band.
// A gate driven by RNNoise's voice activity mutes the denoised signal between
// speech. The C++ side reads the controls from here, writes the meters, and reports
// its latency to the host.
//
// This DSP only passes audio through; attach keeps the controls and meters in the UI.
//
// No Bypass control here: the host wrapper provides one (see
// LibreAudioPlugin.cpp), and input/output trim, phase and M/S come from
// common/input.dsp and common/output.dsp.

numBands = 32;  // LAS_RNNOISE_NB_BANDS in deps/rnnoise-stereo.h

process = attach(_, controls + meters), _;

controls = maxReduction + release + pitchEnhancement + threshold + gracePeriod;
meters = vadMeters :> _ + (bandMeters :> _);

// --- UI structure ---

uiBottom(x)      = hgroup("[8]Stage Bottom", x);
uiBottomLeft(x)  = uiBottom(hgroup("[1]Stage Bottom Left", x));
uiBottomRight(x) = uiBottom(hgroup("[1]Stage Bottom Right", x));
uiMeters(x)      = hgroup("[9]", x);

// --- Parameters ---

maxReduction = uiBottomLeft(hslider("[01]Max Reduction[style:knob][unit:dB][symbol:max_reduction][label:Max Red.][accentcolor:01][bracket:DENOISER][easy]
      [tooltip: Limits how far any band is turned down. 100 dB is no limit]",
                                    100, 0, 100, 1));

release = uiBottomLeft(hslider("[02]Release[style:knob][unit:ms][scale:log][symbol:release][label:Release][accentcolor:03][bracket:DENOISER]
      [tooltip: Fastest a band may fall by 60 dB. RNNoise's own is 135 ms]",
                               135, 10, 1000, 1));

pitchEnhancement = uiBottomLeft(checkbox("[03]Pitch Enhancement[symbol:pitch_enhancement][label:Pitch][accentcolor:06][bracket:DENOISER]
      [tooltip: Restores voice harmonics between the bands, estimated per channel, which can blur the stereo image]"));

threshold = uiBottomRight(hslider("[04]Threshold[style:knob][unit:%][symbol:threshold][label:Threshold][accentcolor:04][bracket:GATE]
      [tooltip: Voice activity needed to open the gate. 0% keeps it open]",
                                  50, 0, 100, 1));

gracePeriod = uiBottomRight(hslider("[05]Grace Period[style:knob][unit:ms][symbol:grace_period][label:Grace][accentcolor:04][bracket:GATE]
      [tooltip: How long the gate stays open after voice activity drops]",
                                    1000, 0, 2000, 1));

// --- Meters, written by the C++ side ---

vadMeters = 0 <: (
    uiMeters(hbargraph("[1]VAD[unit:%][symbol:vad][label:VAD]", 0, 100)),
    uiMeters(hbargraph("[2]VAD Average[unit:%][symbol:vad_average][label:VAD Avg]", 0, 100)),
    uiMeters(hbargraph("[3]VAD Minimum[unit:%][symbol:vad_minimum][label:VAD Min]", 0, 100)),
    uiMeters(hbargraph("[4]VAD Maximum[unit:%][symbol:vad_maximum][label:VAD Max]", 0, 100))
);

// one per RNNoise band, from about 0 Hz up to 20 kHz
bandMeter(n) = 0 : uiMeters(hgroup("[5]Band Reduction",
    hbargraph("[%o]Band %b[unit:dB][symbol:band_%b][label:Band %b]", -100, 0)))
with {
    o = n + 10;  // keeps the order key two digits wide
    b = n + 1;
};
bandMeters = par(i, numBands, bandMeter(i));
