
#include <sys/types.h>
#include <sys/wait.h>
#include "detruntime.h"
#include "heaps.h"
#include "detsync.h"
#include "hook.h"


RuntimeDataMemory * metadata;

//extern MProtectStrategy threadprivateStrategy;
int thread_entry_point(void* args){
	return RUNTIME->threadEntryPoint(args);
}



RuntimeDataMemory::RuntimeDataMemory(){
	
		thread_slot = THREAD_ID_START;
		lock = 0;
		barrierlock = 0;
		barriercount = 0;
		barrierfrontdoor = 0;
		barrierbackdoor = 0;
		slicelock = 0;
		//free.index = 0;
		//free.limit = META_DATA_SIZE - sizeof(struct RuntimeDataMemory);
		//internal_locks = &ilocks;
		numpagefault = 0;
		allocpages = 0;
		memfootprint = 0;
		allocated_size = 0;
		gc_count = 0;
		lockcount = 0;
}



InternalLock* InternalLockMap::createMutex(void* mutex){
	void* ptr = metadata->meta_alloc(sizeof(InternalLock));
	InternalLock* m = new (ptr) InternalLock(mutex);
	//kernal_malloc = true;
	
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(true);
	lockMap[mutex] = m;
	//kernal_malloc = false;
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(false);
	return m;
}

/**
 * @Description: This function builds the mapping between userspace 'mutexes' and internal 'locks'. When a mutex is
 * provided, the function returns a corresponding lock which is global shared.
 * @Bugfix: The read of map (e.g., m = lockMap[mutex]) will also allocate space if mutex is not in lockMap!
 * Hence, the flag "kernal_malloc" should be set before the read and write of lockMap.
 */
InternalLock* InternalLockMap::FindOrCreateLock(void* mutex){
	ASSERT(mutex != NULL, "")
	//printf("FindOrCreateLock: enter critical section\n");
	map <void*, InternalLock*> :: const_iterator iter;
	//kernal_malloc = true;
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(true);
	//SPINLOCK_LOCK(&lock);
	InternalLock* m = lockMap[mutex];
	//InternalLock* m = lockMap.at(mutex);
	//InternalLock* m = NULL;
	//iter = lockMap.find(mutex);
	if(m == NULL) {
		WARNING_MSG("Warning: using lock(%p) without initializing it.\n", mutex);
		//exit(0);
		SPINLOCK_LOCK(&lock);
		m = lockMap[mutex];
		//iter = lockMap.find(mutex);
		if(m == NULL){
			void* ptr = metadata->meta_alloc(sizeof(InternalLock));
			m = new (ptr) InternalLock(mutex);
			//m->magic = 1;
			//lockMap.insert(pair<void*, InternalLock*>(mutex, m));
			lockMap[mutex] = m;
		}
		SPINLOCK_UNLOCK(&lock);
	}
	//kernal_malloc = false;
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(false);
	//SPINLOCK_UNLOCK(&lock);
	return m;
}

InternalCond* InternalCondsMap::createCond(void* cond){
	void* ptr = metadata->meta_alloc(sizeof(InternalLock));
	InternalCond* c = new (ptr) InternalCond(cond);

	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(true);
	condsMap[cond] = c;
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(false);
	return c;
}

InternalCond* InternalCondsMap::findOrCreateCond(void* cond){
	ASSERT(cond != NULL, "")
	//SPINLOCK_LOCK(&lock);
	
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(true);
	InternalCond* c = condsMap[cond];
	if(c == NULL) {
		WARNING_MSG("Using conditional variable(%p) without initialize it\n", cond);
		//exit(0);
		SPINLOCK_LOCK(&lock);
		c = condsMap[cond];
		if(c == NULL){
			void* ptr = metadata->meta_alloc(sizeof(InternalCond));
			c = new (ptr) InternalCond(cond);
			condsMap[cond] = c;
		}
		SPINLOCK_UNLOCK(&lock);
	}
	//SPINLOCK_UNLOCK(&lock);
	
	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(false);
	return c;
}

AdhocSync* AdhocSyncMap::findOrCreateAdhoc(void* sync){
	SPINLOCK_LOCK(&lock);

	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(true);
	AdhocSync* isync = adhocMap[sync];
	if(isync == NULL) {
		void* ptr = metadata->meta_alloc(sizeof(AdhocSync));
		isync = new (ptr) AdhocSync();
		adhocMap[sync] = isync;
	}
	SPINLOCK_UNLOCK(&lock);

	GetHBRuntime()->getThreadPrivate()->SetKernalMalloc(false);
	return isync;
}


int getThreadNum(){
	int count = 0;
	for(int i = 0; i < MAX_THREAD_NUM; i++){
		thread_info_t* thread = &metadata->threads[i];
		if(thread->tid != INVALID_THREAD_ID && !thread->finished){
			count ++;
		}
	}
	return count;
}

ThreadPrivateData threadprivatedata;

HBRuntime::HBRuntime(){
	this->setRunningMode(Mode_RaceFree);
	this->tpdata = &threadprivatedata;
	metadata = &_metadata;
	strategy = tpdata->getStrategy();
	//this->syncpolicy = getSyncPolicy();
}

#ifdef UNUSED
void* shared_data_low_not_used;
void HBRuntime::initConstants(void* heap_low){
	shared_data_low_not_used = (void*)GLOBALS_START;
	DEBUG_MSG("GLOBALS_START = %x\n", (void*)GLOBALS_START);
	if(shared_data_low_not_used > heap_low){
		shared_data_low_not_used = heap_low;
	}
}
#endif

void HBRuntime::printResult(){
	uint64_t elapsetime = Util::watch_time();
	std::cout << "Program elapse time: " << elapsetime << std::endl;
	
#ifdef _PROFILING
	NORMAL_MSG("Thread(%d) gc time: %lld\n", me->tid, me->gc_time);
	NORMAL_MSG("Thread(%d) signal time: %lld\n", me->tid, me->signaltime);
	NORMAL_MSG("Thread(%d) write log time: %lld\n", me->tid, me->writelogtime);
	NORMAL_MSG("Thread(%d) read log time: %lld\n", me->tid, me->readlogtime);
	NORMAL_MSG("Thread(%d) protect shared data time: %lld\n", me->tid, me->protecttime);
	NORMAL_MSG("Thread(%d) rate of serial merge: %f.\n", me->tid, (double)me->serialcount/(me->paracount + me->serialcount));
	printf("Total lock operation number: %d\n", metadata->lockcount);
#endif

	metadata->numpagefault += me->numpagefault;
	//metadata->allocpages += me->myallocator.allocatedpages;
	//metadata->memfootprint += Heap::appHeap()->getUsedMemory(me->tid);
	metadata->gc_count += me->gc_count;
	NORMAL_MSG("HBDet: finalize...\n");
	uint64_t etime = Util::watch_time();
	NORMAL_MSG("HBDet: Program elapse time: %lld\n", etime);
	NORMAL_MSG("HBDet: page fault number: %u\n", metadata->numpagefault / metadata->thread_slot);
	NORMAL_MSG("HBDet: GC count: %d\n", metadata->gc_count);
	NORMAL_MSG("HBDet: allocated pages = %llu\n", metadata->allocpages);
	//printf("HBDet: max memory = %d\n", metadata->UsedMemory());
	//printf("HBDet: shared memory usage = %llu\n", Heap::appHeap()->getUsedMemory() + GLOBALS_SIZE); old version
	NORMAL_MSG("HBDet: shared memory usage = %llu\n", metadata->memfootprint + GLOBALS_SIZE);
	NORMAL_MSG("HBDet: global memory = %llu\n", (uint64)GLOBALS_SIZE);
	NORMAL_MSG("HBDet: total allocated memory = %llu\n", metadata->allocated_size);
}

void HBRuntime::printWelcomeMsg(){
	std::cout << "Running program " << this->getAppname() << " with RaceFree...\n" << std::endl;
}

int HBRuntime::finalize(){
	this->threadExit();
	this->printResult();
}

int HBRuntime::threadExit(){
	NORMAL_MSG("Thread %d begins to exit...\n", me->tid);
	metadata->numpagefault += me->numpagefault;
	//metadata->allocpages += me->myallocator.allocatedpages;
	//metadata->memfootprint += Heap::appHeap()->getUsedMemory(me->tid);
	metadata->gc_count += me->gc_count;

	strategy->endSlice();
	//HBRuntime::takeSnapshot();/*Write the modifications in logs!*/

	me->vclock.incClock(me->tid);//increase my vector clock;

	strategy->beginSlice();
	/*Dequeue this thread from the active list.*/
	me->leaveActiveList();

#ifdef _PROFILING
	NORMAL_MSG("Thread(%d) gc time: %lld\n", me->tid, me->gc_time);
	//printf("Thread(%d) signal time: %lld\n", me->tid, me->signaltime);
	NORMAL_MSG("Thread(%d) write log time: %lld\n", me->tid, me->writelogtime);
	NORMAL_MSG("Thread(%d) read log time: %lld\n", me->tid, me->readlogtime);
	//printf("Thread(%d) protect shared data time: %lld\n", me->tid, me->protecttime);
	NORMAL_MSG("Thread(%d) rate of serial merge: %f.\n", me->tid, (double)me->serialcount/(me->paracount + me->serialcount));
#endif

	me->finished = true;
	//reallocate_thread();
	NORMAL_MSG("Thread %d leaving\n", me->tid);
	
	return 0;	
}

int HBRuntime::threadEntryPoint(void* args){
	NORMAL_MSG("thread_entry_point: args = %p\n", args);
	thread_info_t* thread = (thread_info_t*)args;
	DEBUG_MSG("Enter thread_entry_point, start_routine = %p\n", thread->start_routine);

	me = thread;
	me->init();

	me->finished = false;
	me->vclock.owner = me->tid;
	me->vclock.incClock(me->tid);//increase my vector clock;
	me->pid = getpid();
	//global_twin_list->init(me->tid);


	metadata->initOnThreadEntry(me->tid);
	me->slices.initOnThreadEntry();
	writeSet.initOnThreadEntry(me->tid);
	//localbumppointer.initOnThreadEntry();
	this->strategy->initOnThreadEntry();
	//threadprivateStrategy.initOnThreadEntry();

	
#ifdef _GDB_DEBUG
	printf("Thread(%d): sleep for attach...\n", me->tid, me->pid);
	sleep(20);
#endif

	this->protectSharedData();
	DEBUG_MSG("Thread(%d) start: pid = %d\n", me->tid, me->pid);
	void* res = thread->start_routine(thread->args);
	DEBUG_MSG("Thread(%d) end function:\n", me->tid);
	
	this->threadExit();
	return 0;
}

void HBRuntime::initMainthread(){
	DEBUG_MSG("HBDet: init main thread...\n");
	int tid = metadata->thread_slot;
	metadata->thread_slot ++;
	me = &metadata->threads[tid];
	me->tid = tid;
	me->vclock.owner = me->tid;
	me->pid = getpid();
	//global_twin_list->init(me->tid);

	metadata->initOnThreadEntry(me->tid);
	me->slices.initOnThreadEntry();
	//localbumppointer.initOnThreadEntry();
	DEBUG_MSG("HBDet: main thread OK.\n");

}

int HBRuntime::protectSharedData(){
	if(isSingleThreaded()){
		return 0;
	}
#ifdef _PROFILING
	uint64_t starttime = Util::copy_time();
#endif

	DEBUG_MSG("Protect globals\n");
	protectGlobals();/*Protect the global variables.*/
	//protect_heap();
	DEBUG_MSG("Protect heap\n");
	Heap::getHeap()->protect_heap();
	//protectHeap();
	
#ifdef _PROFILING
	uint64_t endtime = Util::copy_time();
	me->protecttime += (endtime - starttime);
#endif
	DEBUG_MSG("Heap protected\n");
	return 0;
}

int HBRuntime::reprotectSharedData() {
	//global_twin_list->reprotect_pages();
	ASSERT(false, "reprotectShared Data is not implemented!\n");
	return 0;
}


int HBRuntime::unprotectSharedData(){

	if(isSingleThreaded()){
		return 0;
	}
#ifdef _PROFILING
	uint64_t starttime = Util::copy_time();
#endif
	unprotectGlobals();/*Protect the global variables, log the differences.*/
	//unprotect_heap();
	Heap::getHeap()->unprotect_heap();
	//unprotectHeap();

#ifdef _PROFILING
	uint64_t endtime = Util::copy_time();
	me->protecttime += (endtime - starttime);
#endif
	return 0;
}

bool IsSingleThread(){
	return GetHBRuntime()->isSingleThreaded();
}

bool HBRuntime::isSingleThreaded(){
	if(metadata->thread_slot <= 1){
		return true;
	}
	return false;
}

bool HBRuntime::gcpoll(){
	bool poll = (me->slices.gcpoll() || metadata->gcpoll());
	return poll;
}

int HBRuntime::GC(){
#ifdef _PROFILING
	uint64_t starttime = Util::copy_time();
#endif

	int ret;
	ret = me->slices.GC();


#ifdef _PROFILING
	uint64_t endtime = Util::copy_time();
	me->gc_time += (endtime - starttime);
#endif
	return ret;

}

int HBRuntime::clearGC(){
	std::cout << "Calling clearGC" << std::endl;
	me->slices.freeAllSlices();
}


extern int signal_up; //debugging variable

/*Currently prefetchModify is not used.*/
int HBRuntime::prefetchModify(int from, int to, vector_clock* old_time, vector_clock* curr_time){
	VATAL_MSG("prefetchModify is unimplemented!\n");
	return 0;
}


/*this function is used to improve the using in pthread_cond_wait.*/
int HBRuntime::rawMutexLock(pthread_mutex_t * mutex, int synctype, bool usercall){
	DEBUG_MSG("Thread(%d): mutex lock %x\n", me->tid, mutex);
	//me->insync = true;
	/*The first phase: initializing...*/
	//int owner = me->tid;
	bool is_happen_before = false;
	vector_clock old_time = me->vclock;
	if(usercall){
		//int logcount = takeSnapshot(); /*Log the diffs for this clock.*/
		strategy->endSlice();
	}

	//vector_clock predict_time = me->vclock;
	//predict_time.incClock(me->tid);
	me->vclock.incClock(me->tid); /*Increase vector clock, enter the next slice(epoch).*/

	//////////////////////////////Second phase: contend for lock/////////////////////////////////////////////
	/*The second phase: Critical section */
	//TODO: detLock(); //deterministically acquire this lock.

	InternalLock* lock = metadata->ilocks.FindOrCreateLock(mutex);

	if(synctype == SYNC_LOCK){
		syncpolicy->lock(lock);
	}
	else if(synctype == SYNC_TRYLOCK){
		int ret = syncpolicy->trylock(lock);
		if(ret == EBUSY){
			return EBUSY;
		}
	}

	int old_owner = lock->push_owner(me->tid);
	if(old_owner == INVALID_THREAD_ID){
		old_owner = lock->owner;
	}
	
	syncpolicy->unlock(lock);

	//Util::spinlock(&lock->ilock);
	//bool is_happen_before = false;
	//int old_owner = lock->push_owner(me->tid);
	//if(old_owner == INVALID_THREAD_ID){
		//old_owner = lock->owner;
	//}

	if(old_owner != INVALID_THREAD_ID && old_owner != me->tid){
		//predict_time.incClock(&lock->ptime, old_owner);
		is_happen_before = true;
	}

	//lock->ptime = predict_time;

	//Util::unlock(&lock->ilock);
	if(is_happen_before){
		DEBUG_MSG("Thread (%d) Lock happen-before: thread %d-------------------------->thread %d.\n", me->tid, old_owner, me->tid);
	}
	/////////////////////////////////////The third phase/////////////////////
	/* the parallel execution before waiting to enter the serial execution.*/
	if(is_happen_before && usercall){
		//unprotectSharedData();
		strategy->prePropagation();
	}

	int paracount = 0, serialcount = 0;
	int next = lock->next_owner();
	vector_clock predict_time;//TODO: using me->vtime directly. omiting predict_time.
	//if(!predict_time.isZero()){
		//fprintf(stderr, "predict_time is not zero: %s\n", predict_time.toString());
	//}
	while(next != me->tid){
		if(is_happen_before){
			/** 
			 * Below is the buggy code, causing a redundent propagation---a slice will be propagated
			 * to its owner thread. We belive this code may also cause unordered slices pointers.
			 * A thread will pull a slice ahead of time before the releasing thread acquires the lock.
			 * Fix: using the next line (predict_time = lock->vtime;) instead. To optimize, we 
			 * should set lock->vtime right after a thread acquires the lock.
			//predict_time.incClock(&lock->vtime, old_owner);
			 **/
			predict_time = lock->vtime;
			paracount += strategy->doPropagation(old_owner, &old_time, &predict_time, SYNC_LOCK);
			//paracount += restoreSnapshot(old_owner, old_time, &predict_time, HB_REASON_LOCK);
		}
		//Util::wait_for_a_while();
		next = lock->next_owner();
		//count ++;
		//wait here until it is my turn to run.
	}


	//lock->setLocked(true); //lock this mutex
	lock->owner = me->tid;
	me->incs ++;
	/**
	 * Test if there is a happen-before relationship. If so, the current thread should see the memory
	 * modifications of his previous thread.
	 */
	if(is_happen_before){
		me->vclock.incClock(&lock->vtime, old_owner);
		//serialcount = HBRuntime::restoreSnapshot(old_owner, &old_time, &me->vclock, HB_REASON_LOCK);
		serialcount = strategy->doPropagation(old_owner, &old_time, &me->vclock, SYNC_LOCK);
		DEBUG_MSG("Thread %d total merged modifications: %d\n", me->tid, paracount + serialcount);
		//dump_data();
	}

	lock->vtime = me->vclock; //Used to fix the duplicate-slice bug.
	
#ifdef _PROFILING
	me->paracount += paracount;
	me->serialcount += serialcount;
	metadata->lockcount ++;
#endif
	DEBUG_MSG("Thread %d total restored = %d\n", me->tid, paracount + serialcount);

	if(usercall){
		//flushLogs();
		strategy->beginSlice();
	}

	DEBUG_MSG("Thread %d leaving rawMutexLock\n", me->tid);
	return 0;
}

int HBRuntime::rawMutexUnlock(pthread_mutex_t * mutex, bool usercall){

	/**
	 * Write the modifications in logs!
	 * @Reason: As we do not know which thread will get the lock afterwards, we just log the modifications of
	 * this slice conservatively!
	 **/
	DEBUG_MSG("Thread (%d) mutex unlock %x\n", me->tid, mutex);
	if(usercall){
		//int count = takeSnapshot();
		strategy->endSlice();
	}

	InternalLock* lock = metadata->ilocks.FindOrCreateLock(mutex);
	lock->vtime = me->vclock;

	me->vclock.incClock(me->tid);//increase my vector clock;

	lock->pop_owner();
	//lock->setLocked(false);

	/*Thread is about to leave the critical section.*/
	me->incs --;
	if(me->incs < 0){
		me->incs = 0;
	}
	//Util::unlock(&lock->ilock);
	if(usercall){
		//flushLogs();
		strategy->beginSlice();
	}
	//global_twin_list->reprotect_pages();
	//global_twin_list->flush_diffs();/*Flush the twin pages, unmap them!*/

	//flushLogs();
	return 0;
}

int HBRuntime::findTid(){
	for(int i = 0; i < MAX_THREAD_NUM; i++){
		int tid = i;
		Util::spinlock(&metadata->lock);
		if(metadata->threads[tid].tid == INVALID_THREAD_ID){
			metadata->threads[tid].tid = tid;
			Util::unlock(&metadata->lock);
			return tid;
		}
		Util::unlock(&metadata->lock);
	}
	return INVALID_THREAD_ID;
}

int HBRuntime::threadCreate (pthread_t * pid, const pthread_attr_t * attr, void *(*fn) (void *), void * arg){
	/**
	 * Fixme: the tid assignment should be deterministic! using logical time!
	 * */
//#ifdef NOTHING
	bool singlethread = isSingleThreaded();

	Util::spinlock(&metadata->lock);
	thread_id_t tid = metadata->thread_slot;
	if(tid >= MAX_THREAD_NUM){
		tid = INVALID_THREAD_ID;
	}
	else{
		metadata->thread_slot ++;
	}
	Util::unlock(&metadata->lock);

	if(tid == INVALID_THREAD_ID){
		tid = findTid();
	}

	if(tid == INVALID_THREAD_ID){
		VATAL_MSG("Too much thread: HBDet can only support thread number < %d\n", MAX_THREAD_NUM);
		exit(0);
	}

	*pid = tid;
	/*Initialize the thread struct*/
	thread_info_t* thread = &metadata->threads[tid];
	thread->start_routine = fn;
	thread->args = arg;
	thread->tid = tid;
	thread->vclock = me->vclock;
	thread->oldtime = me->vclock;


	/*Add this thread to the active list*/
	me->insertToActiveList(thread);
//#endif
//#ifdef MULTI_ADDRESS_SPACE
	//printf("HBDet: before mmap!\n");
	char* child_stack = (char *) mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(child_stack == NULL){
		fprintf(stderr, "HBDet: cannot allocate stack for child thread\n");
		exit(0);
	}
	child_stack += STACK_SIZE;

	NORMAL_MSG("Thread %d using stack from %x to %x\n", tid, child_stack, child_stack + STACK_SIZE);
	int child = clone(thread_entry_point, child_stack, CLONE_FILES | CLONE_FS | CLONE_IO | SIGCHLD, thread);
	NORMAL_MSG("HBDet: clone thread(%d), pid = %d!\n", thread->tid, child);
	if(child == -1){
		VATAL_MSG("HBRuntime: create thread error!\n");
		exit(0);
		return -1;
	}
	thread->pid = child;
//#else
//	DEBUG_MSG("call real_pthread_create(%x)\n", real_pthread_create);
//	real_pthread_create(pid, attr, thread_entry_point, thread);
//#endif

	me->vclock.incClock(me->tid);

	//printf("--thread %d leave pthread_create\n", me->tid);

	if(singlethread){//changing from single thread to multithreads.
		DEBUG_MSG("In protectSharedData()\n");
		protectSharedData();
		DEBUG_MSG("Thread (%d): After protectSharedData()\n", me->tid);
	}
	else{
		//TODO: takeSnapshot & flushLog
		//beginSlice();
	}
	DEBUG_MSG("Thread (%d): After protectSharedData() ................\n", me->tid);
	NORMAL_MSG("Thread (%d) Create Thread (%d) OK!\n\n", me->tid, tid);
	return 0;
}

/*@Bug: cannot clear the structure unless all thread is unable to read the log from this thread.*/
int clearThreadStructures(int tid){
	thread_info_t* thread = &metadata->threads[tid];
	thread->tid = INVALID_THREAD_ID; /*Reallocate thread structure.*/
	return 0;
}

/*@Done: add a 'pid' field in thread struct.*/
int HBRuntime::threadJoin(pthread_t tid, void ** val){
	int status;
	NORMAL_MSG("Thread %d join thread %d\n", me->tid, tid);


	pid_t pid = metadata->threads[tid].pid;
	pid_t child = waitpid(pid, &status, 0);

	NORMAL_MSG("Thread %d join thread %d\n", me->tid, tid);

	/**
	 * TODO: we could merge the logs that are before join and after join. Hence, we do not need to
	 *      log the modifications and reprotect the shared data.
	 * */
	vector_clock old_time = me->vclock;
	//takeSnapshot();
	strategy->endSlice();
	//unprotectSharedData();
	
	me->vclock.incClock(me->tid);
	if(child != -1){
		DEBUG_MSG("Join-Happen-Before: Thread(%d) ---> Thread(%d)\n", tid, me->tid);
		thread_info_t* thread = &metadata->threads[tid];
		me->vclock.incClock(&thread->vclock, thread->tid);
		//printf("Join-Happen-Before: before resotreSnapshot: \n");
		int count = strategy->doPropagation(tid, &old_time, &me->vclock, SYNC_JOIN);
		DEBUG_MSG("Join-Happen-Before: Merge modifications: %d\n", count);
	}


	//global_twin_list->reprotect_pages();
	//global_twin_list->flush_diffs();/*Flush the twin pages, unmap them!*/

	//flushLogs();
	strategy->beginSlice();
	/*TODO: handle the exceptions!*/
	//printf("pthread_join finished: child = %d\n", child);
	clearThreadStructures(tid);
	DEBUG_MSG("Thread %d join finished!\n", me->tid);

	return 0;
}

int HBRuntime::threadCancel(pthread_t tid){
	//ASSERT(false, "threadCancel is not implemented!, tid = %d\n", tid)
	pid_t pid = metadata->threads[tid].pid;
	kill(pid, SIGKILL);
	return 0;
}

int HBRuntime::mutexInit(pthread_mutex_t* mutex, const pthread_mutexattr_t * attr){
	DEBUG_MSG("Init mutex: %p\n", mutex);
	metadata->ilocks.createMutex(mutex);
	return 0;
}


/**
 * @Thought: How to merge logs with different vector clocks? answer: when these is no path that will have different
 *			 happen-before relation with the paths of the merging logs!
 **/
int HBRuntime::mutexLock(pthread_mutex_t * mutex){
	DEBUG_MSG("Thread %d lock %x\n", me->tid, mutex);
	tryGC();
	me->insync = true;
	int ret = rawMutexLock(mutex, SYNC_LOCK, true);
	me->insync = false;
	return ret;
}

int HBRuntime::mutexTrylock(pthread_mutex_t * mutex){
	//ASSERT(false, "mutextTrylock is not implemented!");
	tryGC();
	me->insync = true;
	int ret = rawMutexLock(mutex, SYNC_TRYLOCK, true);
	me->insync = false;
	return ret;
}


/**
 * Fixme: What if a thread unlock a mutex which is not currently held by it ?
 * 		  Shall we write the owner of this mutex at unlock redundantly?
 */
int HBRuntime::mutexUnlock(pthread_mutex_t * mutex){
	me->insync = true;
	int ret = rawMutexUnlock(mutex, true);
	me->insync = false;
	tryGC();
	return ret;
}

int HBRuntime::condInit(pthread_cond_t* cond, const pthread_condattr_t *attr){
	DEBUG_MSG("Init cond: %p\n", cond);
	metadata->iconds.createCond(cond);
	return 0;
}

/**
 * @condWait
 **/
int HBRuntime::condWait(pthread_cond_t * cond, pthread_mutex_t * mutex){
	//InternalLock* lock = metadata->ilocks.FindOrCreateLock(mutex);

	me->insync = true;
	NORMAL_MSG("Thread %d calling condWait\n", me->tid);
	InternalCond* icond = metadata->iconds.findOrCreateCond(cond);
	icond->enterWaitQueue(me);

	//rawMutexUnlock(mutex);
	//takeSnapshot();
	strategy->endSlice();
	rawMutexUnlock(mutex, false);
	//flushLogs();


	/*There is no need to log difference, as no user codes are executed between the unlock and the lock.*/
	//me->tlog.logDiffs(); /*Log the diffs for this clock.*/
	//unprotectSharedData();
	vector_clock old_time = me->vclock;
	me->vclock.incClock(me->tid);
	//Change this branch to an assert: icond->sender != INVALID_THREAD_ID.
	//unprotectSharedData();
	tryGC();
	/*Wait here...*/
	NORMAL_MSG("Thread %d wait at cond(%x)\n", me->tid, cond);
	icond->Wait(me); /*Wait here!*/
	NORMAL_MSG("Thread %d wake up\n", me->tid);
	ASSERT(icond->sender != INVALID_THREAD_ID, "");

	/*Wake up!!!*/
	me->vclock.incClock(&icond->vtime, icond->sender);
	int count = strategy->doPropagation(icond->sender, &old_time, &me->vclock, HB_REASON_CONDWAIT);
	//printf("Cond Wait: Merge modifications: %d\n", count);

	DEBUG_MSG("Cond Happen-Before: thread %d----------------> thread %d\n", icond->sender, me->tid);
	DEBUG_MSG("Thread %d total merged modifications: %d\n", me->tid, count);
	//dump_data();
	//rawMutexLock(mutex);
	//printf("Thread(%d): in condwait, try to lock again\n", me->tid);
	rawMutexLock(mutex, SYNC_LOCK, false);

	//flushLogs();
	strategy->beginSlice();

	NORMAL_MSG("Thread(%d) comes out of condwait.\n", me->tid);
	me->insync = false;

	return 0;
}

/**
 * The number of threads which could be waked up by conSignal is [0, 1].
 */
int HBRuntime::condSignal(pthread_cond_t * cond){
	me->insync = true;
	NORMAL_MSG("Thread %d calls condSignal\n", me->tid);
	InternalCond* icond = metadata->iconds.findOrCreateCond(cond);
	thread_info_t* thread = icond->Signal();
	if(thread != NULL){
		//takeSnapshot();
		strategy->endSlice();
		icond->vtime = me->vclock;
		icond->sender = me->tid;
		icond->wakeup(thread);
	}
	NORMAL_MSG("Thread %d signal, wake up thread %d\n", me->tid, thread==NULL?(-1):thread->tid);
	me->vclock.incClock(me->tid);

	if(thread != NULL){
		//flushLogs();
		strategy->beginSlice();
	}
	me->insync = false;
	tryGC();
	return 0;
}

/**
 * The number of threads which could be waked up by condBroadcast is [0, n], where
 * 'n' is the number of threads that wait for this conditional variable.
 */
int HBRuntime::condBroadcast(pthread_cond_t * cond){
	me->insync = true;
	int count = 0;
	NORMAL_MSG("Thread %d calls condBroadcast\n", me->tid);
	//sleep(1);
	/*Get the wake up list. If the list is NULL, merge this log with the next slice*/
	InternalCond* icond = metadata->iconds.findOrCreateCond(cond);
	thread_info_t* front = icond->Broadcast();
	if(front != NULL){
		//takeSnapshot();
		strategy->endSlice();
		icond->vtime = me->vclock;
		icond->sender = me->tid;
	}

	while(front != NULL){
		icond->wakeup(front);
		front = front->next;
		count ++;
	}

	me->vclock.incClock(me->tid);/*Increase my logical time.*/

	/*If at least 1 thread is waked up, reprotect the shared data.*/
	if(count >= 1){
		//flushLogs();
		strategy->beginSlice();
	}
	NORMAL_MSG("Thread(%d): come out of broadcast. wake up %d threads\n", me->tid, count);
	me->insync = false;
	tryGC();
	return 0;
}


void HBRuntime::beforeBarrier(){
	me->vclock.incClock(me->tid);
	//RUNTIME::takeSnapshot();
	strategy->endSlice();
	printf("Thread %d enter barrier\n", me->tid);
}

void HBRuntime::barrierDeliver(){
	vector_clock old_time = me->vclock;

	for(int i = 0; i < MAX_THREAD_NUM; i++){
		if(i == me->tid) continue;
		thread_info_t* thread = &metadata->threads[i];
		if(thread->tid == INVALID_THREAD_ID || thread->finished){
			continue;
		}

		me->vclock.incClock(&thread->vclock, i);
		//Fixme: old_time decide the merging order of all modifications, i.e., whether they are identical
		//       in all threads, as the reviewer pointed out.
		strategy->doPropagation(i, &old_time, &me->vclock, SYNC_BARRIER);
	}
}

void HBRuntime::afterBarrier() {
	printf("Thread %d leaving barrier\n", me->tid);
	clearGC();
}

int HBRuntime::barrierImpl(int tnum){


	bool last = false;
	Util::spinlock(&metadata->barrierlock);
	metadata->barriercount ++;
	Util::unlock(&metadata->barrierlock);

	//int tnum = getThreadNum();
	//cout << "barrier: tnum = " << tnum << endl;
	while(!metadata->barrierfrontdoor){
		if(metadata->barriercount >= tnum){
			metadata->barrierbackdoor = 0;
			Util::syncbarrier();
			metadata->barrierfrontdoor = 1;
		}
	}

	barrierDeliver(); //restore the modifications.


	Util::spinlock(&metadata->barrierlock);
	metadata->barriercount --;
	if(metadata->barriercount == 0){
		metadata->barrierfrontdoor = 0;
		Util::syncbarrier();
		metadata->barrierbackdoor = 1;
		last = true;
	}
	Util::unlock(&metadata->barrierlock);

	while(!metadata->barrierbackdoor){}

	//afterBarrierHook();

	return last;
}

/*This structure is from pthreads, it may not be compatible*/
struct pthread_barrier
{
  unsigned int curr_event;
  int lock;
  unsigned int left;
  unsigned int init_count;
  int myprivate;
};


int HBRuntime::barrier_wait(pthread_barrier_t* barr){
	DEBUG_MSG("Thread %d calls pthread_barrier_wait\n", me->tid);

	me->insync = true;
	struct pthread_barrier * ibarrier = (struct pthread_barrier *) barr;
	//cout<<"barrier->size = " << ibarrier->left << endl;
	me->vclock.incClock(me->tid);
	//RUNTIME::takeSnapshot();
	strategy->endSlice();
	NORMAL_MSG("Thread %d enter barrier, tnum = %d\n", me->tid, ibarrier->left);

	//beforeBarrier();
	int last = barrierImpl(ibarrier->left);
	//afterBarrier();
	//ASSERT(false, "HBRuntime::barrier_wait has not been implemented yet.\n");

	me->vclock.incClock(me->tid);
	NORMAL_MSG("Thread %d leaving barrier\n", me->tid);
	clearGC();
	strategy->beginSlice();

	me->insync = false;
}

bool HBRuntime::isSharedMemory(void* addr){
	if(addr >= Heap::getHeap()->start() && addr <= Heap::getHeap()->end() ){
		return true;
	}
	else if(isInGlobals(addr)){
		return true;
	}
	return false;
}

void InternalCond::enterWaitQueue(thread_info_t* thread){
	thread->next = NULL;
	thread->pre = NULL;
	//SPINLOCK_LOCK(&lock);
	//printf("Thread(%d): cond wait...\n", me->tid);
	if(front == NULL){
		front = thread;
		back = thread;
	}
	else{
		back->next = thread;
		thread->pre = back;
		back = thread;
	}
	thread->wcond = this;
	//SPINLOCK_UNLOCK(&lock);
}

void InternalCond::Wait(thread_info_t* thread){
	/* Wait here.*/
	while(thread->wcond.load() != NULL){
		//printf("Cond(%d) wait for a while...\n", me->tid);
		Util::wait_for_a_while();
	}
}

thread_info_t* InternalCond::Broadcast(){
	SPINLOCK_LOCK(&lock);
	DEBUG_MSG("Thread(%d): broadcast...\n", me->tid);
	thread_info_t* thread = front;
	front = NULL;
	back = NULL;
	SPINLOCK_UNLOCK(&lock);
	return thread;
	//__sync_synchronize();
}


void * HBRuntime::malloc(size_t sz){
	if(tpdata->IsKernalMalloc()){
		return _metadata.meta_alloc(sz);
	}

	return Heap::getHeap()->malloc(sz);
}

void  HBRuntime::free(void * addr){
	Heap::getHeap()->free(addr);
}
void * HBRuntime::valloc(size_t sz){
	ASSERT(!tpdata->IsKernalMalloc(), "Could not be kernal malloc");
	return Heap::getHeap()->malloc(sz);
}
void * HBRuntime::calloc(size_t nmemb, size_t sz){
	if(tpdata->IsKernalMalloc()){
		void* ptr = _metadata.meta_alloc(sz * nmemb);
		return ptr;
	}
	return Heap::getHeap()->malloc(nmemb * sz);
}

void * HBRuntime::realloc(void * ptr, size_t sz){
	return Heap::getHeap()->realloc(ptr, sz);
}

SyncPolicy* HBRuntime::createSyncPolicy(){	
	void* buf = _metadata.meta_alloc(sizeof(NondetSyncPolicy));
	SyncPolicy* sp = new (buf) NondetSyncPolicy;
	return sp;
}

int HBRuntime::protectGlobals(){
	mprotect((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ);
	return 0;
}

int HBRuntime::unprotectGlobals(){
	mprotect((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ|PROT_WRITE);
	return 0;
}

bool HBRuntime::isInGlobals(void* addr){
	size_t value = (size_t)addr;
	if(value >= GLOBALS_START && value < GLOBALS_END){
		return true;
	}
	return false;
}


void HBRuntime::init(){
	this->printWelcomeMsg();
	
	strategy->init();
	Heap* heap = Heap::getHeap();
	//void* heap = (void*)getHeap();
	if(heap == NULL){
		fprintf(stderr, "HBDet: initialize heap error!\n");
		exit(0);
	}
	DEBUG_MSG("Heap Init OK...\n");

	//initConstants(heap->start());
	DEBUG_MSG("HBDet: before init_real_functions\n");
	init_real_functions();
	
	DEBUG_MSG("HBDet: before init_mainthread\n");
	initMainthread();
	
	//std::cout << "Linux Page Size = " << getpagesize() << std::endl;
	ASSERT(PAGE_SIZE == getpagesize() && PAGE_SIZE == sysconf(_SC_PAGESIZE), "Page size configuration error!")
	//metadata = this->getMetadata();
	
	syncpolicy = createSyncPolicy();
	
}

RRRuntime::RRRuntime(std::string file, int mode) : logfile (file){
	//std::cerr << "Running with Record&Replay System" << std::endl;
	this->setRunningMode(mode);
}

SyncPolicy* RRRuntime::createSyncPolicy(){
	//std::cerr << "Calling RRRuntime::createSyncPolicy() to create Record&Replay Sync Policy" << std::endl;
	void* buf = this->getMetadata()->meta_alloc(sizeof(RRSyncPolicy));
	SyncPolicy* sp = new (buf) RRSyncPolicy(logfile);
	return sp;
}

int RRRuntime::threadExit(){
	HBRuntime::threadExit();
	SyncPolicy* sp = this->getSyncPolicy();
	RRSyncPolicy* rrsp = dynamic_cast<RRSyncPolicy*>(sp);
	rrsp->closeLogFile(me->tid);
}

void RRRuntime::printResult(){
	uint64_t elapsetime = Util::watch_time();
	std::cout << "Prelock optimization: " << 
				(double)me->paracount / (me->paracount + me->serialcount) << std::endl;
	std::cout << "Program elapse time: " << elapsetime << std::endl;
}

void RRRuntime::printWelcomeMsg(){
	std::string mode;
	if(this->IsRecording()){
		mode = "Recording...";
	}
	else if(this->IsReplaying()){
		mode = "Replaying...";
	}
	std::cout << "Running program " << 
				this->getAppname() << " with " << mode << std::endl;
}

DMTRuntime::DMTRuntime(){
	this->setRunningMode(Mode_DMT);
}
