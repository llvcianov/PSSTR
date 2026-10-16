/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Numero task generatori */
#define NGENERATORS 3

/* Periodo base */
#define BASE_PERIOD_MS 10

/* Priorità dei task */
#define	FIRST_GENERATOR_TASK_PRIORITY		( tskIDLE_PRIORITY + NGENERATORS + 1)
#define RECOGNIZER_TASK_PRIORITY		( tskIDLE_PRIORITY + FIRST_GENERATOR_TASK_PRIORITY + 10 ) 
#define BUDDY_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)

/* Strutture Dati */
struct seq_str
{
	unsigned int bit[3];
	SemaphoreHandle_t mutex_gen_rec;
};
static struct seq_str seq_data;

struct shr_data_str
{
	unsigned int ok;
	unsigned int count;
	SemaphoreHandle_t mutex_bud_rec;
};
static struct shr_data_str shared_data;

/* Periods and phases array */
static int periods[NGENERATORS];
static int coeff_phases[NGENERATORS] = { 1,32,75 };
static int phases[NGENERATORS];

/*----------------------------------- TASK -----------------------------------*/

static void genTask(void* pvParameters) {
	
	TickType_t xNextWakeTime;
	int index = (int)pvParameters;

	/* Converto i periodi e la fasi in TICK*/
	const TickType_t xBlockTime = pdMS_TO_TICKS(periods[index]);
	const TickType_t xPhases = pdMS_TO_TICKS(phases[index]);

	/*Faccio partire i generatori sfasati*/
	vTaskDelay(xPhases);

	/*Prendo il tempo corrente*/
	xNextWakeTime = xTaskGetTickCount();

	printf("Start Generator Task %d\n", index);

	for (;; ) {
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		xSemaphoreTake(seq_data.mutex_gen_rec, portMAX_DELAY);
		if (seq_data.bit[index]) {
			seq_data.bit[index] = 0;
		}
		else {
			seq_data.bit[index] = 1;
		}
		xSemaphoreGive(seq_data.mutex_gen_rec);
	}
}

static void recTask(void* pvParameters) {

	TickType_t xNextWakeTime;
	int index = (int)pvParameters;

	/*Converto il periodo in TICK*/
	const TickType_t xBlockTime = pdMS_TO_TICKS(periods[index]);

	printf("Start Recognizer Task\n");

	unsigned short int sequence[4] = { 0,3,6,5 };	//sequenza da riconoscere
	unsigned short int decimal[4] = { 0,0,0,0 };	//sequenza corrente
	unsigned short int bits[3];						//bit letti dalla memoria
	unsigned short int state = 0;					// state = 1 se la sequenza è stata trovata
	int i = 0;

	/*Prendo il tempo corrente*/
	xNextWakeTime = xTaskGetTickCount();

	for (;; ) {
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		/*Leggo i bit dalla shared memory*/
		xSemaphoreTake(seq_data.mutex_gen_rec, portMAX_DELAY);
		for (i = 0; i < NGENERATORS; i++) {
			bits[i] = seq_data.bit[i];
		}
		xSemaphoreGive(seq_data.mutex_gen_rec);

		/*Effettuo la conversione dei bit letti e aggiorno la sequenza corrente*/
		state = 1;
		for (i = 0; i < 3; i++) {
			decimal[i] = decimal[i + 1];	//shift verso sx dei valori dell'array
		}
		decimal[3] = 1 * bits[0] + 2 * bits[1] + 4 * bits[2];	//effettuo la conversione dei bit in posizione 4 dell'array

		/*Controllo che sia la sequenza giusta*/
		for (i = 0; i < 4; i++) {
			if (decimal[i] != sequence[i]) state = 0;
		}

		xSemaphoreTake(shared_data.mutex_bud_rec, portMAX_DELAY);
		if (state) {
			//se la sequenza è stata trovata aumento il contatore e imposto ok = 1
			shared_data.ok = 1;
			shared_data.count++;
		}
		else {
			shared_data.ok = 0;
		}
		xSemaphoreGive(shared_data.mutex_bud_rec);

		/*DEBUG print sequence*/
		printf("current sequence: %d %d %d %d\n", decimal[0], decimal[1], decimal[2], decimal[3]);
		//printf("sequence to reco: %d %d %d %d\n", sequence[0], sequence[1], sequence[2], sequence[3]);

	}

}

static void buddyTask(void* pvParameters) {

	TickType_t xNextWakeTime;
	int index = (int)pvParameters;

	/*Converto il periodo in TICK*/
	const TickType_t xBlockTime = pdMS_TO_TICKS(periods[index]);
	
	printf("Start Buddy Task\n");
	int i = 0;

	/*Prendo il tempo corrente*/
	xNextWakeTime = xTaskGetTickCount();

	for (;; ) {
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		/* Stampa dei valori ok e count dalla shared memory */
		xSemaphoreTake(shared_data.mutex_bud_rec, portMAX_DELAY);
		printf("Numero sequenze riconosciute: %d \t", shared_data.count);
		if (shared_data.ok) {
			printf("OK! \n");
		}
		else {
			printf("\n");
		}
		xSemaphoreGive(shared_data.mutex_bud_rec);
	}
}

/*----------------------------------- MAIN -----------------------------------*/

void main_recognizer(void){

	/* Inizializzazione array di bit periodi e fase */
	for (int i = 0; i < NGENERATORS; i++) {
		if (i == 0) {
			periods[i] = BASE_PERIOD_MS;
		}
		else {
			periods[i] = (periods[i - 1] * 2);
		}
		seq_data.bit[i] = 0;
	}

	for (int i = 0; i < NGENERATORS; i++) {
		phases[i] = coeff_phases[i] * BASE_PERIOD_MS;
	}

	/* Creo i mutex */
	seq_data.mutex_gen_rec = xSemaphoreCreateMutex();
	shared_data.mutex_bud_rec = xSemaphoreCreateMutex();

	/* Se i mutex non vengono creati rimango in un for infinito */
	if ((seq_data.mutex_gen_rec != NULL) && (shared_data.mutex_bud_rec != NULL)) {
		printf("Semafori creati\n");

		/*Creo i task*/
		for (int i = 0; i < NGENERATORS; i++) {
			xTaskCreate(genTask, "GenTask_" + i, configMINIMAL_STACK_SIZE, (void*)i, (FIRST_GENERATOR_TASK_PRIORITY)-i, NULL);
			printf("GeneratorTask-%d created\n", i);
		}

		xTaskCreate(recTask, "RecTask", configMINIMAL_STACK_SIZE, (void*)0, RECOGNIZER_TASK_PRIORITY, NULL);
		printf("RecognizerTask created\n");

		xTaskCreate(buddyTask, "BuddyTask", configMINIMAL_STACK_SIZE, (void*)0, BUDDY_TASK_PRIORITY, NULL);
		printf("BuddyTask created\n");

		/* Start Scheduler */
		vTaskStartScheduler();
	}

	perror("Errore creazione semafori\n");

	for (;; );

}







