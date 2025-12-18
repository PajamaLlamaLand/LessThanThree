/*
 * source file for input processing
 */ 

#include "inputs.h"

#define THRESH_DEFAULT		1000
#define THRESH_MIN			-8000
#define THRESH_MAX			8000
#define THRESH_INC			200
#define DEFAULT_INVERT		false
#define HYS_DEFAULT			10
#define HYS_MIN				0
#define HYS_MAX				50
#define HYS_INC				1

/*
 *	sets all input settings to their defaults
*/
void setInputDefaults(struct InputSettings *settings) {
	settings->threshold =		THRESH_DEFAULT;
	settings->invert =			DEFAULT_INVERT;
	settings->hysteresis =		HYS_DEFAULT;
	settings->thresholdCv =		CV_NONE;
	settings->invertCv =		CV_NONE;
	settings->hysCv =			CV_NONE;
	sprintf(settings->thresholdStr, "%dmV", THRESH_DEFAULT);
	sprintf(settings->invertStr, "false");
	sprintf(settings->hysStr, "%d0mV", HYS_DEFAULT);
	settings->thresholdDef =	true;
	settings->invertDef =		true;
	settings->hysDef =			true;
}

/*
 *	sets all output state variables to their defaults
*/
void setInputStateDefaults(struct InputState *state) {
	state->input = 0;
	state->comp_prev = false;
	state->input_processed = false;
	state->cv_hys_prev = HYS_DEFAULT;
	state->cv_invert_prev = false;
}

/*
 *	takes a raw ADC read (in mV) and deposits the processed data in 
 *	the InputState struct with current input settings
*/
void processInput(struct InputSettings *settings, struct InputState *state, int16_t in_raw, struct Cv *cv) {
	int16_t thresh = settings->threshold;
	uint8_t hys = settings->hysteresis;
	bool inv = settings->invert;
	
	// CV parameter checks
	// NOTE: for the threshold, we will take the raw CV voltage as the comparator 
	//       threshold, regardless of the selected range setting
	if (settings->thresholdCv != CV_NONE) {
		thresh = cv->value[settings->thresholdCv-1];
	}
	if (settings->hysCv != CV_NONE) {
		hys = normalizeCvUint8(cv, settings->hysCv, HYS_MIN, HYS_MAX, state->cv_hys_prev);
		state->cv_hys_prev = hys;
	}
	if (settings->invertCv != CV_NONE) {
		inv = normalizeCvBool(cv, settings->invertCv, state->cv_invert_prev);
		state->cv_invert_prev = inv;
	}
	
	state->input = in_raw;
	// initial comparator w/ hysteresis calculation
	if (state->comp_prev) {
		state->comp_prev = in_raw > (thresh - (hys*10));
	}
	else {
		state->comp_prev = in_raw > (thresh + (hys*10));
	}
	
	// invert processing
	state->input_processed = inv ? !(state->comp_prev) : state->comp_prev;
}

/*
 *	takes a given channel input struct and inputs in mV and processes the channel
 *	inputs according to the given settings
*/
void processChannelInput(struct Input *input, int16_t in1_mV, int16_t in2_mV, struct Cv *cv) {
	
	processInput(&input->input_settings[0], &input->input_state[0], in1_mV, cv);
	
	if (input->copyIn1) {
		in2_mV = in1_mV;
	}
	processInput(&input->input_settings[1], &input->input_state[1], in2_mV, cv);

}

/*
 *	increment/decrement current threshold parameter by THRESH_INC and
 *	update CV and string params, used by menu functions
*/
void updateThreshold(struct InputSettings *settings, bool inc) {
	
	updateInt16t(&settings->threshold, &settings->thresholdCv, inc, THRESH_MIN, THRESH_MAX, THRESH_INC);
	
	if (settings->thresholdCv != CV_NONE) {
		sprintf(settings->thresholdStr, (settings->thresholdCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->thresholdStr, "%dmV", settings->threshold);
	}
	
	// check if param is default for display invert
	settings->thresholdDef = ((settings->thresholdCv == CV_NONE) && \
						(settings->threshold == THRESH_DEFAULT));
}

/*
 *	increment/decrement current invert parameter and update CV and string parameters,
 *	used by menu functions
*/
void updateInvert(struct InputSettings *settings, bool inc) {
	
	updateBool(&settings->invert, &settings->invertCv, inc);
	
	if (settings->invertCv != CV_NONE) {
		sprintf(settings->invertStr, (settings->invertCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->invertStr, (settings->invert)? "true":"false");
	}
	
	// check if param is default for display invert
	settings->invertDef = ((settings->invertCv == CV_NONE) && (settings->invert == false));
}

/*
 *	increment/decrement the current hysteresis parameter and update CV
 *	and string parameters, used by menu functions
*/
void updateHys(struct InputSettings *settings, bool inc) {
	
	updateUint8t(&settings->hysteresis, &settings->hysCv, inc, HYS_MIN, HYS_MAX, HYS_INC);
	
	if (settings->hysCv != CV_NONE) {
		sprintf(settings->hysStr, (settings->hysCv == CV1)? "CV1":"CV2");
	}
	else {
		sprintf(settings->hysStr, "%d0mV", settings->hysteresis);
	}
	
	// check if param is default for display invert
	settings->hysDef = ((settings->hysCv == CV_NONE) && (settings->hysteresis == HYS_DEFAULT));
}

/*
 *	increment/decrement the current 'copy in1' parameter, no
 *	CV for this option
*/
void updateCopyIn1(struct Input *input, bool inc) {
		
	// only 2 options here, so we just need to toggle :)
	input->copyIn1 = !input->copyIn1;
	sprintf(input->copyIn1Str, (input->copyIn1) ? "Yes":"No");
	input->copyIn1Def = !input->copyIn1;
}

/*
 *	sets isDefault states for all settings within an Input struct
*/
void readInputDefaultStates(struct Input *input) {
	for (uint8_t i=0; i<2; i++) {
		// threshold
		input->input_settings[i].thresholdDef = ((input->input_settings[i].thresholdCv == CV_NONE) && \
					(input->input_settings[i].threshold == THRESH_DEFAULT));
					
		// invert
		input->input_settings[i].invertDef = ((input->input_settings[i].invertCv == CV_NONE) && \
					(input->input_settings[i].invert == false));
					
		// hysteresis
		input->input_settings[i].hysDef = ((input->input_settings[i].hysCv == CV_NONE) && \
					(input->input_settings[i].hysteresis == HYS_DEFAULT));
	}
	
	input->copyIn1Def = !input->copyIn1;
}
