/*
 * utils.h
 *
 *  Created on: Apr 10, 2012
 *      Author: zhouxu
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <execinfo.h>
#include <stdint.h>

#include "defines.h"


#ifdef __linux__
static inline int futex(volatile int* uaddr, int op, int val,
                        const struct timespec* timeout, int* uaddr2, int val2) {
  return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val2);
}
#endif

using namespace std;


#define SPINLOCK_TRYLOCK(l) \
    (__sync_lock_test_and_set((l), 1) == 0)

#define SPINLOCK_LOCK(l) \
    do {} while (!SPINLOCK_TRYLOCK(l))

#define SPINLOCK_UNLOCK(l) \
    (__sync_lock_release(l))
#define YIELD_CPU() pthread_yield()
#define YIELD() pthread_yield()


#define PAGE_ALIGN_DOWN(x) (((uintptr_t) (x)) & ~PAGE_MASK__)
#define PAGE_ALIGN_UP(x) ((((uintptr_t) (x)) + PAGE_MASK__) & ~PAGE_MASK__)


extern int enterpass;
extern int enterpassIteration;
class Util{
	static uint64_t timekeeper;
	static uint64_t starttime;
public:
	static void spinlock(int* lock){
		SPINLOCK_LOCK(lock);
	}
	static bool spintrylock(int* lock){
		return SPINLOCK_TRYLOCK(lock);
	}
	static void unlock(int* lock){
		SPINLOCK_UNLOCK(lock);
	}
	static void syncbarrier(){
		__sync_synchronize();
	}

	static bool is_page_aligned(void* addr){
		if(((uintptr_t)addr & PAGE_MASK__) == 0){
			return true;
		}
		return false;
	}

	static int cyclicIndex(int i, int max){
		if(i >= max){
			i = 0;
		}
		return i;
	}

	static int cyclicInc(int i, int max){
		i ++;
		if(i >= max){
			i = 0;
		}
		return i;
	}

	static void hbassert(bool cond, const char* msg){
		if(!cond){
			fprintf(stderr, "%s\n", msg);
			exit(0);
		}
	}

	static void start_time(){
		struct timeval  FullTime;
		gettimeofday(&FullTime, NULL);
		starttime = FullTime.tv_sec * 1000000 + FullTime.tv_usec;
		timekeeper = starttime;
	}

	static uint64_t watch_time(){
		struct timeval  FullTime;
		gettimeofday(&FullTime, NULL);
		uint64_t current = FullTime.tv_sec * 1000000 + FullTime.tv_usec;
		uint64_t interval = current - starttime;
		timekeeper = current;
		return interval;
	}

	static uint64_t record_time(){
		struct timeval  FullTime;
		gettimeofday(&FullTime, NULL);
		uint64_t current = FullTime.tv_sec * 1000000 + FullTime.tv_usec;
		timekeeper = current;
		return current;
	}

	static uint64_t copy_time(){
		struct timeval  FullTime;
		gettimeofday(&FullTime, NULL);
		return FullTime.tv_sec * 1000000 + FullTime.tv_usec;
	}

	static uint64_t time_interval(){
		struct timeval  FullTime;
		gettimeofday(&FullTime, NULL);
		uint64_t current = FullTime.tv_sec * 1000000 + FullTime.tv_usec;
		uint64_t interval = current - timekeeper;
		return interval;
	}
	static void __attribute__((optimize("O0")))
	halt(){
		while(true){
			wait_for_a_while();
		}
	}
	static void __attribute__((optimize("O0")))  /** prevent optimization */
	wait_for_a_while(){
		//printf("wait for a while...\n");
		int j = 0;
		for(int i = 0; i < 256; i++) {
			j++;
		}
	}

	/*Fixme: this function is problemic*/
	static void dumpStack(){
	#define SIZE 128
		int j, nptrs;
	    void *buffer[SIZE];
	    char **strings;
	    //fprintf(stderr, "\nDump Stack, enterpass = %d, iteration = %d:\n", enterpass, enterpassIteration);
	    nptrs = backtrace(buffer, SIZE);
	    fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);

	     /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	        would produce similar output to the following: */

	    strings = backtrace_symbols(buffer, nptrs);
	    if (strings == NULL) {
	        perror("No backtrace_symbols!");
	        exit(EXIT_FAILURE);
	    }

	    for (j = 0; j < nptrs; j++)
	        fprintf(stderr, "%s\n", strings[j]);

	    //free(strings);
	}
};




#if HB_VERBOSE_LEVEL >= NORMAL_MSG_LEVEL
#define NORMAL_MSG(...) printf(__VA_ARGS__);
#else
#define NORMAL_MSG(...)
#endif

#if HB_VERBOSE_LEVEL >= VATAL_MSG_LEVEL
#define VATAL_MSG(...) fprintf(stderr, "%20s:%-4d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#else
#define VATAL_MSG(...)
#endif

#if HB_VERBOSE_LEVEL >= DEBUG_MSG_LEVEL
#define DEBUG_MSG(...) printf(__VA_ARGS__);
#else
#define DEBUG_MSG(...)
#endif

#if HB_VERBOSE_LEVEL >= WARNING_MSG_LEVEL
#define WARNING_MSG(...) fprintf(stderr, "%20s:%-4d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#else
#define WARNING_MSG(...)
#endif


#define OPEN_ASSERT
#ifdef OPEN_ASSERT
#define ASSERT(cond, ...) if(!(cond)){ \
		fprintf(stderr, "Assert failed: %20s:%-4d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); \
		exit(0); \
}
#else
#define ASSERT(cond, ...)
#endif
//#define ASSERT(cond) ASSERT(cond, "")
/*
#define NORMAL_MSG(...) \
	if(verbose_level >= NORMAL_MSG_LEVEL){ \
		printf(__VA_ARGS__); \
	}

#define FATAL_MSG(...) \
	if(verbose_level >= VATAL_MEG_LEVEL){ \
		printf(__VA_ARGS__); \
	}

*/
#endif /* UTILS_H_ */
