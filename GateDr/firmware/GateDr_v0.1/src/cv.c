/*
 * source file for CV input processing
 */ 

#include "cv.h"

#define CV_RANGE_DEFAULT	UNI_5
#define CV_THRESH_DEFAULT	1000
#define CV_THRESH_MIN		-8000
#define CV_THRESH_MAX		8000
#define CV_THRESH_INC		200
#define CV_HYS				0.15f

// declaration for static inline helper function
static inline float getCvPercent(int16_t x, uint8_t range);

// ADC ranges that correspond with the CvRange enums to help with CV conversions
const int16_t adcRanges[4][2] = {{-8000,8000}, {0, 8000}, {-5000,5000}, {0,5000}};
	
// strings to store enum parameters for display
static const char *cvRangeStrings[] = {"+/-8V", "+8V", "+/-5V", "+5V"};

/*
 *	set all CV settings to their defaults
*/
void setCvDefaults(struct Cv *cv) {
	for(uint8_t i = 0; i < 2; i++) {
		cv->settings[i].range =			CV_RANGE_DEFAULT;
		cv->settings[i].threshold =		CV_THRESH_DEFAULT;
		sprintf(cv->settings[i].rangeStr, "+5V");
		sprintf(cv->settings[i].thresholdStr, "%d", CV_THRESH_DEFAULT);
		cv->settings[i].rangeDef =		true;
		cv->settings[i].thresholdDef =	true;
	}
}

/*
 *	utility function to clamp the CV value within the range given and return it as
 *	a percent between 0-1
*/
static inline float getCvPercent(int16_t x, uint8_t range) {
	float percent;
	
	// clamp CV within range
	if (x > adcRanges[range][1]) {
		x = adcRanges[range][1];
	}
	else if (x < adcRanges[range][0]) {
		x = adcRanges[range][0];
	}
	
	x -= adcRanges[range][0];
	
	percent = (float)x / (float)(adcRanges[range][1] - adcRanges[range][0]);
	
	return percent;
}

/*
 *	determine the uint8_t value for a parameter for a given CV value and selection,
 *	as well as the low limit, high limit, and step size for the target parameter
*/
uint8_t normalizeCvUint8(struct Cv *cv, uint8_t sel, uint8_t lowLimit, uint8_t highLimit, uint8_t prev) {
	float percent;
	float hys;
	uint8_t targetValue;
	
	if (sel != CV1 && sel != CV2) {
		return 0;
	}
	
	percent = getCvPercent(cv->value[sel-1], cv->settings[sel-1].range);
	
	percent *= (float)(highLimit - lowLimit);
	percent += (float)lowLimit;
	
	hys = percent >= (float)prev ? -CV_HYS : CV_HYS;
	percent += hys + 0.5f;
	
	targetValue = (uint8_t)(percent);
	
	return targetValue > highLimit ? highLimit : targetValue;
}

/*
 *	determine the uint16_t value for a parameter for a given CV value and selection,
 *	as well as the low limit, high limit, and step size for the target parameter
*/
uint16_t normalizeCvUint16(struct Cv *cv, uint8_t sel, uint16_t lowLimit, uint16_t highLimit, uint16_t prev) {
	float percent;
	float hys;
	uint16_t targetValue;
	
	if (sel != CV1 && sel != CV2) {
		return 0;
	}
	
	percent = getCvPercent(cv->value[sel-1], cv->settings[sel-1].range);
	
	percent *= (float)(highLimit - lowLimit);
	percent += (float)lowLimit;
	
	hys = percent >= (float)prev ? -CV_HYS : CV_HYS;
	percent += hys + 0.5f;
	
	targetValue = (uint16_t)(percent);
	
	return targetValue > highLimit ? highLimit : targetValue;
}

/*
 *	determine boolean value for a parameter given the current CV value and 
 *	threshold parameter
*/
bool normalizeCvBool(struct Cv *cv, uint8_t sel, bool prev) {
	int16_t thresh;
	
	thresh = cv->settings[sel-1].threshold;
	
	thresh += prev ? -50 : 50;
	
	if (sel != CV_NONE && sel <= CV2) {	// selection value check
		return (cv->value[sel-1] > thresh); 
	}
	else {	// return false just in case this gets called with invalid CV selected
		return false;
	}
}

/*
 *	increment/decrement the CV range parameter & update
 *	the parameter string, used by menu functions
*/
void updateCvRange(struct CvSettings *settings, bool inc) {
	if (inc) {
		settings->range += 1;
		if (settings->range > UNI_5) {	// overflow check
			settings->range = BI_8;
		}
	}
	else {	// decrementing
		settings->range -= 1;
		if (settings->range > UNI_5) {	// underflow check for uint8_t
			settings->range = UNI_5;
		}
	}
	
	sprintf(settings->rangeStr, "%s", cvRangeStrings[settings->range]);
	
	settings->rangeDef = (settings->range == CV_RANGE_DEFAULT);
}

/*
 *	increment/decrement the CV threshold parameter & update
 *	the parameter string, used by menu functions
*/
void updateCvThreshold(struct CvSettings *settings, bool inc) {
	if (inc) {
		settings->threshold += CV_THRESH_INC;
		if (settings->threshold > CV_THRESH_MAX) {	// overflow check
			settings->threshold = CV_THRESH_MIN;
		}
	}
	else {	// decrementing
		settings->threshold -= CV_THRESH_INC;
		if (settings->threshold < CV_THRESH_MIN) {	// underflow check
			settings->threshold = CV_THRESH_MAX;
		}
	}
	
	sprintf(settings->thresholdStr, "%dmV", settings->threshold);
	
	settings->thresholdDef = (settings->threshold == CV_THRESH_DEFAULT);
}

/*
 *	sets isDefault states for all settings within an Output struct
*/
void readCVDefaultStates(struct Cv *cv) {
	for (uint8_t i=0; i<2; i++) {
		// CV range
		cv->settings[i].rangeDef = (cv->settings[i].range == CV_RANGE_DEFAULT);
		
		// CV threshold
		cv->settings[i].thresholdDef = (cv->settings[i].threshold == CV_THRESH_DEFAULT);
	}
}