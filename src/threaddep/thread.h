 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Threading support, using libretro-common rthreads
  *
  * Copyright 1997, 2001 Bernd Schmidt
  */

#ifndef UAE_THREADDEP_THREAD_H
#define UAE_THREADDEP_THREAD_H

#include "rthreads/rthreads.h"

/* Semaphores. rthreads has no counting semaphore, so uae_semaphore is
 * built on an slock/scond pair in thread.cpp with POSIX-like semantics. */
struct uae_semaphore;
typedef struct uae_semaphore *uae_sem_t;

int uae_sem_init (uae_sem_t *sem, int dummy, int init);
void uae_sem_destroy (uae_sem_t *sem);
int uae_sem_post (uae_sem_t *sem);
int uae_sem_wait (uae_sem_t *sem);
int uae_sem_trywait (uae_sem_t *sem);
int uae_sem_getvalue (uae_sem_t *sem);

#include "commpipe.h"

typedef sthread_t *uae_thread_id;
#define BAD_THREAD 0

long uae_start_thread (const TCHAR *name, int (*f) (void *), void *arg, uae_thread_id *tid);
long uae_start_thread_fast (void *(*f) (void *), void *arg, uae_thread_id *tid);
void uae_wait_thread (uae_thread_id thread);
void uae_end_thread (uae_thread_id *tid);

STATIC_INLINE void uae_set_thread_priority (uae_thread_id *id, int pri)
{
}

/* Do nothing; thread exits if thread function returns.  */
#define UAE_THREAD_EXIT do {} while (0)

#endif /* UAE_THREADDEP_THREAD_H */
