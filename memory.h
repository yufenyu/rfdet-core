/*
 * memory.h
 *
 *  Created on: Apr 11, 2012
 *      Author: zhouxu
 */

#ifndef MEMORY_H_
#define MEMORY_H_
#include <sys/mman.h>
#include "defines.h"
#include "utils.h"
#include "common.h"

/**
 * The abstract class for managing a continuous memory regions.
 */
class Memory{

public:

	virtual void* start() = 0;

	virtual void* end() = 0;

	virtual size_t size() = 0;

	virtual size_t used() = 0;

};





/*Implement snapshot memory, using bitmap to describe the allocation.*/
template<size_t TotalSize, size_t ChunkSize, int ThreadNum>
class PageSizeMemory : public Memory{

#define PAGES_IN_CHUNK (ChunkSize / PAGE_SIZE)

	struct page_data_t{
		char data[PAGE_SIZE];
	};

	struct chunk_data_t{
		char data[ChunkSize];
	};
	struct chunk_meta_t{
		int owner;
		int freeSlot;
		//int usedPages;
		page_data_t* start;
		/*TODO: using 1 bit to indicate the allocation: 1 means allocated, 0 means free*/
		char pagemap[PAGES_IN_CHUNK];
	public:
		inline void* allocPageFast(){
			if(freeSlot < PAGES_IN_CHUNK){
				int pageid = freeSlot;
				freeSlot ++;
				//usedPages ++;
				pagemap[pageid] = 1;
				return &start[pageid];
			}
			return NULL;
		}
	};

	template<int PagesInChunk>
	class LocalAllocator{
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
			PageMemory():pre(NULL),
						next(NULL){
				cursor = &data;
				limit = (void*)PAGE_ALIGN_UP(cursor);
			}

			inline void* malloc(size_t size){
				void* ptr = cursor;
				if(cursor + size <= limit){
					cursor += size;
					return ptr;
				}
				return NULL;
			}
			inline void free(void* ptr){

			}
			inline void* dataStart(){
				return &data;
			}
			inline void* dataLimit(){
				return cursor;
			}
			inline void* pageLimit(){
				return limit;
			}
		};
	protected:
		chunk_meta_t* chunkAllocator;

	public:
		//int owner;
		PageMemory* freelistHead;
		PageMemory* freelistTail;
		uint64 allocatedpages;
		LocalAllocator(){
			reset();
		}

		inline void reset(){
			//owner = tid;
			freelistHead = NULL;
			freelistTail = NULL;
			chunkAllocator = NULL;
			allocatedpages = 0;
		}
		void setChunk(chunk_meta_t* achunk){
			ASSERT(achunk != NULL, "setChunk: newchunk == NULL")
			if(chunkAllocator != NULL){
				void* page = chunkAllocator->allocPageFast();
				while(page != NULL){
					this->freePage(page);
					page = chunkAllocator->allocPageFast();
				}
			}
			chunkAllocator = achunk;
			//ASSERT(chunkAllocator->owner == me->tid, "ownership of chunk do not match!");
		}

		void freeChunk(void* chunk, int pagecount){
			page_data_t* pages = (page_data_t*)chunk;
			for(int i = 0; i < pagecount; i++){
				void* page = &pages[i];
				freePage(page);
			}
		}

		void freePage(void* page){
			//ASSERT(owner == me->tid, "")
			if(freelistHead == NULL){
				freelistHead = (PageMemory*)page;
				freelistTail = (PageMemory*)page;
			}
			else{
				freelistTail->next = (PageMemory*)page;
				freelistTail = (PageMemory*)page;
			}
			freelistTail->next = NULL;
		}
		void* allocPage(){
			//ASSERT(owner == me->tid, "")
			if(freelistHead != NULL){
				void* ret = freelistHead;
				freelistHead = freelistHead->next;
				return ret;
			}

			if(chunkAllocator != NULL){
				return chunkAllocator->allocPageFast();
			}
			return NULL;
		}
		void* allocPages(int n){
			if(chunkAllocator == NULL){
				return NULL;
			}
			else if(chunkAllocator->freeSlot + n >= PagesInChunk){
				return NULL;
			}

			int pagestartid = chunkAllocator->freeSlot;
			int pageendid = pagestartid + n;
			for(int i = pagestartid; i < pageendid; i ++){
				chunkAllocator->pagemap[i] = 1;
			}
			chunkAllocator->freeSlot = pageendid;
			//myallocator.chunkAllocator->usedPages += n;
			return &chunkAllocator->start[pagestartid];
		}
	};

public:
	int lock;
	int usedPageCount;
	//int totalPageNum;
	int totalChunkNum;
	int freeCursor;
	//int usedChunkCount;
	//int bitmap[TOTAL_PAGE_COUNT];
	LocalAllocator<PAGES_IN_CHUNK> myallocator[ThreadNum];
	chunk_meta_t chunkmap[TotalSize/ChunkSize];
	//page_data_t* freePages;
	chunk_data_t* chunks;

	//chunk_data_t* currentChunk;
public:
	PageSizeMemory():lock(0),
					freeCursor(0){
		DEBUG_MSG("Creating Shared Memory...\n");

		void* mapped = mmap(NULL, TotalSize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		ASSERT(mapped != NULL, "Cannot allocate memory!")

		/* initialize chunk meta data. */
		chunks = (chunk_data_t*)mapped;
		totalChunkNum = TotalSize / ChunkSize;
		for(int i = 0; i < totalChunkNum; i++){
			chunkmap[i].owner = INVALID_THREAD_ID;
			chunkmap[i].freeSlot = 0;
			chunkmap[i].start = (page_data_t*)&chunks[i];
			memset(chunkmap[i].pagemap, 0, PAGES_IN_CHUNK);
		}

	}


	chunk_meta_t* acquireOneChunk(){
		chunk_meta_t* ret = NULL;
		//printf("totalChunkNum = %d\n", totalChunkNum);
		for(int i = 0; i < totalChunkNum; i++){
			if(chunkmap[i].owner != INVALID_THREAD_ID){
				continue;
			}
			Util::spinlock(&lock);
			if(chunkmap[i].owner == INVALID_THREAD_ID){
				chunkmap[i].owner = CurrThreadID();
				ret = &chunkmap[i];
				Util::unlock(&lock);
				break;
			}
			Util::unlock(&lock);

		}

		ASSERT(ret != NULL, "run out of chunks!\n")
		return ret;
	}

	void* allocOnePage(){
		int tid = CurrThreadID();
		void* ret = myallocator[tid].allocPage();

		if(ret != NULL){
			return ret;
		}

		//if(gcpoll()){
			//RUNTIME::GC();
			//me->snapshotLog.GC();
		//}

		chunk_meta_t* achunk = this->acquireOneChunk();
		ASSERT(achunk != NULL, "Thread(%d): run out of snapshot memories\n", CurrThreadID())

		myallocator[tid].setChunk(achunk);
		//myallocator.freeChunk(achunk, PAGES_IN_CHUNK);
		ret = myallocator[tid].allocPage();
		return ret;
	}

	void freePage(void* ptr){
		int tid = CurrThreadID();
		myallocator[tid].freePage(ptr);
	}

	bool inSpace(void* addr){
		if(addr > chunks && addr < &chunks[totalChunkNum]){
			return true;
		}
		return false;
	}

	/*@description: free all the memories the thread allocated when it finishes.*/
	void freeAll(int tid){
		int count = 0;
		//Util::spinlock(&lock);
		for(int i = 0; i < totalChunkNum; i ++){
			if(chunkmap[i].owner == tid){
				chunkmap[i].owner = INVALID_THREAD_ID;
				chunkmap[i].freeSlot = 0;
				count ++;
				memset(chunkmap[i].pagemap, 0, PAGES_IN_CHUNK);
			}
		}
	}

	int addressToID(void* ptr, int* chunkid, int* pageid){
		*chunkid = (int)((size_t)ptr - (size_t)chunks) / ChunkSize;
		size_t remainsize = ((size_t)ptr - (size_t)chunks) % ChunkSize;
		*pageid = remainsize / PAGE_SIZE;
		ASSERT(*chunkid >= 0 && *chunkid < totalChunkNum &&
				*pageid >= 0 && *pageid < PAGES_IN_CHUNK, "Cannot convert address(%x) to chunk_id or page_id!", ptr)
		return 0;
	}

	inline void initOnThreadEntry(int tid){
		myallocator[tid].reset();
	}

	virtual void* start(){
		return chunks;
	}
	virtual void* end(){
		return chunks + totalChunkNum;
	}
	virtual size_t used(){
		size_t usedchunknum = 0;
		for(int i = 0; i < totalChunkNum; i++){
			if(chunkmap[i].owner != INVALID_THREAD_ID){
				usedchunknum ++;
			}
		}
		return usedchunknum * ChunkSize;
	}

	virtual size_t size(){
		return totalChunkNum * ChunkSize;
	}

};

//extern RuntimeMemory * runtimememory;


template<size_t TotalSize>
class ImmutableMemory : public Memory {
private:
	char* _data;
	char* _limit;
	char* _cursor;
public:
	ImmutableMemory(){
		void* mapped = mmap(NULL, TotalSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		ASSERT(mapped != NULL, "")
		_data = (char*)mapped;
		_cursor = _data;
		_limit = (char*)_data + TotalSize;
	}

	void * malloc(size_t sz){
		if(_cursor + sz < _limit){
			void* ret = _cursor;
			_cursor += sz;
			return ret;
		}
		return NULL;
	}
	virtual void* start(){
		return _data;
	}

	virtual void* end(){
		return _data + TotalSize;
	}

	virtual size_t size(){
		return TotalSize;
	}

	virtual size_t used(){
		return (size_t)_cursor - (size_t)_data;
	}
};


//#define snapshot_memory runtimememory

#endif /* MEMORY_H_ */
