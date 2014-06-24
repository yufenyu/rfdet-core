/*
 * hbdet.cpp
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 *
 */


#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <new>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>

#include "hook.h"

#include "defines.h"
#include "utils.h"
#include "heaps.h"
#include "signal.h"
#include "common.h"
#include "strategy.h"
#include "runtime.h"
#include "detruntime.h"


#if defined(__GNUG__)
void initialize() __attribute__((constructor));
void finalize() __attribute__((destructor));
#endif


extern char *program_invocation_name;
extern char *program_invocation_short_name;


////////////////////////////Global Variables/////////////////////////////

//InternalLockMap* internal_locks;
thread_info_t * me;
static bool initialized = false;
_Runtime* RUNTIME;

static _Runtime* GetRuntime(){
	
	char* rtconfig = getenv("DETRT_CONFIG");
	//fprintf(stderr, "App name: %s\n", program_invocation_name);
	fprintf(stderr, "App short name: %s\n", program_invocation_short_name);

	std::stringstream ss;
	char* logpath = getenv("DETRT_CONFIG_LOGPATH");
	if(logpath != NULL){
		ss << logpath << "/";
	}
	ss << "logs/";
	std::string dir = ss.str();
	fprintf(stderr, "Log Dir: %s\n", dir.c_str());
	if(NULL == opendir(dir.c_str())){
		mkdir(dir.c_str(), 0775);
	}
	
	ss << program_invocation_short_name;
	std::string appname = ss.str();
	fprintf(stderr, "Log file: %s\n", dir.c_str());
	
	if(rtconfig == NULL){
		WARNING_MSG("Not set env DETRT_CONFIG\n");
	}
	else if(strcmp(rtconfig, "Nondet") == 0){
		size_t metadatsize = sizeof(PthreadRuntime) + PAGE_SIZE;
		void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if(mapped == (void*)-1){
			DEBUG("Error: cannot allocate memory for meta data!\n");
			_exit(1);
		}
		PthreadRuntime* rt = new (mapped) PthreadRuntime ();
		return rt;	
	}
	else if(strcmp(rtconfig, "DMT") == 0){
		size_t metadatsize = sizeof(HBRuntime) + PAGE_SIZE;
		void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if(mapped == (void*)-1){
			DEBUG("Error: cannot allocate memory for meta data!\n");
			_exit(1);
		}
		HBRuntime* rt = new (mapped) HBRuntime ();
		return rt;	
	}
	else if(strcmp(rtconfig, "Record") == 0){
		size_t metadatsize = sizeof(HBRuntime) + PAGE_SIZE;
		void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if(mapped == (void*)-1){
			DEBUG("Error: cannot allocate memory for meta data!\n");
			_exit(1);
		}
		RRRuntime* rt = new (mapped) RRRuntime (appname, Mode_Record);
		return rt;	
	}
	else if(strcmp(rtconfig, "Replay") == 0){
		size_t metadatsize = sizeof(HBRuntime) + PAGE_SIZE;
		void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if(mapped == (void*)-1){
			DEBUG("Error: cannot allocate memory for meta data!\n");
			_exit(1);
		}
		RRRuntime* rt = new (mapped) RRRuntime (appname, Mode_Replay);
		return rt;	
	}
	
NotFoundEnv:
	printf("Can not find the deterministic policy %s, using Nondet instead\n", rtconfig);
	size_t metadatsize = sizeof(PthreadRuntime) + PAGE_SIZE;
	void* mapped = mmap(NULL, metadatsize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(mapped == (void*)-1){
		DEBUG("Error: cannot allocate memory for meta data!\n");
		_exit(1);
	}
	_Runtime* rt = new (mapped) PthreadRuntime ();
	return rt;	
	//return NULL;
}

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
	/*Init meta data.*/
	RUNTIME = GetRuntime();
	RUNTIME->init();

	Util::start_time();
	//ASSERT(initialized == true, "initialized = %d\n", initialized);
	initialized = true;
	
}


void finalize() {
	RUNTIME->finalize();
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
	void* ptr = NULL;
	//printf("HBDet: malloc is not initialized!\n");
	if (!initialized) {
		//DEBUG("Pre-initialization malloc call forwarded to mmap");
		printf("malloc is not initialized!\n");
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



