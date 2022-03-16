#ifndef MYPRINTF_H_
#define MYPRINTF_H_

#include <stdint.h>
#include <stdbool.h>
// in the environment that I use printf requires a synchronous UART
// which poorly integrates with a freeRTOS environment, with multiple tasks
// interacting 

#define USE_SERIAL_MONITOR 1

#if USE_SERIAL_MONITOR == 1
#define BUFFERED_PRINTF_BUFFER_SIZE 1024
#else
#define SERIAL_BUFFER_SIZE 1
#endif


typedef struct {
	volatile uint8_t buffer_in[BUFFERED_PRINTF_BUFFER_SIZE];
	uint8_t buffer_out[BUFFERED_PRINTF_BUFFER_SIZE];
	uint16_t cnt_in_begin;
	volatile uint16_t cnt_in_end;
	uint16_t cnt_out_begin;
	uint16_t cnt_out_end;

	bool (*is_tx_empty)(); 
	void (*write)(uint8_t *str_out, uint16_t length);
	uint8_t (*getc)();
} serial_device_t;



void buffered_printf(const char *format, ...);
uint8_t buffered_read(char* buffer); //works as scanf("%s\n",buffer)
void buffered_printf_init();

#endif /* MYPRINTF_H_ */
