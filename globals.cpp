/*
 * hb_globals.cpp
 *
 *  Created on: Apr 9, 2012
 *      Author: zhouxu
 */


#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"


int init_globals(){
#ifdef SOLO_COPYONWRITE
	printf("init globals\n");
	char* tmpbuf = (char *) mmap(NULL, GLOBALS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	ASSERT(tmpbuf != NULL, "mmap failed\n")
	memcpy(tmpbuf, (void *)GLOBALS_START, GLOBALS_SIZE);

	char* globals = (char *) mmap((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	ASSERT(globals == (char *)GLOBALS_START, "mmap shared failed, <globals = %x, GLOBALS_START = %x>\n",
			globals, GLOBALS_START)
	memcpy(globals, tmpbuf, GLOBALS_SIZE);

#endif
	//mprotect((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ);
	NORMAL_MSG("Init globals: GLOBALS_START = %x, GLOBALS_END = %x\n", GLOBALS_START, GLOBALS_START + GLOBALS_SIZE);
	return 0;
}

/* TODO: clear these data structures.
int allocated_global_count = 0;
void* allocated_global_ptrs[10];
size_t allocated_global_length[10];
void add_global_data(void* ptr, size_t length){
	allocated_global_ptrs[allocated_global_count] = ptr;
	allocated_global_length[allocated_global_count] = length;
	allocated_global_count ++;
}
*/

/*TODO: the for iteration seems to be useless.*/
int protect_globals(){
	mprotect((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ);
	//for(int i = 0; i < allocated_global_count; i++){
		//mprotect((void *)allocated_global_ptrs[i], allocated_global_length[i], PROT_READ);
	//}
	return 0;
}

int unprotect_globals()
{
	mprotect((void *)GLOBALS_START, GLOBALS_SIZE, PROT_READ|PROT_WRITE);
	//for(int i = 0; i < allocated_global_count; i++){
		//mprotect((void *)allocated_global_ptrs[i], allocated_global_length[i], PROT_READ|PROT_WRITE);
	//}
	return 0;
}

bool is_in_globals(void* addr){
	size_t value = (size_t)addr;
	if(value >= GLOBALS_START && value < GLOBALS_END){
		return true;
	}
	return false;
}


struct global_private_meta_data * pma;

int init_globalprivate()
{

	void * a = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(a==(void*)-1)
	{
		printf("error mmap in threadprivate.c\n");
		_exit(1);
	}
	pma=(struct global_private_meta_data *)a;

	(pma->start)=pma + 1;
	(pma->last)=pma->start;
	return 0;
}

void * alloc_global_private(int size)
{
	//printf("before alloc:  %x----%x\n",pma->start,pma->last);

	if((unsigned int)(pma->last) + size - (unsigned int)(pma->start) > 4096)
	{
		printf("error in threadprivate.cpp\n");
		_exit(1);
	}
	void * ret = pma->last;
	pma->last = (char*)pma->last + size;

	//printf("after alloc:  %x----%x\n",pma->start,pma->last);
	return ret;
}
