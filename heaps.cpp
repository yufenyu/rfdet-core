/*
 * heap.cpp
 *
 *  Created on: Apr 14, 2012
 *      Author: zhouxu
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <new>
#include <string.h>

#include "heaps.h"
#include "defines.h"
#include "runtime.h"
#include "heaplayers.h"


Heap* Heap::getHeap(){
	static void* buf = mmap(0, PAGE_ALIGN_UP(sizeof(MyHeap)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	ASSERT(buf != NULL, "")
	static Heap* heap = new (buf) MyHeap;
	return heap;
}

using namespace HL;

class TopHeap : public SizeHeap<UniqueHeap<ZoneHeap<MmapHeap, 65536> > > {};

class TheCustomHeapType :
  public ANSIWrapper<KingsleyHeap<AdaptHeap<DLList, TopHeap>, TopHeap> > {
  public:
	  TheCustomHeapType(){
		  cout<<"Constructor of The CustomHeapType()"<<endl;
	  }
};


inline static TheCustomHeapType * getCustomHeap (void) {
  static char thBuf[sizeof(TheCustomHeapType)];
  static TheCustomHeapType * th = new (thBuf) TheCustomHeapType;
  return th;
}




//class ZHeapDemo : public zThreadHeap<ZPerthreadHeap> {};
//class ZThreadHeap : public zThreadHeap<ZperthreadHeap> {};
//class ZHeapDemo : public zSegHeap<zThreadHeap<ZPerthreadHeap>, OneHeapSource> {};

ZHeapDemo * getHeap (void) {
	//static char thBuf[sizeof(ZHeapDemo)];
	static char* thBuf = (char*)mmap(NULL, sizeof(ZHeapDemo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	assert(thBuf != NULL);
	//printf("heap meta data = %x\n", thBuf);
	static ZHeapDemo * th = new (thBuf) ZHeapDemo;
	return th;
}


Memory* MyBigHeapSource::_instance = NULL;

/*
extern "C"{

	void* zmalloc(size_t sz){
		return getHeap()->malloc(sz);
	}

	void zfree(void* ptr){
		getHeap()->free(ptr);
	}
}
*/
