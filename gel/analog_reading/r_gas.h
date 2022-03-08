#ifndef R_GAS_H_
#define R_GAS_H_

#include <stdint.h>

typedef enum {
	r_gas_gas_type_r32=0,
	r_gas_gas_type_r134a,
	r_gas_gas_type_r507,
	r_gas_gas_type_size
}r_gas_gas_type_t;

int32_t r_gas_saturated_pressure_to_temperature(r_gas_gas_type_t gas_type, int32_t pressure_read);


#endif /* R_GAS_H_ */