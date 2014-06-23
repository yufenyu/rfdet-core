/*
 * runtime.h
 *
 *  Created on: Apr 11, 2012
 *      Author: zhouxu
 */

#ifndef RUNTIME_H_
#define RUNTIME_H_



#include <map>
#include <list>
#include "memory.h"

#include "utils.h"
#include "vectorclock.h"
#include "slice.h"
#include "structures.h"
#include "thread.h"
#include "defines.h"
#include "strategy.h"

typedef struct _LockItem{
	int tid;
	int next;
} LockItem;

extern InternalLockMap* internal_locks;


typedef struct _meta_free_space {
	int index;
	int limit;
	int __start;
} meta_free_space;


#define META_CHUNK_SIZE (1024 * 1024 * 4) //4MB
#define META_KERNAL_SPACE_SIZE (1024 * 1024 * 16) //16MB


class MemModSpace : public PageSizeMemory<METADATA_MMP_SPACE_SIZE, META_CHUNK_SIZE, MAX_THREAD_NUM> {};
class KernalSpace : public ImmutableMemory<META_KERNAL_SPACE_SIZE> {};

/**
 * This object is singleton, and global shared!
 */
class RuntimeDataMemory : public Memory {
public:
  //volatile unsigned long thread_index;
	volatile uint32_t thread_slot;
	int lock;
	int barrierlock;
	uint32_t barriercount;
	volatile int barrierfrontdoor;
	volatile int barrierbackdoor;
	int slicelock;
	thread_info_t threads[MAX_THREAD_NUM];
	

	/*TODO: check where are these maps stored? meta data, or the anonymous shared heap? */
	InternalLockMap ilocks;
	InternalCondsMap iconds;
	AdhocSyncMap iadhocsync;  //TODO: currently not supported.
	SliceSpace slicespace;

	/*profiling*/
	uint32_t numpagefault;
	uint64_t allocpages;
	uint64_t memfootprint;
	uint64_t allocated_size;
	uint32_t gc_count;
	uint32_t lockcount;
	
	RuntimeStatus runtimestatus;
	/**/
	KernalSpace kernaldata;
	MemModSpace modstore; /*Storage for memory modifications in the metadata space*/

	RuntimeDataMemory();
	/**
	 * All fields should be added before free. TODO: alignment!
	 */
	meta_free_space free; /*Depreciated*/

	void* meta_alloc(size_t size){
		Util::spinlock(&lock);
		void* ret = kernaldata.malloc(size);
		Util::unlock(&lock);
		ASSERT(ret != NULL, "Run out of memory in medata space.")
		return ret;
	}

	inline void* allocPage(){
		void* ret = modstore.allocOnePage();
		ASSERT(ret != NULL, "Run out of medata space!\n")
		return ret;
	}

	inline void freePage(void* ptr){
		modstore.freePage(ptr);
	}

	inline void freeThreadData(int tid){
		modstore.freeAll(tid);
	}

	inline bool gcpoll(){
		if(modstore.used() > modstore.size() / 2){
			return true;
		}
		return false;
	}
	inline void initOnThreadEntry(int tid){
		modstore.initOnThreadEntry(tid);
	}
	virtual void* start(){
		ASSERT(false, "This function should not be used!")
		return NULL;
	}

	virtual void* end(){
		ASSERT(false, "This function should not be used!")
		return NULL;
	}

	virtual size_t size(){
		return modstore.size() + kernaldata.size();
	}

	virtual size_t used(){
		return modstore.used() + kernaldata.used();
	}
	
	inline RuntimeStatus& getRuntimeStatus(){
		return runtimestatus;
	}
};

extern RuntimeDataMemory *metadata;
extern thread_info_t * me;

void init_mainthread();
//void* thread_entry_point(void* args);

enum HappenBeforeReason{
	HB_REASON_LOCK,
	HB_REASON_UNLOCK,
	HB_REASON_CONDWAIT,
	HB_REASON_JOIN,
	HB_REASON_CREATE
};
class _Runtime {
	
public:
	virtual int threadExit() = 0;
	virtual int threadEntryPoint(void* args) = 0;
	
	virtual void * malloc(size_t sz) = 0;
	virtual void  free(void * addr) = 0;
	virtual void * valloc(size_t sz) = 0;
	virtual void * calloc(size_t nmemb, size_t sz) = 0;
	virtual void * realloc(void * ptr, size_t sz) = 0;
	
	virtual int threadCreate (pthread_t * pid, const pthread_attr_t * attr, void *(*fn) (void *), void * arg) = 0;
	virtual int threadJoin(pthread_t tid, void ** val) = 0;
	virtual int threadCancel(pthread_t tid) = 0;
	virtual int mutexInit(pthread_mutex_t* mutex, const pthread_mutexattr_t * attr) = 0;
	virtual int mutexLock(pthread_mutex_t * mutex) = 0;
	virtual int mutexTrylock(pthread_mutex_t * mutex) = 0;
	virtual int mutexUnlock(pthread_mutex_t * mutex) = 0;
	//static int mutexTrylock(pthread_mutex_t * mutex);
	virtual int condInit(pthread_cond_t* cond, const pthread_condattr_t *attr) = 0;
	virtual int condWait(pthread_cond_t * cond, pthread_mutex_t * mutex) = 0;
	virtual int condSignal(pthread_cond_t * cond) = 0;
	virtual int condBroadcast(pthread_cond_t * cond) = 0;
	virtual int barrier_wait(pthread_barrier_t* barrier) = 0;
	
	virtual void init() = 0;
};

class PthreadRuntime : public _Runtime {
	
	virtual void init();
	
	virtual void * malloc(size_t sz);
	virtual void  free(void * addr);
	virtual void * valloc(size_t sz);
	virtual void * calloc(size_t nmemb, size_t sz);
	virtual void * realloc(void * ptr, size_t sz);
	
	virtual int threadCreate (pthread_t * pid, const pthread_attr_t * attr, void *(*fn) (void *), void * arg);
	virtual int threadJoin(pthread_t tid, void ** val);
	virtual int threadCancel(pthread_t tid);
	virtual int mutexInit(pthread_mutex_t* mutex, const pthread_mutexattr_t * attr);
	virtual int mutexLock(pthread_mutex_t * mutex);
	virtual int mutexTrylock(pthread_mutex_t * mutex);
	virtual int mutexUnlock(pthread_mutex_t * mutex);
	//static int mutexTrylock(pthread_mutex_t * mutex);
	virtual int condInit(pthread_cond_t* cond, const pthread_condattr_t * attr);
	virtual int condWait(pthread_cond_t * cond, pthread_mutex_t * mutex);
	virtual int condSignal(pthread_cond_t * cond);
	virtual int condBroadcast(pthread_cond_t * cond);
	//virtual int barrierImpl(int tnum);
	virtual int barrier_wait(pthread_barrier_t* barrier);
	
};

class ThreadPrivateData{
	bool kernal_malloc;
	MProtectStrategy strategy;
public:
	ThreadPrivateData() : kernal_malloc(false){}
	inline bool IsKernalMalloc(){
		return kernal_malloc;
	}
	inline bool SetKernalMalloc(bool b){
		kernal_malloc = b;
	}
	inline MProtectStrategy* getStrategy(){
		return &strategy;
	}
};

class HBRuntime : public _Runtime{
	RuntimeDataMemory _metadata;
	ThreadPrivateData* tpdata;
	MProtectStrategy* strategy;
private:
	int internalLock(InternalLock* lock);
	int internalUnlock(InternalLock* lock);
	int rawMutexLock(pthread_mutex_t * mutex, int synctype, bool usercall);
	int rawMutexUnlock(pthread_mutex_t * mutex, bool usercall);
	//static int paraLockWait(InternalLock* lock, bool is_happen_before, vector_clock* old_time, int old_owner) __attribute__ ((deprecated));
	bool gcpoll();

public:
	HBRuntime();
	bool isSingleThreaded();
	int GC();
	int tryGC(){
		//printf("tryGC\n");
		if(gcpoll()){
			GC();
			return 1;
		}
		return 0;
	}

	int clearGC(); //Full GC: clear all the diff logs without checking the validation.

public:
	
	virtual int threadExit();
	virtual int threadEntryPoint(void* args);
	
	int protectSharedData();
	int reprotectSharedData();
	int unprotectSharedData();
	//static void protectMemory(void* addr, size_t size, int prot);
	//static void unprotectMemory(void* addr, size_t size);
	int protectOnDemand();
	bool isInGlobals(void* addr);

	//static int flushLogs();
	//static int endSlice();
	//static int beginSlice();

	/*Managing Logs*/
	//static int takeSnapshot();
	//static int restoreSnapshot(int from, vector_clock* old_time, vector_clock* curr_time, int reason);
	int publishLog(Slice* log);
	//static int fetchModify(int from, int to, vector_clock* old_time, vector_clock* curr_time);
	int prefetchModify(int from, int to, vector_clock* old_time, vector_clock* curr_time);
	//static int mergeLog(Slice* flog);
	//static int register_adhoc(void* cond);

	virtual int threadCreate (pthread_t * pid, const pthread_attr_t * attr, void *(*fn) (void *), void * arg);
	virtual int threadJoin(pthread_t tid, void ** val);
	virtual int threadCancel(pthread_t tid);
	virtual int mutexInit(pthread_mutex_t* mutex, const pthread_mutexattr_t * attr);
	virtual int mutexLock(pthread_mutex_t * mutex);
	virtual int mutexTrylock(pthread_mutex_t * mutex);
	virtual int mutexUnlock(pthread_mutex_t * mutex);
	//static int mutexTrylock(pthread_mutex_t * mutex);
	virtual int condInit(pthread_cond_t* cond, const pthread_condattr_t * attr);
	virtual int condWait(pthread_cond_t * cond, pthread_mutex_t * mutex);
	virtual int condSignal(pthread_cond_t * cond);
	virtual int condBroadcast(pthread_cond_t * cond);
	
	virtual int barrier_wait(pthread_barrier_t* barrier);

	int barrierImpl(int tnum);

	bool isSharedMemory(void* addr);
	void dumpSharedRegion(void* pagestart, void* pageend, AddressPage** pages, Slice* stack);
	int dump(AddressMap* am, Slice* log);

	void beforeBarrier();
	void barrierDeliver();
	void afterBarrier();
	//static int doPropagation(int from, vector_clock* old_time, vector_clock* new_time);
	//static int detLock(void * mutex);
	//static int adhocsync_read(int* addr);
	//static int adhocsync_write(int* addr, int value);
	//static int adhocsync_rwait(int* cond, int value);

	
	virtual void * malloc(size_t sz);
	virtual void  free(void * addr);
	virtual void * valloc(size_t sz);
	virtual void * calloc(size_t nmemb, size_t sz);
	virtual void * realloc(void * ptr, size_t sz);
	
	RuntimeDataMemory* getMetadata(){
		return &_metadata;
	}
	
	ThreadPrivateData* getThreadPrivate(){
		return tpdata;
	}
	
	virtual void init();
	
};

extern HBRuntime* RUNTIME;





#endif /* RUNTIME_H_ */
