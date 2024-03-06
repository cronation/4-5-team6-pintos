/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define DON_NEST_DEPTH 8 // lock에서의 priority donation 최대 재귀 깊이

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */
void
sema_down (struct semaphore *sema) {
	// printf("[DBG] sema_down(): down by thread name = %s\n", thread_current()->name); /////////////////
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		list_push_back (&sema->waiters, &thread_current ()->elem);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
// void
// sema_up (struct semaphore *sema) {
// 	enum intr_level old_level;

// 	ASSERT (sema != NULL);

// 	old_level = intr_disable ();
// 	if (!list_empty (&sema->waiters))
// 		thread_unblock (list_entry (list_pop_front (&sema->waiters),
// 					struct thread, elem));
// 	sema->value++;
// 	intr_set_level (old_level);
// }

// P1-PS
// waiters중에서 가장 donated priority가 높은 쓰레드를 선택해 unblock
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();

	
	// printf("[DBG] sema_up(): sema.value increased by thread name = %s\n", thread_current()->name); /////////////////
	struct thread *t = thread_highest_in_list(&sema->waiters);
	thread_test(14);

	/////////////////
	// struct thread *t;
	// if (list_empty (&sema->waiters))
	// 	t = NULL;
	// else
	// 	t = list_entry (list_front(&sema->waiters), struct thread, elem);
	
	// printf("[DBG] sema_up(): {%s} wants to up sema\n", thread_current()->name); //////////////

	if (t != NULL) {
		// printf("[DBG] sema_up(): highest waiter is {%s} with don pri = %d\n", t->name, thread_don_priority(t, 0)); ////////////
		ASSERT(t->elem.next != NULL);
		ASSERT(t->elem.prev != NULL);
		list_remove(&t->elem);
		thread_unblock(t);
	}
	sema->value++;

	intr_set_level (old_level);

	thread_yield();

	// if (!intr_context())
	// 	thread_yield();
	
	// if (!list_empty (&sema->waiters)) {
	// 	struct list_elem *e, *iter_e;
	// 	struct thread *t, *iter_t;
	// 	int temp_pri, max_pri = PRI_MIN -1;

	// 	for (iter_e = list_begin(&sema->waiters);
	// 		iter_e != list_end(&sema->waiters); iter_e = list_next(e)) {
	// 		// 모든 watiers 탐색
	// 		iter_t = list_entry(iter_e, struct thread, elem);
	// 		// temp_pri = thread_don_priority(iter_t, 0);
	// 		temp_pri = iter_t->priority;
	// 		if (temp_pri > max_pri) {
	// 			max_pri = temp_pri;
	// 			e = iter_e;
	// 			t = iter_t;
	// 		}
	// 	}

	// 	// printf("[DBG] sema_up(): max_pri = %d (from thread name = %s)\n", max_pri, t->name); ////////////////
	// 	list_remove(e);
	// 	thread_unblock(t);
	// }

	// intr_set_level (old_level);

	// thread_yield();
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));

	sema_down (&lock->semaphore);
	lock->holder = thread_current ();
	// 쓰레드가 hold중인 lock의 list에 삽입 (P1-PS)
	list_push_back(&(thread_current()->lock_list), &lock->elem);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
		// 쓰레드가 hold중인 lock의 list에 삽입 (P1-PS)
		list_push_back(&(thread_current()->lock_list), &lock->elem);
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));

	list_remove(&lock->elem); // P1-PS
	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem {
	struct list_elem elem;              /* List element. */
	struct semaphore semaphore;         /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	list_push_back (&cond->waiters, &waiter.elem);
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
// void
// cond_signal (struct condition *cond, struct lock *lock UNUSED) {
// 	ASSERT (cond != NULL);
// 	ASSERT (lock != NULL);
// 	ASSERT (!intr_context ());
// 	ASSERT (lock_held_by_current_thread (lock));

// 	if (!list_empty (&cond->waiters))
// 		sema_up (&list_entry (list_pop_front (&cond->waiters),
// 					struct semaphore_elem, elem)->semaphore);
// }

// P1-PS
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters)) {
		// 가장 높은 priority를 가진 쓰레드를 선택
		struct thread *t, *iter_t;
		struct semaphore_elem *se, *iter_se;
		int temp_pri, max_pri = PRI_MIN -1;

		struct list_elem *iter_e;
		for (iter_e = list_begin(&cond->waiters);
				iter_e != list_end(&cond->waiters); iter_e = list_next(iter_e)) {
			iter_se = list_entry(iter_e, struct semaphore_elem, elem);
			iter_t = list_entry(list_begin(&iter_se->semaphore.waiters), struct thread, elem);
			temp_pri = thread_don_priority(iter_t, 0);
			if (temp_pri > max_pri) {
				t = iter_t;
				se = iter_se;
				max_pri = temp_pri;
			} else if (temp_pri == max_pri && iter_t->priority > t->priority) {
				// donate받은 priority가 같으므로 기존 priority를 비교
				t = iter_t;
				se = iter_se;
			}
		}
		list_remove(se);
		sema_up(&se->semaphore);
	}
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
