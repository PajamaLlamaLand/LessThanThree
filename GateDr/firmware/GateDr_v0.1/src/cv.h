/*
 * data structures and methods for processing CV inputs
 */ 


#ifndef CV_H_
#define CV_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "conf_menu.h"	// for parameter string max char limit

enum CvSel {
	CV_NONE,
	CV1,
	CV2
	};
	
enum CvRange {
	BI_8,
	UNI_8,
	BI_5,
	UNI_5
	};

struct CvSettings {
	uint8_t range;		// CV range as per above enum
	int16_t threshold;	// adjustable threshold for binary CV destinations in mV
	
	// mutable strings for printing values to display
	char rangeStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	char thresholdStr[GFX_MONO_MENU_PARAM_MAX_CHAR];
	};

struct Cv {
	int16_t value[2];				// holds the current CV values in mV
	struct CvSettings settings[2];	// settings per CV input
	char *cvParams[4];				// stores pointers to param strings used by UI
	};

struct Cv cv_instance;

void setCvDefaults(struct Cv *cv);
uint8_t normalizeCvUint8(struct Cv *cv, uint8_t sel, uint8_t lowLimit, uint8_t highLimit, uint8_t prev);
uint16_t normalizeCvUint16(struct Cv *cv, uint8_t sel, uint16_t lowLimit, uint16_t highLimit, uint16_t prev);
bool normalizeCvBool(struct Cv *cv, uint8_t sel, bool prev);

/*
 *	functions for UI callbacks during menu interactions
*/
void updateCvRange(struct CvSettings *settings, bool inc);
void updateCvThreshold(struct CvSettings *settings, bool inc);

#endif /* CV_H_ */