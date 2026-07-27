#ifndef INPUT_CAPTURE_H
#define INPUT_CAPTURE_H
#include <stdbool.h>
#include <stdint.h>

void input_capture_init(void);
uint32_t input_capture_get_frequency_hz(void);
#endif
