/*
 * writeset.cpp
 * This file is used to monitor modifications (write set) of user applications
 *  Created on: Nov 19, 2012
 *      Author: zhouxu
 */
#include <string.h>
#include "writeset.h"
#include "defines.h"
#include "runtime.h"
#include "common.h"
#include "heaps.h"

AddressMap::AddressMap() {
	// TODO Auto-generated constructor stub
	pageNum = 0;
}

AddressMap::~AddressMap() {
	// TODO Auto-generated destructor stub
}

size_t AddressMap::find(void* addr){
	size_t pageid = (size_t)addr >> LOG_PAGE_SIZE;
	AddressPage* page = pages[pageid];
	size_t index = ((size_t)addr & PAGE_MASK__) >> 2;
	return page->value[index];
}

#define ADDR_MAP_MASK (PAGE_MASK__ - 3)
bool AddressMap::insert(void* addr, size_t len){
	size_t pageid = (size_t)addr >> LOG_PAGE_SIZE;
	AddressPage* page = pages[pageid];
	if(page == NULL){
		page = (AddressPage*)metadata->allocPage();
		//ASSERT(page != NULL, "")
		memset(page, 0, PAGE_SIZE);
		pages[pageid] = page;
		pageNum ++;
	}
	size_t index = ((size_t)addr & PAGE_MASK__) >> 2;
	page->value[index] = len;

	return true;
}

void AddressMap::clear(){
	for(int i = 0; i < pageNum; i++){
		size_t pageid = writtenPages[i];
		pages[pageid] = NULL;
	}
	pageNum = 0;
}

void AddressMap::reset(){
	memset(pages, 0, PAGE_COUNT * 4);
}

//extern void* shared_data_low;
void AddressMap::dumpSharedRegion(void* pagestart, void* pageend, AddressStack* stack){

	for(char* p = (char*)pagestart; p < (char*)pageend; p += PAGE_SIZE){
		int pageid = (size_t)p >> LOG_PAGE_SIZE;
		AddressPage* page = pages[pageid];
		if(page == NULL){
			continue;
		}

		for(int j = 0; j < PAGE_SIZE; j++){
			if(page->value[j] == 0){
				continue;
			}
			size_t index = j;
			size_t vaddr = (size_t)p | index;
			//DEBUG_MSG("Thread %d try to push_back a value to stack %x\n", me->tid, stack);
			stack->push_back((void*)vaddr, page->value[index]);
		}
	}

}

int AddressMap::dump(AddressStack* stack){

	DEBUG_MSG("Thread %d try to dump write set\n", me->tid);
	void* heapstart = Heap::getHeap()->start();
	void* heapend = Heap::getHeap()->end();
	dumpSharedRegion(heapstart, heapend, stack);
	dumpSharedRegion((void*)GLOBALS_START, (void*)GLOBALS_END, stack);

	/*
	for(size_t i = 0; i < PAGE_COUNT; i ++){
		if(pages[i] == NULL){
			continue;
		}
		//DEBUG_MSG("Thread %d find a page that contains writes\n", me->tid);
		AddressPage* page = pages[i];
		size_t pageaddr = i << LOG_PAGE_SIZE;
		//if(!RUNTIME::isSharedMemory((void*)pageaddr)){
		if(pageaddr < (size_t)shared_data_low){
			continue;
		}
		for(int j = 0; j < PAGE_SIZE; j++){
			if(page->value[j] == 0){
				continue;
			}
			size_t index = j;
			size_t vaddr = pageaddr | index;
			//DEBUG_MSG("Thread %d try to push_back a value to stack %x\n", me->tid, stack);
			stack->push_back((void*)vaddr, page->value[index]);
		}

	}
	*/
	//DEBUG_MSG("Thread %d finishes dumping write set\n", me->tid);
	return 0;
}

AddressMap writeSet;
