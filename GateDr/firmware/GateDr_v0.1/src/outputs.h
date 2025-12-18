/*
 * data structures and methods for processing channel outputs
 */ 


#ifndef OUTPUTS_H_
#define OUTPUTS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "rtc_count.h"
#include "conf_menu.h"	// for parameter string max char limit
#include "inputs.h"
#include "cv.h"
#include "paramUtils.h"

/*
 *	Options for output trig settings
*/
enum TrigOptions {
	TRIG_OFF,			// no modification, output works like a normal logic gate
	TRIG_RISING,		// output gives a trig on every rising edge of the op out
	TRIG_FALLING,		// output gives a trig on every falling edge of the op out
	TRIG_COV,			// output gives a trig on every rising & falling edge of the op out
	TRIG_TOGGLE
	};

/*
 *	Options for which input to use as a reset for the clock divide function
*/
enum DivResetOptions {
	DIV_RST_NONE,
	DIV_RST_CV1,
	DIV_RST_CV2,
	DIV_RST_IN1,
	DIV_RST_IN2
	};

/*
 *	Options for how the channel will process the second output
*/
enum Output2Options {
	OUT2_SEPARATE,		// output 2 will use its own settings separate from out1
	OUT2_FOLLOW,		// output 2 will copy the settings of out1
	OUT2_INVERT,		// output 2 will always output the inverse of out1
	OUT2_BERN			// output 2 will act as the second output of a Bernoulli gate,
						// i.e. will go high when the gate output of the previous input
						// is 'suppressed' 
	};

/*
 *	settings particular to each individual output
*/
struct OutputSettings {
	uint8_t probability;
	uint16_t delay;			// in ms
	uint8_t trig;			// type of trig to use
	uint16_t trigLen;		// trig length in ms
	uint8_t clkDiv;			// clock divider division
	uint8_t clkPhase;		// clock divider phase in steps
	uint8_t divRst;			// chooses which input resets the clock div function
	uint8_t probabilityCv;
	uint8_t delayCv;
	uint8_t trigCv;
	uint8_t trigLenCv;
	uint8_t clkDivCv;
	uint8_t clkPhaseCv;
	
	// mutable strings for printing values to display
	char probabilityStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char delayStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char trigStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char trigLenStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char clkDivStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char clkPhaseStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char divRstStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	
	// bools to keep track of whether current value is at the default,
	// used by gfx_mono_menu for inverting parameter on the display
	bool probabilityDef;
	bool delayDef;
	bool trigDef;
	bool trigLenDef;
	bool clkDivDef;
	bool clkPhaseDef;
	bool divRstDef;
	};	// 12 bytes to NVM, excludes char arrays & 'default' bools

/*
 *	a struct to hold all of the necessary values per-output that represent
 *	the current state of an outputs individual processes
*/
struct OutputState {
	bool op_prev;			// previous op result, pre-output processing
	uint32_t delay_count;	// most recent RTC count for delay processing
	bool delay_out;			// delay output
	bool delay_prev;		// previous delay output
	uint8_t div_count;		// count for clock divider status
	bool div_out;			// clock divider output
	bool div_prev;			// previous clock divider output
	bool divRst_prev;	// previous states for each divRst input
	uint8_t last_roll;		// used to store last probability roll
	bool prob_out;			// probability processing output
	bool prob_prev;			// previous probability output
	uint32_t trig_count;	// most recent RTC count for trigLen processing
	bool trig_out;			// previous trig processing output
	bool trig_prev;			// trig processing output
	bool out_processed;		// final output state
	
	// previous CV conversion values used for hysteresis when under CV selection
	uint8_t cv_delay_prev;
	uint8_t cv_probability_prev;
	uint8_t cv_trig_prev;
	uint16_t cv_trigLen_prev;
	uint8_t cv_clkDiv_prev;
	uint8_t cv_clkPhase_prev;
	};

struct Output {
	struct OutputSettings output_settings[2];
	struct OutputState output_state[2];
	uint8_t out2_settings;		// sets channel out2 settings as per above enum
	uint32_t *rtcCurentCount;	// current RTC count (updated on processing loop start)
	
	char out2Str[GFX_MONO_MENU_PARAM_MAX_CHAR];
	bool out2Def;
	};

void setOutputSettingsDefaults(struct OutputSettings *settings);
void setOutputStateDefaults(struct OutputState *state);
void processOutput(struct OutputState *state, bool op_out, struct OutputSettings *settings, struct Cv *cv, struct Input *input, uint32_t currentCount);
void processChannelOutput(struct Output *out, bool op_out1, bool op_out2, struct Cv *cv, struct Input *input);

/*
 *	functions for UI callbacks during menu interactions
*/
void updateProbability(struct OutputSettings *settings, bool inc);
void updateTrig(struct OutputSettings *settings, bool inc);
void updateTrigLen(struct OutputSettings *settings, bool inc);
void updateClkDiv(struct OutputSettings *settings, bool inc);
void updateClkPhase(struct OutputSettings *settings, bool inc);
void updateDivRst(struct OutputSettings *settings, bool inc);
void updateDelay(struct OutputSettings *settings, bool inc);
void updateOut2Settings(struct Output *out, bool inc);

/*
 *	used after NVM read to set probabilityDef, delayDef, etc
*/
void readOutputDefaultStates(struct Output *out);

#endif /* OUTPUTS_H_ */