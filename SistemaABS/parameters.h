#define TICK_TIME 100000

//******* 1° RUOTA *******//
#define SENSOR_QUEUE_NAME   "/sensor_queue"
#define ACTUATOR_QUEUE_NAME "/actuator_queue"
#define REFERENCE_QUEUE_NAME "/reference_queue"
#define COORDINATION_QUEUE_NAME "/coordination_queue"

//******* 2° RUOTA *******//
#define SENSOR_QUEUE_NAME_2  "/sensor_queue_2"
#define ACTUATOR_QUEUE_NAME_2 "/actuator_queue_2"
#define REFERENCE_QUEUE_NAME_2 "/reference_queue_2"
#define COORDINATION_QUEUE_NAME_2 "/coordination_queue_2"

//******* CODE  DIAG -> POLLING SERVER *******//
#define POLLING_SERVER_QUEUE_NAME "/req_ps"
#define POLLING_SERVER_QUEUE_NAME_2 "/req_ps_2"

//******* CODE  POLLING SERVER -> DIAG *******//
#define DIAG_QUEUE_NAME "/res_ps"
#define DIAG_QUEUE_NAME_2 "/res_ps_2"

//*******
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 1
#define MAX_MSG_SIZE 16
#define BUF_SIZE 3
