#include "temp_conversion.h"

/*
 * 外部温度输入与显示温度的线性标定参数
 *
 * 说明：
 * 1. 外部模拟量满量程等效为 0~5V
 * 2. 显示温度量程定义为 0~150℃
 * 3. 经过前端调理后，外部 0V 在 ADC 侧对应约 2048 code
 * 4. 外部 +5V 在 ADC 侧对应满量程 4095 code
 */
#define ADC_FULL_SCALE_CODE       4095.0f
#define ADC_ZERO_CODE             2048.0f
#define TEMP_FULL_SCALE_C         150.0f

/*
 * ADC 原始值 -> 温度值
 *
 * 当前硬件的温度输入经过调理后带有零点偏置：
 *   2048 code ->   0℃
 *   4095 code -> 150℃
 *
 * 因此需要先扣除 ADC 侧的零点偏置，再把有效上半段映射到 0~150℃。
 * 若 ADC 值低于零点偏置，则按 0℃ 处理。
 */
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
