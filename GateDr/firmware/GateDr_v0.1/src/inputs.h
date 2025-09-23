/*
 *	data structures and methods for processing inputs per channel
 */ 


#ifndef INPUTS_H_
#define INPUTS_H_

#include <stdbool.h>
#include <stdint.h>

#include "port.h"
#include "gfx_mono.h"		// for PROGMEM_STRING_T
#include "conf_menu.h"		// for parameter string max char limit
#include "cv.h"
#include "paramUtils.h"

struct InputSettings {
	int16_t threshold;			// comparator threshold in mV
	bool invert;
	uint8_t hysteresis;			// hysteresis in 10mV increments
	uint8_t thresholdCv;
	uint8_t invertCv;
	uint8_t hysCv;
	
	// mutable strings for printing current values to display
	char thresholdStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char invertStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char hysStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	};	// 7 bytes to NVM, excludes char arrays

struct InputState {
	int16_t input;				// current input in mV
	bool comp_prev;				// previous compare out (before invert) for hysteresis
	bool input_processed;		// final input state
	
	// previous CV conversion values used for hysteresis when under CV selection
	uint8_t cv_hys_prev;
	bool cv_invert_prev;
	};

struct Input {
	struct InputSettings input_settings[2];
	struct InputState input_state[2];
	bool copyIn1;				// input 2 uses ADC conversion from input 1
	
	char copyIn1Str[GFX_MONO_MENU_PARAM_MAX_CHAR];
	};

void setInputDefaults(struct InputSettings *settings);
void setInputStateDefaults(struct InputState *state);
void processInput(struct InputSettings *settings, struct InputState *state, int16_t in_raw, struct Cv *cv);
void processChannelInput(struct Input *input, int16_t in1_mV, int16_t in2_raw, struct Cv *cv);

/*
 *	functions for UI callbacks during menu interactions
*/
void updateThreshold(struct InputSettings *settings, bool inc);
void updateInvert(struct InputSettings *settings, bool inc);
void updateHys(struct InputSettings *settings, bool inc);
void updateCopyIn1(struct Input *input, bool inc);

#endif /* INPUTS_H_ */