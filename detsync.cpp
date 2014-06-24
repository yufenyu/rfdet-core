/*
 * DetSync.cpp
 *
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#include <string>
#include <sstream>

#include "detsync.h"
//#include "kendo.h"
#include "structures.h"
#include "utils.h"
#include "detruntime.h"


int NondetSyncPolicy::lock(InternalLock* lock){
	Util::spinlock(&lock->ilock);
	return 0;
}

int NondetSyncPolicy::trylock(InternalLock* lock){
	if(!Util::spintrylock(&lock->ilock)){
		return EBUSY;
	}
}

int NondetSyncPolicy::unlock(InternalLock* lock){
	Util::unlock(&lock->ilock);
	return 0;
}

int RRSyncPolicy::lock(InternalLock* l){
	RuntimeStatus& rt = metadata->getRuntimeStatus();
	if(rt.IsRecording()){
		return recordLock(l);
	}
	else if(rt.IsReplaying()){
		return replayLock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;
}

int RRSyncPolicy::trylock(InternalLock* l){
	RuntimeStatus& rt = metadata->getRuntimeStatus();
	if(rt.IsRecording()){
		return recordTrylock(l);
	}
	else if(rt.IsReplaying()){
		return replayTrylock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;	
}

int RRSyncPolicy::unlock(InternalLock* l){
	RuntimeStatus& rt = metadata->getRuntimeStatus();
	if(rt.IsRecording()){
		return recordUnlock(l);
	}
	else if(rt.IsReplaying()){
		return replayUnlock(l);
	}
	ASSERT(false, "Never research here.")
	return 0;		
}


/*Lock operation inf record mode. */
int RRSyncPolicy::recordLock(InternalLock* l){
	
	Util::spinlock(&l->ilock);
	lockcount[me->tid] ++;
	writeLog(me->tid, lockcount[me->tid], l->getVersion(), 0);
	l->incVersion();
	return 0;
}

int RRSyncPolicy::replayLock(InternalLock* l){
	lockcount[me->tid] ++;
	ReplayLog log = readLog(me->tid, lockcount[me->tid]);
	waitStatus(log, l);
	//waitLock(me->tid, lockcount[me->tid], l);
	
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
	ASSERT(false, "Not implemented");
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
	ASSERT(false, "Not implemented");
	return EBUSY;
}


int RRSyncPolicy::recordUnlock(InternalLock* l){
	Util::unlock(&l->ilock);
}


int RRSyncPolicy::replayUnlock(InternalLock* l){
	Util::unlock(&l->ilock);
}



void RRSyncPolicy::writeLog(int tid, uint64_t locknum, uint64_t version, uint32_t failednum){
	FILE* file = this->getLogFile(tid);
	ASSERT(file != NULL, "Open file for Thread(%d) failed\n", tid);
	ReplayLog log;
	log.tid = tid;
	log.locknum = locknum;
	log.version = version;
	log.failednum = failednum;
	size_t ret = fwrite(&log, sizeof(log), 1, file);
	ASSERT(ret == 1, "Write log failed, write %d bytes\n", ret);
}


ReplayLog RRSyncPolicy::readLog(int tid, uint64_t locknum){
	FILE* file = this->getLogFile(tid);
	ReplayLog log;
	size_t ret = fread(&log, sizeof(log), 1, file);
	ASSERT(ret == 1, "Read log failed.");
	return log;
}


void RRSyncPolicy::waitStatus(ReplayLog& log, InternalLock* lock){
	ASSERT(lock->getVersion() <= log.version, "lock version increase too quickly");
	while(lock->getVersion() < log.version){
		Util::wait_for_a_while();
	}
	ASSERT(lock->getVersion() == log.version, 
			"Version incorrect: %ul vs %ul\n", lock->getVersion(), log.version);
}


void RRSyncPolicy::moveToNextLog(){
	
}

void RRSyncPolicy::closeLogFile(int tid){
	FILE* file = fds[tid];
	ASSERT(file != NULL, "file = NULL");
	long pos = ftell(file);
	//std::cerr << "Current position for thread " << tid << " : " << pos << std::endl;
	fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	ASSERT(pos == size, "Replay does not seem match Record\n");
	fclose(fds[tid]);
	
}

FILE* RRSyncPolicy::getLogFile(int tid){
	std::stringstream ss;
	ss << logfile << tid;
	std::string logfilename = ss.str();
	RuntimeStatus& rt = metadata->getRuntimeStatus();
	const char* mode = rt.IsRecording() ? "w" : "r";
	if(fds[tid] == NULL){
		//std::cerr << "Open log file " << logfilename << std::endl;
		fds[tid] = fopen(logfilename.c_str(), mode);
		ASSERT(fds[tid] != NULL, "open log file(%s) failed!", logfilename.c_str());
	}
	return fds[tid];
}