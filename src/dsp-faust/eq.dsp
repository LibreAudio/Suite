import("stdfaust.lib");
eq = library("eq.lib");

declare author "Klaus Scheuermann";
declare description "Mid/side parametric EQ modelled on x42-eq (fil4.lv2)";
declare license "GPL-3.0-or-later";
declare name "EQ42";
declare unique_id "LAeq";

//=============================================================================
// Faust port of x42-eq / fil4.lv2 — https://github.com/x42/fil4.lv2
//   (C) Robin Gareus, filter sections (C) Fons Adriaensen, GPL-2+.
//
// Signal chain, in the order the original runs it (src/lv2.c process_channel):
//   input gain -> highpass -> lowpass -> 4x parametric -> lowshelf -> highshelf
//
// Stereo throughout, and mid/side throughout: L/R is encoded once on the way in
// and decoded once on the way out, and every section carries a Stereo/Mid/Side
// selector saying which half it acts on. See the process section at the bottom.
//
// Each filter is transcribed *structurally*, not merely by transfer function,
// because the realisation is a good part of why this EQ sounds the way it does:
//   * parametric sections  = Fons Adriaensen's normalised 2nd-order allpass
//                            ladder, Fil4Paramsect (src/filters.h). Very low
//                            coefficient sensitivity at low frequencies.
//   * shelves              = RBJ shelving biquads in transposed direct form II,
//                            coefficients and topology per src/iir.h.
//   * highpass             = two cascaded one-pole DC blockers with resonance
//                            feedback (src/hip.h).
//   * lowpass              = four one-poles with feedback around the first two
//                            (src/lop.h), plus the fixed -6 dB high shelf at
//                            SR/3 that keeps the slope at -12 dB/oct all the
//                            way to Nyquist (LP_EXTRA_SHELF).
// All parameter maps (RESLP/RESHP resonance curves, the shelf-Q remap, the
// bandwidth-times-7*f/sqrt(g) rule, every clamp) are taken verbatim.
//
// Deliberate differences from the C original:
//   * parameters are smoothed per sample instead of per 32-sample block with
//     0.5x..2x slew limits — same audible result, no zipper noise;
//   * highpass/lowpass on-off crossfades to dry instead of morphing the
//     coefficients toward the identity (which parks poles on the unit circle).
//     Band and shelf on-off still work the original way, by interpolating gain
//     to 1.0, which is an exact bypass for those two forms;
//   * only the parametric sections keep their anti-denormal offset (it is part
//     of Fons' design); elsewhere denormals are left to the host's FTZ/DAZ.
//
// Verified against the C original: 105 single-filter cases across the parameter
// space agree to a median of 4e-5 dB, and with every section set to Stereo a
// full chain of all eight filters agrees to 0.003 dB / 0.03 deg over
// 20 Hz - 22 kHz, with exactly zero L-to-R crosstalk. The residual is
// coefficient rounding - Faust computes the design equations in the sample
// format, the C computes them in double and rounds once - and it shows up as a
// static ~0.003 dB response offset at the very bottom, not as distortion: the
// broadband error floor of the two is the same (-86 vs -88 dBFS in float32).
//=============================================================================

//-------------------------------------------------------------------------- UI
// One vertical section per filter, sections stacked left to right in frequency
// order: input, highpass, low shelf, the four parametric bands, high shelf,
// lowpass. Every continuous control is a knob; the per-section enable stays a
// checkbox and the Stereo/Mid/Side selector a radio, since neither is a dial.
// Both switches bracket the knobs: enable at the head of the section, channel
// selector at the foot.
eqUI(x)      = hgroup("EQ42", x);
inGroup(x)   = eqUI(vgroup("[10] Input", x));
hpGroup(x)   = eqUI(vgroup("[20] Highpass", x));
lsGroup(x)   = eqUI(vgroup("[30] Low Shelf", x));
bGroup(k, x) = eqUI(vgroup("[4%k] Band %k", x));
hsGroup(x)   = eqUI(vgroup("[50] High Shelf", x));
lpGroup(x)   = eqUI(vgroup("[60] Lowpass", x));

// accent colours by role: 01 gain, 02 frequency, 03 bandwidth/resonance,
// 04 input trim - so the same kind of control reads the same across sections.
bypass = inGroup(checkbox("[0] Bypass [symbol:bypass][label:Bypass]"));
gaindb = inGroup(hslider("[1] Gain [style:knob][unit:dB][symbol:gain][label:Gain][accentcolor:04]", 0, -18, 18, 0.1));

hpOn   = hpGroup(checkbox("[0] Highpass [symbol:hp_on][label:On]"));
hpFreq = hpGroup(hslider("[1] Highpass Frequency [style:knob][unit:Hz][scale:log][symbol:hp_freq][label:Freq][accentcolor:02]", 20, 5, 1250, 1));
hpQ    = hpGroup(hslider("[2] Highpass Resonance [style:knob][symbol:hp_q][label:Res][accentcolor:03]", 0.7, 0, 1.4, 0.01));
hpMode = hpGroup(nentry("[3] Highpass Channel [style:radio{'Stereo':0;'Mid':1;'Side':2}][symbol:hp_ms][label:Channel]", 0, 0, 2, 1)) : int;

lsOn   = lsGroup(checkbox("[0] Lowshelf [symbol:ls_on][label:On]"));
lsFreq = lsGroup(hslider("[1] Lowshelf Frequency [style:knob][unit:Hz][scale:log][symbol:ls_freq][label:Freq][accentcolor:02]", 80, 25, 400, 1));
lsQ    = lsGroup(hslider("[2] Lowshelf Bandwidth [style:knob][symbol:ls_q][label:BW][accentcolor:03]", 1.0, 0.0625, 4, 0.01));
lsGain = lsGroup(hslider("[3] Lowshelf Gain [style:knob][unit:dB][symbol:ls_gain][label:Gain][accentcolor:01]", 0, -18, 18, 0.1));
lsMode = lsGroup(nentry("[4] Lowshelf Channel [style:radio{'Stereo':0;'Mid':1;'Side':2}][symbol:ls_ms][label:Channel]", 0, 0, 2, 1)) : int;

hsOn   = hsGroup(checkbox("[0] Highshelf [symbol:hs_on][label:On]"));
hsFreq = hsGroup(hslider("[1] Highshelf Frequency [style:knob][unit:Hz][scale:log][symbol:hs_freq][label:Freq][accentcolor:02]", 8000, 1000, 16000, 1));
hsQ    = hsGroup(hslider("[2] Highshelf Bandwidth [style:knob][symbol:hs_q][label:BW][accentcolor:03]", 1.0, 0.0625, 4, 0.01));
hsGain = hsGroup(hslider("[3] Highshelf Gain [style:knob][unit:dB][symbol:hs_gain][label:Gain][accentcolor:01]", 0, -18, 18, 0.1));
hsMode = hsGroup(nentry("[4] Highshelf Channel [style:radio{'Stereo':0;'Mid':1;'Side':2}][symbol:hs_ms][label:Channel]", 0, 0, 2, 1)) : int;

lpOn   = lpGroup(checkbox("[0] Lowpass [symbol:lp_on][label:On]"));
lpFreq = lpGroup(hslider("[1] Lowpass Frequency [style:knob][unit:Hz][scale:log][symbol:lp_freq][label:Freq][accentcolor:02]", 20000, 500, 20000, 1));
lpQ    = lpGroup(hslider("[2] Lowpass Resonance [style:knob][symbol:lp_q][label:Res][accentcolor:03]", 1.0, 0, 1.4, 0.01));
lpMode = lpGroup(nentry("[3] Lowpass Channel [style:radio{'Stereo':0;'Mid':1;'Side':2}][symbol:lp_ms][label:Channel]", 0, 0, 2, 1)) : int;

//----------------------------------------------------------------- process
// The EQ is stereo and works in the mid/side domain end to end: L/R is encoded
// once on the way in, decoded once on the way out, and everything in between
// runs on mid and side. Stereo is not a special case in that world - it is the
// same filter engaged on both halves, which is identical to filtering L and R,
// because the encode is a linear rotation and the two filters are the same.
//
// Every section therefore holds two instances of its filter, one on mid and one
// on side, and the selector decides which of them is engaged. How a half gets
// disengaged differs by filter type, and deliberately so:
//   * bells and shelves disengage through their own gain, which is an exact
//     bypass at 0 dB. Morphing the gain is phase-coherent, where crossfading
//     the filtered signal against the dry one would comb on the way past;
//   * highpass and lowpass have no gain to morph, so they reuse the same
//     dry/wet crossfade their on-off already needs.
// Either way the disengaged instance keeps running on its half, so its state is
// warm and switching back is seamless.
//
// Encode/decode scaling matches common/input.dsp and common/output.dsp, so the
// suite's global mid/side switch and this one compose without surprises. Note
// the two do compound: with the global switch on, this EQ sees M/S as its L/R,
// and its own Mid becomes the mid of that pair.

band(k) = bGroup(k, eq.bellUI(k, ba.take(k, (150.0, 400.0, 1000.0, 2500.0))));

eqMS = par(i, 2, *(ba.db2linear(gaindb) : eq.smoo))
     : eq.highpass42MS(hpFreq, hpQ, hpOn, hpMode)
     : eq.lowpass42MS (lpFreq, lpQ, lpOn, lpMode)
     : seq(k, 4, band(k + 1))
     : eq.lowshelfMS (lsFreq, lsQ, lsGain, lsOn, lsMode)
     : eq.highshelfMS(hsFreq, hsQ, hsGain, hsOn, hsMode);

process = _, _ <: (_, _, (eq.msEnc : eqMS : eq.msDec)) : blend
with {
    active = 1.0 - bypass : eq.smoo;
    blend(l, r, pl, pr) = si.interpolate(active, l, pl),
                          si.interpolate(active, r, pr);
};
