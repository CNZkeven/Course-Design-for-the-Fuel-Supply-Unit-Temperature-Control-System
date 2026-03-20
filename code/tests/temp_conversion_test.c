#include <assert.h>
#include <math.h>

#include "temp_conversion.h"

static void assert_close(float actual, float expected, float tolerance) {
    assert(fabsf(actual - expected) <= tolerance);
}

int main(void) {
    assert_close(adc2temp(0), 0.0f, 0.1f);
    assert_close(adc2temp(1024), 37.5f, 0.2f);
    assert_close(adc2temp(2048), 75.0f, 0.2f);
    assert_close(adc2temp(3072), 112.5f, 0.2f);
    assert_close(adc2temp(4095), 150.0f, 0.1f);

    assert(adc2temp(3000) > adc2temp(2000));
    assert(adc2temp(1) > 0.0f);

    return 0;
}
