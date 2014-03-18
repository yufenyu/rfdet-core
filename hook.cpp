/*
 * =====================================================================================
 *
 *       Filename:  hook.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/09/2012 05:09:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  (ZHOUXU),
 *        Company:  
 *
 * =====================================================================================
 */

#include "hook.h"

// libc functions
void* (*real_mmap)(void*, size_t, int, int, int, off_t);
void* (*real_malloc)(size_t);
void* (*real_valloc)(size_t);
void  (*real_free)(void *);
void* (*real_realloc)(void *, size_t);
void* (*real_memalign)(size_t, size_t);
size_t (*real_malloc_usable_size)(void *);
ssize_t (*real_read)(int, void*, size_t);
ssize_t (*real_write)(int, const void*, size_t);
int (*real_sigwait)(const sigset_t*, int*);


// pthread basics
int (*real_pthread_create)(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
int (*real_pthread_cancel)(pthread_t);
int (*real_pthread_join)(pthread_t, void**);
void (*real_pthread_exit)(void*);
int (*real_pthread_attr_init)(pthread_attr_t *attr);
int (*real_pthread_attr_destroy)(pthread_attr_t *attr);


// pthread mutexes
int (*real_pthread_mutexattr_init)(pthread_mutexattr_t*);
int (*real_pthread_mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*);
int (*real_pthread_mutex_lock)(pthread_mutex_t*);
int (*real_pthread_mutex_unlock)(pthread_mutex_t*);
int (*real_pthread_mutex_trylock)(pthread_mutex_t*);
int (*real_pthread_mutex_destroy)(pthread_mutex_t*);


// pthread condition variables
int (*real_pthread_condattr_init)(pthread_condattr_t*);
int (*real_pthread_cond_init)(pthread_cond_t*, pthread_condattr_t*);
int (*real_pthread_cond_wait)(pthread_cond_t*, pthread_mutex_t*);
int (*real_pthread_cond_signal)(pthread_cond_t*);
int (*real_pthread_cond_broadcast)(pthread_cond_t*);
int (*real_pthread_cond_destroy)(pthread_cond_t*);

// pthread barriers
int (*real_pthread_barrier_init)(pthread_barrier_t*, pthread_barrierattr_t*, unsigned int);
int (*real_pthread_barrier_wait)(pthread_barrier_t*);
int (*real_pthread_barrier_destroy)(pthread_barrier_t*);


void init_real_functions()
{
	real_mmap=(typeof(real_mmap))dlsym(RTLD_NEXT,"mmap");
	real_malloc=(typeof(real_malloc))dlsym(RTLD_NEXT,"malloc");
	real_valloc=(typeof(real_valloc))dlsym(RTLD_NEXT,"valloc");
	real_free=(typeof(real_free))dlsym(RTLD_NEXT,"free");
	real_realloc=(typeof(real_realloc))dlsym(RTLD_NEXT,"realloc");
	real_memalign=(typeof(real_memalign))dlsym(RTLD_NEXT,"memalign");
	real_malloc_usable_size=(typeof(real_malloc_usable_size))dlsym(RTLD_NEXT,"malloc_usable_size");
	real_read=(typeof(real_read))dlsym(RTLD_NEXT,"read");
	real_write=(typeof(real_write))dlsym(RTLD_NEXT,"write");
	real_sigwait=(typeof(real_sigwait))dlsym(RTLD_NEXT,"sigwait");
	
	void *pthread_handle = dlopen("libpthread.so.0", RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
	if (pthread_handle == NULL) {
		printf("Unable to load libpthread.so.0\n");
		_exit(2);
	}
	
	real_pthread_create=(typeof(real_pthread_create))dlsym(pthread_handle,"pthread_create");
	//real_pthread_create=dlsym(RTLD_NEXT,"pthread_create");
	real_pthread_cancel=(typeof(real_pthread_cancel))dlsym(pthread_handle,"pthread_cancel");
	real_pthread_join=(typeof(real_pthread_join))dlsym(pthread_handle,"pthread_join");
	real_pthread_exit=(typeof(real_pthread_exit))dlsym(pthread_handle,"pthread_exit");
	real_pthread_attr_init = (typeof(real_pthread_attr_init))dlsym(pthread_handle,"pthread_attr_init");
	real_pthread_attr_destroy = (typeof(real_pthread_attr_destroy))dlsym(pthread_handle,"pthread_attr_destroy");

	real_pthread_mutex_init=(typeof(real_pthread_mutex_init))dlsym(pthread_handle,"pthread_mutex_init");
	real_pthread_mutex_lock=(typeof(real_pthread_mutex_lock))dlsym(pthread_handle,"pthread_mutex_lock");
	real_pthread_mutex_unlock=(typeof(real_pthread_mutex_unlock))dlsym(pthread_handle,"pthread_mutex_unlock");
	real_pthread_mutex_trylock=(typeof(real_pthread_mutex_trylock))dlsym(pthread_handle,"pthread_mutex_trylock");
	real_pthread_mutex_destroy=(typeof(real_pthread_mutex_destroy))dlsym(pthread_handle,"pthread_mutex_destroy");
	real_pthread_mutexattr_init=(typeof(real_pthread_mutexattr_init))dlsym(pthread_handle,"pthread_mutexattr_init");

	real_pthread_condattr_init=(typeof(real_pthread_condattr_init))dlsym(pthread_handle,"pthread_condattr_init");
	real_pthread_cond_init=(typeof(real_pthread_cond_init))dlsym(pthread_handle,"pthread_cond_init");
	real_pthread_cond_wait=(typeof(real_pthread_cond_wait))dlsym(pthread_handle,"pthread_cond_wait");
	real_pthread_cond_signal=(typeof(real_pthread_cond_signal))dlsym(pthread_handle,"pthread_cond_signal");
	real_pthread_cond_broadcast=(typeof(real_pthread_cond_broadcast))dlsym(pthread_handle,"pthread_cond_broadcast");
	real_pthread_cond_destroy=(typeof(real_pthread_cond_destroy))dlsym(pthread_handle,"pthread_cond_destroy");

	real_pthread_barrier_init=(typeof(real_pthread_barrier_init))dlsym(pthread_handle,"pthread_barrier_init");
	real_pthread_barrier_wait=(typeof(real_pthread_barrier_wait))dlsym(pthread_handle,"pthread_barrier_wait");
	real_pthread_barrier_destroy=(typeof(real_pthread_barrier_destroy))dlsym(pthread_handle,"pthread_barrier_destroy");

}


