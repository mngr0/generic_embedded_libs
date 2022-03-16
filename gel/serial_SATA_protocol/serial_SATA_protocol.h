#ifndef SERIAL_SATA_H_
#define SERIAL_SATA_H_

#include "AIR_REF/validator_field.h"

//TODO INCLUDE GENERIC SATA HEADER
#define can_address 0

typedef enum{
	read_routine_conf_parameter=0,
	read_routine_status_paramater,
	read_machine_conf_parameter,
	read_machine_status_parameter,
	
	write_routine_conf_parameter,
	write_machine_conf_parameter,
	reply_routine_conf_parameter,
	reply_routine_status_parameter,
	reply_machine_conf_parameter,
	reply_machine_status_parameter,
	reboot,
	hello,
	ack,
	nack,
	nack_invalid_address,
	nack_invalid_value,
}command_type_t;

typedef struct {
	uint16_t device_address:16;
	command_type_t command_type:8;
	uint8_t parameter_address:8;
	uint32_t value:32;
}message_t;


void serial_SATA_init(	validated_field *p_routine_conf, uint32_t p_routine_conf_size,
int32_t *p_routine_status, uint32_t p_routine_status_size,
validated_field *p_machine_conf, uint32_t p_machine_conf_size,
int32_t *p_machine_status, uint32_t p_machine_status_size);

void serial_SATA_receive_message(uint16_t sender_addr, message_t *message);

#endif