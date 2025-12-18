/*
 * globalSettings.h
 *
 * Created: 4/25/2024 7:22:28 PM
 *  Author: tay
 */ 


#ifndef GLOBALSETTINGS_H_
#define GLOBALSETTINGS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "conf_menu.h"	// for parameter string max char limit
#include "cv.h"
#include "eeprom.h"
#include "ui.h"
#include "sysfont.h"

enum ChannelReset {
	RESET_NONE,
	RESET_CH1,
	RESET_CH2,
	RESET_ALL
	};
	
enum ScreenSaverTime {
	SCREENSAVER_5MIN,
	SCREENSAVER_15MIN,
	SCREENSAVER_OFF
	};

struct GlobalSettings {
	uint8_t chReset;			// holds current display when selecting a channel to reset
	uint8_t longPressTime;		// holds current long press count time based on enum
	uint8_t screenSaverTime;	// holds current screen saver timeout value
	
	char chResetStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char longPressTimeStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char screenSaverTimeStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	bool globalDef;
	
	char *globalSettingsParams[6];	// stores pointers to param strings used by UI
	bool *globalSettingsDefaults[6]; // stores 'default' states for params, used by menu.c
	};

struct GlobalSettings globalSettings;

void setGlobalSettingsDefaults(struct GlobalSettings *settings, struct Cv *cv);

void updateChReset(struct GlobalSettings *settings, bool inc);
void updateLongPressTime(struct GlobalSettings *settings, bool inc);
void updateScreenSaverTime(struct GlobalSettings *settings, bool inc);
void writeGlobalStrings(struct GlobalSettings *global, struct Cv *cv); 

/*
 *	non-volatile memory storage and retrieval methods
*/
void readGlobalSettingsNVM(struct GlobalSettings *settings, struct Cv *cv);
void writeGlobalSettingsNVM(struct GlobalSettings *settings, struct Cv *cv);

#endif /* GLOBALSETTINGS_H_ */