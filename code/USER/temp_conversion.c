#include "temp_conversion.h"

/*
 * ADC to temperature calibration:
 * - 0V  corresponds to ADC code 0
 * - 5V  corresponds to ADC code 4095
 * - 0~150C is mapped linearly from ADC 0~4095
 */
#define ADC_FULL_SCALE_CODE 4095.0f
#define ADC_ZERO_CODE       0.0f
#define TEMP_FULL_SCALE_C   150.0f

float adc2temp(uint16_t adc_value) {
    float code = (float)adc_value;
    float active_span = ADC_FULL_SCALE_CODE - ADC_ZERO_CODE;

    if (code > ADC_FULL_SCALE_CODE) {
        code = ADC_FULL_SCALE_CODE;
    }

    if (code <= ADC_ZERO_CODE) {
        return 0.0f;
    }

    return ((code - ADC_ZERO_CODE) * TEMP_FULL_SCALE_C) / active_span;
}
