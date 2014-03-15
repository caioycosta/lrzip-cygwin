#ifndef TICKET_LOCK_H_INCLUDED
#define TICKET_LOCK_H_INCLUDED

#include <pthread.h>

#define PTHREAD_NULL 0
#define TICKET_LOCK_INITIALIZER {PTHREAD_COND_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_NULL,0,0,0u,0u}

typedef struct {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  pthread_t owner;
  int locked;
  int ticket_active;
  unsigned int next_ticket;
  unsigned int last_issued_ticket;
} ticket_lock;

int init_tkt_lock (ticket_lock * lck);

int lock_tkt_lock (ticket_lock * lck);

int unlock_tkt_lock (ticket_lock * lck);

int issue_ticket (ticket_lock * lck, unsigned int *ticket);

int lock_with_ticket (ticket_lock * lck, unsigned int ticket);

int destroy_tkt_lock (ticket_lock * lck);

#endif // TICKET_LOCK_H_INCLUDED
