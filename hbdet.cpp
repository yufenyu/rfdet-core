/*
 * hbdet.cpp
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 *
 */

#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <new>


#include "hook.h"

#include "defines.h"
#include "utils.h"
#include "heaps.h"
#include "signal.h"
#include "common.h"
#include "strategy.h"
#include "runtime.h"

#if defined(__GNUG__)
void initialize() __attribute__((constructor));
void finalize() __attribute__((destructor));
#endif


////////////////////////////Global Variables/////////////////////////////

InternalLockMap* internal_locks;
/*TODO: assert me is not in the region of global variables.*/
thread_info_t * me;
//bool kernal_malloc = false;
static bool initialized = false;

HBRuntime* RUNTIME;


/**
 * _init{malloc, pthread_mutex_lock, etc}| ----> initialize(init real hook functions)| ------> main()
 *
 * Initialize is not the entry point of this program. Before initialize is called, the system will initialize the
 * global data of this program. In this procedure, many hooked functions are to be called, including "malloc",
 * "pthread_mutex_lock" etc. Hence, these hooked function should take attention before the program is not initialized!
 *
 * However, initialize() is called before the main function of the program.
 */
void initialize() {
	NORMAL_MSG("HBDet: initialize...\n");
	/*Init meta data.*/
	size_t metadatsize = sizeof(HBRuntime) + PAGE_SIZE;
	void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(mapped == (void*)-1){
		DEBUG("Error: cannot allocate memory for meta data!\n");
		_exit(1);
	}
	//printf("metadatasize = %d\n", metadatsize);
	//kernal_malloc = true;
	RUNTIME = new (mapped) HBRuntime ();
	DEBUG_MSG("Meta data heap from %x to %x\n", mapped, mapped + metadatsize);
	
	RUNTIME->init();
	//std::cout << "Linux Page Size = " << getpagesize() << std::endl;
	ASSERT(PAGE_SIZE == getpagesize() && PAGE_SIZE == sysconf(_SC_PAGESIZE), "Page size configuration error!")

	DEBUG_MSG("HBDet: before initSnapshotMemory\n");

	Util::start_time();
	//ASSERT(initialized == true, "initialized = %d\n", initialized);
	initialized = true;
	DEBUG_MSG("HBDet: initializing finished!");
}


void finalize() {
#ifdef _PROFILING
	NORMAL_MSG("Thread(%d) gc time: %lld\n", me->tid, me->gc_time);
	NORMAL_MSG("Thread(%d) signal time: %lld\n", me->tid, me->signaltime);
	NORMAL_MSG("Thread(%d) write log time: %lld\n", me->tid, me->writelogtime);
	NORMAL_MSG("Thread(%d) read log time: %lld\n", me->tid, me->readlogtime);
	NORMAL_MSG("Thread(%d) protect shared data time: %lld\n", me->tid, me->protecttime);
	NORMAL_MSG("Thread(%d) rate of serial merge: %f.\n", me->tid, (double)me->serialcount/(me->paracount + me->serialcount));
	printf("Total lock operation number: %d\n", metadata->lockcount);
#endif

	metadata->numpagefault += me->numpagefault;
	//metadata->allocpages += me->myallocator.allocatedpages;
	//metadata->memfootprint += Heap::appHeap()->getUsedMemory(me->tid);
	metadata->gc_count += me->gc_count;
	NORMAL_MSG("HBDet: finalize...\n");
	uint64 etime = Util::watch_time();
	NORMAL_MSG("HBDet: Program elapse time: %lld\n", etime);
	NORMAL_MSG("HBDet: page fault number: %u\n", metadata->numpagefault / metadata->thread_slot);
	NORMAL_MSG("HBDet: GC count: %d\n", metadata->gc_count);
	NORMAL_MSG("HBDet: allocated pages = %llu\n", metadata->allocpages);
	//printf("HBDet: max memory = %d\n", metadata->UsedMemory());
	//printf("HBDet: shared memory usage = %llu\n", Heap::appHeap()->getUsedMemory() + GLOBALS_SIZE); old version
	NORMAL_MSG("HBDet: shared memory usage = %llu\n", metadata->memfootprint + GLOBALS_SIZE);
	NORMAL_MSG("HBDet: global memory = %llu\n", (uint64)GLOBALS_SIZE);
	NORMAL_MSG("HBDet: total allocated memory = %llu\n", metadata->allocated_size);

	//DiffLog::dumpSharedEntryList();
	//int diffs = DiffLog::s_difflog->logDiffs();
	//printf("HBDet: total diffs: %d\n", diffs);
	//dumpPageMapInfo();
}



////////////////////////////////////////Hooked Functions////////////////////////////////////////////////////
extern "C" {

//#define ENABLE_MALLOC_HOOK
#ifdef ENABLE_MALLOC_HOOK

/**
 * Fixed bug!!!: We should not call assert in malloc as assert is going to call malloc, which results in
 * recursive callings.
 */
void * malloc(size_t sz) {
	//if(sz > 1024 * 1024)

	void* ptr = NULL;
	//printf("HBDet: malloc is not initialized!\n");
	if (!initialized) {
		//printf("point 1: malloc, sz = %d\n", sz);
		//DEBUG("Pre-initialization malloc call forwarded to mmap");
		DEBUG_MSG("HBDet: malloc is not initialized!\n");
		ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(ptr == NULL){
			printf("error in hbdet.cpp 127\n");
			_exit(1);
		}
	}
//	else if(kernal_malloc){

//		ptr = metadata->meta_alloc(sz);

//		DEBUG_MSG("HBDet: kernal malloc: ptr = %x\n", ptr);
//	}
	else{
		//WARNING_MSG("malloc, sz = %d\n", sz);
		//ptr = Heap::getHeap()->malloc(sz);
		ptr = RUNTIME->malloc(sz);
		//ptr = getHeap()->malloc(sz);
		DEBUG_MSG("HBDet: malloc is implemented: ptr = %x\n", ptr);
	}
	//printf("HBDet: malloc is returned!\n");
	return ptr;
}

void  free(void * addr)
{	
	//WARNING_MSG("free is not implemented\n");
	if (!initialized) {
		return;
	}
	RUNTIME->free(addr);
	//Heap::getHeap()->free(addr);
	//getHeap()->free(addr);
}

void * valloc(size_t sz) {
	//if(sz > 1024 * 1024)
	//printf("valloc, sz = %d\n", sz);
	void* ptr = NULL;
	//printf("HBDet: malloc is not initialized!\n");
	if (!initialized) {
		//DEBUG("Pre-initialization malloc call forwarded to mmap");
		printf("HBDet: malloc is not initialized!\n");
		ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(ptr == (void *)-1)
		{
			printf("error in hbdet.cpp 127\n");
			_exit(1);
		}
	}
	else{
		//sz = sz/4096;
		//sz += 2;
		//sz = sz * 4096;
		//ptr = Heap::getHeap()->malloc(sz);
		ptr = RUNTIME->valloc(sz);
		//ptr = getHeap()->malloc(sz);
		/*fixme: implement valloc using the next two lines.
		  ptr = Heap::appHeap()->valloc(sz);
		  ptr = (void *)(PAGE_ALIGN_UP((long unsigned int)ptr)); */

		DEBUG_MSG("HBDet: valloc is implemented: ptr = %x\n", ptr);
		//printf("HBDet: malloc is not implemented: ptr = %x\n", ptr);
	}
	//printf("HBDet: malloc is returned!\n");
	return ptr;
}


void * calloc(size_t nmemb, size_t sz) {
	//printf("calloc, sz = %d\n", sz);
	void* ptr = NULL;
	if (!initialized) {
		NORMAL_MSG("HBDet: calloc is not initialized!\n");
		ptr = mmap(NULL, sz * nmemb, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(ptr == (void *)-1)
		{
			printf("error in hbdet.cpp 127\n");
			_exit(1);
		}
	}
	else{
		//ptr = Heap::getHeap()->malloc(sz * nmemb);
		ptr = RUNTIME->calloc(nmemb, sz);
		//ptr = getHeap()->malloc(sz * nmemb);
		DEBUG_MSG("HBDet: calloc is implemented: ptr = %x\n", ptr);
	}
	memset(ptr, 0, sz * nmemb);
	return ptr;
}

void * realloc(void * ptr, size_t sz) {
	//printf("realloc, sz = %d\n", sz);
	if(ptr == NULL){
		return malloc(sz);
	}

	else if(sz == 0){
		free(ptr);
	}
	else{
		//return Heap::getHeap()->realloc(ptr, sz);
		return RUNTIME->realloc(ptr, sz);
		//return getHeap()->realloc(ptr, sz);
	}

	return NULL;
}
#endif


int pthread_create (pthread_t * pid, const pthread_attr_t * attr, void *(*fn) (void *), void * arg) {
	DEBUG_MSG("HBDet pthread_create\n");
	//exit(0);
	ASSERT(initialized, "")
	return RUNTIME->threadCreate(pid, attr, fn, arg);
}

int pthread_attr_init(pthread_attr_t *attr){
	ASSERT(initialized, "")
	return real_pthread_attr_init(attr);
}

int pthread_attr_destroy(pthread_attr_t *attr){
	ASSERT(initialized, "")
	return 0;
}

#ifdef ENABLE_PTHREAD_HOOK
int pthread_join(pthread_t tid, void ** val) {
	//printf("HBDet: calling pthread_join, initialized = %d\n", initialized);
	ASSERT(initialized, "")
	//*val = NULL;
	//printf("HBDet: before calling threadJoin\n");
	int ret = RUNTIME->threadJoin(tid, val);
	if(val != NULL){
		*val = NULL;
	}
	//printf("pthread_join OK, join %d\n", tid);
	return ret;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t * attr) {
	int ret = RUNTIME->mutexInit(mutex, attr);
	return ret;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
	//printf("HBDet: Thread (%d) enter pthread_mutex_lock\n", me->tid);
	if(!initialized){
		return 0;
	}
#ifdef USING_INTERNAL_LOCK
	int ret = RUNTIME->mutexLock(mutex);
	return ret;
#else
	return real_pthread_mutex_lock(mutex);
#endif
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
	//ASSERT(false, "pthread_mutex_trylock is not implemented!\n")
	int ret = RUNTIME->mutexTrylock(mutex);
	return ret;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
	
	if(!initialized){
		return 0;
	}
	int ret = 0;
#ifdef USING_INTERNAL_LOCK
	ret = RUNTIME->mutexUnlock(mutex);

#else
	ret = real_pthread_mutex_unlock(mutex);
#endif
	return ret;
}

pthread_t pthread_self(void) {
	//return real_pthread_self();
	if(me != NULL){
		return me->tid;
	}
	fprintf(stderr, "Error: calling pthread_self before initializing!\n");
#ifdef _DEBUG
	exit(0);
#endif
	return INVALID_THREAD_ID;
}

int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t *attr) {
	ASSERT(initialized, "pthread_cond_init: runtime is not initialized!");
	return RUNTIME->condInit(cond, attr);
	//return real_pthread_cond_init(cond, attr);
}

int pthread_cond_broadcast(pthread_cond_t * cond) {
	//return real_pthread_cond_broadcast(cond);
	//ASSERT(initialized, "pthread_cond_broadcast: runtime is not initialized!")
	if(initialized){
		int ret = RUNTIME->condBroadcast(cond);
		return ret;
	}
	return 0;
}

int pthread_cond_signal(pthread_cond_t * cond) {
	ASSERT(initialized, "pthread_cond_signal: runtime is not initialized!")
	//return real_pthread_cond_signal(cond);
	int ret = RUNTIME->condSignal(cond);
	return ret;
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
	ASSERT(initialized, "pthread_cond_wait: runtime is not initialized!")
	int ret = RUNTIME->condWait(cond, mutex);
	return ret;
	//return real_pthread_cond_wait(cond, mutex);
}

int pthread_cond_destroy(pthread_cond_t * cond) {
	ASSERT(initialized, "pthread_cond_destroy: runtime is not initialized!")
	return 0;
	//return real_pthread_cond_destroy(cond);
}


int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset){
	printf("Thread %d enter sigmask\n", me->tid);
	int ret = sigprocmask(how, set, oldset);
	printf("Thread %d leave sigmask\n", me->tid);
	return 0;
}

int sigwait(const sigset_t *set, int *sig) {
	printf("Thread %d enter sigwait\n", me->tid);
	int ret = real_sigwait(set, sig);
	printf("Thread %d leave sigwait\n", me->tid);

    return ret;
}

int pthread_cancel(pthread_t thread) {

	ASSERT(initialized, "pthread_cond_destroy: runtime is not initialized!")
	int ret = RUNTIME->threadCancel(thread);
	return ret;
}

int pthread_setcanceltype(int type, int *oldtype){
	WARNING_MSG("pthread_setcanceltype is called\n");
	return 0;
}

void pthread_exit(void * value_ptr) {
	WARNING_MSG("pthread_exit is called\n");
	exit(0);
}

int pthread_barrier_wait(pthread_barrier_t* barrier) {
	ASSERT(initialized, "pthread_barrier_wait: runtime is not initialized!")

	return RUNTIME->barrier_wait(barrier);
}

#endif


//__thread void* wsbuffer[1024*1024];
//__thread int wslen[1024*1024];
//__thread int wsindex = 0;
//AddressPage* pages[PAGE_COUNT];
//__thread int pageNum = 0;
/*Recording the write set by using compile-time instrumentation. */
#ifdef UNUSED
void WS_RecordWrites(void* addr, size_t len){
	//ASSERT(initialized, "")
	//ASSERT(me->writeSet != NULL, "")
	//printf("store ");
	//me->writeSet->insert(addr, len);
	//wsbuffer[wsindex] = addr;
	//wslen[wsindex] = len;
	//wsindex ++;
	//if(wsindex >= 1024*1024){
		//wsindex = 0;
	//}
	//return;
	//DEBUG_MSG("Thread %d Records Write of Location %x\n", me->tid, addr);
	size_t pageid = (size_t)addr >> LOG_PAGE_SIZE;
	AddressPage* page = writeSet.pages[pageid];
	if(page == NULL){
		if(!RUNTIME::isSharedMemory(addr)){
			writeSet.pages[pageid] = (AddressPage*)1;
			return;
		}
		page = (AddressPage*)metadata->allocPage();
		writeSet.pages[pageid] = page;
		void* pageaddr = (void*)((size_t)addr & __PAGE_MASK);
		//ASSERT(page != NULL, "")
		memcpy(page, pageaddr, PAGE_SIZE);
		//for(int i = 0; i < PAGE_SIZE / 4; i ++){
			//((int*)page)[i] = ((int*)pageaddr)[i];
		//}
		writeSet.writePage(pageid);
		//writeSet.pageNum ++;
		//DEBUG_MSG("Thread (%d) RecordWrite: pageNum = %d\n", me->tid, writeSet.pageNum);
	}
	//size_t index = (size_t)addr & PAGE_MASK__;
	//page->value[index] = len;

//	return true;
}
#endif

}//extern "C"



