#include <stdint.h>
#include "AIR_REF/AIR_REF.h"

#include "myprintf/myprintf.h"


//separate control and actuate
//add fiend relay_required_status in ar_status
//actuate bypass protection

void valvola_iniezione_liquido_control(validated_field *ar_conf, int32_t * m_status){
	
	switch (ar_conf[control_type_relay_iniezione_liquido].value){
		case control_type_relay_iniezione_liquido_manual_on: {
			if (gpio_get_pin_level(RELE3)!=1){
				myprintf("OPEN RELE3 man on\n");
				gpio_set_pin_level(RELE3,1);
			}
			break;
		}
		case control_type_relay_iniezione_liquido_manual_off: {
			if (gpio_get_pin_level(RELE3)!=0){
				myprintf("CLOSE RELE3 man off\n");
				gpio_set_pin_level(RELE3,0);
			}
			break;
		}
		case control_type_relay_iniezione_liquido_automatic: {
			if (m_status[temperature_gas_scarico] > ar_conf[valvola_iniezione_liquido_open_temperature].value){
				if (gpio_get_pin_level(RELE3)!=1){
					myprintf("OPEN RELE3 auto %d GT %d\n",m_status[temperature_gas_scarico] ,ar_conf[valvola_iniezione_liquido_open_temperature].value);
					gpio_set_pin_level(RELE3,1);
				}
			}
			if (m_status[temperature_gas_scarico] < ar_conf[valvola_iniezione_liquido_close_temperature].value){
				if (gpio_get_pin_level(RELE3)!=0){
					myprintf("CLOSE RELE3 auto%d LT %d\n",m_status[temperature_gas_scarico] ,ar_conf[valvola_iniezione_liquido_close_temperature].value);
					gpio_set_pin_level(RELE3,0);
				}
			}
			break;
		}
	}
}
