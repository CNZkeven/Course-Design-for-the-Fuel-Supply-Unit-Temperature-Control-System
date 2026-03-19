#ifndef __TEMP_CONVERSION_H
#define __TEMP_CONVERSION_H

#include <stdint.h>

/* 根据带零点偏置的 ADC 标定把原始值换算为温度值。 */
float adc2temp(uint16_t adc_value);

#endif
