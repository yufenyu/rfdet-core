/*
 * =====================================================================================
 *
 *       Filename:  hook.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/09/2012 04:53:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  (ZHOUXU),
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef _HOOK_H_
#define _HOOK_H_

//#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <pthread.h>

#include <dlfcn.h>


// libc functions
extern void* (*real_mmap)(void*, size_t, int, int, int, off_t);
extern void* (*real_malloc)(size_t);
extern void* (*real_valloc)(size_t);
extern void  (*real_free)(void *);
extern void* (*real_realloc)(void *, size_t);
extern void* (*real_memalign)(size_t, size_t);
extern size_t (*real_malloc_usable_size)(void *);
extern ssize_t (*real_read)(int, void*, size_t);
extern ssize_t (*real_write)(int, const void*, size_t);
extern int (*real_sigwait)(const sigset_t*, int*);

// pthread basics
extern int (*real_pthread_create)(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
extern int (*real_pthread_cancel)(pthread_t);
extern int (*real_pthread_join)(pthread_t, void**);
extern void (*real_pthread_exit)(void*);
extern int (*real_pthread_attr_init)(pthread_attr_t *attr);
extern int (*real_pthread_attr_destroy)(pthread_attr_t *attr);


// pthread mutexes
extern int (*real_pthread_mutexattr_init)(pthread_mutexattr_t*);
extern int (*real_pthread_mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*);
extern int (*real_pthread_mutex_lock)(pthread_mutex_t*);
extern int (*real_pthread_mutex_unlock)(pthread_mutex_t*);
extern int (*real_pthread_mutex_trylock)(pthread_mutex_t*);
extern int (*real_pthread_mutex_destroy)(pthread_mutex_t*);

// pthread condition variables
extern int (*real_pthread_condattr_init)(pthread_condattr_t*);
extern int (*real_pthread_cond_init)(pthread_cond_t*, const pthread_condattr_t*);
extern int (*real_pthread_cond_wait)(pthread_cond_t*, pthread_mutex_t*);
extern int (*real_pthread_cond_signal)(pthread_cond_t*);
extern int (*real_pthread_cond_broadcast)(pthread_cond_t*);
extern int (*real_pthread_cond_destroy)(pthread_cond_t*);

// pthread barriers
extern int (*real_pthread_barrier_init)(pthread_barrier_t*, pthread_barrierattr_t*, unsigned int);
extern int (*real_pthread_barrier_wait)(pthread_barrier_t*);
extern int (*real_pthread_barrier_destroy)(pthread_barrier_t*);


void init_real_functions();

#endif

