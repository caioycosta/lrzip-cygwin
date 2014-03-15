/*
Ticket lock implementation

In the original Lrzip code, there are locks that are
locked on a thread and unlocked on child threads.

This is undefined behavior of pthreads. because of this it does not run on windows.
So I've implmented this locking mechanism, which enable a thread to emit "tickets",
that can be used to control the order in which threads will acquire a lock.

This way, a thread can "yield" the lock to another, in a safe manner.


Possible error locations are commented with "recoverable" if error is explicitly  not fatal,
or "unrecoverable" if error is fatal (__may__ leave the lock in an unspecified state)
*/

#include "ticket_lock.h"
#include <errno.h>

int lckf (ticket_lock * lck)
{
  return pthread_mutex_lock (&lck->mutex);
}

int ulckf (ticket_lock * lck)
{
  return pthread_mutex_unlock (&lck->mutex);
}

int wtf (ticket_lock * lck)
{
  return pthread_cond_wait (&lck->cond, &lck->mutex);
}

int sigf (ticket_lock * lck)
{
  return pthread_cond_signal (&lck->cond);
}

int brdf (ticket_lock * lck)
{
  return pthread_cond_broadcast (&lck->cond);
}

// if fail, just returns, and just return if fail.
#define TRYEXEC(lk, fun) do{int ret = fun(lk); if (ret) return ret;}while(0)

// unlocks the mutex and returns the specified code
#define FAIL(lk, cd) do{ TRYEXEC(lk, ulckf); return cd;}while(0)

// if fail, try unlocking the mutex before returning
#define TRY(lk, fun) do{int ret = fun(lk); if (ret) FAIL(lk,ret);}while(0)

// these calls will not unlock the mutex if failed, 
// simply returning the error number
#define SUCCESS(a) FAIL(a,0)
#define BEGIN(a) TRYEXEC(a,lckf)

// these calls will unlock the mutex if failed
#define WAIT(a)      TRY(a,wtf)
#define SIGNAL(a)    TRY(a,sigf)
#define BROADCAST(a) TRY(a,brdf)

int init_tkt_lock (ticket_lock * lck)
{
  int ret;
  ret = pthread_cond_init (&lck->cond, NULL);
  if (ret != 0)
    return ret;       	// unrecoverable
  ret = pthread_mutex_init (&lck->mutex, NULL);
  if (ret != 0)
    return ret;		// unrecoverable
  lck->owner = PTHREAD_NULL;
  lck->locked = 0;
  lck->ticket_active = 0;
  lck->next_ticket = 0;
  lck->last_issued_ticket = 0;
  return 0;
}

int lock_tkt_lock (ticket_lock * lck)
{
  BEGIN (lck);	// unrecoverable

  pthread_t current = pthread_self ();

  if (pthread_equal (current, lck->owner) && !lck->ticket_active)
    FAIL(lck,EDEADLK);	// recoverable (only side effect is the lock is not taken)

  while (lck->locked || lck->ticket_active) {
    WAIT (lck);		// unrecoverable 
  }

  lck->locked = 1;
  lck->owner = current;

  SUCCESS (lck);	// unrecoverable
}

int unlock_tkt_lock (ticket_lock * lck)
{
  BEGIN (lck);		// unrecoverable

  pthread_t current = pthread_self ();

  if (!pthread_equal (current, lck->owner))
    FAIL(lck, EPERM);    		// recoverable (side effect is the lock is not released)

  if (lck->ticket_active) {
    lck->next_ticket++;
    BROADCAST (lck);	// unrecoverable
    SUCCESS (lck);	// unrecoverable
  }

  lck->locked = 0;
  lck->owner = PTHREAD_NULL;
  SIGNAL (lck); // unrecoverable

  SUCCESS (lck); // unrecoverable
}

int issue_ticket (ticket_lock * lck, unsigned int *ticket)
{
  BEGIN (lck); // unrecoverable

  unsigned int a = lck->last_issued_ticket;
  a++;

  if (lck->next_ticket == a)
    FAIL(lck, EBUSY);		// recoverable.  tickets exhausted, this means unsigned int has overflown, but if 
				// all threads use their tickets the counter is reset and can be used aggain.

  (*ticket) = a;
  lck->ticket_active = 1;
  lck->last_issued_ticket = a;

  SUCCESS (lck); // unrecoverable
}

int lock_with_ticket (ticket_lock * lck, unsigned int ticket)
{
  BEGIN (lck); // unrecoverable

  if (!lck->ticket_active)
    FAIL(lck,EINVAL); // recoverable (side effect: lock is not taken)
  while (lck->next_ticket != ticket) {
    WAIT (lck); // unrecoverable
  }

  lck->owner = pthread_self ();

  if (lck->last_issued_ticket == ticket) {
    lck->ticket_active = 0;
    lck->next_ticket = 0;
    lck->last_issued_ticket = 0;
  }

  SUCCESS (lck); // unrecoverable
}

int destroy_tkt_lock (ticket_lock * lck)
{
  int err = pthread_mutex_destroy(&lck->mutex);
  if (err) return err;   // unrecoverable
  err = pthread_cond_destroy(&lck->cond);
  if (err) return err;   // unrecoverable
  
  lck->owner = PTHREAD_NULL;
  lck->locked = 0;
  lck->ticket_active = 0;
  lck->next_ticket = 0;
  lck->last_issued_ticket = 0;
  return 0;
}
