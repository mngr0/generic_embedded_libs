#ifndef VALIDATOR_FIELD_H_
#define VALIDATOR_FIELD_H_

#include <stdint.h>
//#define FLASH_MEMORY_ROUTINE_CONF 0x0024000
//#define FLASH_MEMORY_MACHINE_CONF 0x0020000
//#define FLASH_MEMORY_ROUTINE_CONF 0x44000000
//#define FLASH_MEMORY_MACHINE_CONF 0x44008000

#define FLASH_MEMORY_ROUTINE_CONF 0x0002AD00
#define FLASH_MEMORY_MACHINE_CONF 0x0002B000

typedef bool (*fun_validator)(int32_t new_val);

typedef struct {
	int32_t value;
	fun_validator validate_value;
}validated_field;


#define VALIDATOR_STATIC(name1, min_val, max_val )						\
bool name1 ## _validator(int32_t new_val){								\
	return ((new_val>=min_val ) && (new_val<max_val));					\
}


#define VALIDATOR_DYNAMIC_MIN(name1, conf, idx_min_val, max_val )		\
bool name1 ## _validator(int32_t new_val){								\
	return ((new_val>=conf[idx_min_val].value) && (new_val<max_val));	\
}

#define VALIDATOR_DYNAMIC_MAX(name1, conf, min_val, idx_max_val )		\
bool name1 ## _validator(int32_t new_val){								\
	return ( (new_val>=min_val )  && (new_val<conf[idx_max_val].value));	\
}


void VF_read_from_flash(validated_field* fields, const uint32_t VF_size, uint32_t address);
void VF_write_on_flash(validated_field* fields, const uint32_t VF_size, uint32_t address);
void routine_save_config_on_flash(validated_field *ts_conf, validated_field *m_conf);


#endif
