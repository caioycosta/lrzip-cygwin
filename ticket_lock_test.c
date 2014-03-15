#include "ticket_lock.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

ticket_lock lk = TICKET_LOCK_INITIALIZER;
typedef struct {
	unsigned int ticket;
	int numtoprint;
} threaddata;

#define NUMTHREADS 20
#define CHECK(A) do{if(A){printf("A error");return 0;}}while(0)
#define CHECKP(A) do{if(A){printf("A error");return NULL;}}while(0)

static void* prnthread(void* data)
{
	threaddata* d = (threaddata*)data;

	CHECKP(lock_with_ticket(&lk, d->ticket));
	
	usleep((d->numtoprint * 1000000/NUMTHREADS)-1);
	printf("%d\n",d->numtoprint);

	CHECKP(unlock_tkt_lock(&lk));

	return NULL;
}

int main()
{
	pthread_t threads[NUMTHREADS];
	threaddata datas[NUMTHREADS];

	// thread attr start routine arg		
	CHECK(lock_tkt_lock(&lk));
	int i = NUMTHREADS;

	for (; i >= 1;i--) {
		unsigned int ticket;
		CHECK(issue_ticket(&lk, &ticket));
		datas[i-1].ticket = ticket;
		datas[i-1].numtoprint = i;
	}

	for (i =0;i<NUMTHREADS;i++) {
		printf("starting thread for %d\n",datas[i].numtoprint);
		pthread_create(&threads[i],NULL,prnthread,(void*)&datas[i]);
		usleep(500000);
	}

	CHECK(unlock_tkt_lock(&lk));
	CHECK(lock_tkt_lock(&lk));
	printf("everbody ended!\n");
	CHECK(unlock_tkt_lock(&lk));
	// wait for all to finish

	return 0;
}
