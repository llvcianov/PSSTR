#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MIN_SEMIPERIOD 10000			//10ms
#define NSEC_PER_SEC 1000000000ULL
#define NGENERATORS 3

/************************** DATA STRUCTURES *******************************/

/* Sequence data structure */
struct seq_str
{
	unsigned int bit[3];	//Data produced by the generators
	//to do... 
	// Inserire codice per la mutua esclusione
	pthread_mutex_t mutex_generatore_recognizer;
};
static struct seq_str seq_data;

//to do...
//Inserire una struttura dati condivisa e protetta contentente le variabili "count" e "ok" usate dal recognizer
struct struttura {

	int ok;
	int count;
	pthread_mutex_t mutex_buddy_recognizer;

};
static struct struttura variabili_condivise;


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
	printf("Start Generator %d\n",th->index);	//DEBUG
	
	start_periodic_timer(th,th->phase);
	while (1) {
		wait_next_activation(th);

		//to do...
		//accedere alle variabili condivise in mutua esclusione

		pthread_mutex_lock(&seq_data.mutex_generatore_recognizer);

		if(seq_data.bit[th->index]){
			seq_data.bit[th->index] = 0;
		}else{
			seq_data.bit[th->index] = 1;
		}

		pthread_mutex_unlock(&seq_data.mutex_generatore_recognizer);

	}
}

void* rt_recognizer_thread(void* parameter){
	
	//to do...
	//Implementare un riconoscitore di sequenza che 
	//ad ogni periodo verifica se gli ultimi quattro valori letti corrispondono alla sequenza 0,3,6,5
	unsigned int valore_corrente;
	unsigned int stato[3];
	unsigned short int bits[3];	

	for(int i = 0; i<3; i++) stato[i]=-1;

	struct periodic_thread *th = (struct periodic_thread *) parameter;
	printf("Start Recognizer %d\n", th->index);
	start_periodic_timer(th,th->phase);
	while(1) {
		wait_next_activation(th);

		pthread_mutex_lock(&variabili_condivise.mutex_buddy_recognizer);
		if(variabili_condivise.ok==1){
			variabili_condivise.ok=0;
		}
		pthread_mutex_unlock(&variabili_condivise.mutex_buddy_recognizer);

		pthread_mutex_lock(&seq_data.mutex_generatore_recognizer);
		for(int i=0; i<NGENERATORS; i++){
			bits[i] = seq_data.bit[i];
		}
		pthread_mutex_unlock(&seq_data.mutex_generatore_recognizer);
		
		valore_corrente = 1*bits[0]+2*bits[1]+4*bits[2];
		printf("bits : ");			//DEBUG
		for(int i=NGENERATORS-1; i>-1; i--){
			printf("%d ",seq_data.bit[i]);
		}
		printf("\n");

		if(valore_corrente == 0 && stato[0] == -1){
			stato[0] = 0;
		} else if (valore_corrente == 3 && stato[1] == -1 && stato[0] == 0){
			stato[1] = 3;
		} else if (valore_corrente == 6 && stato[2] == -1 && stato[1] == 3 && stato[0] == 0){
			stato[2] = 6;
		} else if (valore_corrente == 5 && stato[2] == 6 && stato[1] == 3 && stato[0] == 0){
			//printf("Sequenza riconosciuta!!!:)\n");
			pthread_mutex_lock(&variabili_condivise.mutex_buddy_recognizer);
			variabili_condivise.ok=1;
			variabili_condivise.count++;
			pthread_mutex_unlock(&variabili_condivise.mutex_buddy_recognizer);
			for(int i = 0; i<3; i++){
				stato[i]=-1;
			} 
		} else {
			for(int i = 0; i<3; i++){
				stato[i]=-1;
			}

		}

	}
	
}

void* nrt_buddy_thread(void* parameter){
	struct periodic_thread *th = (struct periodic_thread *) parameter;
	printf("Start Buddy %d\n",th->index);
	start_periodic_timer(th,th->phase);
	int i = 0;

	while (1) {
		wait_next_activation(th);
		
		//to do... 
		//inserire mutua esclusione per l'accesso alle variabili condivise 
		
		/*printf("bits : ");			//DEBUG
		for(i=NGENERATORS-1; i>-1; i--){
			printf("%d ",seq_data.bit[i]);
		}
		printf("\n");*/
		
		
		//to do...
		//Stampa dei valori "count" e "ok" prodotti dal recognizer
		pthread_mutex_lock(&variabili_condivise.mutex_buddy_recognizer);
		printf("ok = %d\n",variabili_condivise.ok);
		printf("Numero sequenze riconosciute: %d\n", variabili_condivise.count);
		pthread_mutex_unlock(&variabili_condivise.mutex_buddy_recognizer);

	}

}


/***************************** MAIN **************************************/

int main(){	
	
	// Modificare qui per scegliere il periodo e la fase dei thread 
	int arraySemiperiods[NGENERATORS] = {2, 4, 8};
	int arrayPhases[NGENERATORS] = {0, 1, 3};
	int i = 0;

	/* Initialize the shared data*/
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
	
	//to do...
	//Inizializzare i Mutex per le variabili condivise
	pthread_mutexattr_init (&mutex_attr);
	pthread_mutexattr_setprotocol (&mutex_attr, PTHREAD_PRIO_INHERIT);

	pthread_mutex_init(&seq_data.mutex_generatore_recognizer, &mutex_attr);
	pthread_mutex_init(&variabili_condivise.mutex_buddy_recognizer, &mutex_attr);

	/* Generator threads*/
	for(i=0; i<NGENERATORS; i++){
		/*periodic thread initialization*/
		th_g[i].index = i;
		th_g[i].period = arraySemiperiods[i]*MIN_SEMIPERIOD;
		th_g[i].phase = arrayPhases[i]*MIN_SEMIPERIOD;
		th_g[i].priority = 12 - i;
		
		//to do..
		//Inizializza gli attributi dei threads per renderli real-time
		//I thread devono essere schedulati con Rate Monotonic

		pthread_attr_init(&myattr);
		pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
		myparam.sched_priority=(th_g[i].priority);
		pthread_attr_setschedparam(&myattr,&myparam);
		pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);

		/*thread creation*/
		pthread_create(&thread_generator[i], &myattr, rt_generator_thread, &th_g[i]);  
	}



	//to do..
	//Inizializza e crea il thread hard real-time recognizer
	th_r.index = NGENERATORS;
	th_r.period = MIN_SEMIPERIOD;
	th_r.phase =  MIN_SEMIPERIOD;
	th_r.priority = 13;

	pthread_attr_init(&myattr);
	pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
	myparam.sched_priority=(th_r.priority);
	pthread_attr_setschedparam(&myattr,&myparam);
	pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED);

	pthread_create(&thread_recognizer, &myattr, rt_recognizer_thread, &th_r);
	

	
	/* buddy thread */
	th_buddy.index = NGENERATORS+1;
	th_buddy.period = MIN_SEMIPERIOD; //10 ms
	th_buddy.phase =  MIN_SEMIPERIOD; //10 ms
	/*thread creation*/	
	pthread_create(&thread_buddy,NULL,nrt_buddy_thread,&th_buddy);



	//to do...
	// Pulisci gli attributi usati per inizializzare i thread e i mutex

	pthread_mutex_destroy(&seq_data.mutex_generatore_recognizer);
	pthread_mutex_destroy(&variabili_condivise.mutex_buddy_recognizer);
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
