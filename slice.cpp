/*
 * slice.cpp
 *
 *  Created on: Apr 10, 2012
 *      Author: zhouxu
 */

#include <stdio.h>
#include "runtime.h"


int SliceManager::initOnThreadEntry(){
	/*
	int need_size = sizeof(Slice *) * MAX_SLICE_POINTER_NUM;
	void* raw_logs = meta_data->meta_alloc(need_size);
	slicepointers = (Slice **)raw_logs;
	slicePointerNum = 0;
	*/
	slicepointer.initOnThreadEntry();
	freelist = NULL;
	deadlist = NULL;

	return 0;
}


#ifdef UNUSED
/*Allocate a Slice in the thread local buffer for this thread.*/
Slice* SliceManager::allocLocalEntry(){
	//if(entrylist == NULL){
		//init();
	//}
	ASSERT(slicelist != NULL, "")
	Slice* log = &slicelist[freeSliceEntry];
	if(log->isUsed()){
		NORMAL_MSG("HBDet: ran out of Slice! try GC...\n");
		GC();
		log = &slicelist[freeSliceEntry];
		if(log->isUsed()){
			VATAL_MSG("HBDet fatal error: ran out of Slice! Failed\n");
			exit(0);
		}
	}

	log->setUsed(true);
	log->setOwner(me->tid);
	log->status = SLICE_FILLING;

	freeSliceEntry ++;
	if(freeSliceEntry >= MAX_LOGENTRY_NUM){
		freeSliceEntry = 0;
	}

	usedEntryCount ++;
	return log;
}
#endif

Slice* SliceManager::NewSlice(){
	Slice* slice = NULL;
	/*
#ifdef SHARED_Slice_SCHEME
	if(log == NULL){
		NORMAL_MSG("HBDet: ran out of shared entry! try GC...\n");
		GC();
		log = allocSharedEntry();
	}

	if(log == NULL){
		VATAL_MSG("HBDet fatal error: ran out of shared entry! Failed\n");
		exit(0);
	}

	log->setOnwer(me->tid);
	log->incRefCount();
	log->status = FILLING;
	return log;
#else
	slice = allocLocalEntry();
#endif
	ASSERT(slice != NULL, "NewSlice error!\n")

	 */
	if(freelist == NULL){
		freelist = metadata->slicespace.allocateSlices();
	}
	slice = freelist;
	freelist = freelist->next;

	//slice->next = slicelist;
	//slicelist = slice;

	//slice->setUsed(true);
	slice->setOwner(me->tid);
	slice->status = SLICE_BORN;
	//slice->id = Slice::global_id ++;
	return slice;
}
//int Slice::global_id = 0;
void SliceManager::freeSlice(Slice* s){
	//s->freeEntry();
	if(!s->isUsed()){
		WARNING_MSG("Slice is freed more than once, may be caused by duplicate slices\n");
	}
	ASSERT(s->getOwner() == me->tid, "s->owner = %d, me->tid = %d, status = %d, \n",
			s->getOwner(), me->tid, s->status)

	s->reset();
	s->next = freelist;
	freelist = s;
}

void SliceManager::killSlice(Slice* s){
	//s->freeEntry();
	ASSERT(s->getOwner() == me->tid, "kill slice: ")
	//Slice* tmp = deadlist;
	//while(tmp != NULL){
		//if(tmp->id == s->id){
			//fprintf(stderr, "Fuck: duplate slice pointer: id = %d\n", s->id);
			//return;
		//}
		//tmp = tmp->next;
	//}
	s->next = deadlist;
	deadlist = s;
}

void SliceManager::disposeDeads(){
	ASSERT(this == &me->slices, "")

	Slice* s = deadlist;
	Slice* next = NULL;
	Slice* pre = NULL;
	while(s != NULL){
		next = s->next;
		if(s->getRefCount() == 0){
			ASSERT(s->getOwner() == me->tid, "Thread (%d) disposed a dead slice(%d)", 
					me->tid, s->getOwner())
			this->freeSlice(s);
		}
		else{
			s->next = pre;
			pre = s;
		}
		s = next;
	}
	deadlist = pre;
}

Slice* SliceManager::GetCurrentSlice(){
	if(currentslice == NULL){
		currentslice = NewSlice();
	}
	return currentslice;
}

void SliceManager::ClearCurrentSlice(){
	currentslice = NULL;
}

/*
void SliceManager::AddSliceToLocalPointers(Slice* s){

	Slice** slot = me->slices.findOrCreateSlot();
	*slot = s;

}
*/

#ifdef UNUSED
Slice** SliceManager::findOrCreateSlot(){
	//if(entrypointers == NULL){
		//init();
	//}
	ASSERT(slicepointers != NULL, "")
	/*First try*/
	if(slicePointerNum < GC_LIMIT_ENTRYSLOT){
		Slice** log = &slicepointers[slicePointerNum];
		slicePointerNum ++;
		return log;
	}

	NORMAL_MSG("HBDet: run out of local log entry slot, try GC...\n");
	GC();

	/*Second try*/
	if(slicePointerNum < MAX_SLICE_POINTER_NUM){
		Slice** log = &slicepointers[slicePointerNum];
		slicePointerNum ++;
		return log;
	}

	VATAL_MSG("HBDet error: run out of log entry slots\n");
	exit(0);
	return NULL;
}
#endif


#ifdef UNUSED
/*use a local iterator. */
Slice* SliceManager::nextEntry(HBIterator* iter){
	while(iter->i < slicePointerNum){
		if(slicepointers[iter->i]->isUsed()){
			Slice* ret = slicepointers[iter->i];
			iter->i ++;
			return ret;
		}
		iter->i++;
	}
	return NULL;
}
#endif




#ifdef UNUSED
/*semaphore allows compact to be concurrent with read of logs.*/
int SliceManager::compact(){
	Util::spinlock(&lock);
	DEBUG_MSG("Compact: enter the lock, semaphore = %d\n", semaphore);
	while(semaphore != 0){
		Util::unlock(&lock);
		//printf("Compact wait for a while...\n");
		Util::wait_for_a_while();
		Util::spinlock(&lock);
	}
	int max_count = slicePointerNum;
	int j = 0;
	for(int i = 0; i < max_count; i ++){
		//printf("Compact: i = %d\n", i);
		if(slicepointers[i] != NULL){
			slicepointers[j] = slicepointers[i];
			j++;
		}
	}
	slicePointerNum = j;
	Util::unlock(&lock);
	DEBUG_MSG("Compact: finished!\n");
	return j;
}
#endif

bool SliceManager::gcpoll(){

	if(slicepointer.size() > slicepointer.getGCThreshold()){
		return true;
	}
	return false;
}

//int gc_count = 0;
int SliceManager::GC(){/*Perform garbage collection of logs.*/
	//gc_count ++;
#ifndef GC_IN_CRITICAL_SECTION
	if(me->incs){
		WARNING_MSG("warning: trying to GC in critical section\n");
		return 0;
	}
#endif

//#ifdef _PROFILING
	me->gc_count ++;
//#endif
	//Util::record_time();

	//printf("\n////////////////////////////////Thread(%d): GC start...////////////////////////////////////\n", me->tid);


#ifdef UNUSED
	int count = 0;
	for(int i = 0; i < slicePointerNum; i++){ //There is no modification.
		//printf("A log(%s) is ", log->vtime.toString());
		Slice* slice = this->slicepointers[i];
		//Util::spinlock(&metadata->lock);
		//if(log->getOnwer() != me->tid){
		//	printf("Error: log->getOnwer(%d) != me->tid\n", log->getOnwer(), me->tid);
		//	exit(0);
		//}
		if(slice->isUsed() && !slice->isValid()){
			count += slice->freeEntry();
			if(slice->getOwner() == me->tid){
				if(slice->getRefCount() == 0){
					this->freeSlice(slice);
				}
				else{
					this->killSlice(slice);
				}
			}
			//this->reclaimSlice(s);

#if (READLIST_CONFIG == _SINGLE_READLIST)
			this->slicepointers[i] = NULL;
#else if (READLIST_CONFIG == _MULTI_READLIST)
			if(!slice->isUsed()){
				this->slicepointers[i] = NULL;
			}
#endif
		}

	}
#endif

	int slicenum;
	int count = 0;
	Slice** lpointers = slicepointer.readerAcquire(&slicenum);
	for(int i = 0; i < slicenum; i++){
		Slice* slice = lpointers[i];
		ASSERT(slice->isUsed(), "")
		if(!slice->isValid()){
			slice->freeEntry();
			if(slice->getOwner() == me->tid){
				if(slice->getRefCount() == 0){
					//ASSERT(slice->status == SLICE_DEAD, "")
					//ASSERT(slice->getOnwer() == me->tid, "")
					this->freeSlice(slice);
				}
				else{
					this->killSlice(slice);
				}
			}
			lpointers[i] = NULL;
		}
	}
	slicepointer.readerRelease();

	disposeDeads();
	NORMAL_MSG("Thread(%d): GC finished, Total freed log pages: %d\n", me->tid, count);

	//int num1 = freeSlot;
	//int num = compact();
	int oldsize = slicepointer.size();
	slicepointer.compact();
	int size = slicepointer.size();
	slicepointer.adjustGCThreshold(oldsize, size);

	//NORMAL_MSG("Compact to %d entries\n\n", num);

//#ifdef _PROFILING
//	me->gc_time += Util::time_interval();
//#endif
	return 0;
}

void SliceManager::freeAllSlices(){

	//ASSERT(false, "not implemented");
	int slicenum;
	int count = 0;
	Slice** lpointers = slicepointer.readerAcquire(&slicenum);
	for(int i = 0; i < slicenum; i++){
		Slice* slice = lpointers[i];
		if(slice->getOwner() == me->tid){
			slice->freeEntry();
			this->freeSlice(slice);
		}
		lpointers[i] = NULL;
	}
	slicepointer.readerRelease();
	slicepointer.reset();
	//slicepointer.pointend = 0;
	//slicepointer.pointstart = 0;
	//TODO: implement this.
	/*
	int count = 0;
	for(int i = 0; i < slicePointerNum; i++){//There is no modification.
		Slice* log = this->slicepointers[i];
		log->forceFreeEntry();
		this->slicepointers[i] = NULL;
	}
	slicePointerNum = 0;
	*/
}


void Slice::forceFreeEntry(){
	ASSERT(false, "not implemented")
	if(this->owner == me->tid){
		modifications.freeData();
		this->reset();
	}
}

int Slice::freeEntry(){
	int count = 0;
	Util::spinlock(&lock);
	int check = status;
	status = SLICE_DEAD;
	Util::unlock(&lock);

	/* freeData is not thread safe. or we can use local thread to free data.*/
	//if(check != SLICE_DEAD){
	if(owner == me->tid){
		count = modifications.freeData();
	}

	this->decRefCount();
	return count;
}

/*Is this Slice still valid by logical time.*/
bool Slice::isValid(){
	if(status == SLICE_INVALID || status == SLICE_DEAD){
		return false;
	}
	int owner = this->owner;
	for(int i = 0; i < MAX_THREAD_NUM; i++){
		thread_info_t* thread = &metadata->threads[i];
		if(thread->tid == INVALID_THREAD_ID){
			continue;
		}
		if(!this->vtime.compareSmall(owner, &thread->oldtime)){
			return true;
		}
	}
	status = SLICE_INVALID;
	return false;
}


#ifdef UNUSED
PageMemory* Slice::allocateStorePage(){

	void* tmpbuf = snapshot_memory->allocOnePage();
	if(tmpbuf == NULL){
		fprintf(stderr, "HBDet error: ran out of log memory!\n");
		exit(0);
	}
	PageMemory* page = new (tmpbuf) PageMemory;
	if(phead == NULL){
		phead = page;
		pcursor = page;
	}
	else{
		pcursor->next = page;
		pcursor = page;
	}
	return page;
}

#endif

int Slice::recordModifications(){
	int total = 0;
	if(writeSet.isEmpty()){
		DEBUG_MSG("Thread (%d) logDiff: writeSet = %d\n", me->tid, writeSet.getDirtyPageNum());
		return 0;
	}

	//TODO: total = modifications.record(&writeSet);
	total = modifications.record(&writeSet);
	//log_entry->modifications.record(&writeSet);
	DEBUG_MSG("Thread %d finishes dump writeSet\n", me->tid);
	//writeSet.clear();
	//for(int i = 0; i < pcount; i ++){
		//total += calcPageDiffs(global_twin_list->info[i].twin_addr, global_twin_list->info[i].src_addr, log_entry);
	//}

	this->status = SLICE_FILLED;

	//this->incRefCount();
	return total;
}

SliceSpace::SliceSpace(){
	lock = 0;
	freelist = NULL;
}

void SliceSpace::init(){

}

Slice* SliceSpace::pageToSlices(void* page){
	int num = PAGE_SIZE / sizeof(Slice);
	//printf("A page contains %d slices\n", num);
	Slice* slices = new (page) Slice[num];
	Slice* next = NULL;
	for(int i = 0; i < num; i ++){
		slices[i].next = next;
		next = &slices[i];
	}
	return next;
}

Slice* SliceSpace::allocateSlices(){
	//printf("Allocating slices for thread %d\n", me->tid);
	void* page = metadata->allocPage();
	Slice* ret = pageToSlices(page);
	return ret;
}

int SliceSpace::freeSlices(Slice* s){
	return 0;
}


Slice** SlicePointer::readerAcquire(int* num){
	Util::spinlock(&lock);
	semaphore ++;
	Util::unlock(&lock);

	*num = this->pointend - this->pointstart;
	return pointers + pointstart;
}

void SlicePointer::readerRelease(){
	Util::spinlock(&lock);
	semaphore --;
	Util::unlock(&lock);
}

int SlicePointer::compact(){
	int count = pointend - pointstart;
	memcpy(shadowpointers, pointers + pointstart, count * sizeof(Slice*));
	int j = 0;
	for(int i = 0; i < count; i ++){
		if(shadowpointers[i] != NULL){
			shadowpointers[j] = shadowpointers[i];
			j++;
		}
	}

	Util::spinlock(&lock);
	while(semaphore != 0){
		Util::unlock(&lock);
		Util::wait_for_a_while();
		Util::spinlock(&lock);
	}

	Slice** tmp = pointers;
	pointers = shadowpointers;
	shadowpointers = tmp;
	pointstart = 0;
	pointend = j;

	Util::unlock(&lock);

	DEBUG_MSG("Compact: finished!\n");
	return j;
}

void SlicePointer::checkDuplicate(Slice* s, int from, vector_clock* old_time, vector_clock* new_time, int reason){
	char time1[100];
	char time2[100];
	old_time->toString(time1);
	new_time->toString(time2);
	if(s->getOwner() == me->tid){

		fprintf(stderr, "Thread %d Error: propagate a slice back:\n", me->tid);
		fprintf(stderr, "Doing sync: %d\n", reason);
		fprintf(stderr, "propagate from %d to %d: old_time=%s, new_timd=%s\n",
				from, me->tid, time1, time2);
		s->dumpInfo();
	}

	for(int i = 0; i < pointend; i ++){
		if(s == pointers[i]){
			s->dumpInfo();
			ASSERT(false, "Thread %d Deplicate Slice Error: a slice is already in the slicepointers", me->tid)
		}
	}
}

int SlicePointer::addSlice(Slice* s){

	if(pointend >= MAX_SLICE_POINTER_NUM){
		RUNTIME->GC();
	}
	ASSERT(pointend < MAX_SLICE_POINTER_NUM, "Run out of slice pointers.")
	int ret = pointend;
	pointers[pointend] = s;
	s->incRefCount();
	pointend ++;
	return ret;
}

void SlicePointer::initOnThreadEntry(){
	int need_size = sizeof(Slice*) * MAX_SLICE_POINTER_NUM;
	void* raw_logs = metadata->meta_alloc(need_size);
	pointers = (Slice **)raw_logs;
	pointstart = 0;
	pointend = 0;

	raw_logs = metadata->meta_alloc(need_size);
	shadowpointers = (Slice **)raw_logs;
}
