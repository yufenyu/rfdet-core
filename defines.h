/*
 * hbdefines.h
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 */

#ifndef HBDEFINES_H_
#define HBDEFINES_H_

#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MAX_THREAD_NUM 32

#define METADATA_MMP_SPACE_SIZE ((512) * 1024 * 1024)
#define STACK_SIZE (1024 * 1024)
#define HEAP_SIZE (1048576UL * (1024))
#define HEAP_CHUNK_SIZE (1048576UL)
#define CHUNK_SIZE (1048576UL * 8) //unused


/* The page size definitions!*/
#define LOG_PAGE_SIZE 12
#define PAGE_SIZE (1UL << LOG_PAGE_SIZE)
#define PAGE_MASK__ (PAGE_SIZE - 1)
#define __PAGE_MASK (~(PAGE_SIZE - 1))

#define _PROFILING

/////////////////////////////Debugging///////////////////////////////////
#define DEBUG_MSG_LEVEL 3
#define NORMAL_MSG_LEVEL 2
#define WARNING_MSG_LEVEL 1
#define VATAL_MSG_LEVEL 0
#define NO_MSG_LEVEL (-1)
#define HB_VERBOSE_LEVEL 1


#define _GDB_DEBUG
//#define RUNTIME_SELF_CHECK
//#define _OPTIMIZATION
#ifdef RUNTIME_SELF_CHECK
#define OPEN_ASSERT
#endif

/////////////////////////////Configurations//////////////////////////////
#define MODIFICATION DiffModification

#define THREAD_ID_START 0
#define INVALID_THREAD_ID (-1)
#define INVALID_PAGE_VERSION (-1)


#define	_GRANULARITY_BYTE_SIZE 1
#define	_GRANULARITY_INT_SIZE 2
#define DIFF_GRANULARITY_CONFIG _GRANULARITY_INT_SIZE


//////////////////////////////GC Policy//////////////////////////////
#define GC_IN_CRITICAL_SECTION  /*Should we permit GC at critical section. 'No' may improve performance.*/

//////////////////////////////Slice Pointer Size/////////////////////////////
#define MAX_SLICE_POINTER_NUM (4096 * 16) /*EntrySlot is used to store pointers of log entry.*/



#endif /* HBDEFINES_H_ */
