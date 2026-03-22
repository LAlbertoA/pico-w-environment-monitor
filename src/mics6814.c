#include "mics6814.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

static const float VREF = 3.3f; // Pico ADC reference (aprox)

static uint g_co_gpio;
static uint g_nh3_gpio;
static uint g_no2_gpio;


void mics6814_init(uint co_gpio, uint nh3_gpio, uint no2_gpio) {
    g_co_gpio = co_gpio;
    g_nh3_gpio = nh3_gpio;
    g_no2_gpio = no2_gpio;

    adc_init();
    adc_gpio_init(g_co_gpio);
    adc_gpio_init(g_nh3_gpio);
    adc_gpio_init(g_no2_gpio);
}

static inline uint16_t read_adc_gpio(uint gpio) {
    adc_select_input(gpio - 26); // 26->0, 27->1, 28->2
    sleep_us(5);                 // pequeño settle
    return adc_read();           // 0..4095
}

bool mics6814_read(uint16_t *co_raw, uint16_t *nh3_raw, uint16_t *no2_raw) {
    // Simple averaging to reduce noise
    const int N = 64;
    uint32_t co_acc = 0, nh3_acc = 0, no2_acc = 0;
      
    for (int i = 0; i < N; i++) {
        co_acc  += read_adc_gpio(g_co_gpio);
        nh3_acc += read_adc_gpio(g_nh3_gpio);
        no2_acc += read_adc_gpio(g_no2_gpio);
        sleep_us(200);
    }

    *co_raw  = (uint16_t)(co_acc / N);
    *nh3_raw = (uint16_t)(nh3_acc / N);
    *no2_raw = (uint16_t)(no2_acc / N);

    return true;
}

float mics6814_raw_to_volts(uint16_t raw) {
    return (raw * VREF) / 4095.0f;
}