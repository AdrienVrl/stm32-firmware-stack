#ifndef I2S_H
#define I2S_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define I2S_MIC_SAMPLE_RATE_HZ 16000u
#define I2S_MIC_WINDOW_SAMPLES (I2S_MIC_SAMPLE_RATE_HZ)

typedef enum
{
    I2S_MIC_SLOT_LEFT  = 0,
    I2S_MIC_SLOT_RIGHT = 1,
} i2s_mic_slot_t;

void i2s_init(i2s_mic_slot_t mic_slot);

void i2s_start(void);

void i2s_stop(void);

const int16_t *i2s_get_ring(void);

uint32_t i2s_get_ring_write_idx(void);

bool i2s_window_ready(void);

void DMA1_Stream0_IRQHandler(void);

#endif
