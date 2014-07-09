/*
 * common.h
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 * Description:
 * 		define basic C style functions and types.
 */

#ifndef COMMON_H_
#define COMMON_H_
#include "defines.h"

extern "C" {

extern int __data_start;
extern int _end;

extern char _etext;
extern char _edata;

}

// Macros that define the start and end addresses of program-wide globals.

#define GLOBALS_START  PAGE_ALIGN_DOWN((size_t) &__data_start)
#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) &_end - 1))
#define GLOBALS_SIZE   (GLOBALS_END - GLOBALS_START)

#include <iostream>
using namespace std;


class ThreadObject {
public:
	virtual int initOnThreadEntry() = 0;
	virtual int initOnMainThreadEntry(){
		return 0;
	}
};


/**
 * External wrapped functions used by .h files.
 */

int CurrThreadID();
bool IsSingleThread();


typedef int32_t thread_id_t;
typedef uintptr_t address_t;

#endif /* COMMON_H_ */
