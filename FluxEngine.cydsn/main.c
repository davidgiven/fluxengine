#include <stdint.h>
#include <stdbool.h>
#include "project.h"

#define BUFFER_SIZE 63
#define BUFFER_COUNT 2

static uint8_t td[BUFFER_COUNT];
static uint8_t buffer[BUFFER_COUNT][BUFFER_SIZE] __attribute__((aligned()));
static uint8_t dma_channel;

static void init_dma(void)
{
    /* Defines for DMA */
    #define DMA_BYTES_PER_BURST 1
    #define DMA_REQUEST_PER_BURST 1
    #define DMA_SRC_BASE (CYDEV_PERIPH_BASE)
    #define DMA_DST_BASE (CYDEV_SRAM_BASE)

    /* DMA Configuration for DMA */
    dma_channel = DMA_DmaInitialize(DMA_BYTES_PER_BURST, DMA_REQUEST_PER_BURST, 
        HI16(DMA_SRC_BASE), HI16(DMA_DST_BASE));
    for (int i=0; i<BUFFER_COUNT; i++)
        td[i] = CyDmaTdAllocate();
        
    for (int i=0; i<BUFFER_COUNT; i++)
    {
        int nexti = i+1;
        if (nexti == BUFFER_COUNT)
            nexti = 0;

        CyDmaTdSetConfiguration(td[i], BUFFER_SIZE, td[nexti],   
            CY_DMA_TD_INC_DST_ADR | CY_DMA_TD_AUTO_EXEC_NEXT | DMA__TD_TERMOUT_EN);
        CyDmaTdSetAddress(td[i], LO16((uint32)CounterReg_Status_PTR), LO16((uint32)&buffer[i]));
    }
    CyDmaChSetInitialTd(dma_channel, td[0]);
}

CY_ISR(dma_finished_isr)
{
    uint8_t which_td;
    uint8_t state;
    uint8_t last_td;
    
    CyDmaChStatus(dma_channel, &which_td, &state);
    last_td = (which_td == td[0]);

    if (!USBFS_CDCIsReady())
        LED_REG_Write(1);
    else
    {
        LED_REG_Write(0);
        static uint8_t sendbuffer[BUFFER_SIZE + 1];
        sendbuffer[0] = 0;
        memcpy(sendbuffer+1, buffer[last_td], BUFFER_SIZE);
        USBFS_PutData(sendbuffer, sizeof(sendbuffer));
    }
}

int main(void)
{
    CyGlobalIntEnable;
    UART_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    USBFS_CDC_Init();
    DMA_FINISHED_IRQ_StartEx(&dma_finished_isr);
    init_dma();
    
    UART_PutString("GO\r");
    
    for (;;)
    {
        if (!USBFS_GetConfiguration() || USBFS_IsConfigurationChanged())
        {
            CyDmaChDisable(dma_channel);
            UART_PutString("Waiting for USB...\r");
            while (!USBFS_GetConfiguration())
                ;
            UART_PutString("USB ready\r");
            CyDmaChEnable(dma_channel, true);
        }
    }
}
