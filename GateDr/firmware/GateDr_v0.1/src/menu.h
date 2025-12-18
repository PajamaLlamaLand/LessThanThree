/*
 * data structures and methods for processing menu interactions
 */ 


#ifndef MENU_H_
#define MENU_H_

#include <stdio.h>
#include <ssd1306.h>
#include "gfx_mono_menu.h"
#include "rtc_count.h"
#include "inputs.h"
#include "channel.h"
#include "cv.h"
#include "globalSettings.h"

enum Menus {
	MENU_GLOBAL,
	MENU_CHANNEL_1,
	MENU_INPUTS_1,
	MENU_OUTPUTS_1,
	MENU_CHANNEL_2,
	MENU_INPUTS_2,
	MENU_OUTPUTS_2,
	MENU_CV
	};

// used by UI functions to set menu action flags
enum MenuAction {
	ACTION_NONE,
	ACTION_INC,
	ACTION_DEC,
	ACTION_ENTER,
	ACTION_BACK,
	ACTION_CH1,
	ACTION_CH2,
	ACTION_GLOBAL,
	ACTION_CV
	};

struct Menu {
	uint8_t currentMenu;		// index for the current menu, per Menus enum
	uint8_t currentChannel;		// keeps track of the current channel context
	
	uint8_t actionFlag;			// flag to be checked in main() loop to process menu actions
	int8_t enc_count;			// increments or decrements based on encoder pin logic,
								// reset by processMenuAction()
	uint32_t *rtcCurrentCount;	// pointer to the current RTC count (updated on processing
								// loop start)
	uint32_t lastInput;			// keeps track of time since most recent action
	bool screenSaved;			// true when no input for *screenSaverTime* minutes
	
	struct tc_module *tc;		// 10kHz TC module for drawing to screen 
	};

struct gfx_mono_menu globalMenu;
struct gfx_mono_menu channel1Menu;
struct gfx_mono_menu inputs1Menu;
struct gfx_mono_menu outputs1Menu;
struct gfx_mono_menu channel2Menu;
struct gfx_mono_menu inputs2Menu;
struct gfx_mono_menu outputs2Menu;
struct gfx_mono_menu cvMenu;

struct Menu menu;

void menuInit(uint32_t *currentCount, const char *version);
void screenDrawInit(struct tc_module *tc_instance);
void screenDrawCallback(struct tc_module *const tc_instance);

void setMenuFlag(uint8_t flag);
void processMenuAction(void);

void processActionNone(void);
void processActionInc(void);
void processActionDec(void);
void processActionEnter(void);
void processActionBack(void);
void processActionCh1(void);
void processActionCh2(void);
void processActionGlobal(void);
void processActionCv(void);
void checkReset(void);

// generalized pointer to a menu process function to be used in conjunction
// with the action flags
typedef void (*processAction)(void);

static const processAction action_table[9] = {processActionNone, processActionInc, processActionDec,
					processActionEnter, processActionBack, processActionCh1, 
					processActionCh2, processActionGlobal, processActionCv};

#endif /* MENU_H_ */