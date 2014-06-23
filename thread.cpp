/*
 * thread.cpp
 *
 *  Created on: Jul 4, 2012
 *      Author: zhouxu
 */
#include "defines.h"
#include "slice.h"
#include "thread.h"
#include "structures.h"
#include "detruntime.h"


/*These operations should be thread safe.*/
int InternalLock::push_owner(int tid){
		//int next = threads[nextowner];

		int pre = lastowner;
		threads[freeslot] = tid;
		lastowner = freeslot;
		freeslot ++;
		if(freeslot >= MAX_THREAD_NUM){
			freeslot = 0;
		}
		return threads[pre];
		/*
		//printf("Thread %d push owner %d\n", thread->tid, thread->tid);
		Util::syncbarrier();
		thread_info_t* ret = waitlist_tail;
		if(waitlist_head == NULL){
			waitlist_head = thread;
			waitlist_tail = thread;
		}
		else{
			waitlist_tail->nextlockwaiter = thread;
			waitlist_tail = thread;
		}
		thread->nextlockwaiter = NULL;
		if(ret == NULL){
			return INVALID_THREAD_ID;
		}
		return ret->tid;
		*/
	}

int InternalLock::next_owner(){
		//Util::syncbarrier();
		/*
		ASSERT(waitlist_head != NULL, "")
		if(waitlist_head == NULL || locked){
			return INVALID_THREAD_ID;
		}
		return waitlist_head->tid;
		*/
	//if(locked){
		//return INVALID_THREAD_ID;
	//}

	return threads[nextowner];
	//if(readcursor == NULL){
		//readcursor = waitlist_head;
	//}
	//ASSERT(readcursor != NULL, "")
	//if(waitlist_head != readcursor){
		//waitlist_head = readcursor;
	//}
	//return readcursor->tid;
}

void InternalLock::pop_owner(){
		//int tid = threads[nextowner];

		threads[nextowner] = INVALID_THREAD_ID;
		nextowner ++;
		if(nextowner >= MAX_THREAD_NUM){
			nextowner = 0;
		}
		//return tid;

		//printf("Thread %d push owner %d\n", waitlist_head->tid, waitlist_head->tid);
		//Util::syncbarrier();

		//if(readcursor == NULL){
			//readcursor = waitlist_head;
		//}
		//ASSERT(readcursor != NULL, "")
		//readcursor = readcursor->next;

/*
		//Util::spinlock(&ilock);
		ASSERT(waitlist_head != NULL, "")
		thread_info_t* next = waitlist_head->nextlockwaiter;
		if(next != NULL){
			waitlist_head = waitlist_head->nextlockwaiter;
		}
		//if(waitlist_head == NULL){
			//waitlist_tail = NULL;
		//}
*/
		//Util::unlock(&ilock);

}

void InternalLock::dumpOnwers(){
		//printf("nextowner:%d, freeslot:%d, lastowner:%d\n", nextowner, freeslot, lastowner);
		printf("<");
		for(int i = 0; i < MAX_THREAD_NUM; i++){
			printf("%d, ", threads[i]);
			//thread = thread->nextlockwaiter;
		}
		printf(">\n");
}

void InternalLock::setLocked(bool b){
	locked = b;
	//Util::syncbarrier();
}

int thread_info_t::init(){
	this->gc_count = 0;
	metadata->freeThreadData(tid);
	//this->snapshotLog.reinit();
	return 0;
}

int thread_info_t::leaveActiveList(){
	Util::spinlock(&metadata->lock);
	thread_info_t* pre = me;
	while(pre->nextactive != me){
		pre = pre->nextactive;
	}
	pre->nextactive = me->nextactive;
	Util::unlock(&metadata->lock);
	me->nextactive = me;
	return 0;
}

int thread_info_t::insertToActiveList(thread_info_t* thread){
	/*Add this thread to the active list*/
	Util::spinlock(&metadata->lock);
	thread->nextactive = me->nextactive;
	me->nextactive = thread;
	Util::unlock(&metadata->lock);
	return 0;
}


int CurrThreadID(){
	return me->tid;
}
