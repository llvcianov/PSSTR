#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_THREADS	10

void *PrintHello(void *threadid)
{
   sleep((long)threadid);
  
   printf("\n%ld: Hello World! \n", (long)threadid);
   pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
	int rc;
	long t;
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	
	for(t=0;t<NUM_THREADS;t++){

		printf("Creating thread %ld\n", t);
		
		rc = pthread_create(&threads[t], &attr, PrintHello, (void *)t);
		
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	
	for(t=0;t<NUM_THREADS;t++) {
		pthread_join(threads[t],NULL);
	}
	
	pthread_exit(NULL);
}

