/*
 * data structures and methods for processing each channel
 */ 


#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>		// for memcpy()
#include "conf_menu.h"	// for parameter string max char limit
#include "eeprom.h"		// for NVM reading/writing
#include "inputs.h"
#include "operations.h"
#include "outputs.h"
#include "cv.h"
#include "paramUtils.h"

struct Channel {
	struct Input input;
	struct Output out;
	uint8_t op_select[2];
	uint8_t op_cv[2];
	
	// previous CV conversion values used for hysteresis when under CV selection
	uint8_t cv_op_prev[2];
	
	char op1Str[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char op2Str[GFX_MONO_MENU_PARAM_MAX_CHAR];
	bool opDef;
	
	char *inputsMenuParams[7];		// stores pointers to param strings used by menu
	bool *inputsMenuDefaults[7];		// stores 'default' state for inputs menu params used by menu.c
	char *chMenuParams[4];			// includes blanks / '->' for submenus
	bool *chMenuDefaults[4];
	char *outputsMenuParams[15];
	bool *outputsMenuDefaults[15];
	};

// channel instance(s)
struct Channel chan[2];

void initChannel(struct Channel *ch, uint32_t *currentCount, uint8_t num);
void setChannelDefaults(struct Channel *ch, uint8_t num);
void processChannel(struct Channel *ch, int16_t in1, int16_t in2, struct Cv *cv);

/*
 *	functions for UI callbacks during menu interactions
*/
void updateOp(struct Channel *ch, bool i, bool inc);

/*
 *	non-volatile memory storage and retrieval methods
*/
void readChannelNVM(struct Channel *ch, uint8_t i);
void writeChannelNVM(struct Channel *ch, uint8_t i);

#endif /* CHANNEL_H_ */