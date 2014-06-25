/*
 * runtime.cpp
 *
 *  Created on: Apr 12, 2012
 *      Author: zhouxu
 */

//#define _GNU_SOURCE
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "runtime.h"
#include "common.h"
#include "heaps.h"
#include "hook.h"
#include "detsync.h"
#include "strategy.h"




//extern int stackwrites;
//extern int totalwrites;



uint64_t Util::timekeeper = 0;
uint64_t Util::starttime = 0;

extern char *program_invocation_short_name;
char* _Runtime::getAppname(){
	return program_invocation_short_name;
}

	
void PthreadRuntime::init(){
	std::cout << "Running program " << this->getAppname() << " with pthreads..." << std::endl;
	init_real_functions();
	
}
	
int PthreadRuntime::finalize(){
	uint64_t elapsetime = Util::watch_time();
	std::cout << "Program " << this->getAppname() << 
					" with pthreads, elapse time: " << elapsetime << std::endl;
	return 0;
}

int PthreadRuntime::threadExit(){
	
}

int PthreadRuntime::threadEntryPoint(void* args){
	
}

void * PthreadRuntime::malloc(size_t sz){
	return real_malloc(sz);
}
void  PthreadRuntime::free(void * ptr){
	real_free(ptr);
}
void * PthreadRuntime::valloc(size_t sz){
	return real_valloc(sz);
}
void * PthreadRuntime::calloc(size_t nmemb, size_t sz){
	return real_malloc(nmemb * sz);
}
void * PthreadRuntime::realloc(void * ptr, size_t sz){
	return real_realloc(ptr, sz);
}
	
int PthreadRuntime::threadCreate (pthread_t * pid, const pthread_attr_t * attr, 
		void *(*fn) (void *), void * arg){
	return real_pthread_create(pid, attr, fn, arg);
}
int PthreadRuntime::threadJoin(pthread_t tid, void ** val){
	return real_pthread_join(tid, val);
}
int PthreadRuntime::threadCancel(pthread_t tid){
	return real_pthread_cancel(tid);
}
int PthreadRuntime::mutexInit(pthread_mutex_t* mutex, const pthread_mutexattr_t * attr){
	return real_pthread_mutex_init(mutex, attr);
}
int PthreadRuntime::mutexLock(pthread_mutex_t * mutex){
	return real_pthread_mutex_lock(mutex);
}
int PthreadRuntime::mutexTrylock(pthread_mutex_t * mutex){
	return real_pthread_mutex_trylock(mutex);
}

int PthreadRuntime::mutexUnlock(pthread_mutex_t * mutex){
	return real_pthread_mutex_unlock(mutex);
}
	//static int mutexTrylock(pthread_mutex_t * mutex);
int PthreadRuntime::condInit(pthread_cond_t* cond, const pthread_condattr_t *attr){
	return real_pthread_cond_init(cond, attr);
}
int PthreadRuntime::condWait(pthread_cond_t * cond, pthread_mutex_t * mutex){
	return real_pthread_cond_wait(cond, mutex);
}
int PthreadRuntime::condSignal(pthread_cond_t * cond){
	return real_pthread_cond_signal(cond);
}
int PthreadRuntime::condBroadcast(pthread_cond_t * cond){
	return real_pthread_cond_broadcast(cond);
}
int PthreadRuntime::barrier_wait(pthread_barrier_t* barrier){
	return real_pthread_barrier_wait(barrier);
}


