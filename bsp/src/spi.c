#include "spi.h"

#include "gpio.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SPI_DUMMY_BYTE 0xFF // filler value to clock out while reading
#define RCC_BASE       0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40)) // add 0x40 offset for APB1ENR register
#define RCC_AHB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x30)) // add 0x40 offset for APB1ENR register

#define GPIOB_BASE 0x40020400UL
#define GPIOB      ((GPIO_Port *)GPIOB_BASE)

#define SPI2_BASE 0x40003800UL
#define SPI2      ((SPI_TypeDef *)SPI2_BASE)

#define DMA1_BASE        0x40026000UL
#define DMA1             ((DMA_TypeDef *)DMA1_BASE)
#define DMA1_Stream3     (&DMA1->STREAM[3]) // SPI2_RX
#define DMA1_Stream4     (&DMA1->STREAM[4]) // SPI2_TX
#define DMA_SxCR_EN      (1 << 0)
#define DMA_SxCR_TCIE    (1 << 4)
#define DMA_SxCR_MINC    (1 << 10)
#define DMA_SxCR_DIR_M2P (1 << 6)
#define DMA_SxCR_DIR_P2M (0)
#define NVIC_ISER0       (*(volatile uint32_t *)0xE000E100UL)
#define DMA_SxFCR_DMDIS  (1 << 2) // Direct mode disable

#define TIMEOUT (0x5000UL)


static void RXNE_Error(void)
{
    while (1)
    {
    }
}

static void TXE_Error(void)
{
    while (1)
    {
    }
}

static inline void spi_cs_select(void)
{
    GPIO_WritePin(GPIOB, 12, 0); // pull CS low — assert
}

static inline void spi_cs_deselect(void)
{
    GPIO_WritePin(GPIOB, 12, 1); // release CS high — deassert
}

void spi_init(SPI_Config cfg)
{
    // PB12: software-controlled CS, plain GPIO output — NOT alternate function
    GPIO_Config cs_cfg = {.mode  = GPIO_MODE_OUTPUT,
                          .otype = GPIO_OTYPE_PUSH_PULL,
                          .speed = GPIO_SPEED_VERY_HIGH,
                          .pupd  = GPIO_PUPD_NONE};
    GPIO_Init(GPIOB, 12, cs_cfg);
    spi_cs_deselect();
    uint8_t spi2_pins[3] = {13, 14, 15}; // SCK, MISO, MOSI

    for (int i = 0; i < 3; i++)
    {
        GPIO_Config gpio_cfg = {
            .mode      = GPIO_MODE_ALTERNATE,
            .otype     = GPIO_OTYPE_PUSH_PULL,
            .speed     = GPIO_SPEED_VERY_HIGH, // SPI clock lines benefit from fast slew rate
            .pupd      = GPIO_PUPD_NONE,
            .alternate = 5                     // AF5 = SPI1/SPI2 on STM32F4
        };
        GPIO_Init(GPIOB, spi2_pins[i], gpio_cfg);
    }

    RCC_APB1ENR |= (1 << 14);
    volatile uint32_t dummy;
    dummy = RCC_APB1ENR;
    (void)dummy;

    uint32_t cr1 = 0;

    cr1 |= (cfg.mode & 0x3);     // bits 0-1: CPHA, CPOL
    cr1 |= (cfg.prescaler << 3); // bits 3-5: BR[2:0]
    cr1 |= (1 << 2);             // bit 2: MSTR — master mode
    cr1 |= (1 << 8);             // bit 8: SSI — internal slave select high
    cr1 |= (1 << 9);             // bit 9: SSM — software slave management

    SPI2->CR1 = cr1;

    SPI2->CR1 |= (1 << 6); // bit 6: SPE — enable peripheral, set last
}

uint8_t spi_transfer_byte(uint8_t data)
{
    uint32_t timeout = TIMEOUT;

    while (!(SPI2->SR & (1 << 1)))
    {

        if (--timeout == 0UL)
        {
            TXE_Error();
        }
    }
    SPI2->DR = data;

    timeout = TIMEOUT;

    while (!(SPI2->SR & (1 << 0)))
    {

        if (--timeout == 0UL)
        {
            RXNE_Error();
        }
    }
    return (uint8_t)SPI2->DR;
}

void spi_write(const uint8_t *data, uint16_t len)
{
    spi_cs_select();
    for (uint16_t i = 0; i < len; i++)
    {
        spi_transfer_byte(data[i]);
    }
    spi_cs_deselect();
}

void spi_read(uint8_t *data, uint16_t len)
{
    spi_cs_select();
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = spi_transfer_byte(SPI_DUMMY_BYTE);
    }
    spi_cs_deselect();
}

static void dma_stream_disable(DMA_Stream_TypeDef *stream)
{
    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN)
    {
        // wait for hardware to confirm disable
    }
}

void spi_dma_init(void)
{
    RCC_AHB1ENR |= (1 << 21);
    volatile uint32_t dummy;
    dummy = RCC_APB1ENR;
    (void)dummy;

    NVIC_ISER0 |= (1 << 14); // DMA1_Stream3_IRQn
    NVIC_ISER0 |= (1 << 15); // DMA1_Stream4_IRQn
}

static volatile bool spi_tx_dma_done = true;
static volatile bool spi_rx_dma_done = true;
static uint8_t spi_rx_trash;

void spi_transfer_dma(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
    spi_cs_select();
    dma_stream_disable(DMA1_Stream3); // RX
    dma_stream_disable(DMA1_Stream4); // TX

    DMA1->LIFCR = (0x3D << 22);       // clear Stream3 flags
    DMA1->HIFCR = (0x3D << 0);        // clear Stream4 flags

    // RX stream: peripheral -> memory, real rx_buf, increment
    DMA1_Stream3->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream3->M0AR = (uint32_t)rx_buf;
    DMA1_Stream3->NDTR = len;
    DMA1_Stream3->CR   = (0 << 25) | DMA_SxCR_DIR_P2M | DMA_SxCR_MINC | DMA_SxCR_TCIE;

    // TX stream: memory -> peripheral, real tx_buf, increment
    DMA1_Stream4->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream4->M0AR = (uint32_t)tx_buf;
    DMA1_Stream4->NDTR = len;
    DMA1_Stream4->CR   = (0 << 25) | DMA_SxCR_DIR_M2P | DMA_SxCR_MINC | DMA_SxCR_TCIE;

    SPI2->CR2 |= (1 << 0); // RXDMAEN
    SPI2->CR2 |= (1 << 1); // TXDMAEN

    spi_rx_dma_done = false;
    spi_tx_dma_done = false;

    DMA1_Stream3->CR |= DMA_SxCR_EN; // arm RX first
    DMA1_Stream4->CR |= DMA_SxCR_EN; // then start TX — clocking begins here
}

void spi_write_dma(const uint8_t *data, uint16_t len)
{
    spi_cs_select();
    dma_stream_disable(DMA1_Stream3);
    dma_stream_disable(DMA1_Stream4);

    DMA1->HIFCR = 0x3D;
    DMA1->LIFCR = (0x3D << 22);

    DMA1_Stream3->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream3->M0AR = (uint32_t)&spi_rx_trash;
    DMA1_Stream3->NDTR = len;
    DMA1_Stream3->CR   = (0 << 25) | DMA_SxCR_DIR_P2M;

    DMA1_Stream4->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream4->M0AR = (uint32_t)data;
    DMA1_Stream4->NDTR = len;
    DMA1_Stream4->FCR &= ~DMA_SxFCR_DMDIS; // ensure direct mode (bypass FIFO)

    DMA1_Stream4->CR = (0 << 25)           // channel 0
                       | DMA_SxCR_DIR_M2P  // memory-to-peripheral
                       | DMA_SxCR_MINC     // memory address increments
                       | DMA_SxCR_TCIE;    // transfer-complete interrupt

    SPI2->CR2 |= (1 << 1);                 // TXDMAEN
    SPI2->CR2 |= (1 << 0);                 // RXDMAEN

    spi_tx_dma_done = false;
    DMA1_Stream3->CR |= DMA_SxCR_EN;
    DMA1_Stream4->CR |= DMA_SxCR_EN; // start
}

static uint8_t spi_dummy_byte = 0xFF;

void spi_read_dma(uint8_t *data, uint16_t len)
{
    spi_cs_select();
    dma_stream_disable(DMA1_Stream3); // RX
    dma_stream_disable(DMA1_Stream4); // TX (dummy)

    DMA1->LIFCR = (0x3D << 22);       // 0b00111101 clears bits 22, 24, 25, 26, 27
    DMA1->HIFCR = 0x3D;               // 0b00111101 clears bits 0,2,3,4,5 (Stream 4's flags)

    DMA1_Stream3->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream3->M0AR = (uint32_t)data;
    DMA1_Stream3->NDTR = len;
    DMA1_Stream3->CR   = (0 << 25) | DMA_SxCR_DIR_P2M | DMA_SxCR_MINC | DMA_SxCR_TCIE;

    DMA1_Stream4->PAR  = (uint32_t)&SPI2->DR;
    DMA1_Stream4->M0AR = (uint32_t)&spi_dummy_byte;
    DMA1_Stream4->NDTR = len;
    DMA1_Stream4->FCR &= ~DMA_SxFCR_DMDIS; // ensure direct mode (bypass FIFO)
    DMA1_Stream3->FCR &= ~DMA_SxFCR_DMDIS; // ensure direct mode (bypass FIFO)
    DMA1_Stream4->CR = (0 << 25) | DMA_SxCR_DIR_M2P;

    SPI2->CR2 |= (1 << 0); // RXDMAEN
    SPI2->CR2 |= (1 << 1); // TXDMAEN

    spi_rx_dma_done = false;
    DMA1_Stream4->CR |= DMA_SxCR_EN; // start TX first
    DMA1_Stream3->CR |= DMA_SxCR_EN; // then RX
}


void DMA1_Stream4_IRQHandler(void)
{                                   // TX complete (used standalone by spi_write_dma)
    if (DMA1->HISR & (1 << 5))
    {                               // TCIF4
        DMA1->HIFCR     = (1 << 5); // clear TCIF4
        spi_tx_dma_done = true;
    }
}

void DMA1_Stream3_IRQHandler(void)
{                                    // RX complete (used by spi_read_dma)
    if (DMA1->LISR & (1 << 27))
    {                                // TCIF3
        DMA1->LIFCR     = (1 << 27); // clear TCIF3
        spi_rx_dma_done = true;
    }
}

void SR_Error()
{
    while (1)
    {
    }
}

void DMA_Error()
{
    while (1)
    {
    }
}

void spi_dma_wait(void)
{
    uint32_t timeout = TIMEOUT;

    while (!spi_tx_dma_done || !spi_rx_dma_done)
    {
        if (--timeout == 0UL)
        {
            spi_cs_deselect();
            DMA_Error();
        }
    }

    timeout = TIMEOUT;

    while (SPI2->SR & (1 << 7))
    {
        if (--timeout == 0UL)
        {
            SR_Error();
        }
    }
    spi_cs_deselect();
}
