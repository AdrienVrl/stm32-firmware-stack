#include "mel_frontend.h"
#include "mock_registers.h"
#include "reference_mfcc.h"
#include "reference_pcm.h"
#include "reference_quantized.h"
#include "unity.h"

#include <stdint.h>
void setUp()
{
}

void tearDown()
{
}

void test_mel_frontend_process(void)
{
    mel_frontend_init();
    float out_mfcc[49][10];
    mel_frontend_process(REFERENCE_PCM, out_mfcc);
    for (int i = 0; i < 49; i++)
    {
        for (int j = 0; j < 10; j++)
        {

            TEST_ASSERT_FLOAT_WITHIN(0.001f, REFERENCE_MFCC[i][j], out_mfcc[i][j]);
        }
    }
}

void test_mel_frontend_quantize(void)
{
    mel_frontend_init();
    int8_t out_quantized[49][10];
    mel_frontend_quantize(REFERENCE_MFCC, (int8_t *)out_quantized);
    for (int i = 0; i < 49; i++)
    {
        for (int j = 0; j < 10; j++)
        {

            TEST_ASSERT_FLOAT_WITHIN(1, REFERENCE_QUANTIZED[i][j], out_quantized[i][j]);
        }
    }
}

int main(void)
{

    UNITY_BEGIN();

    RUN_TEST(test_mel_frontend_process);
    RUN_TEST(test_mel_frontend_quantize);

    UNITY_END();
    return 0;
}
