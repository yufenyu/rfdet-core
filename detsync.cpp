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
