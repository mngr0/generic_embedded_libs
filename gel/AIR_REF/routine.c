#include <stdint.h>
#include "AIR_REF/AIR_REF.h"
#include "myprintf/myprintf.h"
#include "freertos_task_conf.h"
#include "led/led.h"
#include "serial_SATA_protocol/serial_SATA_protocol.h"

static validated_field *ar_conf_validator;
static validated_field *m_conf_validator;


VALIDATOR_STATIC(operator_password,0,10000);
VALIDATOR_STATIC(control_type,0,control_type_size);
VALIDATOR_STATIC(control_type_relay_defrost,0,control_type_relay_defrost_size);
VALIDATOR_STATIC(control_type_relay_iniezione_liquido,0,control_type_relay_iniezione_liquido_size);
VALIDATOR_STATIC(control_type_compressor,0,control_type_compressor_size);
VALIDATOR_STATIC(control_type_fan,0,control_type_fan_size);
VALIDATOR_STATIC(control_type_termostatica,0,control_type_termostatica_size);
VALIDATOR_STATIC(gas_type,0,10000);
VALIDATOR_STATIC(air_ref_start_interval,0,300000);
VALIDATOR_STATIC(air_ref_timeout_interval,0,300000);
VALIDATOR_STATIC(air_ref_pump_down_pressure_control,0,10000);
VALIDATOR_STATIC(termostatica_surriscaldo_setpoint,0,100000);
VALIDATOR_STATIC(termostatica_coeff_P,-10000,10000);
VALIDATOR_STATIC(termostatica_coeff_I,-10000,10000);
VALIDATOR_STATIC(termostatica_coeff_I_max,-100000,100000);
VALIDATOR_STATIC(termostatica_max_step,0,10000);
VALIDATOR_DYNAMIC_MAX(termostatica_interval_fixed_poistion, ar_conf_validator,0, termostatica_max_step);
VALIDATOR_STATIC(termostatica_initial_fixed_position,0,30000);
VALIDATOR_DYNAMIC_MAX(termostatica_manual_position,ar_conf_validator,0, termostatica_max_step);
VALIDATOR_STATIC(fan_target_pressure,0,50000);
VALIDATOR_STATIC(fan_coeff_P,-10000,10000);
VALIDATOR_STATIC(fan_coeff_offset,-10000,10000);
VALIDATOR_STATIC(fan_min_pressure,0,50000);// use dynamic?
VALIDATOR_STATIC(fan_max_pressure,0,50000);
VALIDATOR_STATIC(fan_manual_speed,0,101);
VALIDATOR_STATIC(compressor_speed_full,0,16380);
VALIDATOR_STATIC(compressor_start_speed,0,16380);
VALIDATOR_STATIC(compressor_defrost_speed,0,16380);
VALIDATOR_STATIC(compressor_pressure_spike,0,10000);
VALIDATOR_STATIC(compressor_manual_speed,0,101);
VALIDATOR_STATIC(compressor_temperature_warning,0,200000);
VALIDATOR_STATIC(compressor_temperature_critical,0,200000)	;
VALIDATOR_STATIC(compressor_temperature_histersys,0,200000)	;
VALIDATOR_STATIC(valvola_iniezione_liquido_open_temperature,0,200000);//dynamic
VALIDATOR_STATIC(valvola_iniezione_liquido_close_temperature,0,200000);
VALIDATOR_STATIC(period_log,0,10000);
VALIDATOR_STATIC(LP_low_pressure_limit,0,100000);//dynamic
VALIDATOR_STATIC(LP_low_pressure_differential,0,100000);
VALIDATOR_STATIC(HP_high_pressure_limit,0,100000);//dynamic
VALIDATOR_STATIC(HP_high_pressure_differential,0,100000);

VALIDATOR_STATIC(can_address, 0, (2^12) );
VALIDATOR_STATIC(termostatica_enabled, 0, 2 );

#define SONDA_PRESSIONE_VALIDATOR(name)										\
VALIDATOR_DYNAMIC_MAX(name ## _V1,m_conf_validator, 0,name ## _V2);		\
VALIDATOR_DYNAMIC_MIN(name ## _V2,m_conf_validator, name ## _V1,5001);	\
VALIDATOR_DYNAMIC_MAX(name ## _P1,m_conf_validator, -1001 ,name ## _P2);	\
VALIDATOR_DYNAMIC_MIN(name ## _P2,m_conf_validator, name ## _P1,100000);	\
VALIDATOR_STATIC(name ## _offset, -100000, 100000 );

SONDA_PRESSIONE_VALIDATOR(cond_pres);
SONDA_PRESSIONE_VALIDATOR(evap_pres);

VALIDATOR_STATIC(temperature_gas_scarico_res25,0,1000000);
VALIDATOR_STATIC(temperature_gas_scarico_beta,0,100000);
VALIDATOR_STATIC(temperature_gas_scarico_offset,-100000,100000);

VALIDATOR_STATIC(temperature_environment_res25,0,1000000);
VALIDATOR_STATIC(temperature_environment_beta,0,100000);
VALIDATOR_STATIC(temperature_environment_offset,-100000,100000);

VALIDATOR_STATIC(temperature_gas_ritorno_res25,0,1000000);
VALIDATOR_STATIC(temperature_gas_ritorno_beta,0,100000);
VALIDATOR_STATIC(temperature_gas_ritorno_offset,-100000,100000);

VALIDATOR_STATIC(temperature_extra_res25,0,1000000);
VALIDATOR_STATIC(temperature_extra_beta,0,100000);
VALIDATOR_STATIC(temperature_extra_offset,-100000,100000);


static TaskHandle_t xAirRefTask;
static TaskHandle_t xPeriodicLogTask;
static TaskHandle_t xLEDTask;
static TaskHandle_t xStepperTask;

static validated_field ar_conf[air_ref_conf_parameters_size];
static validated_field m_conf[machine_conf_parameters_size];
static int32_t ar_status[air_ref_status_parameters_size];
static int32_t m_status[machine_status_parameters_size];


static machine_interface_t m_interface={
	.ADCs={
		{.adc_res=16, .max_adc_read=3300, .offset=0, .ain_ch=4},
		{.adc_res=16, .max_adc_read=3300, .offset=0, .ain_ch=5},
		{.adc_res=16, .max_adc_read=5000, .offset=0, .ain_ch=8},
		{.adc_res=16, .max_adc_read=3300, .offset=0, .ain_ch=9},
		{.adc_res=16, .max_adc_read=3300, .offset=0, .ain_ch=10},
		{.adc_res=16, .max_adc_read=5000, .offset=0, .ain_ch=11},
	}
};

static task_parameter_t tp={
	.ar_conf=ar_conf,
	.m_conf=m_conf,
	.ar_status=ar_status,
	.m_status=m_status,
	.m_interface=&m_interface
};


void configure_routine_defaults(validated_field *ar_conf){
	ar_conf[control_type].value = control_type_digital_input;
	
	ar_conf[control_type_relay_defrost].value = control_type_relay_defrost_automatic;
	ar_conf[control_type_relay_iniezione_liquido].value = control_type_relay_iniezione_liquido_automatic;
	ar_conf[control_type_compressor].value = control_type_compressor_automatic;
	ar_conf[control_type_fan].value = control_type_fan_automatic;
	ar_conf[control_type_termostatica].value = control_type_termostatica_automatic;

	
	ar_conf[operator_password].value = 2;
	ar_conf[gas_type].value=air_ref_gas_R32;
	ar_conf[air_ref_start_interval].value = 500000;
	ar_conf[air_ref_pump_down_pressure_control].value = 1000;
	
	ar_conf[termostatica_max_step].value=500;
	ar_conf[termostatica_surriscaldo_setpoint].value=12000;
	ar_conf[termostatica_coeff_P].value=3000;
	ar_conf[termostatica_coeff_I].value=400;
	ar_conf[termostatica_coeff_I_max].value=(ar_conf[termostatica_max_step].value/2)*1000;
	ar_conf[termostatica_interval_fixed_poistion].value=40000;
	ar_conf[termostatica_initial_fixed_position].value=100;
	ar_conf[termostatica_manual_position].value=0;
	
	//ar_conf[compressor_coeff_I = 10;
	//ar_conf[compressor_coeff_P = 100;
	//ar_conf[compressor_target_pressure=1500;
	ar_conf[compressor_speed_full].value= 14200;
	ar_conf[compressor_start_speed].value= 7200;
	ar_conf[compressor_defrost_speed].value= 10000;
	ar_conf[compressor_pressure_spike].value= 5000;
	
	ar_conf[compressor_manual_speed].value = 0;
	ar_conf[compressor_temperature_warning].value = 110000;
	ar_conf[compressor_temperature_critical].value = 125000;
	ar_conf[compressor_temperature_histersys].value = 5000;
	
	ar_conf[valvola_iniezione_liquido_open_temperature].value = 95000;
	ar_conf[valvola_iniezione_liquido_close_temperature].value = 93000;
	
	ar_conf[fan_coeff_offset].value=2000;
	ar_conf[fan_coeff_P].value=250;
	ar_conf[fan_max_pressure].value=40000;
	ar_conf[fan_min_pressure].value=10000;
	ar_conf[fan_target_pressure].value=22000;
	ar_conf[fan_manual_speed].value=0;
	

	ar_conf[LP_low_pressure_limit].value = 300;
	ar_conf[LP_low_pressure_differential].value = 1800;
	ar_conf[HP_high_pressure_limit].value = 50000;
	ar_conf[HP_high_pressure_differential].value = 10000;
	
	//ar_conf[period_log].value = 1500;


}




void configure_machine_defaults(validated_field *m_conf){
	m_conf[can_address].value = 4;
	
	// 	m_conf[evap_pres_V1].value=500;
	// 	m_conf[evap_pres_P1].value=0;
	// 	m_conf[evap_pres_V2].value=4500;
	// 	m_conf[evap_pres_P2].value=34500;

	m_conf[evap_pres_V1].value=500;
	m_conf[evap_pres_P1].value=-1000;
	m_conf[evap_pres_V2].value=4500;
	m_conf[evap_pres_P2].value=9500;
	m_conf[evap_pres_offset].value=0;
	// 	m_conf[cond_pres_V1].value=500;
	// 	m_conf[cond_pres_P1].value=0;
	// 	m_conf[cond_pres_V2].value=4500;
	// 	m_conf[cond_pres_P2].value=34500;

	m_conf[cond_pres_V1].value=500;
	m_conf[cond_pres_P1].value=0;
	m_conf[cond_pres_V2].value=4500;
	m_conf[cond_pres_P2].value=50000;
	m_conf[cond_pres_offset].value=0;
	
	m_conf[temperature_gas_scarico_res25].value=54900;
	m_conf[temperature_gas_scarico_beta].value=3939;
	m_conf[temperature_gas_scarico_offset].value=-35000;
	
	m_conf[temperature_extra_res25].value=10000;
	m_conf[temperature_extra_beta].value=3099;
	m_conf[temperature_extra_offset].value=0;
	
	m_conf[temperature_gas_ritorno_res25].value=10000;
	m_conf[temperature_gas_ritorno_beta].value=4343;
	m_conf[temperature_gas_ritorno_offset].value=0;
	
	m_conf[temperature_environment_res25].value=10000;
	m_conf[temperature_environment_beta].value=4343;
	m_conf[temperature_environment_offset].value=0;
}

#define ASSIGN_AR_CONF(par_name) ar_conf[par_name].validate_value=par_name ## _validator;
#define ASSIGN_M_CONF(par_name) m_conf[par_name].validate_value=par_name ## _validator;

void init_ar_conf(validated_field* ar_conf){
	ar_conf_validator=ar_conf;
	FOREACH_AR_CONF(ASSIGN_AR_CONF)
}

void init_m_conf(validated_field* m_conf){
	m_conf_validator=m_conf;
	FOREACH_M_CONF(ASSIGN_M_CONF)
}

#define CONFIGURE_PRESSIONE(name)										\
m_interface.name.adc1=VOLT_TO_ADC(16,5000,m_conf[name ## _V1].value);	\
m_interface.name.pressure1=m_conf[name ## _P1].value;					\
m_interface.name.adc2=VOLT_TO_ADC(16,5000,m_conf[name ## _V2].value);	\
m_interface.name.pressure2=m_conf[name ## _P2].value;

#define CONFIGURE_NTC(name)												\
m_interface.name.beta=m_conf[name ## _beta].value;						\
m_interface.name.res_at_25=m_conf[name ## _res25].value;				\
m_interface.name.offset=m_conf[name ## _offset].value;					\
m_interface.name.pull_type=pullup;

void configure_machine(validated_field *m_conf){
	CONFIGURE_PRESSIONE(evap_pres);
	m_interface.evap_pres.adc_ch=&m_interface.ADCs[1];
	
	CONFIGURE_PRESSIONE(cond_pres);
	m_interface.cond_pres.adc_ch=&m_interface.ADCs[0];
	
	CONFIGURE_NTC(temperature_gas_ritorno);
	m_interface.temperature_gas_ritorno.pull_resistor=4700;
	m_interface.temperature_gas_ritorno.adc_ch=&m_interface.ADCs[2];
	
	CONFIGURE_NTC(temperature_gas_scarico);
	m_interface.temperature_gas_scarico.pull_resistor=4700;
	m_interface.temperature_gas_scarico.adc_ch=&m_interface.ADCs[4];
	
	CONFIGURE_NTC(temperature_environment);
	m_interface.temperature_environment.pull_resistor=4700;
	m_interface.temperature_environment.adc_ch=&m_interface.ADCs[3];
	
	CONFIGURE_NTC(temperature_extra);
	m_interface.temperature_extra.pull_resistor=4700;
	m_interface.temperature_extra.adc_ch=&m_interface.ADCs[5];
	
}


static char* ars_strings[]={"timeout","idle","run_start","run_full","defrost","spegnimento_pumpdown"};
static char* cs_strings[]={"block","idle","run","accel"};
static char* ts_strings[]={"full_close","modulating","starting","termostatica_fixed_position"};


void periodic_log(validated_field *ar_conf, int32_t *ar_status, int32_t* m_status,  machine_interface_t *m_interface){
	myprintf("-----------------------------------------------------------\n");
	myprintf("HP=%ld\tLP=%ld\tTGS=%ld\tTritorno=%ld\tTenv=%ld\tTc=%ld\tTevap=%d\tExtC=%d\n",
	m_status[condensation_pressure],
	m_status[evaporation_pressure],
	m_status[temperature_gas_scarico],
	m_status[temperature_gas_ritorno],
	m_status[temperature_environment],
	m_status[temperature_extra],
	m_status[evaporation_temperature],
	m_status[pin_enable]);
	
	myprintf("imc102 state:F%d\tS:%d\tTS:%d\n",
	m_interface->imc102->motor_status[INDEX_MOTOR_FAULT],
	m_interface->imc102->motor_status[INDEX_MOTOR_SPEED],
	m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED]);
	
	myprintf("imm101 state:F%d\tS:%d\tTS:%d\n",
	m_interface->imm101->motor_status[INDEX_MOTOR_FAULT],
	m_interface->imm101->motor_status[INDEX_MOTOR_SPEED],
	m_interface->imm101->motor_status[INDEX_MOTOR_TARGET_SPEED]);
	
	myprintf("surriscaldo:%d\t setpoint:%d\t pos:%d\t target pos:%d,\t Ivalue:%d\n",
	m_status[temperature_extra]-m_status[evaporation_temperature],
	ar_conf[termostatica_surriscaldo_setpoint].value,
	ar_status[termostatica_step_current_position],
	ar_status[termostatica_step_target],
	ar_status[termostatica_I_value]);
	
	myprintf("AR_STATUS:%s\t CCP_STATUS:%s\t TS_STATUS:%s\n",
	ars_strings[ar_status[air_ref_status]],
	cs_strings[ar_status[compressor_status]],
	ts_strings[ar_status[termostatica_status]]);
	
	myprintf("ERROR MOTORS: IMC-ERR:%d IMC-STAT:%d IMM-ERR:%d IMM-STAT:%d\n",
	m_interface->imc102->error_motor,
	m_interface->imc102->running_status,
	m_interface->imm101->error_motor,
	m_interface->imm101->running_status);
	
	// 	myprintf("STATO ERRORI:\t"
	// 	"stato_sonda_pressione_evap:%d\t"
	// 	"stato_sonda_pressione_cond:%d\t"
	// 	"stato_sonda_ntc_gas_scarico:%d\t"
	// 	"stato_sonda_ntc_environment:%d\n"
	// 	"stato_sonda_ntc_gas_ritorno:%d\t"
	// 	"stato_sonda_ntc_extra:%d\t"
	// 	"stato_routine_imm101_motor:%d\t"
	// 	"stato_routine_imc102_motor:%d\t"
	// 	"protezione_overtemperature_gas_scarico_warning:%d\n"
	// 	"protezione_overtemperature_gas_scarico_critical:%d\t"
	// 	"protezione_evap_low_pressure:%d\t"
	// 	"protezione_cond_high_pressure:%d\t"
	// 	"protezione_pressure_difference_start:%d\n",
	// 		m_status[stato_sonda_pressione_evap],
	// 		m_status[stato_sonda_pressione_cond],
	// 		m_status[stato_sonda_ntc_gas_scarico],
	// 		m_status[stato_sonda_ntc_environment],
	// 		m_status[stato_sonda_ntc_gas_ritorno],
	// 		m_status[stato_sonda_ntc_extra],
	// 		m_status[stato_routine_imm101_motor],
	// 		m_status[stato_routine_imc102_motor],
	// 		m_status[protezione_overtemperature_gas_scarico_warning],
	// 		m_status[protezione_overtemperature_gas_scarico_critical],
	// 		m_status[protezione_evap_low_pressure],
	// 		m_status[protezione_cond_high_pressure],
	// 		m_status[protezione_pressure_difference_start]);
	

	
	myprintf("ADC\t0:%d\t1:%d\t2:%d\t3:%d\t4:%d\t5:%d\n",
	read_ADC(&m_interface->ADCs[0],&ADC_0),
	read_ADC(&m_interface->ADCs[1],&ADC_0),
	read_ADC(&m_interface->ADCs[2],&ADC_0),
	read_ADC(&m_interface->ADCs[3],&ADC_0),
	read_ADC(&m_interface->ADCs[4],&ADC_0),
	read_ADC(&m_interface->ADCs[5],&ADC_0) );

	myprintf("TGS beta:%d\t offset:%d\t pull:%d\t pulltype%d\t res_at_25:%d\n",
	m_interface->temperature_gas_scarico.beta,
	m_interface->temperature_gas_scarico.offset,
	m_interface->temperature_gas_scarico.pull_resistor,
	m_interface->temperature_gas_scarico.pull_type,
	m_interface->temperature_gas_scarico.res_at_25);
	
		myprintf("cond adc1:%d\t adc2:%d\t p1:%d\t p2%d\t offset:%d\n",
		m_interface->cond_pres.adc1,
		m_interface->cond_pres.adc2,
		m_interface->cond_pres.pressure1,
		m_interface->cond_pres.pressure2,
		m_interface->cond_pres.offset);
	
}


void periodic_log_task(void *p){
	task_parameter_t *tp = (task_parameter_t *)p;
	os_sleep(5000);
	while (1)
	{
		periodic_log(tp->ar_conf, tp->ar_status, tp->m_status, tp->m_interface);
		os_sleep(2000);
	}
}


void routine_save_config_on_flash(validated_field *ar_conf, validated_field *m_conf){
	VF_write_on_flash(ar_conf, air_ref_conf_parameters_size, FLASH_MEMORY_ROUTINE_CONF );
	VF_write_on_flash(m_conf, machine_conf_parameters_size, FLASH_MEMORY_MACHINE_CONF );
}


void routine_init(){
	init_infineon();
	
	init_ar_conf(ar_conf);
	init_m_conf(m_conf);
	
	VF_read_from_flash(ar_conf, air_ref_conf_parameters_size, FLASH_MEMORY_ROUTINE_CONF );
	VF_read_from_flash(m_conf, machine_conf_parameters_size, FLASH_MEMORY_MACHINE_CONF );

	serial_SATA_init(ar_conf,air_ref_conf_parameters_size, ar_status, air_ref_status_parameters_size, m_conf, machine_conf_parameters_size, m_status,machine_status_parameters_size);
	
	//if(ar_conf[operator_password].value != 2){
		if(1){
		configure_routine_defaults(ar_conf);
		configure_machine_defaults(m_conf);
		routine_save_config_on_flash(ar_conf, m_conf);
		myprintf("CONFIGURATION TO DEFAULT\n");
	}
	
	configure_machine(m_conf);
	
	myprintf("READ configuration:\n");
	//myprintf("\tCOMPRESSOR: target:%ld\tcoeff_P:%ld\tcoeffP_I:%ld\n",
	//ar_conf[compressor_target_pressure,	ar_conf[compressor_coeff_P,	ar_conf[compressor_coeff_I);
	myprintf("\tFAN: offset:%ld\tP:%ld\ttarget:%ld\tmax:%ld\tmin:%ld\n", ar_conf[fan_coeff_offset].value,
	ar_conf[fan_coeff_P].value, ar_conf[fan_target_pressure].value, ar_conf[fan_max_pressure].value, ar_conf[fan_min_pressure].value);
	

	
	if (xTaskCreate(
	air_ref_task, "AirRef", TASK_AIR_REF_STACK_SIZE, &tp, TASK_AIR_REF_STACK_PRIORITY, &xAirRefTask)
	!= pdPASS) {
		while (1) {
			;
		}
	}
	#if 1
	if (xTaskCreate(
	periodic_log_task, "PerLog", TASK_PERIODIC_LOG_STACK_SIZE, &tp, TASK_PERIODIC_LOG_STACK_PRIORITY, &xPeriodicLogTask)
	!= pdPASS) {
		while (1) {
			;
		}
	}
	#endif
	
	if(m_conf[termostatica_enabled].value==1){
		if (xTaskCreate(
		stepper_task, "Stepper", TASK_STEPPER_STACK_SIZE, ar_status, TASK_STEPPER_STACK_PRIORITY, &xStepperTask)
		!= pdPASS) {
			while (1) {
				;
			}
		}
	}
	


	
	if (xTaskCreate(led_task, "LED", TASK_LED_STACK_SIZE, NULL, TASK_LED_STACK_PRIORITY, &xLEDTask)
	!= pdPASS) {
		while (1) {
			;
		}
	}
	delay_ms(1000);
	vTaskStartScheduler();
	
	
}
