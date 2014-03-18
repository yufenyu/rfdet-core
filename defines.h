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
//#define META_DATA_SIZE (64 * 1024 * 1024) //4MB the major usage is for log entries.
//#define LOG_HEAP_SIZE (128 * 1024 * 1024) //68MB
//#define GC_LIMIT_LOG_HEAP (64 * 1024 * 1024)

#define METADATA_MMP_SPACE_SIZE ((512) * 1024 * 1024)
//#define GC_LIMIT_SNAPSHOT_HEAP (128 * 1024 * 1024)

/* The page size definitions!*/
#define LOG_PAGE_SIZE 12
#define PAGE_SIZE (1UL << LOG_PAGE_SIZE)
#define PAGE_MASK__ (PAGE_SIZE - 1)
#define __PAGE_MASK (~(PAGE_SIZE - 1))

/////////////////////////////Debugging///////////////////////////////////
#define DEBUG_MSG_LEVEL 3
#define NORMAL_MSG_LEVEL 2
#define WARNING_MSG_LEVEL 1
#define VATAL_MSG_LEVEL 0
#define NO_MSG_LEVEL (-1)
#define HB_VERBOSE_LEVEL 0

#define _PROFILING
//#define _DEBUG

/////////////////////////////Configurations//////////////////////////////
#define MODIFICATION DiffModification
//namespace configs{
#define USING_INTERNAL_LOCK
#define THREAD_ID_START 0
#define INVALID_THREAD_ID (-1)
#define INVALID_PAGE_VERSION (-1)

#define USING_HBDET

#ifdef USING_HBDET
#define ENABLE_PTHREAD_HOOK
#define ENABLE_MALLOC_HOOK
#endif

#define MULTI_ADDRESS_SPACE
//#define INT_SIZE_DIFF
//#define BYTE_SIZE_DIFF

//#define SOLO_COPYONWRITE

#ifndef SOLO_COPYONWRITE
#define RUNTIME HBRuntime
#else
#define RUNTIME ShadowRuntime
#endif

//#define USING_KENDO
//#define SNAPSHOT_LOG_CONFIG 1
//#define DIFF_LOG_CONFIG 2
//#define LOG_CONFIG
	/*
	enum{
		INT_SIZE_DIFF,
		BYTE_SIZE_DIFF
	};
	const int DIFF_GRANULARITY = INT_SIZE_DIFF;

	enum{
		_SNAPSHOT_LOG,
		_DIFF_LOG
	};
	const int LOG_STRATEGY = _SNAPSHOT_LOG;
	*/
//}

#define	_LOG_SNAPSHOT 1
#define	_LOG_DIFF 2
#define LOG_CONFIG _LOG_SNAPSHOT


#define	_GRANULARITY_BYTE_SIZE 1
#define	_GRANULARITY_INT_SIZE 2
#define DIFF_GRANULARITY_CONFIG _GRANULARITY_INT_SIZE

#define _SINGLE_READLIST 1
#define _MULTI_READLIST 2
#define READLIST_CONFIG _SINGLE_READLIST

//#define LAZY_READ
//#ifndef SINGLE_READLIST
//#define MULTI_READLIST
//#endif

//#if MYCONFIG == myenum.MYNUM
//sdlfjdfljfl
//#endif
//////////////////////////////GC Policy//////////////////////////////
#define GC_IN_CRITICAL_SECTION  /*Should we permit GC at critical section. 'No' may improve performance.*/

//////////////////////////////Log Policy/////////////////////////////
//#define GC_LIMIT_LOGENTRY (1024 * 4)
//#define MAX_LOGENTRY_NUM (1024 * 8) /*LogEntry is used to store the info of a difference log.*/

//#define GC_LIMIT_ENTRYSLOT (4096 * 8) //Do not use this item
#define MAX_SLICE_POINTER_NUM (4096 * 16) /*EntrySlot is used to store pointers of log entry.*/

//#define MAX_SHARED_ENTRY (4096 * 4)

//#define READLIST_LENGTH (1024 * 8) /*multi lists of entryslots.*/


/*
typedef struct _meta_free_space {
	int index;
	int limit;
	int __start;
} meta_free_space;

typedef struct runtime_data {
  //volatile unsigned long thread_index;
	volatile unsigned int thread_slot;
	int mutex;
	thread_info_t threads[MAX_THREAD_NUM];
	meta_free_space free;
	void init(){
		printf("HBDet: init meta data\n");
		thread_slot = 1;
		mutex = 0;
		free.index = 0;
		free.limit = META_DATA_SIZE - sizeof(struct runtime_data);
	}
} runtime_data_t;
*/

#define STACK_SIZE (1024 * 1024)
#define HEAP_SIZE (1048576UL * (1024 + 512))
#define HEAP_CHUNK_SIZE (1048576UL)

#define CHUNK_SIZE (1048576UL * 8) //unused

typedef unsigned long long uint64;
#endif /* HBDEFINES_H_ */
