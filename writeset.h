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

//#include "snapshot.h"
#define USER_SPACE_SIZE 0xc0000000 //3GB user space.
#define PAGE_COUNT (USER_SPACE_SIZE >> LOG_PAGE_SIZE)

class AddressStack {
public:
	virtual int push_back(void* addr, size_t len) = 0;
};

class AddressPage {
public:
	char value[PAGE_SIZE];
};

#define MAX_WRITTEN_PAGE 64000
//extern SnapshotMemory * snapshot_memory;
class AddressMap {
private:
	int tid;

public:
	int pageNum;
	size_t writtenPages[MAX_WRITTEN_PAGE];
	AddressPage* pages[PAGE_COUNT];
public:
	AddressMap();
	virtual ~AddressMap();
	size_t find(void* addr);
	bool insert(void* addr, size_t len);
	void clear();
	void reset();
	int dump(AddressStack* stack);
	bool isEmpty(){
		return pageNum == 0;
	}
	void initOnThreadEntry(int tid){
		this->tid = tid;
		pageNum = 0;
		reset();
	}
	void writePage(size_t pageid){
		//ASSERT(pageid != NULL, "")
		writtenPages[pageNum] = pageid;
		pageNum ++;
		ASSERT(pageNum < MAX_WRITTEN_PAGE, "pageNum = %d\n", pageNum)
	}

	void dumpSharedRegion(void* pagestart, void* pageend, AddressStack* stack);
};

#endif /* ADDRESSMAP_H_ */
