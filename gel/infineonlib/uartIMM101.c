#include <atmel_start.h>
#include "infineonlib.h"
#include "uartIMM101.h"

#include "freertos_task_conf.h"
#include "myprintf/myprintf.h"

static infineon_device_t imm101;

infineon_device_t * get_imm101(){
	return &imm101;
}


static void rx_cb_USART_0_imm(const struct usart_async_descriptor *const io_descr)
{
	infineon_ISR(&imm101);
}

void init_imm101(){
	usart_async_get_io_descriptor(&USART_IMM101, &imm101.io);
	usart_async_register_callback(&USART_IMM101, USART_ASYNC_RXC_CB, rx_cb_USART_0_imm);
	usart_async_enable(&USART_IMM101);
	imm101.address = 0x4;
	imm101.coeff_speed_millis = 19279;
	imm101.minimum_running_speed = 0*imm101.coeff_speed_millis/1000;
	imm101.maximum_running_speed = 900*imm101.coeff_speed_millis/1000;
	imm101.direction = motor_direction_reverse;
	imm101.cnt_begin=0;
	imm101.cnt_end=0;
	imm101.running_status= motor_communication_initializing;
	//create Q
	imm101.command_queue = xQueueCreate(5,8);
	
	//start infineon task
 	NVIC_SetPriority(SERCOM3_0_IRQn,5);
 	NVIC_SetPriority(SERCOM3_1_IRQn,5);
 	NVIC_SetPriority(SERCOM3_2_IRQn,5);
 	NVIC_SetPriority(SERCOM3_3_IRQn,5);
	if (xTaskCreate(
	infineon_task, "IMM101", TASK_IMM101_STACK_SIZE, &imm101, TASK_IMM101_STACK_PRIORITY, &(imm101.xInfineonTask))
	!= pdPASS) {
		while (1) {
			;
		}
	}
	
}


