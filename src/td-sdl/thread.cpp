 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Threading support, using libretro-common rthreads
  *
  * Copyright 1997, 2001 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "td-sdl/thread.h"

/* Counting semaphore on an slock/scond pair. */
struct uae_semaphore
{
	slock_t *lock;
	scond_t *cond;
	int count;
};

int uae_sem_init (uae_sem_t *sem, int dummy, int init)
{
	struct uae_semaphore *s = (struct uae_semaphore *)calloc (1, sizeof (*s));

	if (!s)
		return -1;
	s->lock = slock_new ();
	s->cond = scond_new ();
	if (!s->lock || !s->cond) {
		if (s->cond)
			scond_free (s->cond);
		if (s->lock)
			slock_free (s->lock);
		free (s);
		return -1;
	}
	s->count = init;
	*sem = s;
	return 0;
}

void uae_sem_destroy (uae_sem_t *sem)
{
	struct uae_semaphore *s = *sem;

	if (!s)
		return;
	scond_free (s->cond);
	slock_free (s->lock);
	free (s);
	*sem = NULL;
}

int uae_sem_post (uae_sem_t *sem)
{
	struct uae_semaphore *s = *sem;

	slock_lock (s->lock);
	s->count++;
	scond_signal (s->cond);
	slock_unlock (s->lock);
	return 0;
}

int uae_sem_wait (uae_sem_t *sem)
{
	struct uae_semaphore *s = *sem;

	slock_lock (s->lock);
	while (s->count <= 0)
		scond_wait (s->cond, s->lock);
	s->count--;
	slock_unlock (s->lock);
	return 0;
}

int uae_sem_trywait (uae_sem_t *sem)
{
	struct uae_semaphore *s = *sem;
	int ret = -1;

	slock_lock (s->lock);
	if (s->count > 0) {
		s->count--;
		ret = 0;
	}
	slock_unlock (s->lock);
	return ret;
}

int uae_sem_getvalue (uae_sem_t *sem)
{
	struct uae_semaphore *s = *sem;
	int v;

	slock_lock (s->lock);
	v = s->count;
	slock_unlock (s->lock);
	return v;
}

/* Threads. rthreads entry points return void; the UAE ones return int
 * or void*, so the call goes through a trampoline. A thread nobody can
 * join (no id handed back) is detached at once so rthreads reclaims it
 * when the function returns. */
struct uae_thread_start
{
	int (*f_int) (void *);
	void *(*f_ptr) (void *);
	void *arg;
};

static void uae_thread_trampoline (void *data)
{
	struct uae_thread_start start = *(struct uae_thread_start *)data;

	free (data);
	if (start.f_int)
		start.f_int (start.arg);
	else
		start.f_ptr (start.arg);
}

static long uae_thread_launch (struct uae_thread_start *start, uae_thread_id *tid)
{
	sthread_t *t = sthread_create (uae_thread_trampoline, start);

	if (!t) {
		free (start);
		if (tid)
			*tid = BAD_THREAD;
		return BAD_THREAD;
	}
	if (tid)
		*tid = t;
	else
		sthread_detach (t);
	return (long)(size_t)t;
}

long uae_start_thread (const TCHAR *name, int (*f) (void *), void *arg, uae_thread_id *tid)
{
	struct uae_thread_start *start = (struct uae_thread_start *)calloc (1, sizeof (*start));

	if (!start) {
		if (tid)
			*tid = BAD_THREAD;
		return BAD_THREAD;
	}
	start->f_int = f;
	start->arg = arg;
	return uae_thread_launch (start, tid);
}

long uae_start_thread_fast (void *(*f) (void *), void *arg, uae_thread_id *tid)
{
	struct uae_thread_start *start = (struct uae_thread_start *)calloc (1, sizeof (*start));

	if (!start) {
		if (tid)
			*tid = BAD_THREAD;
		return BAD_THREAD;
	}
	start->f_ptr = f;
	start->arg = arg;
	return uae_thread_launch (start, tid);
}

void uae_wait_thread (uae_thread_id thread)
{
	if (thread)
		sthread_join (thread);
}

/* The caller has already synchronised with the thread's shutdown and
 * will not join it; let rthreads free it once the function returns. */
void uae_end_thread (uae_thread_id *tid)
{
	if (tid && *tid) {
		sthread_detach (*tid);
		*tid = BAD_THREAD;
	}
}
