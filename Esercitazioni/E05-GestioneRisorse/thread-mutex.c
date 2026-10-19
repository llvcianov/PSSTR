// FIFO Threads Priority Inversion simulation

#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

int shared;
pthread_mutex_t shared_mutex;

void * run_high(void * wcet) {

	int i = 1;
	int j;
	int a = 13, b = 17;
	usleep(1);
	printf("Thread High about to lock...\n");
	//simulating a long critical section
	while (i <= (long)wcet) {
		pthread_mutex_lock(&shared_mutex);
		int sh = shared;
		for(j=0; j<100000; j++) a*=b;
		shared = sh+1;
		pthread_mutex_unlock(&shared_mutex);
		if (i%100==0) printf("Thread High, shared=%d\n",shared);
		i++;
	}
	return NULL;
}

void * run_low(void * wcet) {

	int i = 1;
	int j;
	int a = 13, b = 17;
	printf("Thread Low about to lock...\n");
	//simulating a long critical section
	while (i <= (long)wcet) {
		pthread_mutex_lock(&shared_mutex);
		int sh = shared;
		for(j=0; j<100000; j++) a*=b;
		shared = sh+1;
		pthread_mutex_unlock(&shared_mutex);
		if (i%100==0) printf("Thread Low, shared=%d\n",shared);
		i++;
	}
	return NULL;
}

void * run_medium(void * wcet) {

	int i;
	int a = 13, b = 17;

	usleep(2);
	// cpu burn
	for(i=0; i<(long)wcet*100000; i++) {
		a *= b;
		if (i%10000000==0) printf("Thread Medium, CPU BURN!\n");
	}
	return NULL;
}


int main()
{
	pthread_t threads[3];
	
	// imposto il protocollo priority inheritance e inizializzo il mutex che protegge "shared"

	pthread_mutexattr_t mymutexattr;

	pthread_mutexattr_init(&mymutexattr);

	pthread_mutexattr_setprotocol(&mymutexattr,PTHREAD_PRIO_INHERIT);

	pthread_mutex_init(&shared_mutex, &mymutexattr);

	pthread_mutexattr_destroy(&mymutexattr);
	
	
	
  	// init thread attr
  	struct sched_param myparam;
	pthread_attr_t myattr;
	pthread_attr_init(&myattr);
    pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
    pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);
	
  	// start threads
	myparam.sched_priority = 10;
    pthread_attr_setschedparam(&myattr, &myparam); 
    pthread_create(&threads[0], &myattr, run_low, (void*)20000);
    
    myparam.sched_priority = 20;
    pthread_attr_setschedparam(&myattr, &myparam); 
    pthread_create(&threads[1], &myattr, run_high, (void*)5000);
   
    
    myparam.sched_priority = 15;
    pthread_attr_setschedparam(&myattr, &myparam); 
    pthread_create(&threads[2], &myattr, run_medium, (void*)500); 
    
    
    
    
 	// wait threads
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
		
	pthread_attr_destroy(&myattr);
	pthread_mutex_destroy(&shared_mutex);

	printf("Final shared value: %d\n",shared);

    return 0;
}


