//------------------- DIAG.c ---------------------- 

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
#include <sched.h>

//processo che invia le richieste di diagnostica//

int main(void){

    //Imposto la priorità e lo scheduler del processo 
    struct sched_param sp;
    sp.sched_priority = 65;
    sched_setscheduler(0, SCHED_FIFO, &sp);

    //descrittori delle code verso i polling server
    mqd_t polling_server_qd;
    mqd_t polling_server_qd_2;

    //descrittori delle code in ricezione dai polling server
    mqd_t DIAG_qd;
    mqd_t DIAG_qd_2;

    //Messaggi da inviare
    char message_request [MAX_MSG_SIZE];
    sprintf(message_request, "Richiesta 1");
    char message_request_2 [MAX_MSG_SIZE];
    sprintf(message_request_2, "Richiesta 2");

    //Messaggi da ricevere
    char message_response [MAX_MSG_SIZE];
    char message_response_2 [MAX_MSG_SIZE];

    //Apro le code verso i polling server in sola scrittura, le code sono già state create dai polling server e sono non bloccanti
    if ((polling_server_qd = mq_open (POLLING_SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
		perror ("Errore apertura coda server 1");
		exit (1);
	}

    if ((polling_server_qd_2 = mq_open (POLLING_SERVER_QUEUE_NAME_2, O_WRONLY)) == -1) {
		perror ("Errore apertura coda server 2");
		exit (1);
	}

    //Apro le code dai polling server in sola lettura, le code sono già state create dai polling server e sono bloccanti
    if ((DIAG_qd = mq_open (DIAG_QUEUE_NAME, O_RDONLY)) == -1) {
		perror ("Errore apertura coda DIAG 1");
		exit (1);
	}

    if ((DIAG_qd_2 = mq_open (DIAG_QUEUE_NAME_2, O_RDONLY)) == -1) {
		perror ("Errore apertura coda DIAG 2");
		exit (1);
	}

    //Invio la richiesta di diagnostica ai polling server
    if(mq_send(polling_server_qd, message_request, strlen(message_request)+1, 0)==-1) {
        perror("Errore invio richiesta su coda 1");
        exit(1);
    }

    if(mq_send(polling_server_qd_2, message_request_2, strlen(message_request_2)+1, 0)==-1) {
        perror("Errore invio richiesta su coda 1");
        exit(1);
    }

    printf("Richieste di diagnostica inviate...\n\n");

    //Essendo le code bloccanti e lavorando con un singolo messaggio alla volta sono certo di ricostruire i messaggi nell'ordine giusto

    //******* ASPETTO I MESSAGGI SULLA CODA DIAG 1 *******//
    
    printf("Diagnostica ruota 1:\n");

    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("WCET acquire thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
       //Nessun messaggio ricevuto...
    } else {
        printf("WCET control thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("WCET actuate thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Media del buffer: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Comando di controllo: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 1° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 2° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 3° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Reference: %d\n\n", atoi(message_response));
    }

    //******* ASPETTO I MESSAGGI SULLA CODA DIAG 2 *******//
    
    printf("Diagnostica ruota 2:\n");

    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("WCET acquire thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
       //Nessun messaggio ricevuto...
    } else {
        printf("WCET control thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("WCET actuate thread: %ld nanosecondi\n", strtol(message_response, NULL, 10));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Media del buffer: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Comando di controllo: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 1° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 2° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Valore del buffer in 3° posizione: %d\n", atoi(message_response));
    }
    if(mq_receive(DIAG_qd_2, message_response, MAX_MSG_SIZE, NULL) == -1){
        //Nessun messaggio ricevuto...
    } else {
        printf("Reference: %d\n\n", atoi(message_response));
    }
    
    printf("Processo di diagnostica terminato!\n\n");

    //Chiudo le code
    if (mq_close (polling_server_qd) == -1) {
        perror ("Errore chiusura coda polling server 1");
        exit (1);
    }

    if (mq_close (polling_server_qd_2) == -1) {
        perror ("Errore chiusura coda polling server 2");
        exit (1);
    }

    if (mq_close (DIAG_qd) == -1) {
        perror ("Errore chiusura coda DIAG 1");
        exit (1);
    }

    if (mq_close (DIAG_qd_2) == -1) {
        perror ("Errore chiusura coda DIAG 2");
        exit (1);
    }

    return 0;

}



