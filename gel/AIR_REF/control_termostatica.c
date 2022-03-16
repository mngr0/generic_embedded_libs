#include <stdint.h>
#include "AIR_REF.h"

#include "led/led.h"
#include "myprintf/myprintf.h"
#include "freertos_task_conf.h"
#include "drv8889/drv8889.h"

static uint8_t delay_time;

#define FAST_SPEED 10
#define SLOW_SPEED 100

void set_delay_time(uint8_t par){
	delay_time=par;
}

void stepper_task(void *par){
	int32_t *local_ar_status= (int32_t*)par;
	delay_time=FAST_SPEED;//TODO ALREADY INITIALIZED
	while (1)	{
		drv_update_pos();
		local_ar_status[termostatica_step_current_position] = drv_get_pos();
		os_sleep(delay_time);
	}
}

bool termostatica_set_status(int32_t *ar_status, termostatica_machine_states_t new_status){
	
	if(ar_status[termostatica_status] == termostatica_starting){
		return false;
	}
	else if((ar_status[termostatica_status] == termostatica_full_close) && (new_status == termostatica_fixed_position)){
		if(ar_status[termostatica_step_current_position] != 0){
			return false;
		}
		else{
			set_delay_time(FAST_SPEED);
			ar_status[termostatica_status] = termostatica_fixed_position;
			ar_status[termostatica_timestamp_initial_position] = xTaskGetTickCount();
			return true;
		}
	}
	else if(((ar_status[termostatica_status] == termostatica_modulating) ||
	(ar_status[termostatica_status] == termostatica_fixed_position))
	&& (new_status == termostatica_full_close)){
		ar_status[termostatica_step_current_position] += 20;
		ar_status[termostatica_step_target] = 0;
		ar_status[termostatica_I_value]=0;
		set_delay_time(SLOW_SPEED);
		ar_status[termostatica_status] = termostatica_full_close;
		return true;
	}
	else if((ar_status[termostatica_status] == termostatica_fixed_position) && (new_status == termostatica_fixed_position)){
		ar_status[termostatica_timestamp_initial_position] = xTaskGetTickCount();
		return true;
	}
	else{
		myprintf("TERMOSTATICA STATUS CHANGE FROM %d TO %d IGNORED\n",ar_status[termostatica_status],new_status);
		return false;
	}
}


void termostatica_init(validated_field *ar_conf, int32_t *ar_status){
	gpio_set_pin_level(drv_nSLEEP,1);
	drv_init();
	ar_status[termostatica_status] = termostatica_starting;
	led_set_state(led_state_blue);
	//start thread stepper
	set_delay_time(SLOW_SPEED);
	// close valve
	ar_status[termostatica_I_value]=0;
	ar_status[termostatica_step_current_position] = ar_conf[termostatica_max_step].value;
	ar_status[termostatica_step_target] = 0;
	drv_set_pos(ar_status[termostatica_step_current_position]);
	drv_set_target_pos(ar_status[termostatica_step_target]);
	while (ar_status[termostatica_step_current_position] != ar_status[termostatica_step_target]){
		os_sleep(100);
	}
	ar_status[termostatica_status] = termostatica_full_close;
}

// void stai_un_po_aperto(int32_t *ar_status){
// 	ar_status[termostatica_I_value] =0;
// }

void termostatica_control(validated_field *ar_conf, int32_t *ar_status, int32_t* m_status){
	
	switch (ar_conf[control_type_termostatica].value){
		case control_type_termostatica_manual: {
			ar_status[termostatica_step_target] = ar_conf[termostatica_manual_position].value;
			break;
		}
		case control_type_termostatica_automatic: {
			
			if (ar_status[termostatica_status] == termostatica_modulating){
				
				int32_t surriscaldo = m_status[temperature_extra] - m_status[evaporation_temperature];
				int32_t diff = surriscaldo-ar_conf[termostatica_surriscaldo_setpoint].value;
				int32_t Pcorr = (diff * ar_conf[termostatica_coeff_P].value )/1000;

				//int32_t Dcorr = 512*(diff-last_diff)*Dcoeff /100;
				//last_diff = diff;

				ar_status[termostatica_I_value] += (diff * ar_conf[termostatica_coeff_I].value)/1000;
				ar_status[termostatica_I_value] = max(ar_status[termostatica_I_value] , -ar_conf[termostatica_coeff_I_max].value);
				ar_status[termostatica_I_value] = min(ar_status[termostatica_I_value] , ar_conf[termostatica_coeff_I_max].value);

				int32_t tmp_step_target = ar_conf[termostatica_max_step].value/2 + (Pcorr+ar_status[termostatica_I_value])/1000;
				tmp_step_target = max(0, tmp_step_target);
				tmp_step_target = min(ar_conf[termostatica_max_step].value, tmp_step_target);

				ar_status[termostatica_step_target] = tmp_step_target;
			}
			else if(ar_status[termostatica_status] == termostatica_fixed_position){
				ar_status[termostatica_step_target] = ar_conf[termostatica_initial_fixed_position].value;
				if(xTaskGetTickCount() - ar_status[termostatica_timestamp_initial_position] > ar_conf[termostatica_interval_fixed_poistion].value){
					ar_status[termostatica_status] = termostatica_modulating;
					ar_status[termostatica_I_value]=(ar_conf[termostatica_initial_fixed_position].value-256)*1000;
					ar_status[termostatica_I_value] = max(ar_status[termostatica_I_value] , -ar_conf[termostatica_coeff_I_max].value);
					ar_status[termostatica_I_value] = min(ar_status[termostatica_I_value] , ar_conf[termostatica_coeff_I_max].value);
					set_delay_time(SLOW_SPEED);
				}
			}
			break;
		}
	}
	drv_set_target_pos(ar_status[termostatica_step_target]);
	//myprintf("EVENT: STP=%d\n", ar_status[termostatica_step_target]);
}

