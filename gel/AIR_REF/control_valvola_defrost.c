#include <stdint.h>
#include "AIR_REF/AIR_REF.h"

#include <stdint.h>
#include "AIR_REF/AIR_REF.h"

#include "myprintf/myprintf.h"

void control_relay_defrost(validated_field *ar_conf, int32_t* ar_status, int32_t * m_status){
	
	switch (ar_conf[control_type_relay_defrost].value){
		case control_type_relay_defrost_manual_on: {
			if (gpio_get_pin_level(RELE2)!=1){
				myprintf("OPEN RELE1\n");
				gpio_set_pin_level(RELE2,1);
			}
			break;
		}
		case control_type_relay_defrost_manual_off: {
			if (gpio_get_pin_level(RELE2)!=0){
				myprintf("CLOSE RELE1\n");
				gpio_set_pin_level(RELE2,0);
			}
			break;
		}
		case control_type_relay_defrost_automatic: {
			if (ar_status[air_ref_status] == air_ref_status_run_defrost){
				if (gpio_get_pin_level(RELE2)!=1){
					myprintf("OPEN RELE1\n");
					gpio_set_pin_level(RELE2,1);
				}
			}
			else{
				if (gpio_get_pin_level(RELE2)!=0){
					myprintf("CLOSE RELE1\n");
					gpio_set_pin_level(RELE2,0);
				}
			}
			break;
		}
	}
}

