#include <atmel_start.h>
#include <stdbool.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "infineonlib.h"

#include "uartIMC102.h"
#include "uartIMM101.h"
#include "myprintf/myprintf.h"

#define abs(a) (a > 0 ? a : -a)

#define INF_COMM_OK          0
#define INF_COMM_UNREACHABLE 1


static inline uint8_t check_checksum(uint8_t *df_p) {
    return CHECKSUM(df_p) == CONCAT(df_p[7], df_p[6]);
}

static inline void build_frame_command(uint8_t *data_frame_p, uint8_t dest_address, uint8_t command, int16_t data) {
    data_frame_p[0]   = dest_address;
    data_frame_p[1]   = command;
    data_frame_p[2]   = ((uint8_t *)&data)[0];
    data_frame_p[3]   = ((uint8_t *)&data)[1];
    data_frame_p[4]   = 0;
    data_frame_p[5]   = 0;
    uint16_t checksum = CHECKSUM(data_frame_p);
    data_frame_p[7]   = ((uint8_t *)&checksum)[1];
    data_frame_p[6]   = ((uint8_t *)&checksum)[0];
}


static inline void build_frame_register(uint8_t *data_frame_p, uint8_t dest_address, uint8_t command, uint8_t app_id,
                                        uint8_t register_id, uint16_t data) {
    data_frame_p[0]   = dest_address;
    data_frame_p[1]   = command;
    data_frame_p[2]   = app_id;
    data_frame_p[3]   = register_id;
    data_frame_p[4]   = ((uint8_t *)&data)[0];
    data_frame_p[5]   = ((uint8_t *)&data)[1];
    uint16_t checksum = CHECKSUM(data_frame_p);
    data_frame_p[7]   = ((uint8_t *)&checksum)[1];
    data_frame_p[6]   = ((uint8_t *)&checksum)[0];
}

static inline void write_register(infineon_device_t *dev, uint16_t app_id, uint16_t reg, int16_t value) {
    uint8_t frame[8];
    build_frame_register(frame, dev->address, COMMAND_WRITE_REGISTER, app_id, reg, value);
    xQueueSend(dev->command_queue, frame, 10);
}



void clear_fault(infineon_device_t *dev) {
    uint8_t frame[8];
    build_frame_command(frame, dev->address, COMMAND_CLEAR_FAULT, 0);
    xQueueSend(dev->command_queue, frame, 10);
    dev->error_motor = motor_error_none;
}


static inline void set_motor_control(infineon_device_t *dev, uint16_t data) {
    // myprintf("INF DEBUG %s: configure ONOFF to %d\n",pcTaskGetTaskName( dev->xInfineonTask),data);
    write_register(dev, APP_ID_MOTOR, MOTOR_COMMAND, data);
}

// void clear_error(infineon_device_t  * dev){
// 	dev->error_motor=motor_error_none;
// }


void set_target_speed(infineon_device_t *dev, int16_t speed) {
    speed = abs(speed);
    myprintf("INF DEBUG %s: configure speed to %d\n", pcTaskGetTaskName(dev->xInfineonTask), speed);

    if (dev->error_motor != motor_error_none) {
        return;
    }
    if (speed > dev->maximum_running_speed) {
        myprintf("INF DEBUG %s: written reduced \n", pcTaskGetTaskName(dev->xInfineonTask));
        // write_register(dev,APP_ID_MOTOR,MOTOR_TARGET_SPEED,dev->maximum_running_speed);
        speed = dev->maximum_running_speed;
        return;
    }
    if (speed < dev->minimum_running_speed) {
        // write_register(dev,APP_ID_MOTOR,MOTOR_TARGET_SPEED,0);
        speed = dev->minimum_running_speed;
        return;
    }
    if (speed == 0) {
        // motor stopping
        dev->running_status = motor_stopped;
        write_register(dev, APP_ID_MOTOR, MOTOR_TARGET_SPEED, 0);
    } else {
        myprintf("INF DEBUG %s: written \n", pcTaskGetTaskName(dev->xInfineonTask));
        write_register(dev, APP_ID_MOTOR, MOTOR_TARGET_SPEED, speed * dev->direction);
		if(dev->running_status == motor_stopped){
			dev->running_status = motor_starting;
		}
    }
}


// void set_target_speed_percentuale (infineon_device_t  * dev, uint8_t percentuale){
// 	percentuale=min(100, percentuale);
// 	set_target_speed(dev, percentuale*dev->maximum_running_speed/100);
// }

void load_configuration(infineon_device_t *dev, uint8_t conf) {
    // check loaded configuration
    uint8_t frame[8];
    build_frame_register(frame, dev->address, COMMAND_LOAD_PARAMETER_SET, 0, COMMAND_LOAD_PARAMETER_SET, conf);
    xQueueSend(dev->command_queue, frame, 10);
    return;
}




//
// static void print_communication(uint8_t* tx, uint8_t* rx ){
// 	myprintf("tx:");
// 	for (int i= 0;i< 8;i ++)
// 	{
// 		myprintf("%02x ",tx[i]);
// 	}
//
// 	myprintf(" rx:");
// 	for (int i= 0;i< 8;i ++)
// 	{
// 		myprintf("%02x ",rx[i]);
// 	}
// 	myprintf("\n");
// }
//



int transfer_data(infineon_device_t *dev, uint8_t *data, int16_t *reply) {
	//verificare che checksum funziona
	//verificare che frame ricevuti parzialmente
    uint8_t data_frame_rx[8];
    uint8_t timeout_cnt     = 0;
    uint8_t resend_cnt      = 0;
    uint8_t checksum_errors = 0;

    if (dev->cnt_begin != dev->cnt_end) {
        // delay
        os_sleep(100);
        // myprintf("INF DEBUG %s: debug_point1\n",pcTaskGetTaskName( dev->xInfineonTask));
        dev->cnt_begin = dev->cnt_end;
    }

    while (resend_cnt < 4) {
        resend_cnt++;
        io_write(dev->io, data, 8);
        // increase
        uint32_t ret_value = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        if (ret_value) {
            for (int i = 0; i < 8; i++) {
                data_frame_rx[i] = dev->buffer[SUM_MOD(dev->cnt_begin, i)];
            }
            dev->cnt_begin = SUM_MOD(dev->cnt_begin, 8);
            // print_communication(data,data_frame_rx);
            if (!check_checksum(data_frame_rx)) {
                // myprintf("INF DEBUG %s:\t\twrong checksum\n",pcTaskGetTaskName( dev->xInfineonTask));
                checksum_errors++;
                resend_cnt--;
                continue;
            }
            if (data_frame_rx[1] != (data[1] | 0x80)) {
                // myprintf("INF DEBUG %s:\t\tdiscarded %x not %x\n",pcTaskGetTaskName(
                // dev->xInfineonTask),data_frame_rx[1] , (data[1] | 0x80));
                resend_cnt--;
                continue;
            }
            *reply = CONCAT(data_frame_rx[5], data_frame_rx[4]);
            // myprintf("INF DEBUG %s: COMM_DONE\n",pcTaskGetTaskName( dev->xInfineonTask));
            return INF_COMM_OK;
        } else {     // timeout
            timeout_cnt++;
            // myprintf("INF DEBUG %s:\t\tTIMEOUT %d - %d\n",pcTaskGetTaskName( dev->xInfineonTask),
            // timeout_cnt,SUM_MOD(dev->cnt_end, 0-dev->cnt_begin));
            os_sleep(50);
            // if timeout for N consecutives messages, consider unreachable
            if (timeout_cnt > 2) {
                return INF_COMM_UNREACHABLE;
            }
        }
    }
    // myprintf("INF DEBUG %s: RETURN LAST\n",pcTaskGetTaskName( dev->xInfineonTask));
    return INF_COMM_UNREACHABLE;
}


int16_t read_register(infineon_device_t *dev, uint16_t app_id, uint16_t reg) {
    uint8_t frame[8];
    int16_t result;
    build_frame_register(frame, dev->address, COMMAND_WRITE_REGISTER, app_id, reg, 0);
    if (transfer_data(dev, frame, &result) != INF_COMM_OK) {
        myprintf("INF DEBUG %s:WRONG TRANSFER\n", pcTaskGetTaskName(dev->xInfineonTask));
    }
    return result;
}


inline void infineon_ISR(infineon_device_t *dev) {

    io_read(dev->io, (uint8_t *)&(dev->buffer[dev->cnt_end]), 1);
    dev->cnt_end = SUM_MOD(dev->cnt_end, 1);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (SUM_MOD(dev->cnt_end, 0 - dev->cnt_begin) == 8) {
        vTaskNotifyGiveFromISR(dev->xInfineonTask, &xHigherPriorityTaskWoken);
    }
}


static inline bool is_contained(int16_t val, int16_t vmin, int16_t vmax) {
    return ((val >= min(vmin, vmax)) && (val <= max(vmin, vmax)));
}

void actuate(infineon_device_t *dev) {     // check if IMC needs to start or stop

    switch (dev->running_status) {
        case motor_stopped: {

            // if target speed != 0 and speed = 0 , send motor start
            if ((dev->motor_status[INDEX_MOTOR_SPEED] == 0) && (dev->motor_status[INDEX_MOTOR_TARGET_SPEED] != 0) &&
                (dev->error_motor == motor_error_none)) {
                set_motor_control(dev, 1);
                myprintf("IRC DEBUG_START %s: status=%d target_speed=%d speed=%d \n",
                         pcTaskGetTaskName(dev->xInfineonTask), dev->running_status,
                         dev->motor_status[INDEX_MOTOR_TARGET_SPEED], dev->motor_status[INDEX_MOTOR_SPEED]);
                dev->running_status = motor_starting;
                dev->timer_motor    = xTaskGetTickCount();
            } else {
                set_motor_control(dev, 0);
            }
            break;
        }
        case motor_starting: {
            // protect from unable to start
            // if target_speed happens to be 0 -> go motor stopped
            // if motor_speed is higher than minimum -> go running

            if (abs(dev->motor_status[INDEX_MOTOR_SPEED]) < dev->minimum_running_speed) {
                if (xTaskGetTickCount() - dev->timer_motor > 15000) {
                    dev->error_motor    = motor_error_unable_to_start;
                    dev->running_status = motor_stopped;
                    myprintf("INF DEBUG: unable to start, critical\n");
                    set_target_speed(dev, 0);
                }
            }
            // if error is none
            else if ((abs(dev->motor_status[INDEX_MOTOR_SPEED]) > dev->minimum_running_speed) &&
                     (dev->error_motor == motor_error_none)) {
                dev->running_status = motor_running;
                dev->timer_motor    = xTaskGetTickCount();
            }
            break;
        }
        case motor_running: {
            // protect from unable to keep speed
            // if target_speed happens to be 0 -> go motor stopped

            if (!is_contained(dev->motor_status[INDEX_MOTOR_SPEED],
                              dev->motor_status[INDEX_MOTOR_TARGET_SPEED] * 9 / 10,
                              dev->motor_status[INDEX_MOTOR_TARGET_SPEED] * 11 / 10)) {
                // myprintf("INF DEBUG: %d is not contained in %d ...
                // %d\n",dev->motor_status[INDEX_MOTOR_SPEED],dev->motor_status[INDEX_MOTOR_TARGET_SPEED]*9/10
                // ,dev->motor_status[INDEX_MOTOR_TARGET_SPEED]*11/10);
                if (xTaskGetTickCount() - dev->timer_motor > 20000) {
                    dev->error_motor    = motor_error_unable_to_maintain_speed;
                    dev->running_status = motor_stopped;
                    myprintf("INF DEBUG: unable keep speed, critical\n");
                    set_target_speed(dev, 0);
                }
            } else {
                dev->timer_motor = xTaskGetTickCount();
            }


            if ((dev->motor_status[INDEX_MOTOR_SPEED] != 0) && (dev->motor_status[INDEX_MOTOR_TARGET_SPEED] == 0)) {
                if (abs(dev->motor_status[INDEX_MOTOR_SPEED]) < dev->minimum_running_speed * 3 / 2) {
                    set_motor_control(dev, 0);
                    // myprintf("IRC DEBUG_STOP %s: status=%d target_speed=%d speed=%d \n",pcTaskGetTaskName(
                    // dev->xInfineonTask),dev->running_status,dev->motor_status[INDEX_MOTOR_TARGET_SPEED],dev->motor_status[INDEX_MOTOR_SPEED]);
                    if (dev->running_status != motor_stopped) {
                        dev->running_status = motor_stopped;
                        dev->timer_motor    = xTaskGetTickCount();
                    }
                }
            }
            break;
        }
        case motor_communication_initializing: {
            break;
        }
    }
}




void infineon_task(void *p) {
    (void)p;
    infineon_device_t *dev = (infineon_device_t *)p;
    uint8_t            tmp_buffer[8];
    uint8_t            check_motor_states[INDEX_MOTOR_SIZE][8];
    uint8_t            index_last         = 0;
    uint8_t            transmission_fails = 0;
    BaseType_t         is_command         = 0;
    int16_t            result;

    build_frame_command(check_motor_states[INDEX_MOTOR_FAULT], dev->address, COMMAND_READ_STATUS, STATUS_FAULT_FLAGS);
    build_frame_command(check_motor_states[INDEX_MOTOR_SPEED], dev->address, COMMAND_READ_STATUS, STATUS_MOTOR_SPEED);
    build_frame_register(check_motor_states[INDEX_MOTOR_MOTOR_CURRENT], dev->address, COMMAND_READ_REGISTER,
                         APP_ID_MOTOR, MOTOR_CURRENT, 0);
    build_frame_register(check_motor_states[INDEX_MOTOR_TARGET_SPEED], dev->address, COMMAND_READ_REGISTER,
                         APP_ID_MOTOR, MOTOR_TARGET_SPEED, 0);

    // tranfer data for initialization
    // minimum_running_speed=read_register(dev,APP_ID_MOTOR,MOTOR_MIN_RUNNING_SPEED);
    // myprintf("minspeed=%d\n",minimum_running_speed);


    // load required configuration
    dev->running_status = motor_communication_initializing;

    while (1) {
        is_command = xQueueReceive(dev->command_queue, tmp_buffer, 0);
        if (is_command == pdFALSE) {
            for (int i = 0; i < 8; i++) {
                tmp_buffer[i] = check_motor_states[index_last][i];
            }
        }

        while (transfer_data(dev, tmp_buffer, &result) != INF_COMM_OK) {
            transmission_fails++;
            os_sleep(100);
            if (dev->cnt_begin != dev->cnt_end) {
                // myprintf("INF DEBUG %s: debug_point 2\n",pcTaskGetTaskName( dev->xInfineonTask));
                dev->cnt_begin = dev->cnt_end;
            }

            if (transmission_fails > 10) {
                dev->error_motor = motor_error_communication;
                if (is_command == pdTRUE) {
                    myprintf("INF DEBUG %s: COMMUNICATION BROKEN COMMAND\n", pcTaskGetTaskName(dev->xInfineonTask));
                } else {
                    myprintf("INF DEBUG %s: COMMUNICATION BROKEN UPDATE\n", pcTaskGetTaskName(dev->xInfineonTask));
                }
            }
        }
        transmission_fails = 0;
        if (dev->error_motor == motor_error_communication) {
            dev->error_motor = motor_error_none;
        }

        if (!is_command) {
            dev->motor_status[index_last] = result;
            if (index_last == (INDEX_MOTOR_SIZE - 1)) {
                // myprintf("INF DEBUG %s: ROUND DONE IN %d\n",pcTaskGetTaskName(
                // dev->xInfineonTask),xTaskGetTickCount()-time_round); time_round=xTaskGetTickCount();
                if (dev->running_status == motor_communication_initializing) {
                    if (dev->motor_status[INDEX_MOTOR_TARGET_SPEED] > 0) {
                        dev->running_status = motor_running;
                    } else {
                        dev->running_status = motor_stopped;
                    }
                }
                actuate(dev);
            }
            index_last = (index_last + 1) % INDEX_MOTOR_SIZE;
        }
    }
}
