#ifndef UARTLIB_H_
#define UARTLIB_H_

#include <stdint.h>
#include <stdbool.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <utils.h>


#define BUFFER_SIZE 128
#define SUM_MOD(a,b) ((a+BUFFER_SIZE+b)%BUFFER_SIZE)

#define CONCAT(a,b) ( (a<<8) | (b & 0xffff) )

#define CHECKSUM(df_p) ( (0x0000 \
- CONCAT(df_p[1],df_p[0]) \
- CONCAT(df_p[3],df_p[2]) \
- CONCAT(df_p[5],df_p[4]) ) & 0xffff )


typedef enum {
	motor_communication_initializing=0,
	motor_stopped,
	motor_starting,
	motor_running,
} infineon_motor_state_t;

typedef enum {
	motor_error_none=0,
	motor_error_unable_to_start,
	motor_error_unable_to_maintain_speed,
	motor_error_fault,
	motor_error_communication
} infineon_motor_errors_t;

typedef enum{
	INDEX_MOTOR_FAULT =0,
	INDEX_MOTOR_SPEED ,
	//INDEX_MOTOR_MOTOR_STATE ,
	INDEX_MOTOR_MOTOR_CURRENT,
	INDEX_MOTOR_TARGET_SPEED,
	INDEX_MOTOR_SIZE
} index_status_motor_t;

typedef enum{ //TODO spostare in index_status_motor_t
	INDEX_PFC_FAULT =0,
	INDEX_PFC_STICAZZI ,
	INDEX_PFC_SIZE
} index_status_pfc_t;

typedef enum{ //TODO spostare in index_status_motor_t
	motor_direction_forward =1,
	motor_direction_reverse =-1,
	motor_direction_size =2
} motor_direction_t;


typedef struct {
	// COMMUNICATION SECTION
	volatile uint8_t buffer[BUFFER_SIZE];
	struct io_descriptor *io;
	volatile uint16_t cnt_begin;
	volatile uint16_t cnt_end;
	uint8_t address;
	// STATUS MEMORY
	int16_t motor_status[INDEX_MOTOR_SIZE];
	//int16_t pfc_status[INDEX_PFC_SIZE];
	infineon_motor_state_t running_status;
	infineon_motor_errors_t error_motor;
	// FREERTOS
	TaskHandle_t xInfineonTask;
	QueueHandle_t command_queue;
	// private variables
	uint32_t timer_motor;
	int16_t minimum_running_speed;
	int16_t maximum_running_speed;
	int16_t coeff_speed_millis;
	motor_direction_t direction;
} infineon_device_t;


#define COMMAND_READ_STATUS                   0
#define COMMAND_CLEAR_FAULT                   1
#define COMMAND_SELECT_CONTROL_INPUT_MODE     2
#define COMMAND_SET_SPEED                     3
#define COMMAND_READ_REGISTER                 5
#define COMMAND_WRITE_REGISTER                6
#define COMMAND_LOAD_PARAMETER_SET            0x20

#define STATUS_FAULT_FLAGS                    0
#define STATUS_MOTOR_SPEED                    1
#define STATUS_MOTOR_STATE                    2
#define STATUS_NODE_ID                        3

#define INPUT_MODE_UART                       0
#define INPUT_MODE_ANALOG                     1
#define INPUT_MODE_FREQ                       2
#define INPUT_MODE_DUTY                       3

#define APP_ID_SYSTEM                         0
#define APP_ID_MOTOR                          1

//registers
#define MOTOR_COMMAND					      120
#define MOTOR_TARGET_SPEED                    121
#define MOTOR_ACTUAL_SPEED                    125
#define MOTOR_MIN_RUNNING_SPEED               38
#define MOTOR_CURRENT                         154
#define MOTOR_FAULT_CLEAR                     134
#define MOTOR_FAULT_FLAGS                     135
#define MOTOR_VOLTAGE                         158

typedef enum{
	FAULT_GATEKILL                     =   0,
	FAULT_DC_OVER_VOLTAGE_CRITICAL     =   1,
	FAULT_DC_OVER_VOLTAGE              =   2,
	FAULT_DC_UNDER_VOLTAGE             =   3,
	FAULT_PLL                          =   4,
	FAULT_OVER_TEMPERATURE             =   6,
	FAULT_ROTOR_LOCK                   =   7,
	FAULT_PHASE_LOSS                   =   8,
	FAULT_OVER_CPU_LOAD                =   10,
	FAULT_PARAMETER_LOAD               =  12,
	FAULT_UART_TIMEOUT                 =   13,
	FAULT_HALL_TIMEOUT                 =   14,
	FAULT_FALL_NVALID                  =   15,
} infineon_fault_t;

#define INFINEON_ERROR_NONE 0
#define INFINEON_ERROR_TIMEOUT -1
#define INFINEON_ERROR_CHECKSUM -2

void init_infineon();

infineon_device_t * get_imc102();
infineon_device_t * get_imm101();

void set_target_speed ( infineon_device_t * dev,  int16_t speed);
//void set_target_speed_percentuale ( infineon_device_t * dev,  uint8_t percentuale);

void clear_fault( infineon_device_t * dev);

void load_configuration( infineon_device_t* dev, uint8_t conf);

//int16_t read_register(infineon_device_t * dev,uint16_t app_id, uint16_t reg);

//void write_register(infineon_device_t * dev,uint16_t app_id, uint16_t reg, int16_t value);

void infineon_task(void *p);

void infineon_ISR(infineon_device_t *dev);

#endif
