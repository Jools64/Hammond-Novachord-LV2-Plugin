#include "uris.h"
#include "novachord.h"

#include "utils.c"
#include "waveGenerator.c"

void initVibrato(Vibrato* vibrato, f32 frequency, f32 depth)
{
    vibrato->frequency = frequency;
    vibrato->depth = depth;
    vibrato->variance = 2;
    vibrato->enabled = false;
}

void initNovachord(Novachord* novachord, f64 rate)
{
	// Setup the default wave properties
	novachord->rate = rate;
	novachord->frequency = 261.6; // Middle C
	novachord->attackTime = 0.1;
	novachord->releaseTime = 1.0;

	Limiter_init(&novachord->limiter, 50, 500, (u32)rate);

	// Emulate the speaker
	init_3band_state(&novachord->overallEqState, 50, 3500, novachord->rate);
	novachord->overallEqState.lg = novachord->overallEqState.hg = 0;
	novachord->overallEqState.mg = 1;
	// Tone parameters
	init_3band_state(&novachord->eqState, 300, 8000, novachord->rate);
	
	// Resonators
	float slide = 2;
	init_3band_state(&novachord->res1EqState, 500 * slide, 650 * slide, novachord->rate);
	init_3band_state(&novachord->res2EqState, 800 * slide, 1200 * slide, novachord->rate);
	init_3band_state(&novachord->res3EqState, 1500 * slide, 2300 * slide, novachord->rate);
	novachord->res1EqState.lg = novachord->res1EqState.hg = 0;
	novachord->res2EqState.lg = novachord->res2EqState.hg = 0;
	novachord->res3EqState.lg = novachord->res3EqState.hg = 0;
	
	// Mellow filter
	init_3band_state(&novachord->mellowEqState, 500, 4000, novachord->rate);
	novachord->mellowEqState.hg = 0;
	
	// Volume filter
	init_3band_state(&novachord->volumeEqState, 500, 15000, novachord->rate);
	novachord->volumeEqState.mg = novachord->volumeEqState.lg = 1.0;
	novachord->volumeEqState.hg = 0.1;


	// Vibrato filters
	//initVibrato(&novachord->smallVibrato, 8, 0.01);
	initVibrato(&novachord->smallVibrato, 6, 0.006);
	initVibrato(&novachord->normalVibrato, 6, 0.012);
	
	// Initialize note states
	for(u32 t = 0; t < 127; ++t)
		novachord->noteState[t].index = t;

	// Define the frequency of each note
	novachord->noteMap[0]  = 16.35; // C
	novachord->noteMap[1]  = 17.32; // C#
	novachord->noteMap[2]  = 18.35; // D
	novachord->noteMap[3]  = 19.45; // D#
	novachord->noteMap[4]  = 20.60; // E
	novachord->noteMap[5]  = 21.83; // F
	novachord->noteMap[6]  = 23.12; // F#
	novachord->noteMap[7]  = 24.50; // G
	novachord->noteMap[8]  = 25.96; // G#
	novachord->noteMap[9]  = 27.50; // A
	novachord->noteMap[10] = 29.14; // A#
	novachord->noteMap[11] = 30.87; // B
}

float dialToValue(float dialValue, float low, float high, float midCoeficient)
{
	int actualDialValue = (int)roundf(dialValue);
	float delta = high - low;
	switch(actualDialValue)
	{
		case 0: return 0.001;
		case 1: return low;
		case 2: return low + (midCoeficient * delta);
		case 3: return high;
	}
}

typedef struct AttackValues
{
	f32 initialEnvelopPos, attack, sustainVolume, decay, decayVolume;
} AttackValues;

AttackValues attackValues[] = {
	{1.0f, 0.0f, 1.0f, 0.5f, 0.0f}, // 1
	{0.84f, 0.0f, 0.84f, 0.5f, 0.16f}, // 2
	{0.68f, 0.0f, 0.68f, 0.5f, 0.32f}, // 3
	{0.5f, 0.0f, 0.5f, 0.5f, 0.5f}, // 4
	{0.32f, 0.5f, 0.68f, 1.0f, 0.68f}, // 5
	{0.16f, 0.5f, 0.84f, 1.0f, 0.84f}, // 6
	{0.0f, 0.5f, 1.0f,  1.0f, 1.0f}  // 7
};

float safeDivide(float a, float b)
{
	if(a == 0 || b == 0)
		return 0;
	return a / b;
}

#define LOWEST_NOTE 31
#define HIGHEST_NOTE 101

void runNovachord(Novachord* novachord, u32 sampleCount)
{
	f32* const output = novachord->output;
	NovachordParameters params = novachord->parameters;
	
	readPorts(novachord);
	
	WaveSpecification waveSpec = {0};
	waveSpec.gain = *params.gain;
	waveSpec.release = 0;

	int attackState = (int)roundf(*(params.attack));
	AttackValues* attackValue = &attackValues[attackState];
	waveSpec.attack = safeDivide(attackValue->attack, 
		fabs(attackValue->initialEnvelopPos - attackValue->decayVolume));
	waveSpec.sustainVolume = attackValue->sustainVolume;
	waveSpec.decay = safeDivide(attackValue->decay, 
		fabs(attackValue->sustainVolume - attackValue->decayVolume));
	waveSpec.decayVolume = attackValue->decayVolume;
	waveSpec.initialEnvelopPos = attackValue->initialEnvelopPos;

	waveSpec.firstVibrato = novachord->smallVibrato;
	waveSpec.secondVibrato = novachord->normalVibrato;
	
	for (u32 i = 0; i < sampleCount; ++i)
	{
		output[i] = 0;
		// TODO: Optimize this so only notes that are currently playing
		//       something actually trigger the generateSample function.
		
		// TODO: Make wave generator generate blocks of audio?
		
		// Non bass notes
		for(u32 t = LOWEST_NOTE + 18; t < HIGHEST_NOTE; ++t)
		{
			if(t < LOWEST_NOTE + ((HIGHEST_NOTE - LOWEST_NOTE) / 2))
		    	waveSpec.gain = *params.gain - (10 * ((3 - *params.balance)/3));
		    else waveSpec.gain = *params.gain;

			if(t > LOWEST_NOTE + 36)
			{
				waveSpec.release = (max(*novachord->parameters.leftSustain,
					*novachord->parameters.rightSustain) * 2) + 0.2;
			}
			else
				waveSpec.release = (*novachord->parameters.rightSustain * 2) + 0.2;

			output[i] += generateSample(
				&novachord->noteState[t], &waveSpec, 
				novachord->waveOffset, novachord->rate, novachord->noteMap);
		}
		
		if(*params.mellow > 0.5)
		    output[i] = do_3band(&novachord->mellowEqState, output[i]);
		
		    
		for(u32 t = LOWEST_NOTE; t < LOWEST_NOTE + 18; ++t)
		{
			if(t < LOWEST_NOTE + ((HIGHEST_NOTE - LOWEST_NOTE) / 2))
		    	waveSpec.gain = *params.gain - (10 * ((3 - *params.balance)/3));
		    else waveSpec.gain = *params.gain;

			if(t > LOWEST_NOTE + 36)
			{
				waveSpec.release = (max(*novachord->parameters.leftSustain,
					*novachord->parameters.rightSustain) * 2) + 0.2;
			}
			else
				waveSpec.release = (*novachord->parameters.rightSustain * 2) + 0.2;
			
			output[i] += generateSample(
				&novachord->noteState[t], &waveSpec, 
				novachord->waveOffset, novachord->rate, novachord->noteMap);
		}
		
		waveSpec.gain = *params.gain;
		
		novachord->waveOffset++;
	}
		
	novachord->eqState.lg = dialToValue(*params.deepTone, 0.4, 1.6, 0.633);
	novachord->eqState.mg = 0.0; //+ (*params.fullTone * 1.0);
	novachord->eqState.hg = dialToValue(*params.brilliantTone, 0.2, 0.5, 0.655);
	
	novachord->res1EqState.mg = dialToValue(*params.firstResonator, 0.4, 2.6, 0.769);//(*params.firstResonator * 2);
	novachord->res2EqState.mg = dialToValue(*params.secondResonator, 0.4, 2.6, 0.714);
	novachord->res3EqState.mg = dialToValue(*params.thirdResonator, 0.4, 2.6, 0.75);
	
	for (u32 i = 0; i < sampleCount; ++i)
	{
	    f32 o = output[i];
	    output[i] *= dialToValue(*params.fullTone, 0.4, 1.0, 0.686);
	    // Negating because the phase is flipped
	    output[i] -= (float) do_3band(&novachord->eqState, (double)o);
	    output[i] += (float) do_3band(&novachord->res1EqState, (double)o);
	    output[i] += (float) do_3band(&novachord->res2EqState, (double)o);
	    output[i] += (float) do_3band(&novachord->res3EqState, (double)o);
	    
	    output[i] *= *params.swell + 0.5;
	    output[i] *= (*params.volume * 0.6) + 0.4;

	    float highPass = 0;
	    switch((int)roundf(*params.volume))
	    {
	    	case 0:
	    		highPass = 2000;
    			break;
			case 1:
				highPass = 4000;
				break;
			case 2:
				highPass = 6000;
				break;
			case 3:
				highPass = 0;
				break;
	    }

	    if(highPass != 0)
	    {
	    	novachord->volumeEqState.hg = 0;
	    	novachord->volumeEqState.hf = 2 * sin(PI * ((double)highPass / (double)novachord->rate));
	    	output[i] = do_3band(&novachord->volumeEqState, (double) output[i]);
	    }

	    // Limiting
	    output[i] *= 1.0f / 8.0f; // 8 notes at a time before limiting
   		output[i] = clamp(output[i], -1, 1);
		output[i] = 1.5 * output[i] - 0.5 * output[i] * output[i] * output[i];
    }


	Limiter_process(&novachord->limiter, sampleCount, output);

	// This should be optimized with memcpy or a circular buffer
	for(s32 i = clamp(sampleCount-1, 0, PREVIOUS_SAMPLE_COUNT-1); i >= 0; --i)
	{
		for(u32 t = 0; t < PREVIOUS_SAMPLE_COUNT-1; ++t)
			novachord->previousSamples[t] = novachord->previousSamples[t+1];
			
		novachord->previousSamples[PREVIOUS_SAMPLE_COUNT-1] = 
			output[sampleCount - 1 - i];
	}
}

// Called upon new plug-in instance creation.
static LV2_Handle instantiate(
	const LV2_Descriptor* descriptor, f64 rate, const char* bundlePath,
	const LV2_Feature* const* features)
{
	Novachord* novachord = new(Novachord);
	initNovachord(novachord, rate);
	
	// Get MIDI input handle
	LV2_URID_Map* map = null;
	for (u32 i = 0; features[i]; ++i) 
	{
		if (!strcmp(features[i]->URI, LV2_URID__map)) 
		{
			map = (LV2_URID_Map*)features[i]->data;
			break;
		}
		else if (!strcmp(features[i]->URI, LV2_LOG__log)) 
            novachord->log = (LV2_Log_Log*)features[i]->data;
	}

	if (!map)
	{
		fprintf(stderr, "Novachord.lv2 error: Host does not support urid:map\n");
        free(novachord);
		return null;
	}

	novachord->map = map;
	novachord->midiEventUrid = map->map(map->handle, LV2_MIDI__MidiEvent);

	mapNovachordUris(novachord->map, &novachord->uris);
    lv2_atom_forge_init(&novachord->forge, novachord->map);
    lv2_log_logger_init(&novachord->logger, novachord->map, novachord->log);
	
	return (LV2_Handle)novachord;
}

// Called by host to connect to an input or output port.
static void connectPort(LV2_Handle instance, u32 port, void* data)
{
	Novachord* novachord = (Novachord*)instance;

	switch ((PortIndex)port) 
	{
        case NOVACHORD_CONTROL:
        	novachord->control = (LV2_Atom_Sequence*) data;
        	break;
	    case NOVACHORD_NOTIFY:
	    	novachord->notify = (LV2_Atom_Sequence*) data;
	    	break;
		case NOVACHORD_INPUT:
			novachord->input = (LV2_Atom_Sequence*) data;
			break;
		case NOVACHORD_OUTPUT:
			novachord->output = (f32*) data;
			break;
		case NOVACHORD_GAIN:
			novachord->parameters.gain = (f32*) data;
			break;
		case NOVACHORD_DEEP_TONE:
			novachord->parameters.deepTone = (f32*) data;
			break;
		case NOVACHORD_FIRST_RESONATOR:
			novachord->parameters.firstResonator = (f32*) data;
			break;
		case NOVACHORD_SECOND_RESONATOR:
			novachord->parameters.secondResonator = (f32*) data;
			break;
		case NOVACHORD_THIRD_RESONATOR:
			novachord->parameters.thirdResonator = (f32*) data;
			break;
		case NOVACHORD_BRILLIANT_TONE:
			novachord->parameters.brilliantTone = (f32*) data;
			break;
		case NOVACHORD_FULL_TONE:
			novachord->parameters.fullTone = (f32*) data;
			break;
		case NOVACHORD_MELLOW:
			novachord->parameters.mellow = (f32*) data;
			break;
		case NOVACHORD_BALANCE:
			novachord->parameters.balance = (f32*) data;
			break;
		case NOVACHORD_ATTACK:
			novachord->parameters.attack = (f32*) data;
			break;
		case NOVACHORD_VOLUME:
			novachord->parameters.volume = (f32*) data;
			break;
		case NOVACHORD_NORMAL_VIBRATO:
			novachord->parameters.normalVibrato = (f32*) data;
			break;
		case NOVACHORD_SMALL_VIBRATO:
			novachord->parameters.smallVibrato = (f32*) data;
			break;
		case NOVACHORD_LEFT_SUSTAIN:
			novachord->parameters.leftSustain = (f32*) data;
			break;
		case NOVACHORD_SWELL:
			novachord->parameters.swell = (f32*) data;
			break;
		case NOVACHORD_RIGHT_SUSTAIN:
			novachord->parameters.rightSustain = (f32*) data;
			break;
	}
}

// Called upon the host initializing the plugin before running it.
static void activate(LV2_Handle instance)
{
	
}

void updateNovachordMidiState(Novachord* novachord)
{

	LV2_ATOM_SEQUENCE_FOREACH(novachord->input, ev) 
	{
		if (ev->body.type == novachord->midiEventUrid) {
			const u8* const msg = (const u8*)(ev + 1);
			
			Note* note;
			switch (lv2_midi_message_type(msg)) 
			{
				case LV2_MIDI_MSG_NOTE_ON:
					note = &novachord->noteState[msg[1]];
					note->pressed = true;
					note->pressOffset = novachord->waveOffset;//note->waveOffset;
					note->noteStartOffset = novachord->waveOffset;//note->waveOffset;
					break;
							
				case LV2_MIDI_MSG_NOTE_OFF:
					note = &novachord->noteState[msg[1]];
					note->pressed = false;
					note->releaseOffset = novachord->waveOffset;//note->waveOffset;
					note->noteEndOffset = novachord->waveOffset;//note->waveOffset
						//+ novachord->releaseTime * novachord->rate;
					break;
			}
		}
	}
}

static void readPorts(Novachord* novachord)
{
    //novachord->parameters.gain = *novachord->portGain;
}

void processParameters(Novachord* novachord, NovachordParameters* parameters)
{
	novachord->smallVibrato.enabled = (*parameters->smallVibrato > 0.5);
	novachord->normalVibrato.enabled = (*parameters->normalVibrato > 0.5);
}

// Processes a block of audio
static void run(LV2_Handle instance, u32 sampleCount)
{
	Novachord* novachord = (Novachord*)instance;
	updateNovachordMidiState(novachord);
	processParameters(novachord, &novachord->parameters);
	runNovachord(novachord, sampleCount);

	// Handle UI state
	const uint32_t space = novachord->notify->atom.size;
	lv2_atom_forge_set_buffer(&novachord->forge, (uint8_t*)novachord->notify, space);
	lv2_atom_forge_sequence_head(&novachord->forge, &novachord->frame, 0);

	// Process incoming events from GUI
	if (novachord->control) 
	{
		const LV2_Atom_Event* event = 
			lv2_atom_sequence_begin(&(novachord->control)->body);
		// For each incoming message...
		while (!lv2_atom_sequence_is_end(&novachord->control->body, 
										 novachord->control->atom.size, event)) 
		{
			//fprintf(stderr, "novachord.lv2: Received event: \n");
			// If the event is an atom:Blank object
			if (lv2_atom_forge_is_object_type(&novachord->forge, event->body.type)) 
			{
				const LV2_Atom_Object* object = (const LV2_Atom_Object*)&event->body;
				if (object->body.otype == novachord->uris.ui_On) 
				{
					fprintf(stderr, "novachord.lv2: UI on\n");
					// If the object is a ui-on, the UI was activated
					novachord->uiState.active = true;
					novachord->uiState.sendSettings = true;
				} 
				else if (object->body.otype == novachord->uris.ui_Off)
				{
					fprintf(stderr, "novachord.lv2: UI off\n");
					novachord->uiState.active = false;
				}
				else if (object->body.otype == novachord->uris.ui_State) 
				{
					//fprintf(stderr, "novachord.lv2: UI state\n");

					// If the object is a ui-state, it's the current UI settings
					const LV2_Atom* parametersAtom = null;
					lv2_atom_object_get(object, 
									    novachord->uris.ui_Parameters, &parametersAtom,
					                    0);
					if (parametersAtom)
					{
						const LV2_Atom_Vector* vectorAtom = (const LV2_Atom_Vector*) parametersAtom;
					}
				}
			}
			event = lv2_atom_sequence_next(event);
		}
	}

	lv2_atom_forge_pop(&novachord->forge, &novachord->frame);
	
}

// Called by the host after running the plugin. The counterpart to activate.
static void deactivate(LV2_Handle instance)
{
}

// Called when an instance of the plugin is destroyed.
static void cleanup(LV2_Handle instance)
{
	Novachord* novachord = (Novachord*)instance;
	//free(novachord->wave);
	free(novachord);
}

static LV2_State_Status
stateSave(LV2_Handle                instance,
           LV2_State_Store_Function  store,
           LV2_State_Handle          handle,
           uint32_t                  flags,
           const LV2_Feature* const* features)
{
	Novachord* novachord = (Novachord*)instance;
	if (!novachord) {
		return LV2_STATE_SUCCESS;
	}

	/*store(handle, novachord->uris.ui_Parameters,
	      (void*)&novachord->parameters, sizeof(float),
	      novachord->uris.atom_Float,
	      LV2_STATE_IS_POD);*/

	return LV2_STATE_SUCCESS;
}

static LV2_State_Status
stateRestore(LV2_Handle                  instance,
              LV2_State_Retrieve_Function retrieve,
              LV2_State_Handle            handle,
              uint32_t                    flags,
              const LV2_Feature* const*   features)
{
	Novachord* novachord = (Novachord*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	/*const void* parameters = retrieve(
		handle, novachord->uris.ui_Parameters, &size, &type, &valflags);
	if (parameters && size == sizeof(float) && type == novachord->uris.atom_Float) 
	{
		novachord->uiState.sendSettings = true;
	}*/

	return LV2_STATE_SUCCESS;
}

// Returns extension data supported by this plugin
static const void* extensionData(const char* uri)
{
	static const LV2_State_Interface state = {stateSave, stateRestore};
	if (!strcmp(uri, LV2_STATE__interface))
		return &state;
	return null;
}

// The descriptor definition for this plugin
static const LV2_Descriptor descriptor = {
	PLUGIN_URI,
	instantiate,
	connectPort,
	activate,
	run,
	deactivate,
	cleanup,
	extensionData
};

// The host calls this function repeatedly to read the descriptors of all
// the plugins in this executable.
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(u32 index)
{
	switch (index) 
	{
		case 0:
			return &descriptor;
		default:
			return null;
	}
}
