/*
 * globals.h
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 */

#ifndef COMMON_H_
#define COMMON_H_
#include "defines.h"
#include "utils.h"
extern "C" {

extern int __data_start;
extern int _end;

extern char _etext;
extern char _edata;

}

// Macros to align to the nearest page down and up, respectively.
//#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~PAGE_SIZE_MASK)
//#define PAGE_ALIGN_UP(x) ((((size_t) (x)) + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK)

// Macros that define the start and end addresses of program-wide globals.

#define GLOBALS_START  PAGE_ALIGN_DOWN((size_t) &__data_start)
#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) &_end - 1))


#define GLOBALS_SIZE   (GLOBALS_END - GLOBALS_START)

#include <iostream>
using namespace std;


/**
 * External functions exported by hb_globals.cpp
 */




int CurrThreadID();
bool IsSingleThread();


class ThreadObject{
public:
	virtual int initOnThreadEntry() = 0;
	virtual int initOnMainThreadEntry(){
		return 0;
	}
};




typedef int32_t thread_id_t;
typedef uintptr_t address_t;

#endif /* COMMON_H_ */
