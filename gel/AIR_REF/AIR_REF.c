#include <stdint.h>
#include <string.h>

#include "AIR_REF.h"

#include "myprintf/myprintf.h"
#include "conversion/adc_raw/adc_raw.h"
#include "conversion/r_gas/r_gas.h"

#include "led/led.h"

#define RUN_PIN_ACTIVE  0
#define RUN_PIN_DISABLE 1


void UI_led(int32_t *ar_status) {

    switch (ar_status[air_ref_status]) {
        case air_ref_status_critical_error: {
            led_set_state(led_state_red);
            break;
        }
        case air_ref_status_idle: {
            led_set_state(led_state_green);
            break;
        }
        case air_ref_status_timeout: {
            led_set_state(led_state_yellow);
            break;
        }
        default: {
            led_set_state(led_state_blue);
        }
    }
    if ((ar_status[compressor_status] == compressor_status_blocked) &&
        (ar_status[air_ref_status] != air_ref_status_idle)) {
        led_set_state(led_state_yellow);
    }

    // if idle -> green
    // if running -> blue
    // if error -> red
    // if waiting to start compressor -> orange
}



void check_sonda(int32_t *m_status, const machine_status_params_t idx_sonda_value,
                 machine_status_params_t idx_sonda_error, int32_t min_value, int32_t max_value) {
    // TODO ASSER IDXs VALID
    if (m_status[idx_sonda_value] < min_value) {
        m_status[idx_sonda_error] = error_sonda_corto;
    } else if (m_status[idx_sonda_value] > max_value) {
        m_status[idx_sonda_error] = error_sonda_scollegata;
    } else {
        m_status[idx_sonda_error] = error_sonda_none;
    }
}

void update_readings(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status,
                     machine_interface_t *m_interface) {
    // update readings from sensors
    m_status[condensation_pressure] = read_pressure(&(m_interface->cond_pres), &ADC_0);
    m_status[evaporation_pressure]  = read_pressure(&(m_interface->evap_pres), &ADC_0);

    switch (ar_conf[gas_type].value) {
        case air_ref_gas_R32: {
            m_status[evaporation_temperature] =
                r_gas_saturated_pressure_to_temperature(r_gas_gas_type_r32, m_status[evaporation_pressure]);
            break;
        }
        case air_ref_gas_R134a: {
            m_status[evaporation_temperature] =
                r_gas_saturated_pressure_to_temperature(r_gas_gas_type_r134a, m_status[evaporation_pressure]);
            break;
        }
        case air_ref_gas_R507: {
            m_status[evaporation_temperature] =
                r_gas_saturated_pressure_to_temperature(r_gas_gas_type_r507, m_status[evaporation_pressure]);
            break;
        }
        default: {
            // error?
        }
    }

    m_status[temperature_gas_scarico] = read_temp(&(m_interface->temperature_gas_scarico), &ADC_0);
    // myprintf("TGS ADC:%d RES:%d\n",read_ADC(m_interface->NTC_gas_scarico.adc_ch,&ADC_0),  get_ntc_resistance(&
    // m_interface->NTC_gas_scarico,read_ADC(m_interface->NTC_gas_scarico.adc_ch,&ADC_0))  );
    m_status[temperature_gas_ritorno] = read_temp(&(m_interface->temperature_gas_ritorno), &ADC_0);
    m_status[temperature_environment] = read_temp(&(m_interface->temperature_environment), &ADC_0);
    m_status[temperature_extra]       = read_temp(&(m_interface->temperature_extra), &ADC_0);

    int read_val = gpio_get_pin_level(EXT_INT0);

    if (read_val != ar_status[debounce_input_current_state]) {
        if (xTaskGetTickCount() - ar_status[debounce_input_timestamp] > 3000) {
            ar_status[debounce_input_current_state] = read_val;
            m_status[pin_enable]                    = read_val;
        }
    } else {
        ar_status[debounce_input_timestamp] = xTaskGetTickCount();
    }

    check_sonda(m_status, temperature_gas_scarico, stato_sonda_ntc_gas_scarico, -50000, 170000);
    check_sonda(m_status, temperature_gas_ritorno, stato_sonda_ntc_gas_ritorno, -50000, 90000);
    check_sonda(m_status, temperature_environment, stato_sonda_ntc_environment, -50000, 90000);
    check_sonda(m_status, temperature_extra, stato_sonda_ntc_extra, -50000, 90000);
    check_sonda(m_status, evaporation_pressure, stato_sonda_pressione_evap, m_interface->evap_pres.pressure1, 100000);
    check_sonda(m_status, condensation_pressure, stato_sonda_pressione_cond, m_interface->cond_pres.pressure1, 100000);


    ASSERT((imm101_status_INDEX_MOTOR_TARGET_SPEED - imm101_status_INDEX_MOTOR_FAULT) == (INDEX_MOTOR_SIZE - 1));
    ASSERT((imc102_status_INDEX_MOTOR_TARGET_SPEED - imc102_status_INDEX_MOTOR_FAULT) == (INDEX_MOTOR_SIZE - 1));

    for (int i = imm101_status_INDEX_MOTOR_FAULT; i <= imm101_status_INDEX_MOTOR_TARGET_SPEED; i++) {
        m_status[i] = m_interface->imm101->motor_status[i - imm101_status_INDEX_MOTOR_FAULT];
    }
    m_status[imm101_status_routine] = m_interface->imm101->error_motor;

    for (int i = imc102_status_INDEX_MOTOR_FAULT; i <= imc102_status_INDEX_MOTOR_TARGET_SPEED; i++) {
        m_status[i] = m_interface->imc102->motor_status[i - imc102_status_INDEX_MOTOR_FAULT];
    }
    m_status[imc102_status_routine] = m_interface->imc102->error_motor;
}





void air_ref_check_stop(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status) {
    switch (ar_conf[control_type].value) {
        case control_type_digital_input: {
            if (m_status[pin_enable] == RUN_PIN_DISABLE) {
                myprintf("AR_ROUTINE:PIN DISABLED STOP\n");
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
                ar_status[air_ref_status]          = air_ref_status_timeout;
                termostatica_set_status(ar_status, termostatica_full_close);
            }
            break;
        }
        case control_type_manual_off: {
            myprintf("AR_ROUTINE: manual stop\n");
            ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
            ar_status[air_ref_status]          = air_ref_status_timeout;
            termostatica_set_status(ar_status, termostatica_full_close);
            break;
        }
        case control_type_pump_down: {
            if (m_status[evaporation_pressure] < ar_conf[air_ref_pump_down_pressure_control].value) {
                myprintf("AR_ROUTINE:punpdown stop\n");
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
                ar_status[air_ref_status]          = air_ref_status_timeout;
                termostatica_set_status(ar_status, termostatica_full_close);
                break;
            }
        }
    }
}

void air_ref_check_defrost() {
    // TODO DECIDERE in quali stati fare questo controllo
}

static inline void go_timeout(int32_t *ar_status) {
    if (ar_status[air_ref_status] != air_ref_status_critical_error) {
        ar_status[air_ref_status]          = air_ref_status_timeout;
        ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
    }
}

void check_protection(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status,
                      machine_interface_t *m_interface) {
	// does not evade from critical error
	// detects: 
	// - sonde pressione scollegate (-> timeout)
	// - errore routine fan (-> pulire)
	// - errore routine comp (-> pulire + andare timeout)

    if (ar_status[air_ref_status] == air_ref_status_critical_error) {
        return;
    }

    if (m_status[stato_sonda_pressione_evap] != error_sonda_none) {
        go_timeout(ar_status);
    }

    if (m_status[stato_sonda_pressione_evap] != error_sonda_none) {
        go_timeout(ar_status);
    }

    if (m_interface->imm101->motor_status[INDEX_MOTOR_FAULT] != 0) {
        myprintf("CLEAR ERROR %d on imm101\n", m_interface->imm101->motor_status[INDEX_MOTOR_FAULT]);
        clear_fault(m_interface->imm101);
    }

    if (m_interface->imc102->motor_status[INDEX_MOTOR_FAULT] != 0) {
        myprintf("CLEAR ERROR %d on IMC102\n", m_interface->imc102->motor_status[INDEX_MOTOR_FAULT]);
        clear_fault(m_interface->imc102);
        go_timeout(ar_status);
    }

    if (!compressor_check_protection(ar_conf, ar_status, m_status, m_interface)) {
        clear_fault(m_interface->imc102);
        go_timeout(ar_status);
    }
    if (!fan_check_protection(ar_status, m_status, m_interface)) {
        clear_fault(m_interface->imm101);
    }
}

void air_ref_setup(task_parameter_t *tp) {
    validated_field *    ar_conf     = tp->ar_conf;
    validated_field *    m_conf      = tp->m_conf;
    int32_t *            ar_status   = tp->ar_status;
    int32_t *            m_status    = tp->m_status;
    machine_interface_t *m_interface = tp->m_interface;

    m_status[pin_enable]                    = RUN_PIN_DISABLE;
    ar_status[debounce_input_current_state] = RUN_PIN_DISABLE;
    ar_status[debounce_input_timestamp]     = xTaskGetTickCount();
    ar_status[air_ref_status]               = air_ref_status_idle;

    m_interface->imc102 = get_imc102();
    m_interface->imm101 = get_imm101();

    compressor_init(ar_conf, ar_status, m_status, m_interface);
    fan_init(ar_status, m_interface);

    if (m_conf[termostatica_enabled].value == 1) {
        termostatica_init(ar_conf, ar_status);
    }

    wdt_set_timeout_period(&WDT_0, 1000, 4096);
    wdt_enable(&WDT_0);
}

void air_ref_loop(task_parameter_t *tp, ) {
    // lista di test da fare:
    // - entrata e uscita da ogni stato, usando ogni possibile situazione
    validated_field *    ar_conf     = tp->ar_conf;
    validated_field *    m_conf      = tp->m_conf;
    int32_t *            ar_status   = tp->ar_status;
    int32_t *            m_status    = tp->m_status;
    machine_interface_t *m_interface = tp->m_interface;

    wdt_feed(&WDT_0);
    // update readings from sensors
    update_readings(ar_conf, ar_status, m_status, m_interface);

    switch (ar_status[air_ref_status]) {

        case air_ref_status_timeout: {
            // in timeout:
            // spegnere compressore -> idle
            // chiudere termostatica -> full closed
            // il tempo non inizia fino a che non e' tutto spento
            // dopo il tempo si passa in idle
            if ((ar_status[compressor_status] != compressor_status_idle) &&
                (ar_status[compressor_status] != compressor_status_blocked)) {
                compressor_set_status(ar_status, compressor_status_idle, 0);
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
            }

            if (ar_status[termostatica_status] != termostatica_full_close) {
                termostatica_set_status(ar_status, termostatica_full_close);
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
            }
            if (xTaskGetTickCount() - ar_status[air_ref_start_timestamp] > ar_conf[air_ref_start_interval].value) {
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
                ar_status[air_ref_status]          = air_ref_status_idle;
            }
            break;
        }
        case air_ref_status_idle: {
            // in idle:
            // spegnere compressore -> idle
            // chiudere termostatica -> fullclosed
            // il tempo non inizia fino a che non e' tutto spento
            // se necessario passare in run start
            myprintf("AR_ROUTINE:idle\n");
            // myprintf("idle->run_start");
            if (((ar_conf[control_type].value == control_type_digital_input) &&
                 (m_status[pin_enable] == RUN_PIN_ACTIVE)) ||
                ((ar_conf[control_type].value == control_type_pump_down) &&
                 (m_status[evaporation_pressure] > ar_conf[air_ref_pump_down_pressure_control].value)) ||

                (ar_conf[control_type].value == control_type_manual_on)) {
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
                ar_status[air_ref_status]          = air_ref_status_run_start;
                myprintf("AR_ROUTINE:idle->run_start\n");
                myprintf("CTRL TYPE:%d\tPIN_RUN:%d\tPRESSURE:%d\tPUMPDOWN:%d\n", ar_conf[control_type].value,
                         m_status[pin_enable], m_status[evaporation_pressure],
                         ar_conf[air_ref_pump_down_pressure_control].value);
            }
            break;
        }

        case air_ref_status_run_start: {
            // dare la molla al compressore e iniziare intervallo
            // l'intervallo inizia solo quando il compressore riporta "running" (resettato altrimenti)
            // la termostatica e' in posizione fissa fino alla fine del suo intervallo, il quale inizia solo dopo la
            // partenza del comp se necessario andare in timeout

            // (la termostatica va in modulating autonomamente)

            myprintf("AR_ROUTINE:run_start\n");
            ar_status[fan_status] = fan_status_enabled;
            if (ar_status[compressor_status] != compressor_status_run) {
                termostatica_set_status(ar_status, termostatica_fixed_position);     // it also resets timer
                compressor_set_status(ar_status, compressor_status_run, ar_conf[compressor_start_speed].value);
                ar_status[air_ref_start_timestamp] = xTaskGetTickCount();     // reset timer
            }
            if ((ar_status[termostatica_status] != termostatica_modulating) &&
                (ar_status[termostatica_status] != termostatica_fixed_position)) {
                termostatica_set_status(ar_status, termostatica_fixed_position);
            }

            if (xTaskGetTickCount() - ar_status[air_ref_start_timestamp] > ar_conf[air_ref_start_interval].value) {
                if (compressor_set_status(ar_status, compressor_status_run_acceleration,
                                          ar_conf[compressor_speed_full].value)) {
                    myprintf("AR_ROUTINE:run_start->run full\n");
                    ar_status[air_ref_status]          = air_ref_status_run_full;
                    ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
                }
            }
            air_ref_check_stop(ar_conf, ar_status, m_status);
            break;
        }
        case air_ref_status_run_full: {
            // se la termostatica e' ancora chiusa, mandarla in posizione fissa
            // se necessario andare in timeout

            myprintf("AR_ROUTINE:run full\n");
            ar_status[fan_status] = fan_status_enabled;

            if ((ar_status[termostatica_status] != termostatica_modulating) &&
                (ar_status[termostatica_status] != termostatica_fixed_position)) {
                termostatica_set_status(ar_status, termostatica_fixed_position);
            }
            air_ref_check_stop(ar_conf, ar_status, m_status);
            air_ref_check_defrost();
            break;
        }
        case air_ref_status_run_defrost: {
            // ?????????????????????????

            // APRIRE RELE1
            if (gpio_get_pin_level(RELE1) != 1) {
                gpio_set_pin_level(RELE1, 1);
            }
            // TERMOSTATICA CHIUSA
            if (ar_status[termostatica_status] != termostatica_modulating) {
                termostatica_set_status(ar_status, termostatica_full_close);
            }

            // VENTOLA FERMA
            ar_status[fan_status] = fan_status_disabled;

            // COMPRESSORE VEL DEFROST
            ar_status[compressor_target_speed] = ar_conf[compressor_defrost_speed].value;
            // SE CONTATTO CAMBIA USCIRE DA DEFROST E ENTRARE IN air_ref_status_run_start
            // if(){}
            ar_status[air_ref_status]          = air_ref_status_run_start;
            ar_status[air_ref_start_timestamp] = xTaskGetTickCount();
            break;
        }
        // 			case air_ref_status_spegnimento_pumpdown:{
        // 				ar_status[fan_status] = fan_status_enabled;
        // 				if(ar_status[termostatica_status] != termostatica_full_close){
        // 					termostatica_set_status(ar_status,termostatica_full_close);
        // 				}
        // 				if(m_status[evaporation_pressure] <
        // ar_conf[air_ref_pump_down_pressure_control].value){
        // ar_status[air_ref_start_timestamp] = xTaskGetTickCount(); 					ar_status[air_ref_status] = air_ref_status_timeout;
        // 					compressor_set_status(ar_status,compressor_status_blocked,0);
        // 					termostatica_set_status(ar_status,termostatica_full_close);
        // 				}
        // 				break;
        // 			}
        case air_ref_status_critical_error: {
            // uscire, prima o poi
            // ar_status[air_ref_status] = air_ref_status_timeout;
            // andare in timeout

            break;
        }
        default: {
            // go to critical error
            ar_status[air_ref_status] = air_ref_status_critical_error;
            myprintf("WTF STATUS AM I IN????\n");
        }
    }
    compressor_control(ar_conf, ar_status, m_status, m_interface);
    valvola_iniezione_liquido_control(ar_conf, m_status);
    fan_control(ar_conf, ar_status, m_status, m_interface);
    if (m_conf[termostatica_enabled].value == 1) {
        termostatica_control(ar_conf, ar_status, m_status);
    }

    // protection controls
    check_protection(ar_conf, ar_status, m_status, m_interface);

    // apply actions
    fan_actuate(ar_status, m_interface);
    compressor_actuate(ar_conf, ar_status, m_interface);
    // TODO aggiungere iniziezione liquido control

    UI_led(ar_status);

    os_sleep(400);
}




void air_ref_task(void *p) {
    task_parameter_t *tp = (task_parameter_t *)p;
    air_ref_setup(tp);
    while (1) {
        air_ref_loop(tp);
    }
}


void air_ref_state_change_critical_error(int32_t *ar_status) {
    ar_status[air_ref_status] = air_ref_status_critical_error;
    RED_led();
}
