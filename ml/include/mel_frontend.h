#ifndef MEL_FRONTEND
#define MEL_FRONTEND
#include <stdint.h>

void mel_frontend_init(void);
void mel_frontend_process(const int16_t pcm[16000], float out_mfcc[49][10]);
void mel_frontend_quantize(const float mfcc[49][10], int8_t out[490]);

#endif
