/*
 * modification.cpp
 *
 *  Created on: May 9, 2013
 *      Author: zhouxu
 */

#include "modification.h"
#include "runtime.h"

DiffModification::DiffModification() {
	// TODO Auto-generated constructor stub
	phead = NULL;
	pcursor = NULL;
	//slice =s;
	//lock = 0;
}

DiffModification::~DiffModification() {
	// TODO Auto-generated destructor stub
}


int DiffModification::record(AddressMap* ws){
	DEBUG_MSG("Thread %d record diffs: pageNum = %d\n", me->tid, ws->getDirtyPageNum());
	int total = ws->getDirtyPageNum();
	for(int i = 0; i < total; i ++){
		address_t pageaddr = ws->getDirtyPage(i);
		AddressPage* page = (AddressPage*)ws->getDirtyPageSnapshot(i);
		
		DEBUG_MSG("Thread %d: record page addr = %d\n", me->tid, pageaddr);
		ASSERT(page != NULL, "tid = %d, pageaddr = %x, tpage = %p\n", me->tid, pageaddr, page)
		
		calcPageDiffs(page, (void*)pageaddr);
		metadata->freePage(page);
	}
	DEBUG_MSG("Thread %d record diffs OK\n", me->tid);
	return total;
}

void DiffModification::resetIterator(LogIterator* iter){
	iter->pageIterator = phead;
	iter->iterator = 0;
	iter->iteratorLimit = 0;
}

/*
extern void* record_addr;
extern size_t record_size;
int is_in_data(void* addr){
	size_t index = (size_t)addr;
	size_t base = (size_t)record_addr;
	size_t cursor = (index - base);
	if(cursor < record_size && cursor >= 0){
		return cursor;
	}
	return -1;
}
*/


int DiffModification::restore(){
	LogIterator iter;
	resetIterator(&iter);
	//A_diff_t* diff = getNext(&logIterator);
	//printf("HBRuntime: merge log(%x)\n", flog);
	int count = 0;
	while(iter.pageIterator != NULL){

		iter.iterator = (A_diff_t*)iter.pageIterator->dataStart();
		iter.iteratorLimit = (A_diff_t*)(iter.pageIterator->dataLimit());
		while(iter.iterator < iter.iteratorLimit){
			count ++;
			A_diff_t* diff = iter.iterator;
			#if (DIFF_GRANULARITY_CONFIG == _GRANULARITY_BYTE_SIZE)
			//#if defined ( BYTE_SIZE_DIFF )
			//if(configs::DIFF_GRANULARITY == configs::BYTE_SIZE_DIFF){
			char* addr = (char*)diff->addr;
			char value = diff->value;
			//signal_up ++;
			*addr = value;
			//signal_up --;
			//#elif defined (INT_SIZE_DIFF)
			//}
			//else if(configs::DIFF_GRANULARITY == configs::BYTE_SIZE_DIFF){
			#elif (DIFF_GRANULARITY_CONFIG == _GRANULARITY_INT_SIZE)
			//#ifdef INT_SIZE_DIFF
			int* addr = (int*)diff->addr;
			int value = diff->value;
			*addr = value;
			//int tmp = is_in_data(addr);
			//if(tmp != -1){
				//fprintf(stderr, "Thread %d merge data item %d (%x) to %x\n", me->tid, tmp/4, addr, value);
			//}
			//#endif
			//}
			#endif
			iter.iterator += 1;
		}

		iter.pageIterator = iter.pageIterator->next;
		//printf("HBRuntime: in iterations(%d)\n", i);

		//ASSERT(true, "Error: addr = NULL\n")
		//diff = getNext(&logIterator);
	}

	return count;
}

void DiffModification::push_diff(void* addr, int value){
	A_diff_t* adiff = NULL;

	//int tmp = is_in_data(addr);
	//if(tmp != -1){
		//fprintf(stderr, "Thread %d record data item %d (%x) to %x\n", me->tid, tmp/4, addr, value);
	//}
	//DEBUG_MSG("Thread %d push_diff: addr = %x\n", me->tid, addr);
	if(pcursor == NULL){
		//push_diff_fast(addr, value);
		//return;
		push_diff_slow(addr, value);
		return;
	}
	adiff = (A_diff_t* )pcursor->malloc(sizeof(A_diff_t));
	if(adiff == NULL){
		push_diff_slow(addr, value);
		return;
	}
	push_diff_fast(adiff, addr, value);
}

inline void DiffModification::push_diff_fast(A_diff_t* adiff, void* addr, int value){
	adiff->addr = addr;
	adiff->value = value;
}

/**
 * @Description: (1) allocate a page (struct log_page_t) in the LogMemory space
 *               (2) push the new log(address, value) into the stack
 */
void DiffModification::push_diff_slow(void* addr, int value){
	//printf("DiffLog: push_diff_slow\n");
	//printf("DiffLog: push_diff_slow(%x, %d), log_data = %x\n", addr, value, log_data);
	//ASSERT(log_data != NULL, "")
	//log_data->allocPage(1);
	void* tmpbuf = metadata->allocPage();
	if(tmpbuf == NULL){
		//GC();
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
	//printf("Difflog::push_diff_slow: page = %x\n", page);
	//cursor = (A_diff_t*)&page->data;
	//limit = (A_diff_t*)(page + PAGE_SIZE);
	A_diff_t* adiff = (A_diff_t*)pcursor->malloc(sizeof(A_diff_t));
	push_diff_fast(adiff, addr, value);
}

/**
 * @param tpage: twin page that stores the unmodified page
 * @param wpage: working page that stores the modified page.
 */
int DiffModification::calcPageDiffs(void* tpage, void* wpage){
	//std::cout << "tpage = " << tpage << ", wpage = " << wpage << std::endl;
	//int* out = (int*)res;
	if(tpage == 0 || wpage == 0){
		VATAL_MSG("tpage = %p, wpage = %p\n", tpage, wpage);
		exit(0);
	}
	int j = 0;
//#if defined (BYTE_SIZE_DIFF)
#if (DIFF_GRANULARITY_CONFIG == _GRANULARITY_BYTE_SIZE)
	for(int i = 0; i < PAGE_SIZE; i ++){
		if(((char*)tpage)[i] != ((char*)wpage)[i]){
			char* add_ptr = (char*)wpage + i;
			push_diff(add_ptr, *add_ptr);
			j ++;
		}
	}
#elif(DIFF_GRANULARITY_CONFIG == _GRANULARITY_INT_SIZE)
//#elif defined (INT_SIZE_DIFF)
//#ifdef INT_SIZE_DIFF
	int* org = (int*)tpage;
	int* curr = (int*)wpage;
	int* limit = (int*)((char*)tpage + PAGE_SIZE);
	while(org < limit){
		if(*org != *curr){
			push_diff(curr, *curr);
			j++;
		}
		org += 1;
		curr += 1;
	}
#endif

	return j;
}

int DiffModification::freeDataImpl(PageMemory* page){
	int count = 0;
	while(page != NULL){
		PageMemory* tmp = page;
		page = page->next;
		//log_data->freePage(tmp);
		metadata->freePage(tmp);
		count ++;
	}
	return count;
}

int DiffModification::freeData(){
	//Util::spinlock(&lock);
	PageMemory* page = phead;
	phead = NULL;
	//Util::unlock(&lock);
	int count = freeDataImpl(page);
	pcursor = NULL;
}
