/*
 * DetSync.h
 * Implementing deterministic synchronization order.
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#ifndef DETSYNC_H_
#define DETSYNC_H_
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include "defines.h"

class InternalLock;
class SyncPolicy {
public:
	virtual int lock(InternalLock* l) = 0;
	virtual int trylock(InternalLock* l) = 0;
	virtual int unlock(InternalLock* l) = 0;
};

class NondetSyncPolicy : public SyncPolicy {
public:
	virtual int lock(InternalLock* l);
	virtual int trylock(InternalLock* l);
	virtual int unlock(InternalLock* l);
};

struct ReplayLog{
	int tid;
	uint64_t locknum;
	uint64_t version;
	uint32_t failednum;
};

/*An implementation of Record & replay policy*/
class RRSyncPolicy : public SyncPolicy {
	
	std::string logfile;
	FILE* fds[MAX_THREAD_NUM];
	uint64_t lockcount[MAX_THREAD_NUM];
public:
	RRSyncPolicy(std::string file) : logfile(file){
		memset(fds, 0, sizeof(fds));
		memset(lockcount, 0, sizeof(lockcount));
	}
	
	virtual int lock(InternalLock* l);
	virtual int trylock(InternalLock* l);
	virtual int unlock(InternalLock* l);
	
private:
	FILE* getLogFile(int tid);
	
	int recordLock(InternalLock* l);
	int replayLock(InternalLock* l);
	int recordTrylock(InternalLock* l);
	int replayTrylock(InternalLock* l);
	int recordUnlock(InternalLock* l);
	int replayUnlock(InternalLock* l);
	void writeLog(int tid, uint64_t locknum, uint64_t version, uint32_t failednum);
	ReplayLog readLog(int tid, uint64_t locknum);
	void waitStatus(ReplayLog& log, InternalLock* lock);
	void moveToNextLog();
};

/*
class DetSync {
public:
	DetSync();
	virtual ~DetSync();
public:
	static int detLock(void* l);
	static int detTrylock(void* l);
	static int detUnlock(void* l);
};
*/

#endif /* DETSYNC_H_ */
