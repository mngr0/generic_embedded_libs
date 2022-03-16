#include <stdint.h>
#include <stdbool.h>

#include "AIR_REF.h"


bool compressor_set_status(int32_t *ar_status, compressor_machine_states_t new_status, int32_t target_speed) {
    if (ar_status[compressor_status] == compressor_status_blocked) {
        return false;
    } else {
        if ((new_status == compressor_status_blocked) || (new_status == compressor_status_idle)) {
            ar_status[compressor_target_speed] = 0;
        }
        ar_status[compressor_status]       = new_status;
        ar_status[compressor_target_speed] = target_speed;
        return true;
    }
}


void compressor_control(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status,
                        machine_interface_t *m_interface) {
    switch (ar_status[compressor_status]) {
        // blocked ->
        case compressor_status_blocked: {
            ar_status[compressor_speed_to_command] = 0;
            ar_status[compressor_I_value]          = 0;
            break;
        }
        case compressor_status_idle: {
            ar_status[compressor_speed_to_command] = 0;
            ar_status[compressor_I_value]          = 0;
            break;
        }
        case compressor_status_run: {
            ar_status[compressor_speed_to_command] = ar_status[compressor_target_speed];
            break;
        }
        case compressor_status_run_acceleration: {
            if (m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] >= ar_status[compressor_target_speed]) {
                ar_status[compressor_status]           = compressor_status_run;
                ar_status[compressor_speed_to_command] = m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED];
            } else {
                ar_status[compressor_speed_to_command] =
                    min(m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] + 30,
                        ar_status[compressor_target_speed]);
            }
            break;
        }
    }
    ar_status[compressor_calculated_speed] = ar_status[compressor_speed_to_command];
}


bool compressor_check_protection(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status,
                                 machine_interface_t *m_interface) {
    // verificare che cattura gli errori
    // sonde //pressioni
    if (m_interface->imc102->error_motor != motor_error_none) {
        ar_status[compressor_speed_to_command] = 0;
    }

    check_error_status_GT(m_status[temperature_gas_scarico], ar_conf[compressor_temperature_warning].value,
                          ar_conf[compressor_temperature_histersys].value,
                          &m_status[protezione_overtemperature_gas_scarico_warning]);

    check_error_status_GT(m_status[temperature_gas_scarico], ar_conf[compressor_temperature_critical].value,
                          ar_conf[compressor_temperature_histersys].value,
                          &m_status[protezione_overtemperature_gas_scarico_critical]);


    if (m_status[protezione_overtemperature_gas_scarico_warning] == error_level_critical) {
        ar_status[compressor_speed_to_command] = min(ar_status[compressor_calculated_speed], 8000);
    }

    if (m_status[stato_sonda_ntc_gas_scarico] != error_sonda_none) {
        ar_status[compressor_speed_to_command] = min(ar_status[compressor_calculated_speed], 8000);
    }

    if (m_status[stato_sonda_pressione_cond] != error_sonda_none) {
        ar_status[compressor_speed_to_command] = min(ar_status[compressor_calculated_speed], 8000);
    }

    if (m_status[protezione_overtemperature_gas_scarico_critical] == error_level_critical) {
        ar_status[compressor_speed_to_command] = 0;
    }


    check_error_status_LT(m_status[evaporation_pressure], ar_conf[LP_low_pressure_limit].value,
                          ar_conf[LP_low_pressure_differential].value, &m_status[protezione_evap_low_pressure]);

    check_error_status_GT(m_status[condensation_pressure], ar_conf[HP_high_pressure_limit].value,
                          ar_conf[HP_high_pressure_differential].value, &m_status[protezione_cond_high_pressure]);

    // rimosso, controllate direttamente le sonde
    // 	check_error_status_LT(m_status[condensation_pressure],
    // 	ar_conf[LP_low_pressure_limit].value,
    // 	ar_conf[LP_low_pressure_differential].value,
    // 	&m_status[protezione_evap_low_pressure]);

    check_error_status_GT(m_status[condensation_pressure] - m_status[evaporation_pressure],
                          ar_conf[compressor_pressure_spike].value, 0, &m_status[protezione_pressure_difference_start]);

    switch (ar_status[compressor_status]) {

        case compressor_status_blocked: {
            // verificare che sblocca solo se e' tutto ok
            if ((m_status[protezione_overtemperature_gas_scarico_critical] == error_level_none) &&
                (m_status[protezione_pressure_difference_start] == error_level_none) &&
                (m_status[protezione_evap_low_pressure] == error_level_none) &&
                (m_status[protezione_cond_high_pressure] == error_level_none)) {
                myprintf("CCP DEBUG: blocked=0 \n");
                ar_status[compressor_status] = compressor_status_idle;
            }
            break;
        }
        case compressor_status_idle: {
            // controllare che solo la differenza di pressioni blocca il compressore
            // ONLY IN IDLE PRESSURE SPIKE COUNTS
            if (m_status[protezione_pressure_difference_start] == error_level_critical) {
                ar_status[compressor_status] = compressor_status_blocked;
            }
            // BREAK OMITTED ON PURPOSE
        }
        case compressor_status_run:
        case compressor_status_run_acceleration: {
            // run/acceleration
            // if something bad go blocked
            if ((m_status[protezione_cond_high_pressure] == error_level_critical) ||
                (m_interface->imc102->error_motor != motor_error_none) ||
                (m_status[protezione_evap_low_pressure] == error_level_critical) ||
                (m_status[protezione_overtemperature_gas_scarico_critical] == error_level_critical)) {
                myprintf("PROTECTION PREVENTING COMPRESSOR ACTION COND_HIGH_PRESSURE:%d\t error_motor:%d\t "
                         "evap_low_pressure:%d\t overtemperature_gas_scarico:%d\t \n",
                         m_status[protezione_cond_high_pressure], m_interface->imc102->error_motor,
                         m_status[protezione_evap_low_pressure],
                         m_status[protezione_overtemperature_gas_scarico_critical]);
                ar_status[compressor_status]           = compressor_status_blocked;
                ar_status[compressor_speed_to_command] = 0;
                break;
            }
        }
    }

    if ((m_status[protezione_evap_low_pressure] == error_level_critical) ||
        (m_status[protezione_overtemperature_gas_scarico_critical] == error_level_critical) ||
        (m_interface->imc102->error_motor != motor_error_none)) {
        return false;
    } else {
        return true;
    }
}


void compressor_actuate(validated_field *ar_conf, int32_t *ar_status, machine_interface_t *m_interface) {
    // verificare che rispe
    myprintf("compressor ACTUATE target speed %d \n", ar_status[compressor_speed_to_command]);
    // apply actions
    switch (ar_conf[control_type_compressor].value) {
        case control_type_compressor_manual_off: {
            if (m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] != 0) {
                set_target_speed(m_interface->imc102, 0);
                myprintf("EVENT: MANOFF compressor speed change to %d\n", 0);
            }
            break;
        }

        case control_type_compressor_manual_speed: {
            int32_t cmd = m_interface->imc102->maximum_running_speed * ar_conf[compressor_manual_speed].value / 100;
            if (m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] != cmd) {
                set_target_speed(m_interface->imc102, cmd);
                myprintf("EVENT: MANSPEED compressor speed change to %d\n", cmd);
            }
            break;
        }

        case control_type_compressor_automatic: {
            if (m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] != ar_status[compressor_speed_to_command]) {
                set_target_speed(m_interface->imc102, ar_status[compressor_speed_to_command]);
                myprintf("EVENT: AUTO compressor speed change to %d\n", ar_status[compressor_speed_to_command]);
            }
            break;
        }
    }
}

void compressor_init(validated_field *ar_conf, int32_t *ar_status, int32_t *m_status,
                     machine_interface_t *m_interface) {
    uint8_t counter_timeout = 0;
    while (m_interface->imc102->running_status == motor_communication_initializing) {
        os_sleep(100);
        // delay_ms(100);
        counter_timeout++;
        if (counter_timeout > 30) {
            // led rosso and break
            myprintf("COMPRESSORE NO COMM\n");
            air_ref_state_change_critical_error(ar_status);
            break;
        }
    }
    if (m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED] == 0) {
        ar_status[compressor_status] = compressor_status_blocked;
    } else {
        ar_status[air_ref_status]          = air_ref_status_run_start;
        ar_status[compressor_status]       = compressor_status_run;
        ar_status[compressor_target_speed] = m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED];
        myprintf("detected running compressor: %d\n", m_interface->imc102->motor_status[INDEX_MOTOR_TARGET_SPEED]);
    }

    // m_status[index_error_overtemperature_gas_scarico] = error_level_none;
}
