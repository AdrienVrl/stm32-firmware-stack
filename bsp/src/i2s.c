#include "i2s.h"

#include "common.h"
#include "gpio.h"
#include "spi.h"
#include "system_stm32f4xx.h"

#include <stdint.h>
#include <string.h>

#ifndef UNIT_TEST
#define RCC_BASE 0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40)) // add 0x40 offset for APB1ENR register
#define RCC_AHB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x30)) // add 0x40 offset for APB1ENR register
#define RCC_CR         (*(volatile uint32_t *)(RCC_BASE + 0x0)) // add 0x08 offset for CFGR register
#define RCC_CFGR       (*(volatile uint32_t *)(RCC_BASE + 0x08)) // add 0x08 offset for CFGR register
#define RCC_PLLCFGR    (*(volatile uint32_t *)(RCC_BASE + 0x04UL))
#define RCC_PLLI2SCFGR (*(volatile uint32_t *)(RCC_BASE + 0x84UL))

#define GPIOA_BASE 0x40020000UL
#define GPIOC_BASE 0x40020800UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)
#define GPIOC      ((GPIO_Port *)GPIOC_BASE)

#define SPI3_BASE  0x40003C00UL
#define SPI3       ((SPI_TypeDef *)SPI3_BASE)
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100UL)
#endif
#define DMA1_BASE        0x40026000UL
#define DMA1             ((DMA_TypeDef *)DMA1_BASE)
#define DMA1_Stream0     (&DMA1->STREAM[0]) // SPI3_RX
#define DMA_SxCR_EN      (1 << 0)
#define DMA_SxCR_TCIE    (1 << 4)
#define DMA_SxCR_MINC    (1 << 10)
#define DMA_SxCR_DIR_M2P (1 << 6)
#define DMA_SxCR_DIR_P2M (0)
#define DMA_SxFCR_DMDIS  (1 << 2) // Direct mode disable
#define TIMEOUT          (0x5000UL)

#define RAW_HALFWORDS      512u
#define RAW_HALF_HALFWORDS (RAW_HALFWORDS / 2u)
#define FRAME_HALFWORDS    4u

static volatile uint16_t s_raw_dma_buf[RAW_HALFWORDS];
static volatile int16_t s_ring[I2S_MIC_WINDOW_SAMPLES];
static volatile uint32_t s_ring_write_idx   = 0;
static volatile uint32_t s_samples_captured = 0;

static i2s_mic_slot_t s_mic_slot = I2S_MIC_SLOT_LEFT;

static void configure_clocks(void);
static void configure_gpio(void);
static void configure_i2s(void);
static void configure_dma(void);
static void reconstruct_half(const uint16_t *half, size_t half_len_halfwords);

void i2s_init(i2s_mic_slot_t mic_slot)
{
    s_mic_slot         = mic_slot;
    s_ring_write_idx   = 0;
    s_samples_captured = 0;
    memset((void *)s_ring, 0, sizeof(s_ring));
    memset((void *)s_raw_dma_buf, 0, sizeof(s_raw_dma_buf));

    configure_clocks();
    configure_gpio();
    configure_i2s();
    configure_dma();
}

static void PLLI2S_Error(void)
{
    while (1)
    {
    }
}

static void configure_clocks(void)
{

    RCC_AHB1ENR |= (1 << 21); /* DMA1_clock */
    RCC_APB1ENR |= (1 << 15); /* SPI3_clock */

    RCC_PLLI2SCFGR =
        (8u << 0) | (128u << 6) |
        (5u << 28); /* RCC_PLLI2SCFGR_PLLI2SM, RCC_PLLI2SCFGR_PLLI2SN and RCC_PLLI2SCFGR_PLLI2SR */

    RCC_CR |= (1UL << 26); /* RCC_CR_PLLI2SON */

    uint32_t timeout = TIMEOUT;
    while (!(RCC_CR & (1UL << 27))) /* wait for RCC_CR_PLLI2SRDY */
    {                               /* wait for lock */

        if (--timeout == 0)
        {
            PLLI2S_Error();
        }
    }
}

static void configure_gpio(void)
{

    GPIO_Config i2s_cfg = {.mode      = GPIO_MODE_ALTERNATE,
                           .otype     = GPIO_OTYPE_PUSH_PULL,
                           .speed     = GPIO_SPEED_VERY_HIGH,
                           .pupd      = GPIO_PUPD_NONE,
                           .alternate = 6};

    GPIO_Init(GPIOA, 4, i2s_cfg);
    GPIO_Init(GPIOC, 10, i2s_cfg);
    GPIO_Init(GPIOC, 12, i2s_cfg);
}

static void configure_i2s(void)
{
    /* I2SDIV=12, ODD=1, MCKOE=0 (no MCLK output) */
    SPI3->I2SPR = (12u << 0) | (1 << 8);

    /* I2SMOD=1, I2SCFG=11 (Master Receive), I2SSTD=00 (Philips),
     * CKPOL=0, DATLEN=01 (24-bit), CHLEN=1 (32-bit channel) */
    SPI3->I2SCFGR = (1 << 11) | (0x3u << 8) | (0x0u << 4) | (0x1u << 1) | (1 << 0);

    SPI3->CR2 |= (1 << 0);
}

static void DMA1_Error(void)
{
    while (1)
    {
    }
}

static void configure_dma(void)
{
    DMA1_Stream0->CR &= ~(1 << 0); /* DMA_SxCR_EN */

    uint32_t timeout = TIMEOUT;
    while (DMA1_Stream0->CR & (1 << 0))
    { /* wait for disable to take effect */

        if (--timeout == 0)
        {
            DMA1_Error();
        }
    }

    DMA1->LIFCR = (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 0);

    DMA1_Stream0->PAR  = (uint32_t)&SPI3->DR;
    DMA1_Stream0->M0AR = (uint32_t)s_raw_dma_buf;
    DMA1_Stream0->NDTR = RAW_HALFWORDS;

    DMA1_Stream0->CR = (0u << 25)     /* channel 0 */
                       | (0x1u << 10) /* increment memory ptr */
                       | (0x1u << 11) /* 16-bit peripheral */
                       | (0x1u << 13) /* 16-bit memory */
                       | (0x1u << 8)  /* circular, double-buffer pattern */
                       | (0x1u << 3)  /* half-transfer interrupt */
                       | (0x1u << 4); /* transfer-complete interrupt */
    /* DIR left at 00 = peripheral-to-memory (correct for RX) */

    NVIC_ISER0 |= (1 << 11); // DMA1_Stream0_IRQn
}

void i2s_start(void)
{
    DMA1_Stream0->CR |= (1 << 0); /* DMA_SxCR_EN */
    SPI3->I2SCFGR |= (1 << 10);   /* SPI_I2SCFGR_I2SE*/
}

void i2s_stop(void)
{
    SPI3->I2SCFGR &= ~(1 << 10);
    DMA1_Stream0->CR &= ~(1 << 0);
}
const int16_t *i2s_get_ring(void)
{
    return (const int16_t *)s_ring;
}

uint32_t i2s_get_ring_write_idx(void)
{
    return s_ring_write_idx;
}

bool i2s_window_ready(void)
{
    return s_samples_captured >= I2S_MIC_WINDOW_SAMPLES;
}

static void reconstruct_half(const uint16_t *half, size_t half_len_halfwords)
{
    /* slot_offset selects which slot (L=0, R=1) within each stereo frame
     * the mic actually drives, per s_mic_slot. */
    size_t slot_offset = (s_mic_slot == I2S_MIC_SLOT_LEFT) ? 0u : (FRAME_HALFWORDS / 2u);

    uint32_t widx = s_ring_write_idx;

    for (size_t i = slot_offset; i < half_len_halfwords; i += FRAME_HALFWORDS)
    {
        int16_t sample = (int16_t)half[i]; /* MSB half-word = the sample */

        s_ring[widx] = sample;
        widx         = (widx + 1u) % I2S_MIC_WINDOW_SAMPLES;

        if (s_samples_captured < I2S_MIC_WINDOW_SAMPLES)
        {
            s_samples_captured++;
        }
    }

    s_ring_write_idx = widx;
}

void DMA1_Stream0_IRQHandler(void)
{
    if (DMA1->LISR & (1 << 4))  /* DMA_LISR_HTIF0 */
    {
        DMA1->LIFCR = (1 << 4); /* DMA_LIFCR_CHTIF0 */
        reconstruct_half((const uint16_t *)&s_raw_dma_buf[0], RAW_HALF_HALFWORDS);
    }
    if (DMA1->LISR & (1 << 5))  /* DMA_LISR_TCIF0 */
    {
        DMA1->LIFCR = (1 << 5); /* DMA_LIFCR_CTCIF0 */
        reconstruct_half((const uint16_t *)&s_raw_dma_buf[RAW_HALF_HALFWORDS], RAW_HALF_HALFWORDS);
    }
}
