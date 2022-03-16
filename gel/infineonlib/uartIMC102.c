#include <atmel_start.h>
#include "uartIMC102.h"
#include "infineonlib.h"

#include <FreeRTOS.h>
#include <task.h>
#include <hal_rtos.h>

#include "freertos_task_conf.h"

#include "myprintf/myprintf.h"

static infineon_device_t imc102;


static void rx_cb_USART_imc(const struct usart_async_descriptor *const io_descr)
{
	infineon_ISR(&imc102);
}

void init_imc102(){
	usart_async_get_io_descriptor(&USART_IMC102, &imc102.io);
	usart_async_register_callback(&USART_IMC102, USART_ASYNC_RXC_CB, rx_cb_USART_imc);
	usart_async_enable(&USART_IMC102);
	imc102.address = 0x1;
	imc102.coeff_speed_millis = 2270;
	imc102.minimum_running_speed = 0*imc102.coeff_speed_millis/1000;
	imc102.maximum_running_speed = 7200*imc102.coeff_speed_millis/1000;
	imc102.direction = motor_direction_forward;
	imc102.cnt_begin=0;
	imc102.cnt_end=0;
	imc102.running_status= motor_communication_initializing;
	
	//create Q
	imc102.command_queue = xQueueCreate(5,8);
	
	//start infineon task
	NVIC_SetPriority(SERCOM1_0_IRQn,5);
	NVIC_SetPriority(SERCOM1_1_IRQn,5);
	NVIC_SetPriority(SERCOM1_2_IRQn,5);
	NVIC_SetPriority(SERCOM1_3_IRQn,5);
	if (xTaskCreate(
	infineon_task, "IMC102", TASK_IMC102_STACK_SIZE, &imc102, TASK_IMC102_STACK_PRIORITY, &(imc102.xInfineonTask))
	!= pdPASS) {
		while (1) {
			;
		}
	}
}

infineon_device_t * get_imc102(){
	return &imc102;
}

