#ifndef HEAPS_H_
#define HEAPS_H_

#include <stdlib.h>
#include <unistd.h>
#include "memory.h"
#include "defines.h"
#include "heaps.h"
#include "utils.h"

#include "heaplayers.h"

/**
 * @Description:
 */

/**
 * External functions.
 */
//int init_heaps();

class Heap : public Memory {
	//static Heap* s_heap;
public:
	virtual void* malloc(size_t size) = 0;
	virtual void* valloc(size_t size) = 0;
	virtual void* realloc(void* ptr, size_t size) = 0;
	virtual void free(void* prt) = 0;

	//virtual bool isInHeap(void* mem);
	//static Heap* appHeap();
	//static Heap* initHeap();
	static Heap* getHeap();

	virtual void protect_heap() {
		VATAL_MSG("protect_heap is not implemented for Heap!")
		;
		exit(0);
	}
	virtual void unprotect_heap() {
		VATAL_MSG("unprotect_heap is not implemented for Heap!")
		;
		exit(0);
	}
	virtual void reprotectHeap() {
		VATAL_MSG("unprotect_heap is not implemented for Heap!")
		;
		exit(0);
	}

};

namespace HBDet {

inline size_t class2Size(const int i) {
	size_t sz = (size_t) (1UL << (i+3));
	return sz;
}

inline int size2Class(const size_t sz) {
	int cl = HL::ilog2 ((sz < 8) ? 8 : sz) - 3;
	return cl;
}

enum {NUMBINS = 29};

}

template<class SuperHeap> class ProtectHeap : SuperHeap {
public:
	enum {Alignment = SuperHeap::Alignment};

	inline void* malloc(size_t sz) {
		void* ret = SuperHeap::malloc(sz);
		if (!IsSingleThread()) {
			int err = mprotect(ret, PAGE_ALIGN_UP(sz), PROT_READ);
			ASSERT(err == 0, "err != 0\n");
		}
		return ret;
	}

	inline void free(void * ptr) {
		SuperHeap::free(ptr);
		//mprotect(ptr, sz, PROT_READ | PROT_WRITE);
	}
};

template<class SuperHeap> class MyUniqueHeap {
public:

	enum {Alignment = SuperHeap::Alignment};

	/**
	 * Ensure that the super heap gets created,
	 * and add a reference for every instance of unique heap.
	 */
	MyUniqueHeap(void) {
		volatile SuperHeap * forceCreationOfSuperHeap = getSuperHeap();
		addRef();
	}

	/**
	 * @brief Delete one reference to the unique heap.
	 * When the number of references goes to zero,
	 * delete the super heap.
	 */
	~MyUniqueHeap(void) {
		int r = delRef();
		if (r <= 0) {
			getSuperHeap()->SuperHeap::~SuperHeap();
		}
	}

	// The remaining public methods are just
	// thin wrappers that route calls to the static object.

	inline void * malloc(size_t sz) {
		return getSuperHeap()->malloc(sz);
	}

	inline void free(void * ptr) {
		getSuperHeap()->free(ptr);
	}

	inline size_t getSize(void * ptr) {
		return getSuperHeap()->getSize(ptr);
	}

	inline int remove(void * ptr) {
		return getSuperHeap()->remove(ptr);
	}

	inline void clear(void) {
		getSuperHeap()->clear();
	}

private:

	/// Add one reference.
	void addRef(void) {
		getRefs() += 1;
	}

	/// Delete one reference count.
	int delRef(void) {
		getRefs() -= 1;
		return getRefs();
	}

	/// Internal accessor for reference count.
	int& getRefs(void) {
		static int numRefs = 0;
		return numRefs;
	}

	SuperHeap * getSuperHeap(void) {
		//static char superHeapBuffer[sizeof(SuperHeap)];
		static char* superHeapBuffer = (char*)mmap(NULL, sizeof(SuperHeap),
				PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		static SuperHeap * aSuperHeap = new (superHeapBuffer) SuperHeap;
		return aSuperHeap;
	}

	void doNothing() {
	}
};

template<size_t MY_HEAP_SIZE, int MY_CHUNK_SIZE> class BigHeapSource :
	public Memory {
	typedef unsigned int MapItemType;
private:

	MapItemType _heapmeta[MY_HEAP_SIZE/MY_CHUNK_SIZE];
	MapItemType* map;
	MapItemType* map_end;
	char* data;

#define MASK_1_000 0x80000000
#define MASK_0_111 0x7fffffff
	//#define MASK_0_111 ((uintptr_t(-1)) >> 1)
	//#define MASK_1_000 (~(MASK_0_111))
#define CHUNKS_IN_HEAP (MY_HEAP_SIZE/MY_CHUNK_SIZE)
public:
	enum {Alignment = MmapWrapper::Alignment};
	//static BigHeapSource* _instance;

public:
	BigHeapSource() :
		data(NULL), map(NULL), map_end(NULL) {
		void* raw_mem = mmap(NULL, MY_HEAP_SIZE , PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		ASSERT(raw_mem != MAP_FAILED, "mmap failed");
		init(raw_mem);

		DEBUG_MSG("Init BigHeapSource, raw_mem = %p, MASK_0_111 = %p.\n", raw_mem, (void*)MASK_0_111);
	}

	virtual void* start() {
		return data;
	}
	virtual void* end() {
		return data + MY_HEAP_SIZE;
	}

	virtual size_t size() {
		return MY_HEAP_SIZE;
	}

	virtual size_t used() {
		ASSERT(false, "not implemented!")
	}

	void init(void* rawdata) {
		size_t metasize= CHUNKS_IN_HEAP * sizeof(MapItemType);
		//size_t datasize = CHUNKS_IN_HEAP * MY_CHUNK_SIZE;
		map = _heapmeta;
		map_end = map + CHUNKS_IN_HEAP;
		data = (char*)rawdata;
		//map = (MapItemType*)rawdata;
		//map_end = map + (datasize / MY_CHUNK_SIZE);
		memset(map, 0, metasize);
		setLen(map, CHUNKS_IN_HEAP);
	}

	inline void * malloc(size_t sz) {
		ASSERT(data != NULL, "malloc failed, data = NULL\n");
		MapItemType* next = map;
		while (next != map_end) {
			//printf("getSize(next) = %x\n", getSize(next));
			if (getSize(next) >= sz) {
				void* data = split(next, sz);
				return data;
			}
			next = getNext(next);
		}
		//printf("In malloc\n");
		ASSERT(false, "Can not allocate more memory!")
		return NULL;

	}

	inline size_t getSize(MapItemType* code) {
		if (!isEmpty(code)) {
			return 0;
		}
		return getLen(code) * MY_CHUNK_SIZE;
	}

	inline int getLen(MapItemType* code) {
		return (*code) & MASK_0_111;
	}

	inline void setLen(MapItemType* code, int len) {
		*code = (*code & MASK_1_000) + (len & MASK_0_111);
	}

	inline void setUsed(MapItemType* code) {
		*code = (*code) | MASK_1_000;
	}

	inline void setEmpty(MapItemType* code) {
		*code = (*code) & MASK_0_111;
	}

	inline bool isEmpty(MapItemType* code) {
		return (*code & MASK_1_000) == 0;
	}

	inline MapItemType* getNext(MapItemType* code) {
		return code + getLen(code);
	}

	inline void* getData(MapItemType* code) {
		//int len = getLen(code);
		char* ret = data + ((uintptr_t)code - (uintptr_t)map)
				/ sizeof(MapItemType)* MY_CHUNK_SIZE;
		return ret;
	}

	inline void* split(MapItemType* code, size_t sz) {
		int wanted = sz / MY_CHUNK_SIZE;
		int len = getLen(code);
		if (wanted < len) {
			MapItemType* newpos = code + wanted;
			setLen(newpos, len - wanted);
			setEmpty(newpos);
		}

		setLen(code, wanted);
		setUsed(code);
		return getData(code);
	}

	inline void free(void *) {
		//not implemented
	}
};

class MyBigHeapSource : public BigHeapSource<HEAP_SIZE, PAGE_SIZE> {
	static Memory* _instance;
public:
	MyBigHeapSource() {
		//Super();
		ASSERT(_instance == NULL, "_instance is NULL");
		_instance = this;
	}

	static Memory* getInstance() {
		return _instance;
	}

};
/**Just copy codes from heaplayers, and using CurrThreadID() instead of CPUinfo::getThreadID().*/
template<int NumHeaps, class PerThreadHeap> class MyThreadHeap :
	public PerThreadHeap {
public:

	enum {Alignment = PerThreadHeap::Alignment};

	inline void * malloc(size_t sz) {
		unsigned int tid = CurrThreadID();
		assert (tid >= 0);
		assert (tid < NumHeaps);
		return getHeap(tid)->malloc(sz);
	}

	inline void free(void * ptr) {
		unsigned int tid = CurrThreadID();
		;
		assert (tid >= 0);
		assert (tid < NumHeaps);
		getHeap(tid)->free(ptr);
	}

	inline size_t getSize(void * ptr) {
		unsigned int tid = CurrThreadID();
		;
		assert (tid >= 0);
		assert (tid < NumHeaps);
		return getHeap(tid)->getSize(ptr);
	}

private:

	// Access the given heap within the buffer.
	inline PerThreadHeap * getHeap(int index) {
		assert (index >= 0);
		assert (index < NumHeaps);
		return &ptHeaps[index];
	}

	PerThreadHeap ptHeaps[NumHeaps];

};

class MyLockType {
	int _ilock;
public:
	MyLockType() :
		_ilock(0) {
		//printf("_lock(%x) = %d\n", &_ilock, _ilock);
		//_lock = 0;
	}
	inline void lock() {
		//printf("Calling MyLockType::lock, _lock(%x) = %d\n", &_ilock, _ilock);
		Util::spinlock(&_ilock);
		//printf("_lock = %d\n", _ilock);
	}
	inline void unlock() {
		Util::unlock(&_ilock);
	}
};

template<class SuperHeap, size_t ChunkSize> 
class MyZoneHeap : public SuperHeap {
public:

	enum {Alignment = SuperHeap::Alignment};

	MyZoneHeap(void) :
		_sizeRemaining(-1), _currentArena(NULL) {
	}

	~MyZoneHeap(void) {
	}

	inline void * malloc(size_t sz) {
		void * ptr = zoneMalloc (sz);
		assert ((size_t) ptr % Alignment == 0);
		return ptr;
	}

	/// Free in a zone allocator is a no-op.
	inline void free(void *) {
	}

	/// Remove in a zone allocator is a no-op.
	inline int remove(void *) {
		return 0;
	}

private:

	MyZoneHeap(const MyZoneHeap&);
	MyZoneHeap& operator=(const MyZoneHeap&);

	inline void * zoneMalloc(size_t sz) {
		void * ptr;
		// Round up size to an aligned value.
		sz = HL::align<HL::MallocInfo::Alignment>(sz);
		// Get more space in our arena if there's not enough room in this one.
		if ((_currentArena == NULL) || (_sizeRemaining < (int) sz)) {
			// First, add this arena to our past arena list.
			
			// Now get more memory.
			size_t allocSize = ChunkSize;
			if (allocSize < sz) {
				allocSize = sz;
			}
			_currentArena =
			(char*)SuperHeap::malloc (allocSize);
			//printf("_currentArena = %p\n", _currentArena);
			if (_currentArena == NULL) {
				return NULL;
			}
			//_currentArena->arenaSpace = (char *) (_currentArena + 1);
			//_currentArena->nextArena = NULL;
			_sizeRemaining = ChunkSize;
		}
		// Bump the pointer and update the amount of memory remaining.
		_sizeRemaining -= sz;
		ptr = _currentArena;
		_currentArena += sz;
		assert (ptr != NULL);
		assert ((size_t) ptr % SuperHeap::Alignment == 0);
		return ptr;
	}

	/// Space left in the current arena.
	long _sizeRemaining;

	/// The current arena.
	char * _currentArena;

};

template<class PerClassHeap, class BigHeap> class zSegHeap :
	public StrictSegHeap<HBDet::NUMBINS,
		HBDet::size2Class,
		HBDet::class2Size,
		PerClassHeap,
		BigHeap> {
};

template<class PerthreadHeap> class zThreadHeap :
	public MyThreadHeap<MAX_THREAD_NUM, PerthreadHeap> {
};

class OneHeapSource :
	public MyUniqueHeap<ProtectHeap<LockedHeap<MyLockType, MyBigHeapSource > > > {
};
//class
class LocalHeapSource :
	public SizeHeap<MyZoneHeap<OneHeapSource, HEAP_CHUNK_SIZE> > {
};

class GlobalHeapSource : public SizeHeap<OneHeapSource> {
};

class ZPerthreadHeap :
	public zSegHeap<AdaptHeap<DLList, LocalHeapSource>, LocalHeapSource> {
};

class ZHeapDemo :
	public ANSIWrapper<HybridHeap<HEAP_CHUNK_SIZE, zThreadHeap<ZPerthreadHeap>, GlobalHeapSource > > {

public:
	ZHeapDemo() {
	}
};

ZHeapDemo * getHeap(void);

class MyHeap : public Heap {

	ZHeapDemo hlheap;
	Memory* _memory;

public:

	MyHeap() :
		_memory(NULL) {
		_memory = MyBigHeapSource::getInstance();
		ASSERT(_memory != NULL, "_memory is NULL");
	}

	virtual void* malloc(size_t sz) {
		return hlheap.malloc(sz);
	}

	virtual void* realloc(void* ptr, size_t size) {
		return hlheap.malloc(size);
	}

	virtual void* valloc(size_t size) {
		return hlheap.malloc(size);
	}

	virtual void free(void* ptr) {
		hlheap.free(ptr);
	}

	virtual void protect_heap() {
		mprotect(_memory->start(), _memory->size(), PROT_READ);
	}
	virtual void unprotect_heap() {
		mprotect(_memory->start(), _memory->size(), PROT_READ | PROT_WRITE);
	}
	virtual void reprotectHeap() {
		ASSERT(false, "not implemented!\n")
	}

	virtual void* start() {
		return _memory->start();
	}
	virtual void* end() {
		return _memory->end();
	}
	virtual size_t size() {
		return _memory->size();
	}
	virtual size_t used() {
		return _memory->used();
	}
};

#endif /* HEAPS_H_ */
