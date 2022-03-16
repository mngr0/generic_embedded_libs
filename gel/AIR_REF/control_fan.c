#include <stdint.h>
#include <stdbool.h>

#include "AIR_REF.h"

#define abs(a) (a>0?a:-a)

#include "myprintf/myprintf.h"

void fan_control(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status, machine_interface_t *m_interface){
	
	switch (ar_conf[control_type_fan].value){
		case control_type_fan_manual_off: {
			ar_status[fan_speed_to_command] = 0;
			break;
		}
		case control_type_fan_manual_speed: {
			ar_status[fan_speed_to_command] = m_interface->imm101->maximum_running_speed* ar_conf[fan_manual_speed].value/100;
			break;
		}
		case control_type_fan_automatic: {
			if(ar_status[fan_status]==fan_status_disabled){
				ar_status[fan_speed_to_command] = 0;
			}
			else if (m_status[condensation_pressure] < ar_conf[fan_min_pressure].value)	{
				ar_status[fan_speed_to_command] = 0;
			}
			else if (m_status[condensation_pressure] > ar_conf[fan_max_pressure].value){
				ar_status[fan_speed_to_command] = m_interface->imc102->maximum_running_speed;
			}
			else{
				ar_status[fan_speed_to_command] = (m_status[condensation_pressure] - ar_conf[fan_target_pressure].value)/100
				* ar_conf[fan_coeff_P].value + ar_conf[fan_coeff_offset].value;
				if(ar_status[fan_speed_to_command]<0){
					ar_status[fan_speed_to_command]=0;
				}
				//if(ar_state->fan_speed_to_command)
				// 		myprintf("FAN DEBUG: diff:%d\tP:%d\tfinal:%d\n",
				// 				(m_state->condensation_pressure - ar_status[fan_target_pressure),
				// 				(m_state->condensation_pressure - ar_status[fan_target_pressure)/100* ar_status[fan_coeff_P,
				// 				(m_state->condensation_pressure - ar_status[fan_target_pressure)/100* ar_status[fan_coeff_P + ar_status[fan_coeff_offset);
			}
			break;
		}
	}
	
	

}

bool fan_check_protection(int32_t *ar_status, int32_t *m_status,machine_interface_t *m_interface){
	
	if(m_status[stato_sonda_pressione_cond]!=error_sonda_none){
		ar_status[fan_speed_to_command] = m_interface->imm101->maximum_running_speed/2;
	}
	
	
	//check that target pressure is eventually reached
	//timeout_1 : increase P
	//timeout_2 : increase Offset
	//timeout_3 : full_speed
	//timeout_4 : declare error (stop and restart)
	
	//if sonda scollegata full speed
	return (m_interface->imm101->error_motor != motor_error_none) ;
}




void fan_actuate(int32_t *ar_status, machine_interface_t *m_interface){
	//apply actions
	if ( abs(m_interface->imm101->motor_status[INDEX_MOTOR_TARGET_SPEED]) != ar_status[fan_speed_to_command]){
		if(xTaskGetTickCount() > ar_status[fan_time_last_command]+2000){
			set_target_speed(m_interface->imm101, ar_status[fan_speed_to_command]);
			myprintf("EVENT: FAN speed change to %d\n",ar_status[fan_speed_to_command]);
			ar_status[fan_time_last_command]=xTaskGetTickCount();
		}
	}
}


void fan_init(int32_t *ar_status, machine_interface_t *m_interface){
	uint8_t counter_timeout=0;
	while (m_interface->imm101->running_status ==motor_communication_initializing){
		os_sleep(100);
		counter_timeout++;
		if (counter_timeout>30){
			//led rosso and break
			myprintf("VENTOLA NO COMM\n");
			air_ref_state_change_critical_error(ar_status);
			break;
		}
	}
	ar_status[fan_time_last_command]=xTaskGetTickCount();
	if(m_interface->imm101->motor_status[INDEX_MOTOR_TARGET_SPEED]!=0){
		myprintf("detected running fan: %d\n",m_interface->imm101->motor_status[INDEX_MOTOR_TARGET_SPEED]);
	}
}
