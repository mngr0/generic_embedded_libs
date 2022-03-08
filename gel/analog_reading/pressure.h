#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "adc_raw.h"

typedef struct {
    uint32_t       adc1;
    int32_t        pressure1;
    uint32_t       adc2;
    int32_t        pressure2;
    int32_t        offset;
    adc_raw_adc_t *adc_ch;
} pressure_t;

static inline int32_t pressure_adc_to_pressure(volatile pressure_t *p_ch, uint16_t adc_value) {

    int32_t pressure = ((int32_t)adc_value - (int32_t)p_ch->adc1) * (p_ch->pressure2 - p_ch->pressure1) /
                           ((int32_t)p_ch->adc2 - (int32_t)p_ch->adc1) +
                       p_ch->pressure1;
    return pressure + p_ch->offset;
}


static inline int32_t pressure_compact_read(volatile pressure_t *p_ch) {
    return pressure_adc_to_pressure(p_ch, p_ch->adc_ch->read_adc());
}


#endif