#include <stdint.h>
#include <stdbool.h>
#include "project.h"
#include "../protocol.h"

#define BUFFER_COUNT 2

static uint8_t td[BUFFER_COUNT];
static uint8_t buffer[BUFFER_COUNT][BUFFER_SIZE] __attribute__((aligned()));
static uint8_t dma_channel;

#if 0
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
#endif

#if 0
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
        static frame_t sendframe;
        sendframe.id = FLUXENGINE_ID;
        sendframe.type = FRAME_OK;
        memcpy(sendframe.u.buffer, buffer[last_td], BUFFER_SIZE);
        USBFS_PutData((const uint8_t*) &sendframe, sizeof(sendframe));
    }
}
#endif

static void handle_usb_packet(void)
{
    static uint8_t buffer[64];
    int length = USBFS_GetEPCount(FLUXENGINE_CMD_OUT_EP_NUM);
    USBFS_ReadOutEP(FLUXENGINE_CMD_OUT_EP_NUM, buffer, length);
    UART_PutString("read packet\r");
    
    while (USBFS_GetEPState(FLUXENGINE_CMD_IN_EP_NUM) != USBFS_IN_BUFFER_EMPTY)
        ;
    
    char c = 99;
    USBFS_LoadInEP(FLUXENGINE_CMD_IN_EP_NUM, (uint8_t*) &c, sizeof(c));
}

int main(void)
{
    CyGlobalIntEnable;
    UART_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    //USBFS_CDC_Init();
    //DMA_FINISHED_IRQ_StartEx(&dma_finished_isr);
    //init_dma();
    
    UART_PutString("GO\r");
    LED_REG_Write(0);

    for (;;)
    {
        if (!USBFS_GetConfiguration() || USBFS_IsConfigurationChanged())
        {
            //CyDmaChDisable(dma_channel);
            UART_PutString("Waiting for USB...\r");
            while (!USBFS_GetConfiguration())
                ;
            UART_PutString("USB ready\r");
            //CyDmaChEnable(dma_channel, true);
            USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP_NUM);
        }
        
        if (USBFS_GetEPState(FLUXENGINE_CMD_OUT_EP_NUM) == USBFS_OUT_BUFFER_FULL)
            handle_usb_packet();
    }
}
