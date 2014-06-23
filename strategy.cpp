/*
 * Strategy.cpp
 *
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "strategy.h"
#include "defines.h"
#include "detruntime.h"
#include "writeset.h"


Strategy::Strategy() {
	// TODO Auto-generated constructor stub

}

Strategy::~Strategy() {
	// TODO Auto-generated destructor stub
}

int Strategy::mergeSlice(Slice* s){
	return s->modifications.restore();
	//return s->modifications.merge();
}

/**
 * Merge the modifications between the two logical time(old_time, curr_time).
 **/
int Strategy::deliver(int from, vector_clock* old_time, vector_clock* new_time, int sync){
	thread_info_t* former = &metadata->threads[from];
	//thread_info_t* latter = &meta_data->threads[to];
	thread_info_t* latter = me;
	int count = 0;
	//DEBUG_MSG("Thread(%d): HBRuntime::Merge: current time = %s\n", me->tid, new_time->toString());

	/**
	 *  Important! the loop should be protected by lock, or, we should use local iterators.
	 *  Concurrent thread could read the modifications.
	 * */
#ifdef UNUSED
	Util::spinlock(&former->slices.lock);
	former->slices.semaphore ++;
	Util::unlock(&former->slices.lock);

	Slice* logs[MAX_SLICE_POINTER_NUM];
	int logcount = former->slices.slicePointerNum;
	memcpy(logs, former->slices.slicepointers, logcount * sizeof(Slice*));

	Util::spinlock(&former->slices.lock);
	former->slices.semaphore --;
	Util::unlock(&former->slices.lock);
#endif
	int logcount;
	Slice** logs = former->slices.slicepointer.readerAcquire(&logcount);


	//DEBUG_MSG("Thread(%d): fetchModify, logcount = %d\n", me->tid, logcount);
	int validlog = 0;
	for(int i = 0; i < logcount; i++){
		Slice* flog = logs[i];
		if(flog == NULL){
			continue;
		}
		if(flog->status != SLICE_FILLED){
			//printf("reading an un finished log\n");
			continue;
		}
		if(flog->vtime.compareSmall(flog->getOwner(), &me->oldtime)){
			continue;
		}

		if(flog->vtime.compareSmall(flog->getOwner(), new_time) && //the logentry should be seen by me!
						!flog->vtime.compareSmall(flog->getOwner(), old_time)){ //the logentry has not be seen by me!

			/* Point to the diff log of the former thread. */
			//Slice** empty_entry = latter->slices.findOrCreateSlot();
			/* Copy the log entry.*/
			//*empty_entry = flog;
			//flog->incRefCount();

			//latter->slices.slicepointer.checkDuplicate(flog, from, old_time, new_time, sync);

			latter->slices.slicepointer.addSlice(flog);

			//signal_up --;
			count += mergeSlice(flog);

			me->oldtime.setBigger(&flog->vtime, flog->getOwner());
			DEBUG("Thread(%d) merge LogEntry(%x)[%d", me->tid, flog, flog->getOwner());
			flog->vtime.DEBUG_VALUE();
			DEBUG("]\n");
			validlog ++;

			//logs[j] = logs[i];
			//j ++;
		}

	}
	former->slices.slicepointer.readerRelease();
	//DEBUG_MSG("Thread(%d): fetchModify, validlog = %d\n", me->tid, validlog);
	//me->oldtime = *curr_time;
	return count;
}

int Strategy::doPropagation(int from, vector_clock* old_time, vector_clock* new_time, int type){

#ifdef _PROFILING
	uint64 starttime = Util::copy_time();
#endif

	int ret;
	ret = deliver(from, old_time, new_time, type);

#ifdef _PROFILING
	uint64 endtime = Util::copy_time();
	me->readlogtime += (endtime - starttime);
#endif

	return ret;
	//return ret;
}

MProtectStrategy* MProtectStrategy::instance = NULL;
MProtectStrategy::MProtectStrategy(){
	rwPageNum = 0;
	ASSERT(instance == NULL, "instance = %p\n", instance)
	instance = this;
}

int MProtectStrategy::endSlice(){
	//RUNTIME::takeSnapshot();
	if(GetHBRuntime()->isSingleThreaded()){
		return 0;
	}
#ifdef _PROFILING
	uint64 starttime = Util::copy_time();
#endif

	int ret;

	Slice* aslice = me->slices.GetCurrentSlice();
	ret = aslice->recordModifications();

	//This is an optimization, to omit a slice if it does not have any modifications.
	if(ret != 0){
		/*TODO: me->slices.CloseCurrentSlice(me->vclock)*/
		me->slices.ClearCurrentSlice();
		aslice->vtime = me->vclock;
		/*TODO: we do not need slot if we use multi readlist.*/
		//me->slices.AddSliceToLocalPointers(aslice);
		me->slices.slicepointer.addSlice(aslice);
	}
	//ret = me->slices.recordMLog(); /*Log the diffs for this clock.*/

#ifdef _PROFILING
	uint64 endtime = Util::copy_time();
	me->writelogtime += (endtime - starttime);
#endif
	return ret;
}


int MProtectStrategy::beginSlice(){
	//RUNTIME::flushLogs();
	me->currslice = NULL;
	writeSet.clear();
	reprotectRWPages();
	me->slices.GetCurrentSlice();
	return 0;
}

int MProtectStrategy::prePropagation(){
	//doing nothing for now.
}

extern void* record_addr;
extern size_t record_size;
//unsigned int numPageFault = 0;

/** The signal handler for first writing into a page. */
int MProtectStrategy::writeMemory(void* addr, size_t len){

	me->numpagefault ++;
	instance->unprotectMemory(addr, PAGE_SIZE);

	//std::cout << "writeMemory: addr = " << addr << std::endl;
	
	address_t pageaddr = ((address_t)addr >> LOG_PAGE_SIZE) << LOG_PAGE_SIZE;
	//printf("Thread %d handleWrite: pageid = %d, insync = %d\n", me->tid, pageid, me->insync);
	if(!me->insync){ /*Page fault is caused by modification propagation.*/
		//AddressPage* page = writeSet.pages[pageid];
		//ASSERT(page == NULL, "page = %x, tid = %d\n", page, me->tid);
		//printf("Thread %d take snapshot of memory %d\n", me->tid, pageid);
		address_t page = (address_t)metadata->allocPage();
		ASSERT(page != (address_t)NULL, "page = %p, tid = %d\n", (void*)page, me->tid);
		//printf("Thread %d: Set pageid(%x) to page(%x)\n", me->tid, pageid, page);
		//void* pageaddr = (void*)((size_t)addr & __PAGE_MASK);
		memcpy((void*)page, (void*)pageaddr, PAGE_SIZE);
		
		//writeSet.pages[pageid] = page;
		writeSet.writePage(pageaddr, page);
	}

}

void dumpStack(){
	int j, nptrs;
    void *buffer[1024];
    char **strings;
    //fprintf(stderr, "\nDump Stack, enterpass = %d, iteration = %d:\n", enterpass, enterpassIteration);
    nptrs = backtrace(buffer, 100);
    fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("No backtrace_symbols!");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        fprintf(stderr, "%s\n", strings[j]);

    //free(strings);
}


struct sigaction * old_siga;
int tmp_signal_up = 0;
void segvHandle(int signum, siginfo_t * siginfo, void * context)
{
	//fprintf(stderr, "Thread %d calling segvHandle: signal_up = %d\n", me->tid, signal_up);
    void * addr = siginfo->si_addr; // address of access

    // Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR){
    	MProtectStrategy::writeMemory(addr, 0);
    }
    else{
    	//SEGV_MAPERR = 1,		/* Address not mapped to object.*/

    	VATAL_MSG("HBDet error: Thread %d cannot handle signal code(%d), sig_addr=%p, signal_up = %d\n",
                                me->tid, siginfo->si_code, siginfo->si_addr, tmp_signal_up);
    	//dumpStack();
    	while(1){}
        (old_siga->sa_sigaction)(signum, siginfo, context);
        exit(0);
    }
}



struct sigaction gsiga;
void MProtectStrategy::init(){

    //old_siga = &gsiga;
    struct sigaction old_siga;
    struct sigaction siga;
    sigemptyset(&siga.sa_mask);

    sigaddset(&siga.sa_mask, SIGSEGV);
    sigaddset(&siga.sa_mask, SIGALRM);

    sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL);

    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;

    siga.sa_sigaction = segvHandle;
    if (sigaction(SIGSEGV, &siga, &old_siga) == -1) {
      perror("sigaction(SIGSEGV)");
      exit(-1);
    }
    sigprocmask(SIG_UNBLOCK, &siga.sa_mask, NULL);
}

void MProtectStrategy::protectMemory(void* addr, size_t size){
	if(0 != mprotect((void*)PAGE_ALIGN_DOWN(addr), size, PROT_READ)){
		VATAL_MSG("mprotect error: addr = %p, size = %zu\n", addr, size);
		_exit(1);
	}
}

void MProtectStrategy::unprotectMemory(void* addr, size_t size){
	if(0 != mprotect((void*)PAGE_ALIGN_DOWN(addr), size, PROT_READ | PROT_WRITE)){
		VATAL_MSG("mprotect error: addr = %p, size = %zu\n" , addr, size);
		_exit(1);
	}
	ASSERT(this != NULL, "instance = %p\n", this)
	this->rwPages[this->rwPageNum] = addr;
	this->rwPageNum ++;
	ASSERT(this->rwPageNum < MAX_READ_WRITE_PAGE, "Too many pages to unprotect")
}

void MProtectStrategy::reprotectRWPages(){
	for(int i = 0; i < this->rwPageNum; i ++){
		protectMemory(this->rwPages[i], PAGE_SIZE);
	}
	this->rwPageNum = 0;
}

void MProtectStrategy::initOnThreadEntry(){
	this->rwPageNum = 0;
}

