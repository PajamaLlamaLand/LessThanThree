/*
 *	
 */ 

#include "globalSettings.h"

#define LONG_PRESS_COUNT_DEFAULT	LONG_PRESS_MED
#define	SCREENSAVER_DEFAULT			SCREENSAVER_5MIN

// byte sizes for packing/unpacking data to/from NVM
#define SIZE_CV		3
#define SIZE_GLOBAL	2

const char *chResetStrings[] = {"None", "CH1", "CH2", "ALL"};
const char *longPressStrings[] = {"Short", "Med", "Long"};
const char *screenSaverTimeStrings[] = {"5mins", "15mins", "Off"};
char globalSubmenuStr[3] = "->";	// parameter display 'value' for submenu
static const char *cvRangeStrings[] = {"+/-8V", "+8V", "+/-5V", "+5V"};

// helper function declaration
static inline void assignGlobalStrings(struct GlobalSettings *global, struct Cv *cv);

/*
 *	set all global settings defaults
*/
void setGlobalSettingsDefaults(struct GlobalSettings *settings, struct Cv *cv) {
	settings->chReset = RESET_NONE;
	settings->longPressTime = LONG_PRESS_COUNT_DEFAULT;
	settings->screenSaverTime = SCREENSAVER_DEFAULT;
	
	// write the default long press time to the UI struct instance
	writeLongPressTimes(settings->longPressTime);
	
	// since CV settings are paired in NVM with the global settings, we'll 
	// also initialize them here
	setCvDefaults(cv);
	
	writeGlobalStrings(settings, cv);
	assignGlobalStrings(settings, cv);
	
	// write all of our changes to non-volatile
	writeGlobalSettingsNVM(settings, cv);
}

/*
 *	increment/decrement the current channel reset value
*/
void updateChReset(struct GlobalSettings *settings, bool inc) {
	if (inc) {
		settings->chReset += 1;
		if (settings->chReset > RESET_ALL) {	// overflow check
			settings->chReset = RESET_NONE;
		}
	}
	else {	// decrementing
		settings->chReset -= 1;
		if (settings->chReset > RESET_ALL) {	// underflow check for uint8_t
			settings->chReset = RESET_ALL;
		}
	}
	
	sprintf(settings->chResetStr, chResetStrings[settings->chReset]);
}

/*
 *	increment/decrement the current long press time, using the
 *	times defined in ui.h
*/
void updateLongPressTime(struct GlobalSettings *settings, bool inc) {
	if (inc) {
		settings->longPressTime += 1;
		if (settings->longPressTime > LONG_PRESS_LONG) {	// overflow check
			settings->longPressTime = LONG_PRESS_SHORT;
		}
	}
	else {	// decrementing
		settings->longPressTime -= 1;
		if (settings->longPressTime > LONG_PRESS_LONG) {	// underflow check for uint8_t
			settings->longPressTime = LONG_PRESS_LONG;
		}
	}
	
	writeLongPressTimes(settings->longPressTime);
	
	sprintf(settings->longPressTimeStr, longPressStrings[settings->longPressTime]);
}

/*
 *	increment/decrement the current screen saver time
*/
void updateScreenSaverTime(struct GlobalSettings *settings, bool inc) {
	if (inc) {
		settings->screenSaverTime += 1;
		if (settings->screenSaverTime > SCREENSAVER_OFF) {
			settings->screenSaverTime = SCREENSAVER_5MIN;
		}
	}
	else {	// decrementing
		settings->screenSaverTime -= 1;
		if (settings->screenSaverTime > SCREENSAVER_OFF) {
			settings->screenSaverTime = SCREENSAVER_OFF;
		}
	}
	
	sprintf(settings->screenSaverTimeStr, screenSaverTimeStrings[settings->screenSaverTime]);
}

/*
 *	read the global and CV settings stored in non-volatile memory, and 
 *	unpack into their respective working memory structs
*/
void readGlobalSettingsNVM(struct GlobalSettings *global, struct Cv *cv) {
	uint8_t buffer[EEPROM_PAGE_SIZE];
	uint8_t i;
	
	eeprom_emulator_read_page(2, buffer);

	// CV settings x2
	for (i=0; i<2; i++) {
		uint16_t temp = 0;
		cv->settings[i].range = buffer[i*3];
		temp += buffer[(i*3)+1] << 8;
		temp += buffer[(i*3)+2]; 
		cv->settings[i].threshold = temp;
	}
	
	// Global settings
	global->longPressTime = buffer[6];
	global->screenSaverTime = buffer[7];
	
	writeGlobalStrings(global, cv);
	assignGlobalStrings(global, cv);
}

/*
 *	pack up a GlobalSettings and 2 CvSettings structs and write them to 
 *	non-volatile memory. Will always write to logical page 3 of NVM
*/
void writeGlobalSettingsNVM(struct GlobalSettings *global, struct Cv *cv) {
	uint8_t buffer[EEPROM_PAGE_SIZE] = {0};
	uint8_t i;

	// CV settings x2
	for (i=0; i<2; i++) {
		buffer[i*3] = cv->settings[i].range;
		buffer[(i*3)+1] = (uint8_t)((cv->settings[i].threshold >> 8) & 0xFF);
		buffer[(i*3)+2] = (uint8_t)((cv->settings[i].threshold) & 0xFF);
	}
	
	// Global settings
	buffer[6] = global->longPressTime;
	buffer[7] = global->screenSaverTime;
	
	eeprom_emulator_write_page(2, buffer);
}

/*
 *	write the CV and global settings strings based on the currently
 *	loaded values
*/
void writeGlobalStrings(struct GlobalSettings *global, struct Cv *cv) {
	// CV settings
	for (uint8_t i=0; i<2; i++) {
		sprintf(cv->settings[i].rangeStr, cvRangeStrings[cv->settings[i].range]);
		sprintf(cv->settings[i].thresholdStr, "%dmV", cv->settings[i].threshold);
	}
	
	// Global settings
	sprintf(global->chResetStr, chResetStrings[global->chReset]);
	sprintf(global->longPressTimeStr, longPressStrings[global->longPressTime]);
	sprintf(global->screenSaverTimeStr, screenSaverTimeStrings[global->screenSaverTime]);
}

/*
 *	assigns string pointers for use by the menu/display functions
*/
static inline void assignGlobalStrings(struct GlobalSettings *global, struct Cv *cv) {
	// global settings
	global->globalSettingsParams[0] = globalSubmenuStr;
	global->globalSettingsParams[1] = globalSubmenuStr;
	global->globalSettingsParams[2] = globalSubmenuStr;
	global->globalSettingsParams[3] = global->chResetStr;
	global->globalSettingsParams[4] = global->longPressTimeStr;
	global->globalSettingsParams[5] = global->screenSaverTimeStr;
	
	// CV settings
	cv->cvParams[0] = cv->settings[0].rangeStr;
	cv->cvParams[1] = cv->settings[0].thresholdStr;
	cv->cvParams[2] = cv->settings[1].rangeStr;
	cv->cvParams[3] = cv->settings[1].thresholdStr;
}