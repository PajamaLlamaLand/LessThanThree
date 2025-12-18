/*
 * source file for output processing
 */ 

#include "outputs.h"

#define PROB_DEFAULT		100
#define PROB_MIN			0
#define PROB_MAX			100
#define PROB_INC			5
#define DELAY_DEFAULT		0
#define DELAY_MIN			0
#define DELAY_MAX			1000
#define DELAY_INC			5
#define TRIG_DEFAULT		TRIG_OFF
#define TRIG_LEN_DEFAULT	200
#define TRIG_LEN_MIN		20
#define TRIG_LEN_MAX		2000
#define TRIG_LEN_INC		20
#define DIV_DEFAULT			1
#define DIV_MIN				1
#define DIV_MAX				32
#define DIV_INC				1
#define DIV_PHASE_DEFAULT	1
#define DIV_PHASE_MIN		1
#define DIV_PHASE_MAX		32
#define DIV_RST_DEFAULT		DIV_RST_NONE

// strings to store enum parameters for display
static const char *trigStrings[] = {"off", "rise", "fall", "COV", "toggle"};
static const char *divRstStrings[] = {"none", "CV1", "CV2", "In1", "In2"};
static const char *out2Strings[] = {"sep", "foll", "inv", "bern"};

/*
 *	set all output settings to their defaults
*/
void setOutputSettingsDefaults(struct OutputSettings *settings) {
	settings->probability =		PROB_DEFAULT;
	settings->delay =			DELAY_DEFAULT;
	settings->trig =			TRIG_DEFAULT;
	settings->trigLen =			TRIG_LEN_DEFAULT;
	settings->clkDiv =			DIV_DEFAULT;
	settings->clkPhase =		DIV_PHASE_DEFAULT;
	settings->divRst =			DIV_RST_DEFAULT;
	settings->probabilityCv =	CV_NONE;
	settings->delayCv =			CV_NONE;
	settings->trigLenCv =		CV_NONE;
	settings->clkDivCv =		CV_NONE;
	settings->clkPhaseCv =		CV_NONE;
	sprintf(settings->probabilityStr, "%d%%", PROB_DEFAULT);
	sprintf(settings->delayStr, "%dms", DELAY_DEFAULT);
	sprintf(settings->trigStr, trigStrings[settings->trig]);
	sprintf(settings->trigLenStr, "%dms", TRIG_LEN_DEFAULT);
	sprintf(settings->clkDivStr, "/%d", DIV_DEFAULT);
	sprintf(settings->clkPhaseStr, "%d", DIV_PHASE_DEFAULT);
	sprintf(settings->divRstStr, divRstStrings[settings->divRst]);
	settings->probabilityDef =	true;
	settings->delayDef =		true;
	settings->trigDef =			true;
	settings->trigLenDef =		true;
	settings->clkDivDef =		true;
	settings->clkPhaseDef =		true;
	settings->divRstDef =		true;
}

/*
 *	set all output state variables to their defaults
*/
void setOutputStateDefaults(struct OutputState *state) {
	state->op_prev =		false;
	state->last_roll =		0;
	state->prob_out =		false;
	state->prob_prev =		false;
	state->delay_out =		false;
	state->delay_prev =		false;
	state->trig_count =		0;
	state->trig_out =		false;
	state->trig_prev =		false;
	state->div_count =		0;
	state->div_out =		false;
	state->div_prev =		false;
	state->divRst_prev =	false;
	state->out_processed =	false;
	
	state->cv_probability_prev = PROB_DEFAULT;
	state->cv_delay_prev = DELAY_DEFAULT;
	state->cv_trig_prev = TRIG_DEFAULT;
	state->cv_trigLen_prev = TRIG_LEN_DEFAULT;
	state->cv_clkDiv_prev = DIV_DEFAULT;
	state->cv_clkPhase_prev = DIV_PHASE_DEFAULT;
}

/*
 *	processes an individual output given a current output state,
 *	the op out, and a settings struct
*/
void processOutput(struct OutputState *state, bool op_out, struct OutputSettings *settings, struct Cv *cv, struct Input *input, uint32_t currentCount) {
	uint8_t prob = settings->probability;
	uint16_t dly = settings->delay;
	uint8_t trg = settings->trig;
	uint16_t trgLen = settings->trigLen;
	uint8_t clkDv = settings->clkDiv;
	uint8_t clkPhs = settings->clkPhase;
	
	// CV parameter checks
	if (settings->probabilityCv != CV_NONE) {
		prob = normalizeCvUint8(cv, settings->probabilityCv, PROB_MIN, PROB_MAX, state->cv_probability_prev);
		state->cv_probability_prev = prob;
	}
	if (settings->delayCv != CV_NONE) {
		dly = normalizeCvUint16(cv, settings->delayCv, DELAY_MIN, DELAY_MAX, state->cv_delay_prev);
		state->cv_delay_prev = dly;
	}
	if (settings->trigCv != CV_NONE) {
		trg = normalizeCvUint8(cv, settings->trigCv, TRIG_OFF, TRIG_COV, state->cv_trig_prev);
		state->cv_trig_prev = trg;
	}
	if (settings->trigLenCv != CV_NONE) {
		trgLen = normalizeCvUint16(cv, settings->trigLenCv, TRIG_LEN_MIN, TRIG_LEN_MAX, state->cv_trigLen_prev);
		state->cv_trigLen_prev = trgLen;
	}
	if (settings->clkDivCv != CV_NONE) {
		clkDv = normalizeCvUint8(cv, settings->clkDivCv, DIV_MIN, DIV_MAX, state->cv_clkDiv_prev);
		state->cv_clkDiv_prev = clkDv;
	}
	if (settings->clkPhaseCv != CV_NONE) {
		clkPhs = normalizeCvUint8(cv, settings->clkPhaseCv, DIV_PHASE_MIN, DIV_PHASE_MAX, state->cv_clkPhase_prev);
		state->cv_clkPhase_prev = clkPhs;
	}
	
	// update previous output state parameters, used for edge detection for
	// the various processing blocks
	state->prob_prev = state->prob_out;
	state->delay_prev = state->delay_out;
	state->trig_prev = state->trig_out;
	state->div_prev = state->div_out;
	
	// process clock divider settings
	if (!(op_out) || clkDv == 1) {	// skip if we're not dividing or don't care
		state->div_out = op_out;
	}
	else {
		clkPhs -= 1;	// update clkPhs to 0-based for easier math
		
		if (!(state->op_prev) && op_out) {	// rising edge check
			state->div_count = (state->div_count + 1) % clkDv;
		}
		
		// force clock phase to be < clock divisor
		clkPhs = clkPhs % clkDv;
		state->div_out = op_out && (state->div_count == clkPhs);
	}
	
	bool reset = false;
	// check div reset here
	switch (settings->divRst) {
		case DIV_RST_CV1:
		reset = cv->value[0] > cv->settings[0].threshold;
		break;
		case DIV_RST_CV2:
		reset = cv->value[1] > cv->settings[1].threshold;
		break;
		case DIV_RST_IN1:
		reset = input->input_state[0].input_processed;
		break;
		case DIV_RST_IN2:
		reset = input->input_state[1].input_processed;
		break;
	}
	
	// check for reset rising edge
	if (reset && !state->divRst_prev) {
		state->div_count = clkDv-1;
	}
	state->divRst_prev = reset;
	
	// update op_prev now that we're done using it
	state->op_prev = op_out;
	
	// process delay
	if (dly == 0) {
		state->delay_out = state->div_out;
	}
	else {
		// rising edge check
		if (state->div_out && !state->div_prev) {
			state->delay_count = currentCount;
		}
		
		state->delay_out = state->div_out && (dly <= (currentCount - state->delay_count));
	}
	
	// process output probability
	if (!(state->delay_prev) && state->delay_out) {	// rising edge
		state->last_roll = rand() % 100;
	}
	
	state->prob_out = state->delay_out && (prob > state->last_roll);
	
	// process trig settings
	switch (trg) {
		case TRIG_OFF:
			state->trig_out = state->prob_out;
			break;
		case TRIG_RISING:
			if (!state->prob_prev && state->prob_out) {	// rising edge
				state->trig_out = true;
				state->trig_count = currentCount;
			}
			else {
				if (state->trig_prev) {	// check if currently in a trig event
					// check RTC count to determine if gate should stop
					state->trig_out = (currentCount - state->trig_count) < trgLen;
				}
				else {
					state->trig_out = false;
				}
			}
			break;
		case TRIG_FALLING:
			if (state->prob_prev && !(state->prob_out)) {	// falling edge
				state->trig_out = true;
				state->trig_count = currentCount;
			}
			else {
				if (state->trig_prev) {	// check if currently in a trig event
					// check RTC count to determine if gate should stop
					state->trig_out = (currentCount - state->trig_count) < trgLen;
				}
				else {
					state->trig_out = false;
				}
			}
			break;
		case TRIG_COV:
			if ((!(state->prob_prev) && state->prob_out) || (state->prob_prev && !(state->prob_out))) {	// COV check
				state->trig_out = true;
				state->trig_count = currentCount;
			}
			else {
				if (state->trig_prev) {	// check if currently in a trig event
					// check RTC count to determine if gate should stop
					state->trig_out = (currentCount - state->trig_count) < trgLen;
				}
				else {
					state->trig_out = false;
				}
			}
			break;
		case TRIG_TOGGLE:
			if (!(state->prob_prev) && state->prob_out) {	// rising edge
				state->trig_out = !state->trig_prev;
			}
			break;
	}
	
	state->out_processed = state->trig_out;
}

/*
 *	takes a given channel output struct and op outs and determines the final outputs
 *	based on the output settings and associated channel 2 setting 
*/
void processChannelOutput(struct Output *out, bool op_out1, bool op_out2, struct Cv *cv, struct Input *input) {
	processOutput(&(out->output_state[0]), op_out1, &(out->output_settings[0]), cv, input, *out->rtcCurentCount);
	
	switch (out->out2_settings) {
		case OUT2_SEPARATE:
			processOutput(&(out->output_state[1]), op_out2, &(out->output_settings[1]), cv, input, *out->rtcCurentCount);
			break;
		case OUT2_FOLLOW:
			processOutput(&(out->output_state[1]), op_out2, &(out->output_settings[0]), cv, input, *out->rtcCurentCount);
			break;
		case OUT2_INVERT:
			out->output_state[1].out_processed = !(out->output_state[0].out_processed);
			break;
		case OUT2_BERN:
			out->output_state[1].out_processed = !(out->output_state[0].out_processed) && input->input_state[0].input_processed;
			break;
	}
}

/*
 *	increment/decrement probability & update CV and string parameters,
 *	used by menu functions
*/
void updateProbability(struct OutputSettings *settings, bool inc) {
	
	updateUint8t(&settings->probability, &settings->probabilityCv, inc, PROB_MIN, PROB_MAX, PROB_INC);
	
	if (settings->probabilityCv != CV_NONE) {
		sprintf(settings->probabilityStr, (settings->probabilityCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->probabilityStr, "%d%%", settings->probability);
	}
	
	// check if param is default for display invert
	settings->probabilityDef = ((settings->probabilityCv == CV_NONE) && \
						(settings->probability == PROB_DEFAULT));
}

/*
 *	increment/decrement trig setting & update CV and parameter string,
 *	used by menu functions
*/
void updateTrig(struct OutputSettings *settings, bool inc) {
	
	updateUint8t(&settings->trig, &settings->trigCv, inc, TRIG_OFF, TRIG_TOGGLE, 1);
	
	if (settings->trigCv != CV_NONE) {
		sprintf(settings->trigStr, (settings->trigCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->trigStr, "%s", trigStrings[settings->trig]);
	}
	
	// check if param is default for display invert
	settings->trigDef = ((settings->trigCv == CV_NONE) && (settings->trig == TRIG_DEFAULT));
}

/*
 *	increment/decrement the trig length parameter, updating the CV and
 *	 parameter string, used by menu functions
*/
void updateTrigLen(struct OutputSettings *settings, bool inc) {
	
	updateUint16t(&settings->trigLen, &settings->trigLenCv, inc, TRIG_LEN_MIN, TRIG_LEN_MAX, TRIG_LEN_INC);
	
	if (settings->trigLenCv != CV_NONE) {
		sprintf(settings->trigLenStr, (settings->trigLenCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->trigLenStr, "%dms", settings->trigLen);
	}
	
	// check if param is default for display invert
	settings->trigLenDef = ((settings->trigLenCv == CV_NONE) && (settings->trigLen == TRIG_LEN_DEFAULT));
}

/*
 *	increment/decrement clock divider parameter, update CV and parameter
 *	string, used by menu functions
*/
void updateClkDiv(struct OutputSettings *settings, bool inc) {
	
	updateUint8t(&settings->clkDiv, &settings->clkDivCv, inc, DIV_MIN, DIV_MAX, DIV_INC);
	
	if (settings->clkDivCv != CV_NONE) {
		sprintf(settings->clkDivStr, (settings->clkDivCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->clkDivStr, "/%d", settings->clkDiv);
	}
	
	// check if param is default for display invert
	settings->clkDivDef = ((settings->clkDivCv == CV_NONE) && (settings->clkDiv == DIV_DEFAULT));
}

/*
 *	increment/decrement the clock divider phase & update the CV and parameter
 *	string, used by menu functions
*/
void updateClkPhase(struct OutputSettings *settings, bool inc) {
	
	updateUint8t(&settings->clkPhase, &settings->clkPhaseCv, inc, DIV_PHASE_MIN, DIV_PHASE_MAX, DIV_INC);
	
	if (settings->clkPhaseCv != CV_NONE) {
		sprintf(settings->clkPhaseStr, (settings->clkPhaseCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->clkPhaseStr, "%d", settings->clkPhase);
	}
	
	// check if param is default for display invert
	settings->clkPhaseDef = ((settings->clkPhaseCv == CV_NONE) && (settings->clkPhase == DIV_PHASE_DEFAULT));
}

/*
 *	increment/decrement the assignment for the clock divider reset
 *	input and update parameter string (no CV control), used by menu functions
*/
void updateDivRst(struct OutputSettings *settings, bool inc) {
	
	if (inc) {
		settings->divRst += 1;
		if (settings->divRst > DIV_RST_IN2) {	// overflow check
			settings->divRst = DIV_RST_NONE;
		}
	}
	else {	// decrementing
		settings->divRst -= 1;
		if (settings->divRst > DIV_RST_IN2) {	// underflow check for uint8_t
			settings->divRst = DIV_RST_IN2;
		}
	}
	sprintf(settings->divRstStr, "%s", divRstStrings[settings->divRst]);
	
	// check if param is default for display invert
	settings->divRstDef = (settings->divRst == DIV_RST_DEFAULT);
}

/*
 *	increment/decrement the delay parameter & update CV and parameter string,
 *	used by menu functions
*/
void updateDelay(struct OutputSettings *settings, bool inc) {
	
	updateUint16t(&settings->delay, &settings->delayCv, inc, DELAY_MIN, DELAY_MAX, DELAY_INC);
	
	if (settings->delayCv != CV_NONE) {
		sprintf(settings->delayStr, (settings->delayCv == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(settings->delayStr, "%dms", settings->delay);
	}
	
	// check if param is default for display invert
	settings->delayDef = ((settings->delayCv == CV_NONE) && (settings->delay == DELAY_DEFAULT));
}

/*
 *	increment/decrement the assignment for how output 2 processes its
 *	settings and update parameter string (no CV control), used by menu functions
*/
void updateOut2Settings(struct Output *out, bool inc) {
	
	if (inc) {
		out->out2_settings += 1;
		if (out->out2_settings > OUT2_BERN) {	// overflow check
			out->out2_settings = OUT2_SEPARATE;
		}
	}
	else {	// decrementing
		out->out2_settings -= 1;
		if (out->out2_settings > OUT2_BERN) {	// underflow check for uint8_t
			out->out2_settings = OUT2_BERN;
		}
	}
	sprintf(out->out2Str, "%s", out2Strings[out->out2_settings]);
	
	// check if param is default for display invert
	out->out2Def = (out->out2_settings == OUT2_SEPARATE);
}

/*
 *	sets isDefault states for all settings within an Output struct
*/
void readOutputDefaultStates(struct Output *out) {
	for (uint8_t i=0; i<2; i++) {
		// probability
		out->output_settings[i].probabilityDef = ((out->output_settings[i].probabilityCv == CV_NONE) && \
		(out->output_settings[i].probability == PROB_DEFAULT));
		
		// trig type
		out->output_settings[i].trigDef = ((out->output_settings[i].trigCv == CV_NONE) && \
					(out->output_settings[i].trig == TRIG_DEFAULT));
		
		// trig length
		out->output_settings[i].trigLenDef = ((out->output_settings[i].trigLenCv == CV_NONE) && \
					(out->output_settings[i].trigLen == TRIG_LEN_DEFAULT));
		
		// clock division
		out->output_settings[i].clkDivDef = ((out->output_settings[i].clkDivCv == CV_NONE) && \
					(out->output_settings[i].clkDiv == DIV_DEFAULT));
		
		// clock div phase
		out->output_settings[i].clkPhaseDef = ((out->output_settings[i].clkPhaseCv == CV_NONE) && \
					(out->output_settings[i].clkPhase == DIV_PHASE_DEFAULT));
		
		// clock div reset input
		out->output_settings[i].divRstDef = (out->output_settings[i].divRst == DIV_RST_DEFAULT);
		
		// delay
		out->output_settings[i].delayDef = ((out->output_settings[i].delayCv == CV_NONE) && \
		(out->output_settings[i].delay == DELAY_DEFAULT));
	}
	
	// output 2 setting
	out->out2Def = (out->out2_settings == OUT2_SEPARATE);
}