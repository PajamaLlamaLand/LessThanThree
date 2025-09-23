/*
 * source file for operation definitions
 */ 

#include "operations.h"

bool op_AND(bool a, bool b, bool prev, bool i) {return a&b;}
bool op_NAND(bool a, bool b, bool prev, bool i) {return !(a&b);}
bool op_OR(bool a, bool b, bool prev, bool i) {return a||b;}
bool op_NOR(bool a, bool b, bool prev, bool i) {return !(a||b);}
bool op_XOR(bool a, bool b, bool prev, bool i) {return a!=b;}
bool op_XNOR(bool a, bool b, bool prev, bool i) {return a==b;}

/*
 *	S-R latch
 *	No change if both S=R active
*/
bool op_SR(bool a, bool b, bool prev, bool i) {
	// set
	if (a&!b) {
		return true;
	}
	// reset
	else if (!a & b) {
		return false;
	}
	else {
		return prev;
	}
}

/*
 *	D-latch
*/
bool op_D(bool a, bool b, bool prev, bool i) {
	if (b) {
		return a;
	}
	else {
		return prev;
	}
}

/*
 *	Bypass- this is the only function right now to make use
 *	        of the 'num' parameter indicating if the op is
 *          the first or second in the channel
*/
bool op_BYP(bool a, bool b, bool prev, bool i) {
	return i ? b : a;
}