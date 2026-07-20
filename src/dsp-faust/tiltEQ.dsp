import("stdfaust.lib");

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Tilt EQ";
declare unique_id "LAti";

// constants
Nch = 2; //stereo
Order = 4;

// UI
alpha = hslider("[1] Slope of Spectral Tilt across Band[symbol:alpha]",0,-1,1,0.001);
f0 = hslider("[2] Band Start Frequency [unit:Hz][symbol:f0]",100,20,10000,1);
bw = hslider("[3] Band Width [unit:Hz][symbol:bw]",5000,100,10000,1);

// process
//process = par(i,Nch,spectral_tilt(Order));
process = tiltEQ_masterme;

// spectral tilt
spectral_tilt(Order) = fi.spectral_tilt(Order,f0,bw,alpha);

// experiment with single shelf
tiltEQ = fi.highshelf(1,shelf_gain,shelf_freq);

shelf_freq = hslider("[1] [scale:log] center frequency", 700,20,20000,1);
shelf_gain = hslider("[2] [unit:dB] tilt gain",0,-24,24,0.1);

gain_compensation = _ * ((shelf_gain / 2 : ma.neg) : ba.db2linear) ;

// master_me approach
tiltEQ_masterme = par(i,2,_) : par(i,2, fi.lowshelf(N, -gain, freq) : fi.highshelf(N, gain, freq)) with{
    N = 1;
    gain = vslider("[1]eq tilt gain [unit:dB] [symbol:eq_tilt_gain]",0,-6,6,0.5):si.smoo;
    freq = vslider("[2]eq tilt freq [unit:Hz] [scale:log] [symbol:eq_tilt_freq]", 630, 200, 2000,1);
};

