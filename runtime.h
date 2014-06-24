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




#define META_CHUNK_SIZE (1024 * 1024 * 4) //4MB
#define META_KERNAL_SPACE_SIZE (1024 * 1024 * 16) //16MB



extern thread_info_t * me;

enum HappenBeforeReason{
	HB_REASON_LOCK,
	HB_REASON_UNLOCK,
	HB_REASON_CONDWAIT,
	HB_REASON_JOIN,
	HB_REASON_CREATE
};
enum RuningMode{
	Mode_DMT,
	Mode_Record,
	Mode_Replay,
	Mode_RaceFree,
	Mode_Pthreads
};


class _Runtime {
private:
	int runningmode;
	
public:
	inline void setRunningMode(int mode){
		runningmode = mode;
	}
	
	bool IsRecording(){
		return runningmode == Mode_Record;
	}
	bool IsReplaying(){
		return runningmode == Mode_Replay;
	}
	bool IsDMT(){
		return runningmode == Mode_DMT;
	}
	char* getAppname();
	
public: //Interface methods:
	
	virtual int finalize() = 0;
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
public:
	PthreadRuntime(){
		this->setRunningMode(Mode_Pthreads);
	}
public:
	virtual void init();
	virtual int finalize();
	virtual int threadExit();
	virtual int threadEntryPoint(void* args);
	
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

extern _Runtime* RUNTIME;


#endif /* RUNTIME_H_ */
