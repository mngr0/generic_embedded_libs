#ifndef AIR_REF_H_
#define AIR_REF_H_


#include "infineonlib/infineonlib.h"
#include "conversion/adc_raw/adc_raw.h"
#include "conversion/ntc/ntc.h"
#include "conversion/pressure/pressure.h"

#include "validator_field.h"

#define FAN_DIRECTION -1

typedef enum {
	control_type_manual_off =0,
	control_type_manual_on,
	control_type_digital_input,
	control_type_pump_down,
	control_type_size,
}control_type_t;

typedef enum {
	control_type_relay_iniezione_liquido_manual_off =0,
	control_type_relay_iniezione_liquido_manual_on,
	control_type_relay_iniezione_liquido_automatic,
	control_type_relay_iniezione_liquido_size,
}control_type_relay_iniezione_liquido_t;

typedef enum {
	control_type_relay_defrost_manual_off =0,
	control_type_relay_defrost_manual_on,
	control_type_relay_defrost_automatic,
	control_type_relay_defrost_size,
}control_type_relay_defrost_t;

typedef enum {
	control_type_compressor_manual_off =0,
	control_type_compressor_manual_speed,
	control_type_compressor_automatic,
	control_type_compressor_size,
}control_type_compressor_t;

typedef enum {
	control_type_fan_manual_off =0,
	control_type_fan_manual_speed,
	control_type_fan_automatic,
	control_type_fan_size,
}control_type_fan_t;

typedef enum {
	control_type_termostatica_manual=0,
	control_type_termostatica_automatic,
	control_type_termostatica_size,
}control_type_termostatica_t;

typedef enum {
	error_sonda_none=0,
	error_sonda_scollegata,
	error_sonda_corto
}error_sonda_t;


typedef enum {
	error_level_none=0,
	error_level_critical
} error_level_t;

typedef enum {
	air_ref_status_timeout=0,
	air_ref_status_idle,
	air_ref_status_run_start,
	air_ref_status_run_full,
	air_ref_status_run_defrost,
	air_ref_status_spegnimento_pumpdown,
	air_ref_status_critical_error
} air_ref_machine_states_t;

typedef enum {
	compressor_status_blocked=0,
	compressor_status_idle,
	compressor_status_run,
	compressor_status_run_acceleration,
} compressor_machine_states_t;

typedef enum {
	valvola_gas_scarico_status_open=0,
	valvola_gas_scarico_status_close,
} valvola_gas_scarico_machine_states_t;

typedef enum {
	fan_status_enabled=0,
	fan_status_disabled,
} fan_machine_states_t;


typedef enum {
	termostatica_full_close=0,
	termostatica_modulating,
	termostatica_starting,
	termostatica_fixed_position
} termostatica_machine_states_t;

typedef enum {
	air_ref_gas_R32=0,
	air_ref_gas_R134a,
	air_ref_gas_R507,
	air_ref_gas_max
}air_ref_gas_type_t;


typedef enum{
	device_type_termostatica =1,
	device_type_inverter,
	device_type_termostato,
	device_type_my_feeder
}device_type_t;


#define ENUMERATE(name) name,
#define STRINGIFY(name) #name,


#define FOREACH_AR_CONF(OPERATION)					\
OPERATION(operator_password)						\
OPERATION(control_type)								\
OPERATION(control_type_relay_defrost)				\
OPERATION(control_type_relay_iniezione_liquido)		\
OPERATION(control_type_compressor)					\
OPERATION(control_type_fan)							\
OPERATION(control_type_termostatica)				\
OPERATION(gas_type)									\
OPERATION(air_ref_start_interval)					\
OPERATION(air_ref_timeout_interval)					\
OPERATION(air_ref_pump_down_pressure_control)		\
OPERATION(termostatica_surriscaldo_setpoint)		\
OPERATION(termostatica_coeff_P)						\
OPERATION(termostatica_coeff_I)						\
OPERATION(termostatica_coeff_I_max)					\
OPERATION(termostatica_max_step)					\
OPERATION(termostatica_interval_fixed_poistion)		\
OPERATION(termostatica_initial_fixed_position)		\
OPERATION(termostatica_manual_position)				\
OPERATION(fan_target_pressure)						\
OPERATION(fan_coeff_P)								\
OPERATION(fan_coeff_offset)							\
OPERATION(fan_min_pressure)							\
OPERATION(fan_max_pressure)							\
OPERATION(fan_manual_speed)							\
OPERATION(compressor_speed_full)					\
OPERATION(compressor_start_speed)					\
OPERATION(compressor_defrost_speed)					\
OPERATION(compressor_pressure_spike)				\
OPERATION(compressor_manual_speed)					\
OPERATION(compressor_temperature_warning)			\
OPERATION(compressor_temperature_critical)			\
OPERATION(compressor_temperature_histersys)			\
OPERATION(valvola_iniezione_liquido_open_temperature)	\
OPERATION(valvola_iniezione_liquido_close_temperature)	\
OPERATION(LP_low_pressure_limit)					\
OPERATION(LP_low_pressure_differential)				\
OPERATION(HP_high_pressure_limit)					\
OPERATION(HP_high_pressure_differential)


#define FOREACH_AR_STATUS(OPERATION)					\
OPERATION(air_ref_start_timestamp)						\
OPERATION(air_ref_status)								\
OPERATION(compressor_status)							\
OPERATION(termostatica_status)							\
OPERATION(fan_status)									\
OPERATION(compressor_I_value)							\
OPERATION(compressor_speed_to_command)					\
OPERATION(compressor_calculated_speed)					\
OPERATION(compressor_target_speed)						\
OPERATION(fan_speed_to_command)							\
OPERATION(fan_time_last_command)						\
OPERATION(termostatica_I_value)							\
OPERATION(termostatica_step_target)						\
OPERATION(termostatica_step_current_position)			\
OPERATION(termostatica_timestamp_initial_position)		\
OPERATION(debounce_input_timestamp)						\
OPERATION(debounce_input_current_state)


#define FOREACH_M_STATUS(OPERATION)								\
OPERATION(device_type)											\
OPERATION(device_version)										\
OPERATION(evaporation_pressure)									\
OPERATION(evaporation_temperature)								\
OPERATION(condensation_pressure)								\
OPERATION(temperature_gas_scarico)								\
OPERATION(temperature_environment)								\
OPERATION(temperature_gas_ritorno)								\
OPERATION(temperature_extra)									\
OPERATION(imc102_status_INDEX_MOTOR_FAULT)						\
OPERATION(imc102_status_INDEX_MOTOR_SPEED)						\
OPERATION(imc102_status_INDEX_MOTOR_MOTOR_CURRENT)				\
OPERATION(imc102_status_INDEX_MOTOR_TARGET_SPEED)				\
OPERATION(imc102_status_routine)								\
OPERATION(imm101_status_INDEX_MOTOR_FAULT)						\
OPERATION(imm101_status_INDEX_MOTOR_SPEED)						\
OPERATION(imm101_status_INDEX_MOTOR_MOTOR_CURRENT)				\
OPERATION(imm101_status_INDEX_MOTOR_TARGET_SPEED)				\
OPERATION(imm101_status_routine)								\
OPERATION(pin_enable)											\
OPERATION(stato_sonda_pressione_evap)							\
OPERATION(stato_sonda_pressione_cond)							\
OPERATION(stato_sonda_ntc_gas_scarico)							\
OPERATION(stato_sonda_ntc_environment)							\
OPERATION(stato_sonda_ntc_gas_ritorno)							\
OPERATION(stato_sonda_ntc_extra)								\
OPERATION(stato_routine_imm101_motor)							\
OPERATION(stato_routine_imc102_motor)							\
OPERATION(protezione_overtemperature_gas_scarico_warning)		\
OPERATION(protezione_overtemperature_gas_scarico_critical)		\
OPERATION(protezione_evap_low_pressure)							\
OPERATION(protezione_cond_high_pressure)						\
OPERATION(protezione_pressure_difference_start)

#define SONDA_NTC(op, name)	\
op(name ## _res25)			\
op(name ## _beta)			\
op(name ## _offset)

#define SONDA_PRESSIONE(op, name)	\
op(name ## _V1)						\
op(name ## _V2)						\
op(name ## _P1)						\
op(name ## _P2)						\
op(name ## _offset)


#define FOREACH_M_CONF(OPERATION)				\
OPERATION(can_address)							\
OPERATION(termostatica_enabled)					\
SONDA_PRESSIONE(OPERATION, evap_pres)			\
SONDA_PRESSIONE(OPERATION, cond_pres)			\
SONDA_NTC(OPERATION, temperature_gas_scarico)	\
SONDA_NTC(OPERATION, temperature_environment)	\
SONDA_NTC(OPERATION, temperature_gas_ritorno)	\
SONDA_NTC(OPERATION, temperature_extra)	


typedef enum{
	FOREACH_M_CONF(ENUMERATE)
	machine_conf_parameters_size,
}machine_conf_t;

typedef enum{//public
	FOREACH_M_STATUS(ENUMERATE)
	machine_status_parameters_size
}machine_status_params_t;

typedef enum{//READONLY
	FOREACH_AR_STATUS(ENUMERATE)
	air_ref_status_parameters_size
} air_ref_status_params_t;

typedef enum { //public
	FOREACH_AR_CONF(ENUMERATE)
	air_ref_conf_parameters_size
}air_ref_conf_parameters_t;


typedef struct {//private
	adc_t ADCs [6];
	pressure_t cond_pres;
	pressure_t evap_pres;
	ntc_t temperature_gas_scarico;
	ntc_t temperature_gas_ritorno;
	ntc_t temperature_environment;
	ntc_t temperature_extra;
	infineon_device_t * imc102;
	infineon_device_t * imm101;
} machine_interface_t;

typedef struct {
	validated_field *ar_conf;
	validated_field *m_conf;
	int32_t *ar_status;
	int32_t *m_status;
	machine_interface_t* m_interface;
}task_parameter_t;

static inline void check_error_status_GT(int32_t argument, int32_t limit, int32_t isteresi, int32_t* error_ptr){
	switch(*error_ptr ){
		case error_level_none:{
			if(argument > limit ){
				*error_ptr = error_level_critical;
			}
			break;
		}
		case error_level_critical:{
			if(argument < limit - isteresi){
				*error_ptr = error_level_none;
			}
			break;
		}
	}
}

static inline void check_error_status_LT(int32_t argument, int32_t limit, int32_t isteresi, int32_t* error_ptr){
	switch(*error_ptr ){
		case error_level_none:{
			if(argument < limit ){
				*error_ptr = error_level_critical;
			}
			break;
		}
		case error_level_critical:{
			if(argument > limit + isteresi){
				*error_ptr = error_level_none;
			}
			break;
		}
	}
}


void routine_init();
void air_ref_update_state();
void air_ref_save_config_on_flash();

void fan_init(int32_t *ar_status, machine_interface_t *m_interface);
void fan_control(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status, machine_interface_t *m_interface);
bool fan_check_protection(int32_t *ar_status, int32_t *m_status,machine_interface_t *m_interface);
void fan_actuate(int32_t *ar_status, machine_interface_t *m_interface);

void compressor_init(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status, machine_interface_t *m_interface);
void compressor_control(validated_field *ar_conf, int32_t *ar_status, int32_t* m_status, machine_interface_t *m_interface);
bool compressor_check_protection(validated_field *ar_conf, int32_t *ar_status, int32_t* m_status, machine_interface_t *m_interface);
void compressor_actuate(validated_field *ar_conf,int32_t *ar_status, machine_interface_t *m_interface);
bool compressor_set_status( int32_t *ar_status, compressor_machine_states_t new_status, int32_t target_speed);

void termostatica_init(validated_field *ar_conf, int32_t *ar_status);
void termostatica_control(validated_field *ar_conf, int32_t *ar_status, int32_t* m_status);
bool termostatica_set_status(int32_t *ar_status, termostatica_machine_states_t new_status);

void valvola_iniezione_liquido_control(validated_field *ar_conf, int32_t* m_status);
//void valvola_gas_scarico_actuate(air_ref_status_t *ar_status, machine_interface_t *m_interface);

void configure_machine(validated_field *m_conf);

void air_ref_state_change_critical_error(int32_t *ar_status);

void stepper_task(void *par);
void air_ref_task(void *p);

#endif /* AIR_REF_H_ */
