#ifndef __TEMP_CONVERSION_H
#define __TEMP_CONVERSION_H

#include <stdint.h>

/* Convert raw ADC code to temperature in Celsius. */
float adc2temp(uint16_t adc_value);

#endif
