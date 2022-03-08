#ifndef NTC_H_
#define NTC_H_

#include "math.h"
#include <stdint.h>
#include "adc_raw.h"

enum pull { pullup = 0, pulldown };

typedef struct {
    enum pull      pull_type;
    uint32_t       pull_resistor;
    uint32_t       beta;
    uint32_t       res_at_25;
    int32_t        offset;
    adc_raw_adc_t *adc_ch;
} ntc_t;

static inline uint32_t ntc_resistance_calculate(ntc_t *ch, uint16_t adc_val) {
    switch (ch->pull_type) {
        case pullup: {
            return (uint32_t)(ch->pull_resistor * adc_val) /
                   ((1 << ch->adc_ch->adc_res) - adc_val);     // thanks to lady ada;
        }
        case pulldown: {
            return 0;
        }
    }
}


static inline int32_t ntc_temperature_calculate(ntc_t *ch, uint16_t adc_val) {
    uint32_t ntc_res = ntc_resistance_calculate(ch, adc_val);
    // 250 as t1
    float t1 = 25 + 273;
    // 10k as r1
    uint32_t r1 = ch->res_at_25;
    // i could use given t1 and r1
    uint32_t beta = ch->beta;
    /*

    T = 1/( (1/Tnominal) + ln(Rth/Rnominal)/B )

    Where: T = Temperature (K), Tnominal = Nominal Temperature, Rth = Thermistor Resistance, Rnominal = Nominal
    Resistance, B = Beta

    moltiplica numeratore e denomicantore per B*Tnominal

    t1= Tnominal
    r1= Rnominal
    */
    float t_kelvin = (beta * t1) / (beta + (t1 * (log2((float)ntc_res) - log2((float)r1)) / log2(2.7)));
    return t_kelvin * 1000 - 273100 + ch->offset;
}

static inline float ntc_beta_calculate(int t1, uint32_t r1, int t2, uint32_t r2) {
    /*
    B= ((T2 * T1) / (T2 - T1)) * ln (R1/R2)
    */
    float  my_t1 = (t1 + 2731) / 10.0;
    float  my_t2 = (t2 + 2731) / 10.0;
    float beta  = (log2((float)r1 / (float)r2) * (my_t1 * my_t2)) / ((my_t2 - my_t1) * log2(2.7));
    return beta;
}

static inline int32_t ntc_temperature_compact_read(ntc_t *ch) {
    return ntc_temperature_calculate(ch, ch->adc_ch->read_adc());
}

static inline int32_t ntc_resistance_compact_read(ntc_t *ch) {
    return ntc_resistance_calculate(ch, ch->adc_ch->read_adc());
}

#endif