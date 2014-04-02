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

//size_t AddressMap::find(void* addr){
//	size_t pageid = (size_t)addr >> LOG_PAGE_SIZE;
//	AddressPage* page = pages[pageid];
//	size_t index = ((size_t)addr & PAGE_MASK__) >> 2;
//	return page->value[index];
//}
/*
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
*/
void AddressMap::clear(){
	//for(int i = 0; i < pageNum; i++){
		//size_t pageid = writtenPages[i];
		//pages[pageid] = NULL;
	//}
	pageNum = 0;
}

//void AddressMap::reset(){
	//memset(pages, 0, PAGE_COUNT * 4);
//}


AddressMap writeSet;
