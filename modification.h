/*
 * modification.h
 *
 *  Created on: May 9, 2013
 *      Author: zhouxu
 */

#ifndef MODIFICATION_H_
#define MODIFICATION_H_
#include "writeset.h"
//#include "heaps.h"
//#include "slice.h"
class PageMemory{
public:
	struct PageMemory* pre;
	struct PageMemory* next;
	//int count;
private:
public://for debugging
	void* cursor;
	void* limit;
	int data;
public:
	void init(){
		pre = NULL;
		next = NULL;
		//count = 0;
		cursor = &data;
		limit = (void*)PAGE_ALIGN_UP(cursor);
	}
	PageMemory(){
		init();
	}

	inline void* malloc(size_t size){
		void* ptr = cursor;
		if((char*)cursor + size <= (char*)limit){
			cursor = (char*)cursor + size;
			return ptr;
		}
		return NULL;
	}

	void* dataStart(){
		return &data;
	}
	void* dataLimit(){
		return cursor;
	}
	void* pageLimit(){
		return limit;
	}
};

typedef struct _A_diff_t{
	void* addr;
	int value;
}A_diff_t;

class HBIterator{
public:
	int i;
	HBIterator(){i = 0;}
	int inc(){i ++; return i;};
};

class LogIterator: public HBIterator{
public:
	A_diff_t* iterator;
	A_diff_t* iteratorLimit;
	PageMemory* pageIterator;
	LogIterator(){
		pageIterator = NULL;
		iterator = 0;
		iteratorLimit = 0;
	}
};

class Modification {
public:
	virtual int record(AddressMap* ws){return 0;}
	virtual int restore(){return 0;}
	virtual int freeData(){return 0;};
};

class DiffModification : public Modification{
private:
	//Slice* slice;
	PageMemory* phead;
	PageMemory* pcursor;
	//int lock;
public:
	DiffModification();
	virtual ~DiffModification();

protected:
	void resetIterator(LogIterator* iter);
	int calcPageDiffs(void* tpage, void* wpage);
	void push_diff(void* addr, int value);
	void push_diff_fast(A_diff_t* adiff, void* addr, int value);
	void push_diff_slow(void* addr, int value);
	int freeDataImpl(PageMemory* page);

	/*Currently, this function is unused. */
	void push_back(void* addr, size_t len){
		//DEBUG_MSG("Thread push_back\n");
		ASSERT(len >=0 && len <= 8, "")
		//switch(len){
		//case 1:
		//}

		for(int i = 0; i < len; i ++){
			char* p = (char*)addr + i;
			push_diff(p, *p);
		}
	}

public:
	virtual int record(AddressMap* ws);
	virtual int restore();
	virtual int freeData();

};

#endif /* MODIFICATION_H_ */
