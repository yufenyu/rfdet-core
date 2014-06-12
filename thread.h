/*
 * thread.h
 *
 *  Created on: Jun 5, 2012
 *      Author: zhouxu
 */

#ifndef THREAD_H_
#define THREAD_H_
#include <atomic>
#include "defines.h"
#include "slice.h"
#include "writeset.h"


typedef struct _thread_info_t {
	thread_id_t tid;
	pid_t pid; /*The pid of this thread in the OS kernal.*/
	struct _thread_info_t* next;
	struct _thread_info_t* pre;
	std::atomic<void*> wcond; /*The waited conditional variable.*/
	//unsigned long global_twin_start;
	//unsigned long global_twin_end;
	//unsigned long global_diff_log;
	//twin_page_info_list * twin_list;
	void* (*start_routine)(void*);
	void* args;
	vector_clock vclock;  /*The current logical time of this thread!*/
	vector_clock oldtime;

	struct _thread_info_t* nextactive;/*Point to the next active thread*/
	struct _thread_info_t* nextlockwaiter; /*Point to the next thread which waits for the same lock.*/

	bool kernal_malloc; /*indicate whether this thread should malloc from meta data space.*/
	uint32_t incs; /* Increase by 1 when entering a critical section, decrease by 1 when leaving a critical section. */
	bool insync; /* If thread is performing synchronization in the runtime */
	bool ingc; /*thread is performing GC.*/
	/*TODO: use status instead*/
	bool finished;
	

	uint64_t gc_count;
	uint64_t numpagefault; //profiling

#ifdef _PROFILING
	uint64_t gc_time;
	uint64_t signaltime;
	uint64_t writelogtime;
	uint64_t readlogtime;
	uint64_t protecttime;
	uint64_t paracount;
	uint64_t serialcount;
#endif

	SliceManager slices;         /*Diff log for this thread.*/
	//SnapshotLog snapshotLog;
	//LocalAllocator myallocator;
	Slice* currslice;

	/*TODO: string_buf could be moved to global (thread private) data structure!*/

	_thread_info_t(){
		
		tid = INVALID_THREAD_ID;
		wcond = NULL;
		next = NULL;
		pre = NULL;
		//twin_list = global_twin_list;
		incs = 0;
		nextactive = this;
		kernal_malloc = false;
		finished = false;
		ingc = false;
		gc_count = 0;
		insync = false;
		numpagefault = 0;
		//printf("nextactive == this\n");
#ifdef _PROFILING
		gc_time = 0;
		signaltime = 0;
		writelogtime = 0;
		readlogtime = 0;
		protecttime = 0;

		paracount = 0;
		serialcount = 0;
#endif
		//printf("Testing Construction of _thread_info_t is called\n");
	}
	void incClock(){
		vclock.incClock(tid);
	}
	int init();
	int leaveActiveList();
	int insertToActiveList(_thread_info_t* thread);
	inline Slice* currentSlice(){
		return currslice;
	}
} thread_info_t;


class _ThreadPrivateType{

};

//_ThreadPrivateType privateData;

extern AddressMap writeSet;


#endif /* THREAD_H_ */
