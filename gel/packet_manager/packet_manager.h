
/*

Serialization library.

This library is made to manage the transmission of data with variable size over a stream-like communication (UART).
It defines a packet, described by length, checksum, protocol version, and start/end delimiters. 

Start and End delimiters are made with repeated bytes. 
Given a stream of packets, the delimiters are used to maintain the synchronization with the sender:
if a byte is lost the packet is discarded (because of wrong checksum) and the consequent packet will be successfully recognized.
Since a particular sequence is used in the delimiter it is not possible to use it in the data. 
Hopefully this will not break any communication.

Data structure has to be managed at a higher level, supposedly using protocols like json/protobuf/flatbuf, or similar.

The underlying level is supposed to manage the device addresing.
This library is developed while using a UART communication. So the network is supposed to be made of 2 devices.


In the future will adapt this library to CAN, which will change the assumption:
CAN already defines a packet, which has a maximum size of 64 Bytes.
If it will not be enough this library will expand to consider framing of the packets.


*/


#ifndef SERIAL_LOGGER_FRAME_H_
#define SERIAL_LOGGER_FRAME_H_

#include <stddef.h>
#include <stdint.h>

#define PROTOCOL_VERSION 1
#define START_OF_PACKET 0xa2
#define END_OF_PACKET 0xdc

//hardware specific function that actually send data
//possible improvement: pass it as pointer
extern int send_buffer(uint8_t* data, int lenght);



#define DELIMITER_SIZE 3
#define HEADER_SIZE DELIMITER_SIZE+3//1 for protocol version, 2 for size
#define FOOTER_SIZE DELIMITER_SIZE+2 //2 for checksum


#define FRAME_MAX_PACKET_SIZE 500 //max size of the frame in the message
#define LOGGER_BUF_SIZE 1024 // size of the phisical buffer 
#define LOGGER_SUM_MOD(a,b) ((a+LOGGER_BUF_SIZE+b)%LOGGER_BUF_SIZE)
#define LOGGER_I(i) LOGGER_SUM_MOD(i,io_logger.cnt_in_begin)

//management of the ringbuffer for received data
typedef struct {
	volatile uint8_t buffer_in[LOGGER_BUF_SIZE];
	uint16_t cnt_in_begin;
	volatile uint16_t cnt_in_end;
} packet_ringbuffer_t;

typedef struct {
	uint8_t start_of_frame[DELIMITER_SIZE];
	uint8_t protocol_version;
	uint16_t frame_size;
	uint8_t buffer[FRAME_MAX_PACKET_SIZE];
	uint16_t checksum;
	uint8_t end_of_frame[DELIMITER_SIZE];

}packet_received_t;

#define DELIMITER_PACKET(buffer,size) \
	buffer[0]=START_OF_PACKET; \
	buffer[1]=START_OF_PACKET; \
	buffer[2]=START_OF_PACKET; \
	buffer[HEADER_SIZE + size + 0]=END_OF_PACKET; \
	buffer[HEADER_SIZE + size + 1]=END_OF_PACKET; \
	buffer[HEADER_SIZE + size + 2]=END_OF_PACKET; \


//receive data to send as parameter and send it as a valid packet
//possible improvement: frame the packet if size is greater than FRAME_MAX_PACKET_SIZE
int packet_manager_send_data(uint8_t *data, size_t size);

//serialization peripheral might not manage packetization, for example UART (CAN does).
//if data is received partially, then it must be buffered in order to search for valid packets
void packet_manager_put(packet_ringbuffer_t *io_logger,uint8_t *new_buf, int16_t length);


//searches in the ringbuffer for packets. It checks only for SOP and EOP
//later checks, like protocol version, and checksum validity are left to the receive_frame
//possible improvement: unify the 2 functions
int16_t packet_manager_pop(packet_ringbuffer_t *io_logger,uint8_t *frame_buf);


//checks for protocol version, and checksum validity.
//it checks also that declared packet length and actual length are the same
int8_t packet_is_valid(packet_received_t *reply, uint8_t *data, int length);

void packet_manager_init(packet_ringbuffer_t *io_logger);


#endif