//------------------- CONTROLLER.C ---------------------- 

#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>
#include "rt-lib.h"
#include "parameters.h"

//emulates the controller

static int keep_on_running = 1;

struct shared_int {
	int value;
	pthread_mutex_t lock;
};
static struct shared_int shared_avg_sensor;
static struct shared_int shared_control;

//area di memoria condivisa DIAG
struct shared_mix {
	time_t WCET_acquire;
	time_t WCET_control;
	time_t WCET_actuate;
	unsigned int media_buffer;
	int comando_controllo;
	int elem_buffer[3];
	unsigned int my_reference;
	pthread_mutex_t mutex;
};
static struct shared_mix shared_diag;

int buffer[BUF_SIZE];
int head = 0;


void * acquire_filter_loop(void * par) {
	
	periodic_thread *th = (periodic_thread *) par;
	start_periodic_timer(th,TICK_TIME);

	//variabili per il calcolo del WCET
	struct timespec start_time;
	struct timespec end_time;
	time_t t_exec_corrente = 0;

	// Messaggio da prelevare dal driver
	char message [MAX_MSG_SIZE];

	/* Coda */
	struct mq_attr attr;

	attr.mq_flags = 0;				
	attr.mq_maxmsg = MAX_MESSAGES;	
	attr.mq_msgsize = MAX_MSG_SIZE; 
	attr.mq_curmsgs = 0;
	
	// Apriamo la coda sensor del plant in lettura 
	mqd_t sensor_qd;
	if ((sensor_qd = mq_open (SENSOR_QUEUE_NAME_2, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS,&attr)) == -1) {
		perror ("acquire filter loop: mq_open (sensor)");
		exit (1);
	}
	unsigned int sum = 0;
	int cnt = BUF_SIZE;
	while (keep_on_running)
	{
		wait_next_activation(th);

		//calcolo lo start_time per il WCET appena inizia ad eseguire il thread
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

		// PRELIEVO DATI dalla coda del PLANT
		if (mq_receive(sensor_qd, message,MAX_MSG_SIZE,NULL) == -1){
			perror ("acquire filter loop: mq_receive (actuator)");	
			break;						//DEBUG
		}
		else{ 
			buffer[head] = atoi(message);
			
			//scrivo gli elementi del buffer nel diag
			pthread_mutex_lock(&shared_diag.mutex);
			shared_diag.elem_buffer[head] = buffer[head];
			pthread_mutex_unlock(&shared_diag.mutex);

			sum += buffer[head];
			head = (head+1)%BUF_SIZE;
			cnt--;

			//printf("\t\t\t\tbuffer[%d]=%d, sum=%d\n",head,buffer[head],sum); //DEBUG

			// calcolo media sulle ultime BUF_SIZE letture
			if (cnt == 0) {
				cnt = BUF_SIZE;
				pthread_mutex_lock(&shared_avg_sensor.lock);
				shared_avg_sensor.value = sum/BUF_SIZE;
				//printf("\t\t\t\tavg_sensor.value=%d\n",shared_avg_sensor.value); //DEBUG
				pthread_mutex_unlock(&shared_avg_sensor.lock);
				//scrivo la media nel diag
				pthread_mutex_lock(&shared_diag.mutex);
				shared_diag.media_buffer = sum/BUF_SIZE;
				pthread_mutex_unlock(&shared_diag.mutex);
				sum = 0;
			}	
		}

		//Calcolo l'end_time per il WCET quando il thread termina il periodo
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
		t_exec_corrente = (end_time.tv_sec*1000000000 + end_time.tv_nsec) - (start_time.tv_sec*1000000000 + start_time.tv_nsec);

		//printf("L'esecuzione corrente è durata %ld\n", t_exec_corrente);  //DEBUG

		//Scrivo il WCET in memoria condivisa verificando se il valore precedente è maggiore di quello ottenuto (definizione di WCET)
		pthread_mutex_lock(&shared_diag.mutex);
		if(t_exec_corrente > shared_diag.WCET_acquire){
			shared_diag.WCET_acquire=t_exec_corrente;
		}
		pthread_mutex_unlock(&shared_diag.mutex);

	}		

	/* Clear */
    if (mq_close (sensor_qd) == -1) {
        perror ("acquire filter loop: mq_close sehsor_qd");
        exit (1);
    }

	return 0;
}


void * control_loop(void * par) {

	periodic_thread *th = (periodic_thread *) par;
	start_periodic_timer(th,TICK_TIME);

	//variabili per il calcolo del WCET
	struct timespec start_time;
	struct timespec end_time;
	time_t t_exec_corrente = 0;
	
	// Messaggio da prelevare dal reference
	char message [MAX_MSG_SIZE];

	//Messaggio da prelevare dalla ruota 1 per il coordinamento
	char message_coordination_receive [MAX_MSG_SIZE];

	//Messaggio da inviare alla ruota 1 per il coordinamento
	char message_coordination_send [MAX_MSG_SIZE];
	
	/* Coda */
	struct mq_attr attr;

	attr.mq_flags = 0;				
	attr.mq_maxmsg = MAX_MESSAGES;	
	attr.mq_msgsize = MAX_MSG_SIZE; 
	attr.mq_curmsgs = 0;
	
	// Apriamo la coda per il reference, in lettura e non bloccante
	mqd_t reference_qd;
	if ((reference_qd = mq_open (REFERENCE_QUEUE_NAME_2, O_RDONLY | O_CREAT | O_NONBLOCK, QUEUE_PERMISSIONS,&attr)) == -1) {
		perror ("control loop: mq_open (reference)");
		exit (1);
	}

	// Apro la coda per il coordinamento (ricevo dalla ruota 1) con la ruota 1, in lettura e non bloccante
	mqd_t coordination_qd_2;
	if ((coordination_qd_2 = mq_open (COORDINATION_QUEUE_NAME_2, O_RDONLY | O_CREAT | O_NONBLOCK, QUEUE_PERMISSIONS,&attr)) == -1) {
		perror ("control loop: mq_open (coordination_2)");
		exit (1);
	}

	// Apro la coda per il coordinamento (invio alla ruota 1) con la ruota 1, in scrittura e non bloccante
	mqd_t coordination_qd;
	if ((coordination_qd = mq_open (COORDINATION_QUEUE_NAME, O_WRONLY | O_CREAT | O_NONBLOCK, QUEUE_PERMISSIONS,&attr)) == -1) {
		perror ("control loop: mq_open (coordination)");
		exit (1);
	}

	//Variabili conversione messaggio
	unsigned int reference = 110;

	unsigned int coordination_receive = 0;

	unsigned int plant_state = 0;

	//valore prededente della media dei valori del plant per evitare lo skating
	unsigned int plant_state_old = 0;

	//valore di controllo messaggio ricevuto da ruota 1
	unsigned int state = 0; //0 vuol dire che non ho ricevuto nessun messaggio dalla ruota 1

	int error = 0;
	unsigned int control_action = 0;
	while (keep_on_running)
	{
		wait_next_activation(th);

		//calcolo lo start_time per il WCET appena inizia ad eseguire il thread
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

		// legge il plant state 
		pthread_mutex_lock(&shared_avg_sensor.lock);
		plant_state = shared_avg_sensor.value;
		pthread_mutex_unlock(&shared_avg_sensor.lock);

		//riazzero lo stato ad ogni periodo
		state = 0;

		// riceve la reference (in modo non bloccante)
		if (mq_receive(reference_qd, message,MAX_MSG_SIZE,NULL) == -1){
			//printf ("No reference ...\n");	//DEBUG
		}
		else{
			//printf ("Reference received: %s.\n",message);			//DEBUG
			reference = atoi(message);

			//scrivo la reference nel diag
			pthread_mutex_lock(&shared_diag.mutex);
			shared_diag.my_reference = reference;
			pthread_mutex_unlock(&shared_diag.mutex);
		}

		// se reference = 0, control action di frenata
		if (reference == 0) {
			//controllo se l'altra ruota sta slittando (ricevo messaggio ABS da ruota 1 in modo non bloccante)
			if (mq_receive(coordination_qd_2, message_coordination_receive, MAX_MSG_SIZE, NULL) == -1){
				//printf("Messaggio ABS non ricevuto dalla ruota 1\n") //DEBUG
			} else {
				coordination_receive = atoi(message_coordination_receive);
				control_action = coordination_receive;
				printf("Ricevuto messaggio di decelerazione da ruota 1: %d\n", control_action);
				state = 1;//aggiorno lo stato perchè ho ricevuto il messaggio
			}
			//controllo la mia ruota se e solo se non ho ricevuto il messaggio (se l'ho ricevuto devo decellerare a prescindere che io stia slittando o meno)
			// se gli ultimi 6 valori del plant sono uguali (cioè abbiamo 2 medie uguali consecutivamente), interrompo la frenata e decellero (ABS)
			if((plant_state_old == plant_state) && (state == 0)){
				control_action = 2; 
				//invio comando alla ruota 1
				sprintf(message_coordination_send, "%d", control_action);
				if (mq_send(coordination_qd, message_coordination_send, strlen(message_coordination_send)+1, 0) == -1){
					perror("Errore invio messaggio alla ruota 1\n");
				} else {
					printf("La ruota slitta decelero e invio messaggio alla ruota 1\n");
				}
			} else if(state == 0) {
				plant_state_old = plant_state;
				control_action = 4; //freno normalmente perchè non sto slittando
				printf("Freno normalmente perchè non slitto\n");
			}
		
		} else { // calcolo della legge di controllo
				error = reference - plant_state;
				if (error > 0) control_action = 1;
				else if (error < 0) control_action = 2;
				else control_action = 3;
			}
		
		// aggiorna la control action
		pthread_mutex_lock(&shared_control.lock);
		shared_control.value = control_action;
		pthread_mutex_unlock(&shared_control.lock);

		// scrivo la control action nel diag
		pthread_mutex_lock(&shared_diag.mutex);
		shared_diag.comando_controllo = control_action;
		pthread_mutex_unlock(&shared_diag.mutex);

		//Calcolo l'end_time per il WCET quando il thread termina il periodo
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
		t_exec_corrente = (end_time.tv_sec*1000000000 + end_time.tv_nsec) - (start_time.tv_sec*1000000000 + start_time.tv_nsec);
		//printf("L'esecuzione corrente è durata %ld\n", t_exec_corrente);  //DEBUG

		//Scrivo il WCET in memoria condivisa verificando se il valore precedente è maggiore di quello ottenuto (definizione di WCET)
		pthread_mutex_lock(&shared_diag.mutex);
		if(t_exec_corrente > shared_diag.WCET_control){
			shared_diag.WCET_control=t_exec_corrente;
		}
		pthread_mutex_unlock(&shared_diag.mutex);	 
	}
	/* Clear */
    if (mq_close (reference_qd) == -1) {
        perror ("control loop: mq_close reference_qd");
        exit (1);
    }

	if (mq_close (coordination_qd_2) == -1) {
        perror ("control loop: mq_close coordination_qd_2");
        exit (1);
    }

	if (mq_close (coordination_qd) == -1) {
        perror ("control loop: mq_close coordination_qd");
        exit (1);
    }

	return 0;
}

void * actuator_loop(void * par) {

	periodic_thread *th = (periodic_thread *) par;
	start_periodic_timer(th,TICK_TIME);

	//variabili per il calcolo del WCET
	struct timespec start_time;
	struct timespec end_time;
	time_t t_exec_corrente = 0;

	// Messaggio da prelevare dal driver
	char message [MAX_MSG_SIZE];

	/* Coda */
	struct mq_attr attr;

	attr.mq_flags = 0;				
	attr.mq_maxmsg = MAX_MESSAGES;	
	attr.mq_msgsize = MAX_MSG_SIZE; 
	attr.mq_curmsgs = 0;
	
	// Apriamo la coda actuator del plant in scrittura 
	mqd_t actuator_qd;
	if ((actuator_qd = mq_open (ACTUATOR_QUEUE_NAME_2, O_WRONLY|O_CREAT, QUEUE_PERMISSIONS,&attr)) == -1) {
		perror ("actuator  loop: mq_open (actuator)");
		exit (1);
	}	

	unsigned int control_action = 0;
	unsigned int control = 0;
	while (keep_on_running)
	{
		wait_next_activation(th);

		//calcolo lo start_time per il WCET appena inizia ad eseguire il thread
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

		// prelievo della control action
		pthread_mutex_lock(&shared_control.lock);
		control_action = shared_control.value;
		pthread_mutex_unlock(&shared_control.lock);
		
		switch (control_action) {
			case 1: control = 1; break;
			case 2:	control = -1; break;
			case 3:	control = 0; break;
			case 4: control = -2; break;
			default: control = 0;
		}
		printf("Control: %d\n",control); //DEBUG
		sprintf (message, "%d", control);
		//invio del controllo al driver del plant
		if (mq_send (actuator_qd, message, strlen (message) + 1, 0) == -1) {
		    perror ("Sensor driver: Not able to send message to controller");
		    continue;
		}

		//Calcolo l'end_time per il WCET quando il thread termina il periodo
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
		t_exec_corrente = (end_time.tv_sec*1000000000 + end_time.tv_nsec) - (start_time.tv_sec*1000000000 + start_time.tv_nsec);
		//printf("L'esecuzione corrente è durata %ld\n", t_exec_corrente);  //DEBUG

		//Scrivo il WCET in memoria condivisa verificando se il valore precedente è maggiore di quello ottenuto (definizione di WCET)
		pthread_mutex_lock(&shared_diag.mutex);
		if(t_exec_corrente > shared_diag.WCET_actuate){
			shared_diag.WCET_actuate=t_exec_corrente;
		}
		pthread_mutex_unlock(&shared_diag.mutex);
	}
	/* Clear */
    if (mq_close (actuator_qd) == -1) {
        perror ("Actuator loop: mq_close actuator_qd");
        exit (1);
    }
	return 0;
}

//ROUTINE POLLING SERVER THREAD

void * polling_server_loop(void * par){

	periodic_thread *th = (periodic_thread *) par;
	start_periodic_timer(th,TICK_TIME);

	//Messaggio di richiesta da ricevere dal DIAG
	char request [MAX_MSG_SIZE];

	//Messaggi di diagnostica da inviare al DIAG
	char mess_diag1 [MAX_MSG_SIZE]; //WCET acquire
	char mess_diag2 [MAX_MSG_SIZE]; //WCET control
	char mess_diag3 [MAX_MSG_SIZE]; //WCET actuate
	char mess_diag4 [MAX_MSG_SIZE]; //media buffer
	char mess_diag5 [MAX_MSG_SIZE]; //comando controllo
	char mess_diag6 [MAX_MSG_SIZE]; //elemento 1 del buffer
	char mess_diag7 [MAX_MSG_SIZE]; //elemento 2 del buffer
	char mess_diag8 [MAX_MSG_SIZE]; //elemento 3 del buffer
	char mess_diag9 [MAX_MSG_SIZE]; //reference

	//Descrittori delle code
    mqd_t polling_server_qd_2;
	mqd_t DIAG_qd_2;

	//Imposto gli attributi delle code
	struct mq_attr attr;

	attr.mq_flags = 0;				
	attr.mq_maxmsg = MAX_MESSAGES;	
	attr.mq_msgsize = MAX_MSG_SIZE; 
	attr.mq_curmsgs = 0;

	//Creo e apro la coda per la ricezione della richiesta di diagnostica, la coda è in lettura ed è non bloccante
	if((polling_server_qd_2 = mq_open(POLLING_SERVER_QUEUE_NAME_2, O_RDONLY | O_CREAT | O_NONBLOCK, QUEUE_PERMISSIONS, &attr)) == -1){
		perror("Errore apertura coda polling server 2");
		exit(1);
	}

	//Creo e apro la coda per l'invio della diagnostica, la coda è in scrittura ed è bloccante
	if((DIAG_qd_2 = mq_open(DIAG_QUEUE_NAME_2, O_WRONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1){
		perror("Errore apertura coda DIAG 2");
		exit(1);
	}

	while(keep_on_running){

		wait_next_activation(th);

		//aspetto di ricevere la richiesta di diagnostica
		if(mq_receive(polling_server_qd_2, request, MAX_MSG_SIZE, NULL) == -1){
			//printf("Nessun messaggio ricevuto...\n")
		} else {
			//prendo i dati dalla struttura condivisa
			pthread_mutex_lock(&shared_diag.mutex);
			sprintf(mess_diag1, "%ld", shared_diag.WCET_acquire);
			sprintf(mess_diag2, "%ld", shared_diag.WCET_control);
			sprintf(mess_diag3, "%ld", shared_diag.WCET_actuate);
			sprintf(mess_diag4, "%d", shared_diag.media_buffer);
			sprintf(mess_diag5, "%d", shared_diag.comando_controllo);
			sprintf(mess_diag6, "%d", shared_diag.elem_buffer[0]);
			sprintf(mess_diag7, "%d", shared_diag.elem_buffer[1]);
			sprintf(mess_diag8, "%d", shared_diag.elem_buffer[2]);
			sprintf(mess_diag9, "%d", shared_diag.my_reference);
			pthread_mutex_unlock(&shared_diag.mutex);

			//invio i messaggi al diag
			if(mq_send(DIAG_qd_2, mess_diag1, strlen(mess_diag1)+1, 0) == -1){
				perror("Errore invio messaggio 1 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag2, strlen(mess_diag2)+1, 0) == -1){
				perror("Errore invio messaggio 2 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag3, strlen(mess_diag3)+1, 0) == -1){
				perror("Errore invio messaggio 3 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag4, strlen(mess_diag4)+1, 0) == -1){
				perror("Errore invio messaggio 4 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag5, strlen(mess_diag5)+1, 0) == -1){
				perror("Errore invio messaggio 5 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag6, strlen(mess_diag6)+1, 0) == -1){
				perror("Errore invio messaggio 6 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag7, strlen(mess_diag7)+1, 0) == -1){
				perror("Errore invio messaggio 7 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag8, strlen(mess_diag8)+1, 0) == -1){
				perror("Errore invio messaggio 8 al DIAG");
			}

			if(mq_send(DIAG_qd_2, mess_diag9, strlen(mess_diag9)+1, 0) == -1){
				perror("Errore invio messaggio 9 al DIAG");
			}
		}
	}

	//Chiudo le code

	if(mq_close(polling_server_qd_2) == -1){
		perror("Errore chiusura coda polling server 2");
		exit(1);
	}

	if(mq_close(DIAG_qd_2) == -1){
		perror("Errore chiusura coda DIAG 2");
		exit(1);
	}

	return 0;
}

int main(void)
{
	printf("The controller 2 is STARTED! [press 'q' to stop]\n");

	//Inizializzo le variabili in memoria condivisa prima di creare i thread
	shared_diag.comando_controllo = 0;
	shared_diag.media_buffer = 0;
	shared_diag.my_reference = 0;
	shared_diag.WCET_acquire = 0;
	shared_diag.WCET_actuate = 0;
	shared_diag.WCET_control = 0;
	for(int i = 0; i < 3; i++){
		shared_diag.elem_buffer[i] = 0;
	}
 	
	pthread_t acquire_filter_thread;
    pthread_t control_thread;
    pthread_t actuator_thread;

	//identificatore polling thread
	pthread_t polling_server_thread;

	pthread_mutex_init(&shared_avg_sensor.lock, NULL);
	pthread_mutex_init(&shared_control.lock, NULL);

	//Inizializzo il mutex diag impostando il protocollo priority inheritance per evitare la priority inversion
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setprotocol(&mutex_attr, _POSIX_THREAD_PRIO_INHERIT);
	pthread_mutex_init(&shared_diag.mutex, &mutex_attr);

	//Vengono settati gli attributi per creare i thread
	pthread_attr_t myattr;
	struct sched_param myparam;

	pthread_attr_init(&myattr);
	pthread_attr_setschedpolicy(&myattr, SCHED_FIFO);
	pthread_attr_setinheritsched(&myattr, PTHREAD_EXPLICIT_SCHED); 

	// ACQUIRE FILTER THREAD
	periodic_thread acquire_filter_th;
	acquire_filter_th.period = TICK_TIME;
	acquire_filter_th.priority = 50;

	myparam.sched_priority = acquire_filter_th.priority;
	pthread_attr_setschedparam(&myattr, &myparam); 
	pthread_create(&acquire_filter_thread,&myattr,acquire_filter_loop,(void*)&acquire_filter_th);

	// CONTROL THREAD
	periodic_thread control_th;
	control_th.period = TICK_TIME*BUF_SIZE;
	control_th.priority = 45;

	myparam.sched_priority = control_th.priority;
	pthread_attr_setschedparam(&myattr, &myparam); 
	pthread_create(&control_thread,&myattr,control_loop,(void*)&control_th);

	// ACTUATOR THREAD
	periodic_thread actuator_th;
	actuator_th.period = TICK_TIME*BUF_SIZE;
	actuator_th.priority = 45;

	pthread_attr_setschedparam(&myattr, &myparam); 
	pthread_create(&actuator_thread,&myattr,actuator_loop,(void*)&actuator_th);

	// POLLING SERVER THREAD
	periodic_thread polling_server_th;
	polling_server_th.period = TICK_TIME/2;
	polling_server_th.priority = 55;

	myparam.sched_priority = polling_server_th.priority;
	pthread_attr_setschedparam(&myattr, &myparam);
	pthread_create(&polling_server_thread, &myattr, polling_server_loop, (void*)&polling_server_th);

	//Distruggo tutto
	pthread_attr_destroy(&myattr);
	pthread_mutexattr_destroy(&mutex_attr);
	
	
	/* Wait user exit commands*/
	while (1) {
   		if (getchar() == 'q') break;
  	}
	keep_on_running = 0;

	if (mq_unlink (REFERENCE_QUEUE_NAME_2) == -1) {
        perror ("Main: mq_unlink reference queue");
        exit (1);
    }

	if (mq_unlink (COORDINATION_QUEUE_NAME_2) == -1) {
        perror ("Main: mq_unlink coordination_2 queue");
        exit (1);
    }

	if(mq_unlink (POLLING_SERVER_QUEUE_NAME_2) == -1) {
		perror("Main: mq_unlink polling server queue 2");
		exit(1);
	}

	if(mq_unlink(DIAG_QUEUE_NAME_2) == -1) {
		perror("Main: mq_unlink DIAG queue 2");
		exit(1);
	}

 	printf("The controller is STOPPED\n");
	return 0;
}




