#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void *fun(void *arg){

	for(int i = 0; i<10; i++){
		printf("UeUe %ld\n", (long)arg);
	}
	
	pthread_exit(NULL);
}

int main(){

	pthread_t th1, th2;
	
	pthread_create(&th1, NULL, fun, (void*)1);
	pthread_create(&th2, NULL, fun, (void*)2);
	
	pthread_join(th1, NULL); //il secondo parametro prende il risultato passato dalla pthread_exit di fun
	pthread_join(th2, NULL);
	
	
	sleep(1);
	
	pthread_exit(NULL); //posso evitare di metterla se metto le join ai thread
	
	return 0;
}
