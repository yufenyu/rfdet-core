/*
 * writeset.cpp
 * This file is used to monitor modifications (write set) of user applications
 *  Created on: Nov 19, 2012
 *      Author: zhouxu
 */
#include <string.h>
#include "writeset.h"
#include "defines.h"
#include "runtime.h"
#include "common.h"
#include "heaps.h"



AddressMap writeSet;
