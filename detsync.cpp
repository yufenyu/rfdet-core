/*
 * DetSync.cpp
 *
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#include "detsync.h"
//#include "kendo.h"
#include "structures.h"
#include "utils.h"
#include "runtime.h"

DetSync::DetSync() {
	// TODO Auto-generated constructor stub

}

DetSync::~DetSync() {
	// TODO Auto-generated destructor stub
}

int DetSync::detLock(void* mutex){
	InternalLock* lock = (InternalLock*) mutex;
	//int old_owner = detLock(lock);
#ifdef USING_KENDO
	//return kendo_lock(lock);
#else
	Util::spinlock(&lock->ilock);
	return 0;
#endif
}

int DetSync::detTrylock(void* mutex){
	InternalLock* lock = (InternalLock*) mutex;
#ifdef USING_KENDO
	return kendo_trylock(lock);
#else
	if(!Util::spintrylock(&lock->ilock)){
		return EBUSY;
	}
	return 0;
#endif
}

int DetSync::detUnlock(void* mutex){
	InternalLock* lock = (InternalLock*) mutex;
#ifdef USING_KENDO
	//return kendo_unlock(mutex);
#else
	Util::unlock(&lock->ilock);
	return 0;
#endif
}



int RRSyncPolicy::lock(InternalLock* l){
	if(runtime.runningmode == Mode_Record){
		return recordLock(l);
	}
	else if(runtime.runningmode == Mode_Replay){
		return replayLock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;
}

int RRSyncPolicy::trylock(InternalLock* l){
	if(runtime.runningmode == Mode_Record){
		return recordTrylock(l);
	}
	else if(runtime.runningmode == Mode_Replay){
		return replayTrylock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;	
}

int RRSyncPolicy::unlock(InternalLock* l){
	if(runtime.runningmode == Mode_Record){
		return recordUnlock(l);
	}
	else if(runtime.runningmode == Mode_Replay){
		return replayUnlock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;		
}


/*Lock operation inf record mode. */
int RRSyncPolicy::recordLock(InternalLock* l){
	
	Util::spinlock(&l->ilock);
	thread->lockcount ++;
	writeLog(tid, thread->lockount, l->getVersion(), 0);
	l->incVersion();
	return 0;
}

int RRSyncPolicy::replayLock(InternalLock* l){
	thread->lockcount ++;
	waitLock(tid, thread->lockcount, l);
	
	Util::spinlock(&l->ilock);
	l->incVersion();
	return 0;
}

/* TODO: implement trylock. */
int RRSyncPolicy::recordTrylock(InternalLock* l){
//	int ret = Util::spintrylock(&l->ilock);
//	if(ret){
	//	thread->lockcount ++;
	//	writeLog(tid, thread->lockount, l->getVersion(), thread->failednum);
	//	l->incVersion();
	//	thread->failednum = 0;
	//	return 0;
	//}
	//else{
	//	thread->failednum ++;
	//}
	ASSERT(false, "Not implemented")
	return EBUSY;
}

int RRSyncPolicy::replayTrylock(InternalLock* l){
	//int ret = Util::spintrylock(&l->ilock);
	//if(thread->failednum < failednum){
	//	return EBUSY;
	//}
	
	//if(ret){
	//	thread->lockcount ++;
		//writeLog(tid, thread->lockount, l->getVersion(), thread->failednum);
	//	l->incVersion();
	//	thread->failednum = 0;
	//	return 0;
	//}
	//else{
	//	thread->failednum ++;
	//}
	ASSERT(false, "Not implemented")
	return EBUSY;
}

int RRSyncPolicy::recordUnlock(InternalLock* l){
	Util::unlock(&l->ilock);
}

int RRSyncPolicy::replayUnlock(InternalLock* l){
	Util::unlock(&l->ilock);
}


void RRSyncPolicy::writeLog(int tid, uint64_t locknum, uint64_t version, uint32_t failednum = 0){
	
}

ReplayLog& RRSyncPolicy::readLog(int tid, uint64_t locknum){
	
}

void RRSyncPolicy::waitStatus(ReplayLog& log){
	
}

void RRSyncPolicy::moveToNextLog(){
	
}