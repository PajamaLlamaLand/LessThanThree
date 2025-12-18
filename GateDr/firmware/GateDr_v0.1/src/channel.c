/*
 * source file for channel processing
 */ 

#include "channel.h"

#define DEFAULT_OP_1 OP_AND
#define	DEFAULT_OP_2 OP_OR
#define DEFAULT_OUT2_SETTINGS OUT2_SEPARATE

// byte sizes for packing/unpacking data to/from NVM
#define SIZE_INPUT	7
#define SIZE_OUTPUT	15
#define SIZE_OP 2

const char *opStrings[] = {"AND", "NAND", "OR", "NOR", "XOR", "XNOR", "S-R", "D", "BYP"};
char submenuStr[3] = "->";					// parameter display 'value' for submenu 
const char *cvStr[] = {" ", "CV1", "CV2"};	// corresponds to CvSel enum
static const char *trigStrings[] = {"off", "rise", "fall", "COV"};
static const char *divRstString[] = {"none", "CV1", "CV2", "In1", "In2"};
static const char *out2Strings[] = {"sep", "foll", "inv"};


// helper function declaration
static inline void writeChannelStrings(struct Channel *ch);
static inline void readChannelDefaultStates(struct Channel *ch);
static inline void assignChannelStrings(struct Channel * ch);

/*
 *	initialize channel, 
*/
void initChannel(struct Channel *ch, uint32_t *currentCount, uint8_t num) {
	ch->out.rtcCurentCount = currentCount;
	assignChannelStrings(ch);
}

/*
 *	set all input, output and channel defaults
*/
void setChannelDefaults(struct Channel *ch, uint8_t num) {
	// initialize ops
	ch->op_select[0] =	DEFAULT_OP_1;
	ch->op_select[1] =	DEFAULT_OP_2;
	ch->op_cv[0] =		CV_NONE;
	ch->op_cv[1] =		CV_NONE;
	ch->cv_op_prev[0] = DEFAULT_OP_1;
	ch->cv_op_prev[1] = DEFAULT_OP_2;
	sprintf(ch->op1Str, opStrings[ch->op_select[0]]);
	sprintf(ch->op2Str, opStrings[ch->op_select[1]]);
	ch->opDef = true;
	
	// initialize inputs and outputs
	for(uint8_t i = 0; i < 2; i++) {
		setInputDefaults(&(ch->input.input_settings[i]));
		setInputStateDefaults(&(ch->input.input_state[i]));
		setOutputSettingsDefaults(&(ch->out.output_settings[i]));
		setOutputStateDefaults(&(ch->out.output_state[i]));
	}
	
	// initialize shared input, output and channel settings
	ch->input.copyIn1 = false;
	sprintf(ch->input.copyIn1Str, "No");
	ch->input.copyIn1Def = true;
	ch->out.out2_settings = DEFAULT_OUT2_SETTINGS;
	sprintf(ch->out.out2Str, "sep");
	ch->out.out2Def = true;
		
	// write settings to NVM
	writeChannelNVM(ch, num);
}

/*
 *	process the inputs, operations, and outputs for a given channel,
 *	using their respective process functions
*/
void processChannel(struct Channel *ch, int16_t in1, int16_t in2, struct Cv *cv) {
	// process inputs
	processChannelInput(&ch->input, in1, in2, cv);
	
	// process each operation from the inputs, selecting from CV if necessary
	uint8_t op;
	bool result[2]; 
	
	for (uint8_t i=0; i<2; i++) {
		op = ch->op_select[i];
		
		if (ch->op_cv[i] != CV_NONE) {	// replace op with CV selection if applicable
			op = normalizeCvUint8(cv, ch->op_cv[i], OP_AND, OP_BYP, ch->cv_op_prev[i]);
			ch->cv_op_prev[i] = op;
		}
		
		// call the selected function using the op_table[] in operations.h
		result[i] = (op_table[op])(ch->input.input_state[0].input_processed, 
							ch->input.input_state[1].input_processed, 
							ch->out.output_state[i].op_prev, i);
	}
	
	// process the outputs
	processChannelOutput(&(ch->out), result[0], result[1], cv, &ch->input);
}

/*
 *	increment/decrement the current op of index 'i', update the
 *	corresponding CV enum and string parameter
*/
void updateOp(struct Channel *ch, bool i, bool inc) {
	updateUint8t(&ch->op_select[i], &ch->op_cv[i], inc, OP_AND, OP_BYP, 1);
	
	if (ch->op_cv[i] != CV_NONE) {
		sprintf(ch->chMenuParams[i+1], (ch->op_cv[i] == CV1)? "CV1":"CV2");
	}
	else {	// not under CV control
		sprintf(ch->chMenuParams[i+1], opStrings[ch->op_select[i]]);
	}
}

/*
 *	read a channel's non-volatile memory settings at the memory index i,
 *	unpack into the struct given by *ch, and write the variable strings
*/
void readChannelNVM(struct Channel *ch, uint8_t i) {
	uint8_t buffer[EEPROM_PAGE_SIZE];
	uint8_t addr = 0;
	uint8_t j;
	
	eeprom_emulator_read_page(i, buffer);

	// input settings x2
	// 1.threshold 2.invert 3.hysteresis
	// 4.threshold CV 5.invert CV 6.hysteresis CV
	for (j=0; j<2; j++) {
		uint16_t temp = 0;
		temp += buffer[j*SIZE_INPUT] << 8;
		temp += buffer[(j*SIZE_INPUT)+1];
		ch->input.input_settings[j].threshold = temp;
		ch->input.input_settings[j].invert = buffer[(j*SIZE_INPUT)+2];
		ch->input.input_settings[j].hysteresis = buffer[(j*SIZE_INPUT)+3];
		ch->input.input_settings[j].thresholdCv = buffer[(j*SIZE_INPUT)+4];
		ch->input.input_settings[j].invertCv = buffer[(j*SIZE_INPUT)+5];
		ch->input.input_settings[j].hysCv = buffer[(j*SIZE_INPUT)+6];
	}
	
	addr += SIZE_INPUT * 2;
	// input 2 copy input 1
	ch->input.copyIn1 = buffer[addr];
	addr++;
	
	// op settings x2
	// 1. operation 2.op CV
	for (j=0; j<2; j++) {
		ch->op_select[j] = buffer[addr+(j*SIZE_OP)];
		ch->op_cv[j] = buffer[addr+(j*SIZE_OP)+1];
	}
	
	addr += SIZE_OP * 2;
	// output settings x2
	// 1.probability 2.trig 3.trigLen 4.clkDiv 5.clkPhase 6.divRst 
	// 7.probability CV 8.trig CV 9.trigLen CV 10.clkDiv CV 11. clkPhase CV
	// 12. delay 13. delay CV
	for (j=0; j<2; j++) {
		uint16_t temp = 0;
		ch->out.output_settings[j].probability = buffer[addr+(j*SIZE_OUTPUT)];
		ch->out.output_settings[j].trig = buffer[addr+(j*SIZE_OUTPUT)+1];
		temp += buffer[addr+(j*SIZE_OUTPUT)+2] << 8;
		temp += buffer[addr+(j*SIZE_OUTPUT)+3];
		ch->out.output_settings[j].trigLen = temp;
		ch->out.output_settings[j].clkDiv = buffer[addr+(j*SIZE_OUTPUT)+4];
		ch->out.output_settings[j].clkPhase = buffer[addr+(j*SIZE_OUTPUT)+5];
		ch->out.output_settings[j].divRst = buffer[addr+(j*SIZE_OUTPUT)+6];
		ch->out.output_settings[j].probabilityCv = buffer[addr+(j*SIZE_OUTPUT)+7];
		ch->out.output_settings[j].trigCv = buffer[addr+(j*SIZE_OUTPUT)+8];
		ch->out.output_settings[j].trigLenCv = buffer[addr+(j*SIZE_OUTPUT)+9];
		ch->out.output_settings[j].clkDivCv = buffer[addr+(j*SIZE_OUTPUT)+10];
		ch->out.output_settings[j].clkPhaseCv = buffer[addr+(j*SIZE_OUTPUT)+11];
		temp = 0;
		temp += buffer[addr+(j*SIZE_OUTPUT)+12] << 8;
		temp += buffer[addr+(j*SIZE_OUTPUT)+13];
		ch->out.output_settings[j].delay = temp;
		ch->out.output_settings[j].delayCv = buffer[addr+(j*SIZE_OUTPUT)+14];
	}
	
	addr += SIZE_OUTPUT * 2;
	// output 2 mode
	ch->out.out2_settings = buffer[addr];
	
	// update string settings for display
	writeChannelStrings(ch);
	readChannelDefaultStates(ch);
	assignChannelStrings(ch);
}

/*
 *	pack up a struct's current settings and write them to non-volatile
 *	memory at the memory channel index i
*/
void writeChannelNVM(struct Channel *ch, uint8_t i) {
	uint8_t buffer[EEPROM_PAGE_SIZE] = {0};
	uint8_t addr = 0;
	uint8_t j;
	
	// input settings x2
	// 1.threshold 2.invert 3.hysteresis
	// 4.threshold CV 5.invert CV 6.hysteresis CV
	for (j=0; j<2; j++) {
		buffer[j*SIZE_INPUT] = (uint8_t)((ch->input.input_settings[j].threshold >> 8) & 0xFF);
		buffer[(j*SIZE_INPUT)+1] = (uint8_t)((ch->input.input_settings[j].threshold) & 0xFF);
		buffer[(j*SIZE_INPUT)+2] = (uint8_t)(ch->input.input_settings[j].invert);
		buffer[(j*SIZE_INPUT)+3] = ch->input.input_settings[j].hysteresis;
		buffer[(j*SIZE_INPUT)+4] = ch->input.input_settings[j].thresholdCv;
		buffer[(j*SIZE_INPUT)+5] = ch->input.input_settings[j].invertCv;
		buffer[(j*SIZE_INPUT)+6] = ch->input.input_settings[j].hysCv;
	}
	
	addr += SIZE_INPUT * 2;
	// input 2 copy input 1
	buffer[addr] = ch->input.copyIn1;
	addr++;
	
	// op settings x2
	// 1. operation 2.op CV
	for (j=0; j<2; j++) {
		buffer[addr+(j*SIZE_OP)] = ch->op_select[j];
		buffer[addr+(j*SIZE_OP)+1] = ch->op_cv[j];
	}
	
	addr += SIZE_OP * 2;
	// output settings x2
	// 1.probability 2.trig 3.trigLen 4.clkDiv 5.clkPhase 6.divRst
	for (j=0; j<2; j++) {
		buffer[addr+(j*SIZE_OUTPUT)] = ch->out.output_settings[j].probability;
		buffer[addr+(j*SIZE_OUTPUT)+1] = ch->out.output_settings[j].trig;
		buffer[addr+(j*SIZE_OUTPUT)+2] = (uint8_t)((ch->out.output_settings[j].trigLen >> 8) & 0xFF);
		buffer[addr+(j*SIZE_OUTPUT)+3] = (uint8_t)((ch->out.output_settings[j].trigLen) & 0xFF);
		buffer[addr+(j*SIZE_OUTPUT)+4] = ch->out.output_settings[j].clkDiv;
		buffer[addr+(j*SIZE_OUTPUT)+5] = ch->out.output_settings[j].clkPhase;
		buffer[addr+(j*SIZE_OUTPUT)+6] = ch->out.output_settings[j].divRst;
		buffer[addr+(j*SIZE_OUTPUT)+7] = ch->out.output_settings[j].probabilityCv;
		buffer[addr+(j*SIZE_OUTPUT)+8] = ch->out.output_settings[j].trigCv;
		buffer[addr+(j*SIZE_OUTPUT)+9] = ch->out.output_settings[j].trigLenCv;
		buffer[addr+(j*SIZE_OUTPUT)+10] = ch->out.output_settings[j].clkDivCv;
		buffer[addr+(j*SIZE_OUTPUT)+11] = ch->out.output_settings[j].clkPhaseCv;
		buffer[addr+(j*SIZE_OUTPUT)+12] = (uint8_t)((ch->out.output_settings[j].delay >> 8) & 0xFF);
		buffer[addr+(j*SIZE_OUTPUT)+13] = (uint8_t)((ch->out.output_settings[j].delay) & 0xFF);
		buffer[addr+(j*SIZE_OUTPUT)+14] = ch->out.output_settings[j].delayCv;
	}
	
	addr += SIZE_OUTPUT * 2;
	
	// output 2 mode
	buffer[addr] = ch->out.out2_settings;
	
	eeprom_emulator_write_page(i, buffer);
}

/*
 *	write the input, output, and channel strings based on the currently 
 *	loaded variables
*/
static inline void writeChannelStrings(struct Channel *ch) {
	uint8_t j;
	
	// write the channel input & output strings
	for (j=0; j<2; j++) {
		// input threshold
		if (ch->input.input_settings[j].thresholdCv != CV_NONE) {
			sprintf(ch->input.input_settings[j].thresholdStr, 
						cvStr[ch->input.input_settings[j].thresholdCv]);
		}
		else {	// not under CV control
			sprintf(ch->input.input_settings[j].thresholdStr, "%dmV", 
						ch->input.input_settings[j].threshold);
		}
		
		// input invert
		if (ch->input.input_settings[j].invertCv != CV_NONE) {
			sprintf(ch->input.input_settings[j].invertStr, 
						cvStr[ch->input.input_settings[j].invertCv]);
		}
		else {	// not under CV control
			sprintf(ch->input.input_settings[j].invertStr, 
						ch->input.input_settings[j].invert ? "true" : "false");
		}
		
		// input hysteresis
		if (ch->input.input_settings[j].hysCv != CV_NONE) {
			sprintf(ch->input.input_settings[j].hysStr,
						cvStr[ch->input.input_settings[j].hysCv]);
		}
		else {	// not under CV control
			sprintf(ch->input.input_settings[j].hysStr, "%d0mV",
						ch->input.input_settings[j].hysteresis);
		}
		
		// output probability
		if (ch->out.output_settings[j].probabilityCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].probabilityStr,
						cvStr[ch->out.output_settings[j].probabilityCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].probabilityStr, "%d%%",
						ch->out.output_settings[j].probability);
		}
		
		// output delay
		if (ch->out.output_settings[j].delayCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].delayStr, 
						cvStr[ch->out.output_settings[j].delayCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].delayStr, "%dms",
						ch->out.output_settings[j].delay);
		}
		
		// output trig
		if (ch->out.output_settings[j].trigCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].trigStr, 
						cvStr[ch->out.output_settings[j].trigCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].trigStr, 
						trigStrings[ch->out.output_settings[j].trig]);
		}
		
		// output trig length
		if (ch->out.output_settings[j].trigLenCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].trigLenStr,
						cvStr[ch->out.output_settings[j].trigLenCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].trigLenStr, "%dms",
						ch->out.output_settings[j].trigLen);
		}
		
		// output clk division
		if (ch->out.output_settings[j].clkDivCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].clkDivStr,
						cvStr[ch->out.output_settings[j].clkDivCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].clkDivStr, "/%d",
						ch->out.output_settings[j].clkDiv);
		}
		
		// output clk phase
		if (ch->out.output_settings[j].clkPhaseCv != CV_NONE) {
			sprintf(ch->out.output_settings[j].clkPhaseStr,
						cvStr[ch->out.output_settings[j].clkPhaseCv]);
		}
		else {	// not under CV control
			sprintf(ch->out.output_settings[j].clkPhaseStr, "%d",
						ch->out.output_settings[j].clkPhase);
		}
		
		// output clk reset (no CV control)
		sprintf(ch->out.output_settings[j].divRstStr, "%s",
					divRstString[ch->out.output_settings[j].divRst]);
	}
	
	// channel settings (in2, out2 & op)
	sprintf(ch->input.copyIn1Str, (ch->input.copyIn1) ? "Yes":"No");
	sprintf(ch->out.out2Str, out2Strings[ch->out.out2_settings]);
	if (ch->op_cv[0] != CV_NONE) {	// OP 1
		sprintf(ch->op1Str, cvStr[ch->op_cv[0]]);
	}
	else {	// not under CV control
		sprintf(ch->op1Str, opStrings[ch->op_select[0]]);
	}
	if (ch->op_cv[1] != CV_NONE) {	// OP 2
		sprintf(ch->op2Str, cvStr[ch->op_cv[1]]);
	}
	else {	// not under CV control
		sprintf(ch->op2Str, opStrings[ch->op_select[1]]);
	}
}

/*
 *	write the input, output, and channel default states based on the currently 
 *	loaded variables
*/
static inline void readChannelDefaultStates(struct Channel *ch) {
	ch->opDef = true;
	
	readInputDefaultStates(&ch->input);
	readOutputDefaultStates(&ch->out);
}

/*
 *	assigns string pointers for use by the menu/display functions
*/
static inline void assignChannelStrings(struct Channel * ch) 
{
	
	// initialize our arrays of string pointers to the strings just initialized
	// by the 'setXDefaults' functions
	ch->inputsMenuParams[0] = ch->input.input_settings[0].thresholdStr;
	ch->inputsMenuParams[1] = ch->input.input_settings[0].hysStr;
	ch->inputsMenuParams[2] = ch->input.input_settings[0].invertStr;
	ch->inputsMenuParams[3] = ch->input.copyIn1Str;
	ch->inputsMenuParams[4] = ch->input.input_settings[1].thresholdStr;
	ch->inputsMenuParams[5] = ch->input.input_settings[1].hysStr;
	ch->inputsMenuParams[6] = ch->input.input_settings[1].invertStr;
	
	ch->inputsMenuDefaults[0] = &ch->input.input_settings[0].thresholdDef;
	ch->inputsMenuDefaults[1] = &ch->input.input_settings[0].hysDef;
	ch->inputsMenuDefaults[2] = &ch->input.input_settings[0].invertDef;
	ch->inputsMenuDefaults[3] = &ch->input.copyIn1Def;
	ch->inputsMenuDefaults[4] = &ch->input.input_settings[1].thresholdDef;
	ch->inputsMenuDefaults[5] = &ch->input.input_settings[1].hysDef;
	ch->inputsMenuDefaults[6] = &ch->input.input_settings[1].invertDef;
	
	ch->chMenuParams[0] = submenuStr;
	ch->chMenuParams[1] = ch->op1Str;
	ch->chMenuParams[2] = ch->op2Str;
	ch->chMenuParams[3] = submenuStr;
	
	ch->chMenuDefaults[0] = &ch->opDef;
	ch->chMenuDefaults[1] = &ch->opDef;
	ch->chMenuDefaults[2] = &ch->opDef;
	ch->chMenuDefaults[3] = &ch->opDef;
	
	ch->outputsMenuParams[0] = ch->out.output_settings[0].clkDivStr;
	ch->outputsMenuParams[1] = ch->out.output_settings[0].clkPhaseStr;
	ch->outputsMenuParams[2] = ch->out.output_settings[0].divRstStr;
	ch->outputsMenuParams[3] = ch->out.output_settings[0].delayStr;
	ch->outputsMenuParams[4] = ch->out.output_settings[0].probabilityStr;
	ch->outputsMenuParams[5] = ch->out.output_settings[0].trigStr;
	ch->outputsMenuParams[6] = ch->out.output_settings[0].trigLenStr;
	ch->outputsMenuParams[7] = ch->out.out2Str;
	ch->outputsMenuParams[8] = ch->out.output_settings[1].clkDivStr;
	ch->outputsMenuParams[9] = ch->out.output_settings[1].clkPhaseStr;
	ch->outputsMenuParams[10] = ch->out.output_settings[1].divRstStr;
	ch->outputsMenuParams[11] = ch->out.output_settings[1].delayStr;
	ch->outputsMenuParams[12] = ch->out.output_settings[1].probabilityStr;
	ch->outputsMenuParams[13] = ch->out.output_settings[1].trigStr;
	ch->outputsMenuParams[14] = ch->out.output_settings[1].trigLenStr;
	
	ch->outputsMenuDefaults[0] = &ch->out.output_settings[0].clkDivDef;
	ch->outputsMenuDefaults[1] = &ch->out.output_settings[0].clkPhaseDef;
	ch->outputsMenuDefaults[2] = &ch->out.output_settings[0].divRstDef;
	ch->outputsMenuDefaults[3] = &ch->out.output_settings[0].delayDef;
	ch->outputsMenuDefaults[4] = &ch->out.output_settings[0].probabilityDef;
	ch->outputsMenuDefaults[5] = &ch->out.output_settings[0].trigDef;
	ch->outputsMenuDefaults[6] = &ch->out.output_settings[0].trigLenDef;
	ch->outputsMenuDefaults[7] = &ch->out.out2Def;
	ch->outputsMenuDefaults[8] = &ch->out.output_settings[1].clkDivDef;
	ch->outputsMenuDefaults[9] = &ch->out.output_settings[1].clkPhaseDef;
	ch->outputsMenuDefaults[10] = &ch->out.output_settings[1].divRstDef;
	ch->outputsMenuDefaults[11] = &ch->out.output_settings[1].delayDef;
	ch->outputsMenuDefaults[12] = &ch->out.output_settings[1].probabilityDef;
	ch->outputsMenuDefaults[13] = &ch->out.output_settings[1].trigDef;
	ch->outputsMenuDefaults[14] = &ch->out.output_settings[1].trigLenDef;
}