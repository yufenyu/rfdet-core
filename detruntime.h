#ifndef DMTRUNTIME_H_
#define DMTRUNTIME_H_

#include "runtime.h"
#include "detsync.h"

enum HappenBeforeReason{
	HB_REASON_LOCK,
	HB_REASON_UNLOCK,
	HB_REASON_CONDWAIT,
	HB_REASON_JOIN,
	HB_REASON_CREATE
};

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
	
	//RuntimeStatus runtimestatus;
	/**/
	KernalSpace kernaldata;
	MemModSpace modstore; /*Storage for memory modifications in the metadata space*/

	RuntimeDataMemory();
	/**
	 * All fields should be added before free. TODO: alignment!
	 */
	char* free; /*Depreciated*/

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
	
};

extern RuntimeDataMemory *metadata;

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
	
	SyncPolicy* syncpolicy;
	ThreadPrivateData* tpdata;
	MProtectStrategy* strategy;
	
private:
	int internalLock(InternalLock* lock);
	int internalUnlock(InternalLock* lock);
	int rawMutexLock(pthread_mutex_t * mutex, int synctype, bool usercall);
	int rawMutexUnlock(pthread_mutex_t * mutex, bool usercall);
	//static int paraLockWait(InternalLock* lock, bool is_happen_before, vector_clock* old_time, int old_owner) __attribute__ ((deprecated));
	bool gcpoll();
	int findTid();
	//void initConstants(void* heap_low);
	int protectGlobals();
	int unprotectGlobals();
	
protected:
	virtual SyncPolicy* createSyncPolicy();
	virtual void printResult();
	virtual void printWelcomeMsg();
	
public:
	inline SyncPolicy* getSyncPolicy(){
		return syncpolicy;
	}
	
	
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
	
	
	/* Internal methods for other runtime components. */
public:
	void initMainthread();
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
	RuntimeDataMemory* getMetadata(){
		return &_metadata;
	}
	
	ThreadPrivateData* getThreadPrivate(){
		return tpdata;
	}
	
	
	
public: /* Multithreading interface methods */
	virtual int finalize();
	virtual int threadExit();
	virtual int threadEntryPoint(void* args);
	
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
	virtual int barrier_init(pthread_barrier_t *__restrict barrier,
      const pthread_barrierattr_t *__restrict attr, unsigned count);

	virtual void * malloc(size_t sz);
	virtual void  free(void * addr);
	virtual void * valloc(size_t sz);
	virtual void * calloc(size_t nmemb, size_t sz);
	virtual void * realloc(void * ptr, size_t sz);

	virtual void init();
	
};

inline HBRuntime* GetHBRuntime(){
	HBRuntime* rt = dynamic_cast<HBRuntime*>(RUNTIME);
	ASSERT(rt != NULL, "RUNTIME is not HBRuntime");
	return rt;
}


/*Deterministic multithreading with Kendo algorithm*/
class DMTRuntime : public HBRuntime {
public:
	DMTRuntime();
protected:
	virtual void printResult(){
		std::cout << "Program elapse time: " << std::endl;
	}
	virtual void printWelcomeMsg(){
		std::cout << "Running program " << this->getAppname() << " with DMT" << std::endl;
	}
};

class RRRuntime : public HBRuntime {
	std::string logfile;
	
protected:
	virtual SyncPolicy* createSyncPolicy();
	virtual void printResult();
	virtual void printWelcomeMsg();
	
public:
	
	RRRuntime(std::string file, int mode);
	virtual int threadExit();
	//virtual void init();
	//virtual int finalize();
};

#endif /*DMTRUNTIME_H_*/
