#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "buffered_printf.h"

static serial_device_t io_ext;

static char str_out[BUFFERED_PRINTF_BUFFER_SIZE];
static char str_tmp[BUFFERED_PRINTF_BUFFER_SIZE];


#define SUM_MOD(a,b) ((a+BUFFERED_PRINTF_BUFFER_SIZE+b)%BUFFERED_PRINTF_BUFFER_SIZE)



uint8_t myread(char* tmp_buffer){
	if(io_ext.buffer_in[io_ext.cnt_in_begin]==0){
		return 0;
	}
	//return 1 if message found, 0 otherwise
	//be sure to pass a tmp_buffer long enough
	for (int i=0; i<BUFFERED_PRINTF_BUFFER_SIZE; i++){
		if (io_ext.buffer_in[SUM_MOD(io_ext.cnt_in_begin,i)]=='\n'){
			for (int j=0; j<=i; j++){
				tmp_buffer[j]=io_ext.buffer_in[SUM_MOD(io_ext.cnt_in_begin,j)];
				io_ext.buffer_in[SUM_MOD(io_ext.cnt_in_begin,j)]=0;
			}
			tmp_buffer[i]=0;
			//critical section??
			io_ext.cnt_in_begin=SUM_MOD(io_ext.cnt_in_begin,i+1);
			return 1;
		}
	}
	return 0;
	//start from io_ext.cnt
	//search for a 0
	//if found, copy to a buffer, clear my buffer, update cnt
}

void myprintf(const char *format, ...){
	// my printf can be called from any task, and requires a buffer to format the string and place in on the ringbuffer
	// heap could be used, but stack has been preferred. str_tmp serve this purpose: it is outside 
	va_list args;
	va_start(args, format);
	vsnprintf(str_tmp,BUFFERED_PRINTF_BUFFER_SIZE,format, args);
	int i;
	for (i=0; i<strlen(str_tmp); i++){
		io_ext.buffer_out[SUM_MOD(io_ext.cnt_out_end,i)]=str_tmp[i];
	}
	str_tmp[i]=0;
	
	io_ext.cnt_out_end=SUM_MOD(io_ext.cnt_out_end, strlen(str_tmp));
}

void buffered_printf_send(){
	// the device on which this has been tested does not offer a memory buffer for the USART output.
	// str_out is this buffer, and must survive buffered_printf_send invocation 
	if(io_ext.is_tx_empty()){
		if (io_ext.cnt_out_begin!=io_ext.cnt_out_end)
		{
			int i;
			for (i=0; i<SUM_MOD(0-io_ext.cnt_out_begin,io_ext.cnt_out_end); i++){
				str_out[i]=io_ext.buffer_out[SUM_MOD(io_ext.cnt_out_begin,i)];
			}
			str_out[i]=0;
			io_ext.write((uint8_t*)str_out,i);
			io_ext.cnt_out_begin=io_ext.cnt_out_end;
		}
	}
}

static void async_receive_callback() 
{
	io_ext.buffer_in[io_ext.cnt_in_end] = io_ext.getc();
	io_ext.cnt_in_end=SUM_MOD(io_ext.cnt_in_end,1);
}

void buffered_printf_init(){
	for (int i=0; i<BUFFERED_PRINTF_BUFFER_SIZE; i++){
		io_ext.buffer_in[i]=0;
	}
	io_ext.cnt_out_begin=0;
	io_ext.cnt_out_end=0;
	io_ext.cnt_in_begin=0;
	io_ext.cnt_in_end=0;

	
}
