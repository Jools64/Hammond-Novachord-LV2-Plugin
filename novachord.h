#include <math.h>
#include <stdlib.h>

#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "novachordShared.h"

// Plugin contants
#define PLUGIN_URI "http://epihaumut.com/plugins/novachord"
#define PI 3.141592
#define PREVIOUS_SAMPLE_COUNT 8
#define MAX_PORTS 32

// TODO: This should not be here!
#include "threeband.c"
#include "limiter.h"

typedef struct NovachordParameters
{
    f32* gain, 
       * attack, 
       * deepTone, * brilliantTone, * fullTone, 
       * firstResonator, * secondResonator, * thirdResonator, 
       * balance, 
       * mellow,
       * volume,
       * normalVibrato, * smallVibrato,
       * leftSustain, * rightSustain, * swell;
} NovachordParameters;

typedef enum NoteState
{
	NOTE_ATTACK,
	NOTE_SUSTAIN,
	NOTE_DECAY,
	NOTE_RELEASE
} NoteState;

typedef struct Note
{
	u8 pressed, playing, sustaining;
	f64 waveOffset, envelope;
	u32 index, noteStartOffset, noteEndOffset, pressOffset, releaseOffset;
	NoteState state;
} Note;

typedef struct Vibrato
{
    f32 depth, frequency, variance;
    bool enabled;
} Vibrato;

typedef struct UIState
{
    bool active;
    bool sendSettings;
} UIState;

// The main datastructure for the plugin
typedef struct Novachord
{
	// Parameter Port buffers
	f32* portGain;
	f32* floatPorts[MAX_PORTS];
	
	// Parameters
	NovachordParameters parameters;
	    
	Vibrato smallVibrato, normalVibrato;
	    
    //TODO: this is temp
	EQSTATE eqState, res1EqState, res2EqState, res3EqState, mellowEqState,
		overallEqState, volumeEqState;
	Limiter limiter;
		
	const LV2_Atom_Sequence* input;
	f32* output;
	const LV2_Atom_Sequence* control;
	LV2_Atom_Sequence*       notify;
	
	LV2_URID midiEventUrid;
	LV2_URID_Map* map;
	NovachordUris uris;
	LV2_Atom_Forge       forge;
    LV2_Atom_Forge_Frame frame;

	LV2_Log_Log*   log;
    LV2_Log_Logger logger;
	
	u32 waveOffset;
	f32 frequency, noteMap[12], attackTime, releaseTime;
	f32 previousSamples[PREVIOUS_SAMPLE_COUNT];
	f64 rate;
	Note noteState[127];
	
	UIState uiState;
} Novachord;

static void readPorts(Novachord* novachord);

/*const char* presentNames[] = { "default" };

const NovachordParameters presets[] = {
	{
		0.0f, 				// Gain 
	 	0.0f, 0.5f,			// Attack and release
	 	1.0f, 1.0f, 1.0f,	// Deep, brilliant and full tone controls
	 	1.0f, 1.0f, 1.0f,	// First, second and third resonators
	 	2.0f, 				// Balance
	 	1.0f,				// Mellow
	 	1.0f,				// Volume
	 	0.0f, 1.0f			// Large and small vibrato
	}
};*/