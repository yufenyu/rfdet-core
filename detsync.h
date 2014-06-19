/*
 * DetSync.h
 * Implementing deterministic synchronization order.
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#ifndef DETSYNC_H_
#define DETSYNC_H_
class InternalLock;

class SyncPolicy {
public:
	virtual int lock(void* l) = 0;
	virtual int trylock(void* l) = 0;
	virtual int unlock(void* l) = 0;
};

/*An implementation of Record & replay policy*/
class RRSyncPolicy : public SyncPolicy {
	struct ReplayLog{
		int tid;
		uint64_t locknum;
		uint64_t version;
		uint32_t failednum;
	};
public:
	virtual int lock(InternalLock* l);
	virtual int trylock(InternalLock* l);
	virtual int unlock(InternalLock* l);
private:
	int recordLock(InternalLock* l);
	int replayLock(InternalLock* l);
	int recordTrylock(InternalLock* l);
	int replayTrylock(InternalLock* l);
	int recordUnlock(InternalLock* l);
	int replayUnlock(InternalLock* l);
	void writeLog(int tid, uint64_t locknum, uint64_t version, uint32_t failednum = 0);
	ReplayLog& readLog(int tid, uint64_t locknum);
	void waitStatus(ReplayLog& log);
	void moveToNextLog();
};

class DetSync {
public:
	DetSync();
	virtual ~DetSync();
public:
	static int detLock(void* l);
	static int detTrylock(void* l);
	static int detUnlock(void* l);
};

#endif /* DETSYNC_H_ */
