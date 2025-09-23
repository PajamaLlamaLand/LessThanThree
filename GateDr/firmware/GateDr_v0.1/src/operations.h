/*
 * data structures and methods for main channel operations
 */ 


#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include <stdbool.h>
#include <stdint.h>

enum Operations {
	OP_AND,
	OP_NAND,
	OP_OR,
	OP_NOR,
	OP_XOR,
	OP_XNOR,
	OP_SR,
	OP_D,
	OP_BYP
	};

/*
 *	here is a generalized pointer to a function of return type bool
 *	and four boolean args to be used as a Channel struct member
*/
typedef bool (*op_FN)(bool a, bool b, bool prev, bool i);

bool op_AND(bool a, bool b, bool prev, bool i);
bool op_NAND(bool a, bool b, bool prev, bool i);
bool op_OR(bool a, bool b, bool prev, bool i);
bool op_NOR(bool a, bool b, bool prev, bool i);
bool op_XOR(bool a, bool b, bool prev, bool i);
bool op_XNOR(bool a, bool b, bool prev, bool i);
bool op_SR(bool a, bool b, bool prev, bool i);
bool op_D(bool a, bool b, bool prev, bool i);
bool op_BYP(bool a, bool b, bool prev, bool i);

// operations table to be used when channel is under CV control
static const op_FN op_table[9] = {op_AND, op_NAND, op_OR, op_NOR, op_XOR, op_XNOR,
	 op_SR, op_D, op_BYP};

#endif /* OPERATIONS_H_ */