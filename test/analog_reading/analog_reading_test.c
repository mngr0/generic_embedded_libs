#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "analog_reading/analog_reading.h"
#include "unity.h"



uint16_t read_adc1(){
    return 20000;
}

static adc_raw_adc_t adc1 = {
    .adc_res=16, 
    .max_adc_read=3300,
    .offset=0,
    .read_adc = read_adc1
};

static adc_raw_adc_t adc2 = {
    .adc_res=16, 
    .max_adc_read=5000,
    .offset=0,
    .read_adc = read_adc1
};

static pressure_t pressure1 = {
    .adc_ch=&adc1,
    .adc1=ADC_RAW_VOLT_TO_ADC((&adc1), 500), 
    .adc2=ADC_RAW_VOLT_TO_ADC((&adc1), 4500),
    .pressure1=-1000,
    .pressure2=9000,
    .offset=0,
};

// static pressure_t pressure2 = {
//     .adc_ch=&adc1,
//     .adc1=ADC_RAW_VOLT_TO_ADC(&adc1, 500), 
//     .adc2=ADC_RAW_VOLT_TO_ADC(&adc1, 4500),
//     .pressure1=0,
//     .pressure2=35000,
//     .offset=0,
// };

// static ntc_t ntc1 = {
//     .adc_ch=&adc1,
//     .pull_type=pulldown, 
//     .pull_resistor=4700,
//     .res_at_25 = 10000,
//     .beta=4000,
//     .offset=0,
// };

// static ntc_t ntc2 = {
//     .adc_ch=&adc1,
//     .pull_type=pulldown, 
//     .pull_resistor=10000,
//     .res_at_25 = 100000,
//     .beta=3232,
//     .offset=0,
// };


void setUp(void) {

}

void tearDown(void) {}

void test_adc_reading(void){
    TEST_ASSERT_EQUAL(0, 1);
}
