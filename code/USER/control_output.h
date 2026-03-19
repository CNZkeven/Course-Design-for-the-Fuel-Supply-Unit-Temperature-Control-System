#ifndef __CONTROL_OUTPUT_H
#define __CONTROL_OUTPUT_H

#include <stdint.h>

/*
 * DAC 输出级标定参数
 *
 * 当前外部调理级的零电压点位于 DAC 中点附近：
 * - DAC 0~2048   -> 0V
 * - DAC 2048~4095 -> 0~5V
 *
 * 因此软件侧把 2048 视为“零输出码值”，只使用上半段作为有效调压区间。
 */
#define DAC_CODE_MAX 4095u
#define DAC_ZERO_CODE 2048u
#define DAC_OUTPUT_MAX_VOLTAGE 5.0f

/* 把 PID 控制量按上限线性映射为 12 位 DAC 码值。 */
uint16_t control_to_dac_code(float control_output, float output_limit);

/* 把 DAC 码值换算为外部执行级的等效控制电压，便于显示和串口上报。 */
float dac_code_to_output_voltage(uint16_t dac_code);

#endif
