#include "control_output.h"

/*
 * 将控制器输出映射为 DAC 码值
 *
 * PID 计算得到的是“控制强度”，而不是直接可写入 DAC 的码值。
 * 由于外部调理级在 DAC=2048 时对应 0V，因此这里把有效控制量映射到
 * DAC 的上半段区间 [2048, 4095]，从而让 0 控制量对应 0V、满量程对应 5V。
 */
uint16_t control_to_dac_code(float control_output, float output_limit) {
    const float active_span = (float)(DAC_CODE_MAX - DAC_ZERO_CODE);
    float scaled;

    if (output_limit <= 0.0f) {
        return DAC_ZERO_CODE;
    }

    if (control_output > output_limit) {
        control_output = output_limit;
    }
    /*
     * 当前执行端是加热装置，负控制量没有实际意义，因此直接钳位为 0。
     * 对应到 DAC 码值时，也就是返回 0V 对应的零点码值 2048。
     */
    if (control_output < 0.0f) {
        control_output = 0.0f;
    }

    scaled = (float)DAC_ZERO_CODE + (control_output * active_span) / output_limit;
    if (scaled > DAC_CODE_MAX) {
        scaled = DAC_CODE_MAX;
    }

    return (uint16_t)(scaled + 0.5f);
}

/*
 * 按外部功率调节级的标定结果，把 DAC 码值转换为实际控制电压。
 *
 * 当前硬件的零电压点位于 DAC 中点，因此下半段码值统一视为 0V，
 * 只有上半段按 0~5V 线性映射。这样 LCD 和串口上报的 OUT 电压
 * 才能和实际执行级保持一致。
 */
float dac_code_to_output_voltage(uint16_t dac_code) {
    float code = (float)dac_code;
    float active_span = (float)(DAC_CODE_MAX - DAC_ZERO_CODE);

    if (code > DAC_CODE_MAX) {
        code = DAC_CODE_MAX;
    }

    if (code <= (float)DAC_ZERO_CODE) {
        return 0.0f;
    }

    return ((code - (float)DAC_ZERO_CODE) * DAC_OUTPUT_MAX_VOLTAGE) / active_span;
}
