#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

struct periodic_thread {
	int index;
	struct timespec r;
	int period;
	int wcet;
	int priority;
};

#define NSEC_PER_SEC 1000000000ULL
static inline void timespec_add_us(struct timespec *t, uint64_t d)
{
    d *= 1000;
    t->tv_nsec += d;
    t->tv_sec += t->tv_nsec / NSEC_PER_SEC;
    t->tv_nsec %= NSEC_PER_SEC;
}

void wait_next_activation(struct periodic_thread * thd)
{
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(thd->r), NULL);
    timespec_add_us(&(thd->r), thd->period);
}

void start_periodic_timer(struct periodic_thread * thd, uint64_t offs)
{
    clock_gettime(CLOCK_MONOTONIC, &(thd->r));
    timespec_add_us(&(thd->r), offs);
}

void * run (void * par) {
	struct periodic_thread *th = (struct periodic_thread *) par;
	
	start_periodic_timer(th,2000000);
    
    int cnt = 0;
    uint64_t start = 0;
    uint64_t t;
    struct timeval tv;
    
    while(1) {
        wait_next_activation(th);
        if (start == 0) {
        	gettimeofday(&tv, NULL);
			start = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
    	}
        
    	gettimeofday(&tv, NULL);
    	t = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
    	if (cnt && (cnt % 100) == 0) {
        	printf("th %d: Avg time: %f\n", th->index, (double)(t - start) / (double)cnt);
    	}
    	cnt++;
    }
}

int main()
{
	pthread_t thread_1;

	struct periodic_thread th1;
	th1.index = 1;
	th1.period = 5000;
	
    pthread_create(&thread_1, NULL, run, &th1); 
    
	while (1) {
   		if (getchar() == 'q') break;
  	}

    return 0;
}


