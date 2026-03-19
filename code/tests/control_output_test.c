#include <assert.h>
#include <math.h>
#include <stdint.h>

uint16_t control_to_dac_code(float control_output, float output_limit);
float dac_code_to_output_voltage(uint16_t dac_code);

int main(void) {
    assert(control_to_dac_code(0.0f, 100.0f) == 2048);
    assert(control_to_dac_code(100.0f, 100.0f) == 4095);
    assert(control_to_dac_code(-100.0f, 100.0f) == 2048);
    assert(control_to_dac_code(50.0f, 100.0f) == 3072);

    assert(control_to_dac_code(25.0f, 100.0f) == 2560);
    assert(control_to_dac_code(-25.0f, 100.0f) == 2048);
    assert(control_to_dac_code(5.0f, 100.0f) == 2150);

    assert(fabsf(dac_code_to_output_voltage(0) - 0.0f) < 0.01f);
    assert(fabsf(dac_code_to_output_voltage(1024) - 0.0f) < 0.01f);
    assert(fabsf(dac_code_to_output_voltage(2048) - 0.0f) < 0.01f);
    assert(fabsf(dac_code_to_output_voltage(3072) - 2.50f) < 0.02f);
    assert(fabsf(dac_code_to_output_voltage(4095) - 5.0f) < 0.01f);

    return 0;
}
