
#include "atmel_start.h"
#include "serial_SATA_protocol.h"
#include "serial_packet_manager/serial_packet_manager.h"

static validated_field *routine_conf;
static int32_t *routine_status;
static validated_field *machine_conf;
static int32_t *machine_status;

static uint32_t routine_conf_size;
static uint32_t routine_status_size;
static uint32_t machine_conf_size;
static uint32_t machine_status_size;

void serial_SATA_init(	validated_field *p_routine_conf, uint32_t p_routine_conf_size,
int32_t *p_routine_status, uint32_t p_routine_status_size,
validated_field *p_machine_conf, uint32_t p_machine_conf_size,
int32_t *p_machine_status, uint32_t p_machine_status_size){
	routine_conf=p_routine_conf;
	routine_conf_size = p_routine_conf_size;
	
	routine_status=p_routine_status;
	routine_status_size=p_routine_status_size;
	
	machine_conf=p_machine_conf;
	machine_conf_size=p_machine_conf_size;
	
	machine_status=p_machine_status;
	machine_status_size=p_machine_status_size;
	packet_manager_routine_init();
}

void send_message( message_t *message){
	packet_manager_send_data((uint8_t *)message, 8);
}


void write_conf(uint16_t sender_addr, message_t *message, validated_field* conf, uint8_t conf_size){
	message_t reply_msg;
	reply_msg.device_address=sender_addr;
	if((message->parameter_address >=0) && (message->parameter_address < conf_size)){
		if(conf[message->parameter_address].validate_value(message->value)){
			conf[message->parameter_address].value=message->value;
			reply_msg.command_type=ack;
			reply_msg.parameter_address=message->parameter_address;
			reply_msg.value=message->value;
			send_message(&reply_msg);
			//ack
		}
		else{
			reply_msg.command_type=nack_invalid_value;
			reply_msg.parameter_address=message->parameter_address;
			reply_msg.value=message->value;
			send_message(&reply_msg);
			//nack invalid value
		}
	}
	else{
		reply_msg.command_type=nack_invalid_address;
		reply_msg.parameter_address=message->parameter_address;
		send_message(&reply_msg);
		//nack invalid address
	}
}

void read_status (uint16_t sender_addr, message_t *message, int32_t *status, uint8_t status_size, command_type_t reply_type){
	message_t reply_msg;
	reply_msg.device_address=sender_addr;
	reply_msg.parameter_address= message->parameter_address;
	if((message->parameter_address >= 0) && (message->parameter_address < status_size )){
		reply_msg.command_type = reply_type;
		reply_msg.value=status[message->parameter_address];
		send_message(&reply_msg);
	}
	else{
		reply_msg.command_type=nack_invalid_address;
		reply_msg.parameter_address=message->parameter_address;
		send_message(&reply_msg);
	}
}


void read_conf(uint16_t sender_addr, message_t *message, validated_field *conf, uint8_t conf_size, command_type_t reply_type){
	message_t reply_msg;
	reply_msg.device_address=sender_addr;
	reply_msg.parameter_address= message->parameter_address;
	if((message->parameter_address >= 0) && (message->parameter_address < conf_size )){
		reply_msg.command_type = reply_type;
		reply_msg.value=conf[message->parameter_address].value;
		send_message(&reply_msg);
	}
	else{
		reply_msg.command_type=nack_invalid_address;
		send_message(&reply_msg);
	}
}

void serial_SATA_receive_message(uint16_t sender_addr, message_t *message){
	//broadcast messages?
	if((message->device_address != machine_conf[can_address].value) && (message->device_address != 0) ){
		return;
	}

	switch (message->command_type){
		case read_routine_conf_parameter:{
			read_conf(sender_addr, message, routine_conf, routine_conf_size, reply_routine_conf_parameter);
			break;
		};
		case read_machine_conf_parameter:{
			read_conf(sender_addr, message, machine_conf, machine_conf_size, reply_machine_conf_parameter);
			break;
		};
		case read_routine_status_paramater:{
			read_status (sender_addr, message, routine_status, routine_status_size, reply_routine_status_parameter);
			break;
		};
		case read_machine_status_parameter:{
			read_status (sender_addr, message, machine_status, machine_status_size, reply_machine_status_parameter);
			break;
		};
		case write_routine_conf_parameter:{
			write_conf(sender_addr,message,routine_conf,routine_conf_size);
			VF_write_on_flash(routine_conf, routine_conf_size, FLASH_MEMORY_ROUTINE_CONF );
			break;
		};
		case write_machine_conf_parameter:{
			write_conf(sender_addr,message,machine_conf,machine_conf_size);
			VF_write_on_flash(machine_conf, machine_conf_size, FLASH_MEMORY_MACHINE_CONF );
			break;
		};
		case hello:{
			//reply ack
			//or... reply hello, not in broadcast
			break;
		};
		// 		case reply_routine_conf_parameter:{
		// 			break;
		// 		};
		// 		case reply_routine_status_paramater:{
		// 			break;
		// 		};
		// 		case reply_machine_status_parameter:{
		// 			break;
		// 		};
		// 		case ack:{
		// 			break;
		// 		};
		// 		case nack:{
		// 			break;
		// 		};
		default:{
			break;
		};
	}
}
