/*
 * Utility functions for parameter update functions
 */ 

#include "paramUtils.h"

/*
 *	boolean 
*/
void updateBool(bool *param, uint8_t *cv, bool inc) {
	switch (*cv) {
		case CV_NONE:
			if ((*param)==true && inc) {
				*cv = CV1;
			}
			else if ((*param)==false && !inc) {
				*cv = CV2;
			}
			else {
				*param = !(*param);
			}
			break;
		case CV1:
			if (inc) {
				*cv = CV2;
			}
			else {	// decrementing
				*cv = CV_NONE;
				*param = true;
			}
			break;
		case CV2:
			if (inc) {
				*cv = CV_NONE;
				*param = false;
			}
			else {	// decrementing
				*cv = CV1;
			}
			break;
	}
}

/*
 *	uint8_t
*/
void updateUint8t(uint8_t *param, uint8_t *cv, bool inc, uint8_t min, uint8_t max, uint8_t step) {
	switch (*cv) {
		case CV_NONE:
			if (inc) {
				*param += step;
				
				if (*param > max) {	// overflow check
					*cv = CV1;
				}
			}
			else {	// decrementing
				*param -= step;
				
				if ((*param < min) || (*param > max)) {	// underflow check 
					*cv = CV2;
				}
			}
			break;
		case CV1:
			if (inc) {
				*cv = CV2;
			}
			else {	// decrementing
				*cv = CV_NONE;
				*param = max;
			}
			break;
		case CV2:
			if (inc) {
				*cv = CV_NONE;
				*param = min;
			}
			else {	// decrementing
				*cv = CV1;
			}
			break;
	}
}

/*
 *	uint16_t
*/
void updateUint16t(uint16_t *param, uint8_t *cv, bool inc, uint16_t min, uint16_t max, uint16_t step) {
		switch (*cv) {
			case CV_NONE:
			if (inc) {
				*param += step;
				
				if (*param > max) {	// overflow check
					*cv = CV1;
				}
			}
			else {	// decrementing
				*param -= step;
				
				if ((*param < min) || (*param > max)) {	// underflow check
					*cv = CV2;
				}
			}
			break;
			case CV1:
			if (inc) {
				*cv = CV2;
			}
			else {	// decrementing
				*cv = CV_NONE;
				*param = max;
			}
			break;
			case CV2:
			if (inc) {
				*cv = CV_NONE;
				*param = min;
			}
			else {	// decrementing
				*cv = CV1;
			}
			break;
		}
}

/*
 *	int16_t
*/
void updateInt16t(int16_t *param, uint8_t *cv, bool inc, int16_t min, int16_t max, int16_t step) {
		switch (*cv) {
			case CV_NONE:
			if (inc) {
				*param += step;
				
				if (*param > max) {	// overflow check
					*cv = CV1;
				}
			}
			else {	// decrementing
				*param -= step;
				
				if (*param < min) {	// underflow check
					*cv = CV2;
				}
			}
			break;
			case CV1:
			if (inc) {
				*cv = CV2;
			}
			else {	// decrementing
				*cv = CV_NONE;
				*param = max;
			}
			break;
			case CV2:
			if (inc) {
				*cv = CV_NONE;
				*param = min;
			}
			else {	// decrementing
				*cv = CV1;
			}
			break;
		}
}
