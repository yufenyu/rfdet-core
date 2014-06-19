/*
 * structures.h
 *
 *  Created on: Jun 5, 2012
 *      Author: zhouxu
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_
#include <map>
#include "thread.h"

/**
 * The implementation of internal Lock and conditional variables
 * InternalLock should support nested lock.
 */
using namespace std;
class InternalLock{
	void init(){
		mutex = NULL;
		owner = INVALID_THREAD_ID;
		locked = false;
		//oldowner = INVALID_THREAD_ID;

		ilock = 0;
		//magic = 0;
		//lclock = 0;
		lastowner = 0;
		nextowner = 0;
		freeslot = 0;
		for(int i = 0; i < MAX_THREAD_NUM; i++){
			threads[i] = INVALID_THREAD_ID;
		}
		//waitlist_head = NULL;
		//waitlist_tail = NULL;
		//readcursor = NULL;
	}
public:
	InternalLock(){
		init();
		//printf("InternalLock(%x) is called! owner = %d\n", this, owner);
	}
	/*@Bugfixed: Constructor cannot call Constructor. Otherwise, */
	InternalLock(void* m){
		//InternalLock();
		init();
		mutex = m;
		//printf("InternalLock(%x) is called! owner = %d\n", this, owner);
	}
	bool OwnedByThread(int tid){
		return owner == tid;
	}

	/*These operations should be thread safe.*/
	int push_owner(int tid);
	int next_owner();
	void pop_owner();
	void dumpOnwers();
	void setLocked(bool b);
	
	inline uint64_t getVersion(){
		return version;
	}
	inline void incVersion(){
		version ++;
	}
	inline void setVersion(uint64_t v){
		version = v;
	}
	
private:
	/**
	 * Field 'locked' indicates whether this lock is currently held by a thread. If 'locked' is equal to 0,
	 * 'owner' means the old owner(last owner). Otherwise 'owner' means the current owner.
	 * 'locked' is used to support nested lock.
	 * */
	bool locked;
	uint64_t version;
public:
	void* mutex; /*The corresponding user application mutex.*/
	int ilock;  /*internal lock*/
	vector_clock ptime;
	vector_clock vtime; /*Logical time.*/
	int owner; /*Current owner of this lock*/
	int oldowner;

	//int magic;

	int lastowner;
	int nextowner;
	int freeslot;
	/*Fixed: a thread can only enter one waiting queue at a time. using linked list like condwait. */
	int threads[MAX_THREAD_NUM];

	//thread_info_t* waitlist_head;
	//thread_info_t* waitlist_tail;
	//thread_info_t* readcursor;
};

extern bool kernal_malloc;
class InternalLockMap{
	map<void*, InternalLock*> lockMap;
	int lock; /*shared*/
public:
	int extlock; /*external shared lock.*/
public:
	InternalLockMap(){lock = 0; extlock = 0;}
	InternalLock* FindOrCreateLock(void* mutex);
	InternalLock* createMutex(void* mutex);
};

/**
 * A thread can only wait for at most one conditional variable at a time.
 **/
class InternalCond{
//list<thread_info_t*> listThread;
public:
thread_info_t* front;
thread_info_t* back;
vector_clock vtime; /*Logical time.*/
int lock;
int sender;
//int value;

//int version;

private:
	void init(){
		mutex = NULL;
		cond = 0;
		lock = 0;
		front = NULL;
		back = NULL;
		sender = INVALID_THREAD_ID;
		//version = 0;
	}
public:
	InternalCond(){
		init();
	}
	InternalCond(void* c){
		init();
		cond = c;
	}
public:
	void* mutex;
	void* cond;
	volatile int condvar;
	void Wait(thread_info_t* thread);
	void enterWaitQueue(thread_info_t* thread);
	void wakeup(thread_info_t* thread){
		if(thread->wcond == this){
			thread->wcond = NULL;
		}
	}

	thread_info_t* Broadcast();
	thread_info_t* Signal(){
		thread_info_t* thread = NULL;
		SPINLOCK_LOCK(&lock);
		if(front != NULL){
			thread = front;
			front = front->next;
			if(front == NULL){
				back = NULL;
			}
		}
		SPINLOCK_UNLOCK(&lock);
		__sync_synchronize();
		return thread;
	}

	//int incVersion(){
		//int v;
		//Util::spinlock(&lock);
		//version ++;
		//v = version;
		//Util::unlock(&lock);
		//return v;
	//}
};

class AdhocSync{
	int lock;
public:
	/**
	 * @Description: The version is increased each time an adhoc sync variable is written.
	 * Currently, this variable is not used.
	 * TODO: using global write version and local read version.
	 **/
	int version;
	int value;
	vector_clock write_vtime;
	int write_tid;

	/*Currently, access_tid and read_tid are not used.*/
	int access_tid;
	int read_tid;
	//int rversion[MAX_THREAD_NUM];

public:
	AdhocSync(){
		lock = 0;
		value = 0;
		//variable = NULL;
		version = 0;
		//rversion = 0;
		write_tid = INVALID_THREAD_ID;
		read_tid = INVALID_THREAD_ID;
		access_tid = INVALID_THREAD_ID;
	}

	/*@return: the tid ot the latest thread that writes the variable.*/
	AdhocSync write(int v, thread_info_t* thread){

		AdhocSync tmpsync;

		Util::spinlock(&lock);
		tmpsync = *this;
		version ++;
		value = v;

		access_tid = thread->tid;
		write_tid = thread->tid;
		write_vtime = thread->vclock;
		Util::unlock(&lock);

		return tmpsync;
	}

	/*@return: the tid of the thread that writes the value. */
	AdhocSync read(int* v, thread_info_t* thread){

		AdhocSync tmpsync;

		Util::spinlock(&lock);
		tmpsync = *this;
		*v = value;
		access_tid = thread->tid;
		read_tid = thread->tid;
		Util::unlock(&lock);

		return tmpsync;
	}

};

/*The class that manages internal conditional variables!*/
class InternalCondsMap{
	map<void*, InternalCond*> condsMap;
	int lock;
public:
	InternalCondsMap(){lock = 0;}
	InternalCond* findOrCreateCond(void* cond);
	InternalCond* createCond(void* cond);
};

/*The class that manages internal conditional variables!*/
class AdhocSyncMap{
	map<void*, AdhocSync*> adhocMap;
	int lock;
public:
	AdhocSyncMap(){lock = 0;}
	AdhocSync* findOrCreateAdhoc(void* sync);
};


#endif /* STRUCTURES_H_ */
