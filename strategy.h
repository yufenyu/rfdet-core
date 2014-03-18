/*
 * Strategy.h
 *
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#ifndef STRATEGY_H_
#define STRATEGY_H_
#include "vectorclock.h"
#include "slice.h"

enum _SyncType{
	SYNC_LOCK,
	SYNC_TRYLOCK,
	SYNC_UNLOCK,
	SYNC_BROADCAST,
	SYNC_SIGNAL,
	SYNC_CREATE,
	SYNC_ENTRY,
	SYNC_EXIT,
	SYNC_JOIN,
	SYNC_WAIT,
	SYNC_BARRIER
};

class Strategy {

public:
	Strategy();
	virtual ~Strategy();
	static int beginSlice();
	static int endSlice();
	static int prepropagation();
public:
	static int mergeSlice(Slice* s);
	static int deliver(int from, vector_clock* old_time, vector_clock* new_time, int type); //deliver modifications.
	static int doPropagation(int from, vector_clock* old_time, vector_clock* new_time, int type);

};

#define MAX_READ_WRITE_PAGE 64000
class MProtectStrategy : public Strategy{
private:
	void* rwPages[MAX_READ_WRITE_PAGE];
	int rwPageNum;
protected:
	static MProtectStrategy* instance;
public:
	MProtectStrategy();
	void initOnThreadEntry();
public:

	/*TODO: add a type parameter to beginSlice and endSlice to indicate what kind of synchronization
	  cut the slice.*/
	static int beginSlice();
	static int endSlice();
	static int prePropagation();
	/*A callback function invoked by signal handler or instrumentation functions.*/
	static int writeMemory(void* addr, size_t len);
	static void init();
	static void protectMemory(void* addr, size_t size); // Read
	static void unprotectMemory(void* addr, size_t size); // Read/Write
	static void reprotectRWPages();
};


#define STRATEGY MProtectStrategy

#endif /* STRATEGY_H_ */
