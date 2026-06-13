import("stdfaust.lib");

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Tilt EQ";
declare unique_id "LAte";

Nch = 2; //stereo

process = par(i,Nch, tiltEQ : gain_compensation);

tiltEQ = fi.highshelf(1,shelf_gain,shelf_freq);

shelf_freq = hslider("[1] [scale:log] center frequency", 700,20,20000,1);
shelf_gain = hslider("[2] [unit:dB] tilt gain",0,-24,24,0.1);

gain_compensation = _ * ((shelf_gain / 2 : ma.neg) : ba.db2linear) ;
