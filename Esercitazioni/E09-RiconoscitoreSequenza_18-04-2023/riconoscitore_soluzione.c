#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MIN_SEMIPERIOD 10000			//10ms
#define NSEC_PER_SEC 1000000000ULL
#define NGENERATORS 3

/************************** DATA STRUCTURES *******************************/

/* Sequence data structure */
struct seq_str
{
	unsigned int bit[3];	//Data produced by the generators
	pthread_mutex_t lock;
};
static struct seq_str seq_data;


/* (Count + ok bit) data structure */
struct shr_data_str
{
	unsigned int ok;		//ok-bit recognizer
	unsigned int count;		//counter recognizer
	pthread_mutex_t lock;	//mutex lock
};
static struct shr_data_str shared_data;


/* periodic thread */
struct periodic_thread {
	int index;
	struct timespec r;
	int period;
	int phase;
	int wcet;
	int priority;
};


/***************************** UTILITY FUNCTIONS ***********************************/

static inline void timespec_add_us(struct timespec *t, uint64_t d)
{
    d *= 1000;
    t->tv_nsec += d;
    t->tv_sec += t->tv_nsec / NSEC_PER_SEC;
    t->tv_nsec %= NSEC_PER_SEC;
}

void wait_next_activation(struct periodic_thread * thd)
{
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &(thd->r), NULL);
    timespec_add_us(&(thd->r), thd->period);
}

void start_periodic_timer(struct periodic_thread * thd, uint64_t offs)
{
    clock_gettime(CLOCK_REALTIME, &(thd->r));
    timespec_add_us(&(thd->r), offs);
}

/***************************** THREADS ************************************/

void* rt_generator_thread(void* parameter){
	struct periodic_thread *th = (struct periodic_thread *) parameter;
	printf("Start Generator %d\n",th->index);
	
	start_periodic_timer(th,th->phase);
	while (1) {
		wait_next_activation(th);
		
		pthread_mutex_lock(&seq_data.lock);
		if(seq_data.bit[th->index]){
			seq_data.bit[th->index] = 0;
		}else{
			seq_data.bit[th->index] = 1;
		}
		pthread_mutex_unlock(&seq_data.lock);
	}
}

void* rt_recognizer_thread(void* parameter){
	struct periodic_thread *th = (struct periodic_thread *) parameter;
	printf("Start Recognizer %d\n",th->index);

	unsigned short int sequence[4]={0,3,6,5};	// sequence to recognize
	unsigned short int decimal[4]={0,0,0,0};	// current sequence
	unsigned short int bits[3];					// current bits 
	unsigned short int state = 0;				// state = 1 if the sequence has been found
	int i = 0;		
	
	
	start_periodic_timer(th,th->phase);
	while (1) {
		wait_next_activation(th);		

		/*read current bits from shared memory*/
		pthread_mutex_lock(&seq_data.lock);
		for(i=0; i<NGENERATORS; i++){
			bits[i] = seq_data.bit[i];
		}
		pthread_mutex_unlock(&seq_data.lock);

		/*update current sequence*/
		state = 1;								
		for(i=0; i<3; i++){
			decimal[i] = decimal[i+1];					//shift 
		}
		decimal[3] = 1*bits[0]+2*bits[1]+4*bits[2];

		/*verify if the current sequence is the sequence to recognize*/
		for(i=0; i<4; i++){
			if(decimal[i] != sequence[i]) state = 0;	//if not found state=0
		}

		pthread_mutex_lock(&shared_data.lock);
		if(state){
			shared_data.ok=1;
			shared_data.count++;
		}else{
			shared_data.ok=0;
		}
		pthread_mutex_unlock(&shared_data.lock);

		/*DEBUG print sequence*/
		printf("current sequence: %d %d %d %d\n",decimal[0],decimal[1],decimal[2],decimal[3]);
		printf("sequence to reco: %d %d %d %d\n",sequence[0],sequence[1],sequence[2],sequence[3]);
		
	}
	
}

void* nrt_buddy_thread(void* parameter){
	struct periodic_thread *th = (struct periodic_thread *) parameter;
	printf("Start Buddy %d\n",th->index);
	start_periodic_timer(th,th->phase);
	int i = 0;

	while (1) {
		wait_next_activation(th);
		
		/* DEBUG: print bits*/
		printf("bits : ");
		pthread_mutex_lock(&seq_data.lock);
		for(i=NGENERATORS-1; i>-1; i--){
			printf("%d ",seq_data.bit[i]);
		}
		pthread_mutex_unlock(&seq_data.lock);
		printf("\n");
		

		/* Print ok + count from shared memory*/
		pthread_mutex_lock(&shared_data.lock);
		printf("count: %d \t",shared_data.count);
		if(shared_data.ok) {
			printf("OK! \n");
		}
		else{	
			printf("\n");
		}
		pthread_mutex_unlock(&shared_data.lock);

	}

}


/***************************** MAIN **************************************/

int main(){	
	
	//Take data in input (Phases and Periods of generators)
	int arraySemiperiods[NGENERATORS] = {1, 2, 4};
	int arrayPhases[NGENERATORS] = {0, 1, 3};
	int i = 0;

	/*Check input*/

	// Initialize the shared data
	for(i=0; i<NGENERATORS; i++){
		seq_data.bit[i] = 0;
	}

	struct periodic_thread th_g[NGENERATORS];
	struct periodic_thread th_r;
	struct periodic_thread th_buddy;

	pthread_t thread_generator[NGENERATORS];
	pthread_t thread_recognizer;
	pthread_t thread_buddy;

	pthread_attr_t myattr;
	struct sched_param myparam;
	
	pthread_mutexattr_t mutex_attr;	


	/*Mutex attributes initialization*/
	pthread_mutexattr_init (&mutex_attr);
	pthread_mutexattr_setprotocol (&mutex_attr, PTHREAD_PRIO_INHERIT);	// can be PTHREAD_PRIO_NONE, PTHREAD_PRIO_INHERIT, PTHREAD_PRIO_PROTECT
	//pthread_mutexattr_setprioceiling(&mutex_attr, pceiling);		    // if PTHREAD_PRIO_PROTECT mutex (set pceiling)
	
	/*Mutex initialization*/
	pthread_mutex_init(&shared_data.lock,&mutex_attr);
	pthread_mutex_init(&seq_data.lock,&mutex_attr);



	/* Generator threads*/
	for(i=0; i<NGENERATORS; i++){
		/*periodic thread initialization*/
		th_g[i].index = i;
		th_g[i].period = arraySemiperiods[i]*MIN_SEMIPERIOD;
		th_g[i].phase = arrayPhases[i]*MIN_SEMIPERIOD;
		th_g[i].priority = 10 - i;
		/*thread attribute initialization*/
		pthread_attr_init(&myattr);
		pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
		myparam.sched_priority = th_g[i].priority;
		pthread_attr_setschedparam(&myattr, &myparam);  
		pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);
		/*thread creation*/
		pthread_create(&thread_generator[i], &myattr, rt_generator_thread, &th_g[i]);  
	}



	/* Recognizer thread*/
	th_r.index = NGENERATORS;
	th_r.period = MIN_SEMIPERIOD;	//10 ms
	th_r.phase =  MIN_SEMIPERIOD;	//10 ms
	th_r.priority = 20;
	/*thread attribute initialization*/
	pthread_attr_init(&myattr);
	pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
	myparam.sched_priority = th_r.priority;
	pthread_attr_setschedparam(&myattr, &myparam);  
	pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);	
	/*thread creation*/
	pthread_create(&thread_recognizer, &myattr, rt_recognizer_thread, &th_r);  
	


	/* buddy thread */
	th_buddy.index = NGENERATORS+1;
	th_buddy.period = MIN_SEMIPERIOD; //10 ms
	th_buddy.phase =  MIN_SEMIPERIOD; //10 ms
	/*thread creation*/	
	pthread_create(&thread_buddy,NULL,nrt_buddy_thread,&th_buddy);



	/* Clear attr*/
	pthread_mutexattr_destroy(&mutex_attr);
	pthread_attr_destroy(&myattr);
	
	/* Wait user exit commands*/
	while (1) {
   		if (getchar() == 'q') break;
  	}
  	
  	/* Clear */
	for(i=0; i<NGENERATORS; i++){
  		pthread_kill(thread_generator[i],0);
	}
	pthread_kill(thread_recognizer,0);
	pthread_kill(thread_buddy,0);

  	printf("EXIT!\n");

	return 0;
}
