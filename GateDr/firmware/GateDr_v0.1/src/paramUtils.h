/*
 *	Utility functions for parameter update functions
 */ 


#ifndef PARAMUTILS_H_
#define PARAMUTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include "cv.h"		// for CV enums

void updateBool(bool *param, uint8_t *cv, bool inc);
void updateUint8t(uint8_t *param, uint8_t *cv, bool inc, uint8_t min, uint8_t max, uint8_t step);
void updateUint16t(uint16_t *param, uint8_t *cv, bool inc, uint16_t min, uint16_t max, uint16_t step);
void updateInt16t(int16_t *param, uint8_t *cv, bool inc, int16_t min, int16_t max, int16_t step);

#endif /* PARAMUTILS_H_ */