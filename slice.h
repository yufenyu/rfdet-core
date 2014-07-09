/*
 * slice.h
 *
 *  Created on: Apr 10, 2012
 *      @Author: zhouxu
 *      @Description: Manage Slices.
 */


#ifndef SLICE_H_
#define SLICE_H_
#include "defines.h"
#include "signal.h"
#include "memory.h"
//#include "logmemory.h"
#include "vectorclock.h"
#include "writeset.h"
#include "modification.h"
//#include "runtime.h"

#define SIMPLIFIED_SLICE


enum LogEntryStatus{
	SLICE_UNUSED,
	SLICE_BORN,    //the slice is allocated for use
	//SLICE_FILLING,
	SLICE_FILLED,  //the slice is filled with modifications.
	SLICE_INVALID, //the slice is invalid as all threads have see its modifications.
	SLICE_DEAD     //the slice is dead, its body remains for collection.
};

/**
 * A log entry for a special vector clock.
 * @Store: A logentry is stored at runtime meta data.
 * Note that this structure will point to a list of pages (log_page_t) which contains the diff values.
 * These log pages are stored in the LogMemory Space!
 */
class Slice{
private:
	//bool used; //whether this entry is used.
	int lock;
	uint32_t refcount;
	thread_id_t owner;

//#define phead modifications.phead
//#define pcursor modifications.pcursor
public:
	//static int global_id;
	int id;  //maybe unused

	int status;
	Slice* next;
public:

	//int oldowner;
	/**
	 * @Explain: Since vtime is memory consuming, we only create a logentry for each log. Hence the logentry copies should be
	 * pointers instead of shadow logentries. Hence, the logentry pointers "backpointer" which points to the original
	 * logentry and 'tpointer' which points thread the original logentry and its copies, should be abandoned!
	 * */
	vector_clock vtime;
	MODIFICATION modifications;
	//LogEntry* backpointer; //point to the owner
	//LogEntry* tpointer; //thread pointer
	//static Slice* currentslice;

private:

	//void push_diff_slow(void* addr, int value);
	//void push_diff_fast(A_diff_t* adiff, void* addr, int value);

public:
	void reset(){
		//used = false;
		owner = INVALID_THREAD_ID;
		refcount = 0;
		status = SLICE_UNUSED;
		id = 0;
	}
	
	Slice(){
		lock = 0;
		//oldowner = INVALID_THREAD_ID;
		//id = global_id ++;
		reset();
		id = -1;
	}


	//static Slice* CurrentSlice(){
		//return currentslice;
	//}
	//static Slice* NewSlice(){

	//}
	//static void SetCurrentSlice(Slice* s){
		//currentslice = s;
	//}

	int recordModifications();
	int mergeModifications();
	int reclaimMe();



	//void setUsed(bool b){
		//used = b;
	//}

	//int freeData(PageMemory* page); //Free the log pages of this entry.
	int freeEntry();
	void forceFreeEntry();

	int incRefCount(){
		int c;
		Util::spinlock(&lock);
		refcount ++;
		c = refcount;
		Util::unlock(&lock);
		return c;
	}

	int decRefCount(){
		int c;
		Util::spinlock(&lock);
		refcount --;
		c = refcount;
		Util::unlock(&lock);
		return c;
	}

	int getRefCount(){
		return refcount;
	}

	bool isUsed(){return status != SLICE_UNUSED;}
	void setOwner(int tid){owner = tid;}
	int getOwner(){return owner;}

	bool isValid();

	/*Below are used for debugging*/
	void dumpInfo(){
		fprintf(stderr, "<id=%d, status=%d, owner=%d, refcount=%d, time=%s\n",
				id, status, owner, refcount, vtime.toString());
	}

	//PageMemory* allocateStorePage();
};

class SlicePointer {

	int lock;
	int semaphore;
	Slice** pointers;
	int pointstart;
	int pointend;
	int gc_threshold;
	Slice** shadowpointers;
	//int shadowcursor;
#define	GC_THRESHOLD_STEP (MAX_SLICE_POINTER_NUM/4)
public:
	SlicePointer(){
		lock = 0;
		semaphore = 0;
		gc_threshold = GC_THRESHOLD_STEP;
	}
	
	void checkDuplicate(); //debugging function
	void initOnThreadEntry();
	Slice** readerAcquire(int* num);
	void readerRelease();

	int compact();
	void reset(){
		pointstart = 0;
		pointend = 0;
		//TODO: check pointers is all NULL.
	}

	int addSlice(Slice* s);
	//int deleteSlice(Slice* s, int index);
	int size(){
		return pointend - pointstart;
	}

	int getGCThreshold(){
		return gc_threshold;
	}

	/*
	 * oldsize is the size of slice pointer before a GC
	 * newsize is the size of slice pointer after the GC
	 */
	int adjustGCThreshold(int oldsize, int newsize){
		gc_threshold = newsize + GC_THRESHOLD_STEP;
		if(gc_threshold > MAX_SLICE_POINTER_NUM) gc_threshold = MAX_SLICE_POINTER_NUM;
	}

	/*Debuging routines*/
	void checkDuplicate(Slice* flog, int from, vector_clock* old_time, vector_clock* new_time, int reason);
};

/**
 * @DiffLog is the interface to read and write difference logs.
 */
class SliceManager : public ThreadObject {

private:
	/*slice list is used to store slices which are created by local thread.*/
	//Slice* slicelist; //deprecated. using freelist and deadlist instead.
	Slice* freelist;
	Slice* deadlist;


	//int freeSliceEntry; //deprecated
	//int usedEntryCount;
public:
	//int usedEntryCount;

	/*Slice Pointers are used for storing the propagated slices.*/
	//Slice** slicepointers; /*This structure is stored in Runtime meta_data!*/
	//int slicePointerNum;
	SlicePointer slicepointer;

	//int usedHead; /*The first slot that should be used.*/
	//int maxItems; /*Not used*/
	//int lock;
	//int semaphore;
	//vector_clock maxTime;


public:
	//static DiffLog* s_difflog;

	SliceManager(){
		/*slicePointerNum = 0;*/
		//freeSliceEntry = 0;
		//lock = 0;
		//semaphore = 0;
		/*slicepointers = NULL;*/
		//slicelist = NULL;
		//deadlist = NULL;
		currentslice = NULL;
	}

	//void init(); /*Init the SliceList if it is NULL!*/

	//Slice* allocLocalEntry();
	/*Slice** findOrCreateSlot();*/

	virtual int initOnThreadEntry();

#if (READLIST_CONFIG == _MULTI_READLIST)
	void initReadlist(int tid);
	void pushReadEntry(int tid, Slice* log);
	Slice* popReadEntry(int tid);
	Slice* frontEntry(int tid);
	void popFrontEntry(int tid);
	static int publishLog(Slice* log, int tid);
#endif

	//int logDiffs(); /*Write modifications to an log entry with current time.*/


	/*GC*/
	bool gcpoll();
	int GC();/*Perform garbage collection of logs.*/
	void freeAllSlices();
	//int compact();/*Compact log entries.*/
	//int checkAndPerformGC(); /*check if we should perform garbage collection.*/

	//Slice* frontEntry();
	//void resetIterator(HBIterator* iter){
		//iter->i = 0;
	//}
	//Slice* nextEntry(HBIterator* iter);


	/*Used for debugging.*/
	//int debuginfoSharedEntry();
	//int dumpLocalEntryList();
	//static int dumpSharedEntryList();

	//bool isEmpty(){
		//return usedEntryCount == 0;
	//}


	/*New function for Slice Manager*/
private:
	Slice* currentslice;
	Slice* NewSlice();
	void killSlice(Slice* s);
	void freeSlice(Slice*);
	void disposeDeads();
public:
	Slice* GetCurrentSlice();
	void ClearCurrentSlice();
	//void AddSliceToLocalPointers(Slice* slice);
};


class SliceSpace {
	Slice* freelist;
	int lock;
public:
	SliceSpace();
protected:
	Slice* pageToSlices(void* page);
public:
	void init();
	Slice* allocateSlices();
	int freeSlices(Slice* s);
};

#endif /* SLICE_H_ */
