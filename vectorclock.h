/*
 * vectorclock.h
 *
 *  Created on: Apr 16, 2012
 *      Author: zhouxu
 */

#ifndef VECTORCLOCK_H_
#define VECTORCLOCK_H_
#include <string.h>
#include "defines.h"
//#include "runtime.h"

/**
 * @Description: Vector clock is a vector of n values, each describes the clock value of a thread.
 * The local value is driven by lock events, including all thread communication operations:
 * such as, lock, unlock, pthread_create, pthread_join, pipe_read, pipe_write. etc.
 *
 * @Design: Each time the clock is increased, it is potential to flush the diff to an new log entry.
 *
 */
#include<stdint.h>


class vector_clock{
#define MAX_STRING_LENGTH 128
public:
	int owner;
private:
	uint64_t clocks[MAX_THREAD_NUM];
private:
	char* toString(int num){
		static char string_buf[MAX_STRING_LENGTH];
		char* buf = string_buf;
		//TODO: use the following function call for simplicity.
		//toString(num, buf);
		//return buf;
		int index = 0;
		index += sprintf(buf, "<");
		for(int i = 0; i < num; i++){
			if(i == num - 1){
				index += sprintf(buf + index, "%lu>", this->clocks[i]);
			}
			else{
				index += sprintf(buf + index, "%lu, ", this->clocks[i]);
			}
		}
		buf[index] = 0;
		return buf;
	}

	void toString(int num, char* buf){
		int index = 0;
		index += sprintf(buf, "<");
		for(int i = 0; i < num; i++){
			if(i == num - 1){
				index += sprintf(buf + index, "%lu>", this->clocks[i]);
			}
			else{
				index += sprintf(buf + index, "%lu, ", this->clocks[i]);
			}
		}
		buf[index] = 0;
		//return buf;
	}
public:
	vector_clock(){
		reset();
	}
	void DEBUG_VALUE(){
#ifdef _DEBUG
		printf("<%lu, %lu, %lu>", clocks[0], clocks[1], clocks[2]);
#endif
	}
#define VECTOR_CLOCK_LEN 5
	void DEBUG_TIME(int tid){
		//printf("Thread(%d) inc (%d): ", me->tid, tid);
		DEBUG_VALUE();
		printf("\n");
	}

	char* toString(){
		return toString(VECTOR_CLOCK_LEN);
	}

	void toString(char* buf){
		return toString(VECTOR_CLOCK_LEN, buf);
	}

	void reset(){
		//printf("sizeof(clocks) = %d\n", sizeof(clocks));
		memset(clocks, 0, sizeof(clocks));
	}

	int incClock(int tid){
		//int tid = me->tid;
		clocks[tid] ++;
		//DEBUG_TIME(tid);
		//DEBUG_VALUE();
		return clocks[tid];
	}

	/**
	 * Increase clock according to the clock owner.
	 * Supporting invoking multi-times, e.g.,
	 * incClock(time, tid);
	 * incClock(time, tid);
	 * if time does not change, it is equal calling incClock(time, tid) once.
	 * */
	void incClock(vector_clock* time, int owner){
		//vector_clock new_value = *time;
		//new_value.incClock(owner);
		//vector_clock* new_value = time;
		for(int i = 0; i < MAX_THREAD_NUM; i++){
			if(time->clocks[i] > clocks[i]){
				clocks[i] = time->clocks[i];
			}
		}
		if(clocks[owner] == time->clocks[owner]){
			clocks[owner] ++;
		}
	}

	void setBigger(vector_clock* time, int owner){
		for(int i = 0; i < MAX_THREAD_NUM; i++){
			if(time->clocks[i] > clocks[i]){
				clocks[i] = time->clocks[i];
			}
		}
		if(time->clocks[owner] == clocks[owner]){
			clocks[owner] ++;
		}
	}

	/*Fixme: can I compare big? The tid is chosen as the smaller one!*/
	bool compareSmall(int tid, vector_clock* value){
		//int tid = me->tid;
		if(clocks[tid] < value->clocks[tid]){
			return true;
		}
		return false;
	}
	bool isZero(){
		for(int i = 0; i < MAX_THREAD_NUM; i++){
			if(clocks[i] != 0){
				return false;
			}
		}
		return true;
	}
};

#endif /* VECTORCLOCK_H_ */
