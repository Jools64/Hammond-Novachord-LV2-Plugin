//---------------------------------------------------------------------------
//
// 3 Band EQ :)
//
// EQ.H - Header file for 3 band EQ
//
// (c) Neil C / Etanza Systems / 2K6
//
// Shouts / Loves / Moans = etanza at lycos dot co dot uk 
//
// This work is hereby placed in the public domain for all purposes, including
// use in commercial applications.
//
// The author assumes NO RESPONSIBILITY for any problems caused by the use of
// this software.
//
//----------------------------------------------------------------------------

#ifndef __EQ3BAND__
#define __EQ3BAND__


// ------------
//| Structures |
// ------------

typedef struct
{
// Filter #1 (Low band)

double lf; // Frequency
double f1p0; // Poles ...
double f1p1; 
double f1p2;
double f1p3;

// Filter #2 (High band)

double hf; // Frequency
double f2p0; // Poles ...
double f2p1;
double f2p2;
double f2p3;

// Sample history buffer

double sdm1; // Sample data minus 1
double sdm2; // 2
double sdm3; // 3

// Gain Controls

double lg; // low gain
double mg; // mid gain
double hg; // high gain

} EQSTATE; 


// ---------
//| Exports |
// ---------

extern void init_3band_state(EQSTATE* es, int lowfreq, int highfreq, int mixfreq);
extern double do_3band(EQSTATE* es, double sample);


#endif // #ifndef __EQ3BAND__
//---------------------------------------------------------------------------


//----------------------------------------------------------------------------
//
// 3 Band EQ :)
//
// EQ.C - Main Source file for 3 band EQ
//
// (c) Neil C / Etanza Systems / 2K6
//
// Shouts / Loves / Moans = etanza at lycos dot co dot uk 
//
// This work is hereby placed in the public domain for all purposes, including
// use in commercial applications.
//
// The author assumes NO RESPONSIBILITY for any problems caused by the use of
// this software.
//
//----------------------------------------------------------------------------

// NOTES :
//
// - Original filter code by Paul Kellet (musicdsp.pdf)
//
// - Uses 4 first order filters in series, should give 24dB per octave
//
// - Now with P4 Denormal fix :)


//----------------------------------------------------------------------------


// -----------
//| Constants |
// -----------

static double vsa = (1.0 / 4294967295.0); // Very small amount (Denormal Fix)


// ---------------
//| Initialise EQ |
// ---------------

// Recommended frequencies are ...
//
// lowfreq = 880 Hz
// highfreq = 5000 Hz
//
// Set mixfreq to whatever rate your system is using (eg 48Khz)

void init_3band_state(EQSTATE* es, int lowfreq, int highfreq, int mixfreq)
{
// Clear state 

memset(es,0,sizeof(EQSTATE));

// Set Low/Mid/High gains to unity

es->lg = 1.0;
es->mg = 1.0;
es->hg = 1.0;

// Calculate filter cutoff frequencies

es->lf = 2 * sin(PI * ((double)lowfreq / (double)mixfreq)); 
es->hf = 2 * sin(PI * ((double)highfreq / (double)mixfreq));
}


// ---------------
//| EQ one sample |
// ---------------

// - sample can be any range you like :)
//
// Note that the output will depend on the gain settings for each band 
// (especially the bass) so may require clipping before output, but you 
// knew that anyway :)

double do_3band(EQSTATE* es, double sample)
{
// Locals

double l,m,h; // Low / Mid / High - Sample Values

// Filter #1 (lowpass)

es->f1p0 += (es->lf * (sample - es->f1p0)) + vsa;
es->f1p1 += (es->lf * (es->f1p0 - es->f1p1));
es->f1p2 += (es->lf * (es->f1p1 - es->f1p2));
es->f1p3 += (es->lf * (es->f1p2 - es->f1p3));

l = es->f1p3;

// Filter #2 (highpass)

es->f2p0 += (es->hf * (sample - es->f2p0)) + vsa;
es->f2p1 += (es->hf * (es->f2p0 - es->f2p1));
es->f2p2 += (es->hf * (es->f2p1 - es->f2p2));
es->f2p3 += (es->hf * (es->f2p2 - es->f2p3));

h = es->sdm3 - es->f2p3;

// Calculate midrange (signal - (low + high))

m = es->sdm3 - (h + l);

// Scale, Combine and store

l *= es->lg;
m *= es->mg;
h *= es->hg;

// Shuffle history buffer 

es->sdm3 = es->sdm2;
es->sdm2 = es->sdm1;
es->sdm1 = sample; 

// Return result

return(l + m + h);
}


//----------------------------------------------------------------------------
