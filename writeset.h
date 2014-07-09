/*
 * AddressMap.h
 *
 *  Created on: Nov 19, 2012
 *      Author: zhouxu
 */

#ifndef ADDRESSMAP_H_
#define ADDRESSMAP_H_

#include "defines.h"
#include "utils.h"
#include "common.h"

//#include "snapshot.h"
//#define USER_SPACE_SIZE 0xc0000000 //3GB user space.
//#define PAGE_COUNT (USER_SPACE_SIZE >> LOG_PAGE_SIZE)

class AddressStack {
public:
	virtual int push_back(void* addr, size_t len) = 0;
};

class AddressPage {
public:
	char value[PAGE_SIZE];
};

class PageSnapshot{
public:
	address_t pageaddr; //the original page 
	address_t snapshot; //the snapshot page
};

#define MAX_WRITTEN_PAGE 64000
//extern SnapshotMemory * snapshot_memory;
class AddressMap {
private:
	int tid;

private:
	int pageNum;
	PageSnapshot dirtypages[MAX_WRITTEN_PAGE];
	//AddressPage* pages[PAGE_COUNT];
public:
	AddressMap() : pageNum(0){}
	virtual ~AddressMap(){}
	//size_t find(void* addr);
	//bool insert(void* addr, size_t len);
	void clear(){
		pageNum = 0;
	}
	//void reset();
	//int dump(AddressStack* stack);
	bool isEmpty(){
		return pageNum == 0;
	}
	
	void initOnThreadEntry(int tid){
		this->tid = tid;
		pageNum = 0;
		//reset();
	}
	
	void writePage(address_t pageaddr, address_t spage){
		ASSERT(pageaddr != (address_t)NULL, "pageaddr = %p\n", (void*)pageaddr)
		dirtypages[pageNum].pageaddr = pageaddr;
		dirtypages[pageNum].snapshot = spage;
		pageNum ++;
		ASSERT(pageNum < MAX_WRITTEN_PAGE, "pageNum = %d\n", pageNum)
	}

	uint32_t getDirtyPageNum(){
		return pageNum;
	}
	
	address_t getDirtyPage(uint32_t index){
		return dirtypages[index].pageaddr;
	}
	
	address_t getDirtyPageSnapshot(uint32_t index){
		return dirtypages[index].snapshot;
	}
	//void dumpSharedRegion(void* pagestart, void* pageend, AddressStack* stack);
};

#endif /* ADDRESSMAP_H_ */
