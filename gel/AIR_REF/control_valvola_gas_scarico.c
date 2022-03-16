#include <stdint.h>
#include "AIR_REF/AIR_REF.h"

#include "myprintf/myprintf.h"

void valvola_gas_scarico_control(validated_field *ar_conf, int32_t * m_status){
	if (m_status[temperature_gas_scarico] > ar_conf[valvola_iniezione_liquido_open_temperature].value){
		if (gpio_get_pin_level(RELE2)!=1){
			gpio_set_pin_level(RELE2,1);
			myprintf("HIGH TEMPEARATURE WARNING, open rele2\n");
		}
	}
	if (m_status[temperature_gas_scarico] < ar_conf[valvola_iniezione_liquido_close_temperature].value){
		if (gpio_get_pin_level(RELE2)!=0){
			gpio_set_pin_level(RELE2,0);
			myprintf("HIGH TEMPEARATURE WARNING, close rele2\n");
		}
	}
}
