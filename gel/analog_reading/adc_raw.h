#ifndef ADC_RAW_H_
#define ADC_RAW_H_

#include <stdint.h>

struct adc;

typedef struct adc {
    uint8_t  adc_res;
    uint32_t max_adc_read;
    int32_t  offset; //yet to use
    uint16_t (*read_adc)();
} adc_raw_adc_t;


#define VOLT_TO_ADC(adc_res, max_read, volt)      ((2 << (adc_res - 1)) * (volt) / max_read)
#define ADC_TO_VOLT(adc_res, max_read, adc_value) (adc_value) * max_read / (2 << (adc_res - 1))

#define ADC_RAW_VOLT_TO_ADC(adc_ch, volt) (uint16_t)VOLT_TO_ADC(adc_ch->adc_res, adc_ch->max_adc_read, volt)
#define ADC_RAW_ADC_TO_VOLT(adc_ch, adc_value) (uint16_t)ADC_TO_VOLT(adc_ch->adc_res, adc_ch->max_adc_read, adc_value)

#endif /* ADC_RAW_H_ */