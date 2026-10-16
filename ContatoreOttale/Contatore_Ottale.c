/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"



/* define main blinky */
#define mainNO_KEY_PRESS_VALUE              ( -1 )
#define mainRESET_TIMER_KEY                 ( 'r' )

/* Numero di task */
#define TASK_NUM 6

/* Cardinalit  del contatore */
#define COUNTER_CARDINALITY 8

/* Periodo di Base */
#define BASE_PERIOD_MS 10

/* Indice del task di stampa (Polling Server) */
#define PRINT_TASK_POSITION 0

/* Priorit  dei task */
#define	FIRST_COUNTER_TASK_PRIORITY		( tskIDLE_PRIORITY + TASK_NUM )
#define PRINT_TASK_PRIORITY		( tskIDLE_PRIORITY + FIRST_COUNTER_TASK_PRIORITY + 1 ) 

/*-----------------------------------------------------------*/

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/* Definizione dei task da implementare */
static void countTask(void* pvParameters);
static void printTask(void* pvParameters);

/*-----------------------------------------------------------*/

/* Array dei periodi */
static int periods[TASK_NUM];

//To do...
/*
 * Definire la risorsa condivisa contatore "counter" (vettore di interi)
 * e un mutex "xSemaphore" per gestire la mutua esclusione nell'accesso al contatore
 */

static int buffer[TASK_NUM];
static SemaphoreHandle_t xSemaphore;  


/*-----------------------------------------------------------*/

void main_blinky(void)
{
    /* Inizializzare i periodi dei task */
    for (int i = 0; i < TASK_NUM; i++) {

        if (i == 0) {
            periods[i] = BASE_PERIOD_MS;
        }
        else {
            periods[i] = periods[i - 1] * COUNTER_CARDINALITY;
        }
    }

    // To do...
    /*
     * Inizializza il contatore "counter" a 0
    */
    for (int i = 0; i < TASK_NUM; i++) {
        buffer[i] = 0;
    }

    // To do...
    /*
    * Creare il Mutex tramite le API freeRTOS 
    * Verificare che il Mutex sia stato creato correttamente
    */
    if ((xSemaphore = xSemaphoreCreateMutex()) == NULL) {
        perror("Errore creazione mutex\n");
    }

    // To do...
    /*
    * Creare i task periodici secondo Rate Monotonic
    * Creare il Polling Server con priorit  pi  alta di tutti i task periodici
    * Avviare lo scheduler
    */
    for (int i = 0; i < TASK_NUM; i++) {
        xTaskCreate(countTask, "contatore" + i, configMINIMAL_STACK_SIZE, (void*)i, (FIRST_COUNTER_TASK_PRIORITY) - i, NULL);
    }
    printf("Task count creati\n");
    
    xTaskCreate(printTask, "stampa", configMINIMAL_STACK_SIZE, (void*)PRINT_TASK_POSITION, PRINT_TASK_PRIORITY, NULL);
    printf("Task print creato\n");

    vTaskStartScheduler();
  
}
/*-----------------------------------------------------------*/

static void countTask(void* pvParameters)
{
    // To do...
    /*
     * Inizializzare le variabili utili all'implementazione di un task periodico e
     * Gestire il parametro di ingresso
    */
    TickType_t Tempo_corrente;
    int indice = (int)pvParameters;
    const TickType_t Periodo = pdMS_TO_TICKS(periods[indice]);

    Tempo_corrente = xTaskGetTickCount();

    for (;; )
    {
        // To do...
        /*
         * Incrementare periodicamente il contatore utilizzando il mutex per mantere la mutua esclusione
        */
        vTaskDelayUntil(&Tempo_corrente, Periodo);
       

        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        if (buffer[indice] >= (COUNTER_CARDINALITY - 1)) {
            buffer[indice] = 0;
        }
        else {
            buffer[indice] = buffer[indice] + 1;
        }

        xSemaphoreGive(xSemaphore);
    }
}
/*-----------------------------------------------------------*/

static void printTask(void* pvParameters)
{
    // To do...
    /*
     * Inizializzare le variabili utili all'implementazione del Polling Server e
     * Gestire il parametro di ingresso
    */
    TickType_t Tempo_corrente;
    int indice = (int)pvParameters;
    const TickType_t Periodo = pdMS_TO_TICKS(periods[indice]);

    Tempo_corrente = xTaskGetTickCount();


    for (;; )
    {
        //To do...
        /*
         * Implentare il comportamento periodico
        */
        vTaskDelayUntil(&Tempo_corrente, Periodo);

        /* Verificare che   stata premuto un input da tastiera */
        if (_kbhit() != 0)
        {
            /* Rimuovere il carattere letto dall'input buffer */
            (void)_getch();

            // To do....
            /*
             * Stampare il contenuto del contatore utilizzando il semaforo per accedere in maniera mutuamente esclusiva alla risorsa
            */
            xSemaphoreTake(xSemaphore, portMAX_DELAY);

            printf("Contatore: ");
            for (int i = 0; i < TASK_NUM; i++) {
                printf("%d ", buffer[TASK_NUM - (i + 1)]);
            }
            printf("\n");
           

            xSemaphoreGive(xSemaphore);
            

        }
    }
}
/*-----------------------------------------------------------*/

/* Called from prvKeyboardInterruptSimulatorTask(), which is defined in main.c. */
void vBlinkyKeyboardInterruptHandler(int xKeyPressed)
{
    /* Handle keyboard input. */
    switch (xKeyPressed)
    {
    case mainRESET_TIMER_KEY:

        if (xTimer != NULL)
        {
            /* Critical section around printf to prevent a deadlock
               on context switch. */
            taskENTER_CRITICAL();
            {
                printf("\r\nResetting software timer.\r\n\r\n");
            }
            taskEXIT_CRITICAL();

            /* Reset the software timer. */
            xTimerReset(xTimer, portMAX_DELAY);
        }

        break;

    default:
        break;
    }
}

