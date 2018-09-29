#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>
#include "project.h"
#include "../protocol.h"

#define MOTOR_ON_TIME 5000 /* milliseconds */
#define STEP_INTERVAL_TIME 3 /* ms */
#define STEP_SETTLING_TIME 18 /* ms */

#define BUFFER_COUNT 2

#define STEP_TOWARDS0 1
#define STEP_AWAYFROM0 0

static uint32_t clock = 0;
static bool motor_on = false;
static uint32_t motor_on_time = 0;
static bool homed = false;
static int current_track = 0;

#if 0
static uint8_t td[BUFFER_COUNT];
static uint8_t buffer[BUFFER_COUNT][BUFFER_SIZE] __attribute__((aligned()));
static uint8_t dma_channel;
#endif

#define DECLARE_REPLY_FRAME(STRUCT, TYPE) \
    STRUCT r = {.f = { .type = TYPE, .size = sizeof(STRUCT) }}

static void system_timer_cb(void)
{
    CyGlobalIntDisable;
    clock++;
    CyGlobalIntEnable;
}

static void start_motor(void)
{
    if (!motor_on)
    {
        MOTOR_REG_Write(1);
        CyDelay(1000);
    }
    
    motor_on_time = clock;
    motor_on = true;
}

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

static void send_reply(struct any_frame* f)
{
    while (USBFS_GetEPState(FLUXENGINE_CMD_IN_EP_NUM) != USBFS_IN_BUFFER_EMPTY)
        ;
    USBFS_LoadInEP(FLUXENGINE_CMD_IN_EP_NUM, (uint8_t*) f, f->f.size);
}

static void send_error(int code)
{
    DECLARE_REPLY_FRAME(struct error_frame, F_FRAME_ERROR);
    r.error = code;
    send_reply((struct any_frame*) &r);
}

static void cmd_get_version(struct any_frame* f)
{
    DECLARE_REPLY_FRAME(struct version_frame, F_FRAME_GET_VERSION_REPLY);
    r.version = FLUXENGINE_VERSION;
    send_reply((struct any_frame*) &r);
}

static void step(int dir)
{
    STEP_REG_Write(dir | 2);
    CyDelay(1);
    STEP_REG_Write(dir);
    CyDelay(STEP_INTERVAL_TIME);
}

static void cmd_seek(struct seek_frame* f)
{
    start_motor();
    if (!homed)
    {
        while (!TRACK0_REG_Read())
            step(STEP_TOWARDS0);
        
        homed = true;
        current_track = 0;
        CyDelay(1); /* for direction change */
    }
    
    while (f->track != current_track)
    {
        if (f->track > current_track)
        {
            step(STEP_AWAYFROM0);
            current_track++;
        }
        else if (f->track < current_track)
        {
            step(STEP_TOWARDS0);
            current_track--;
        }
    }
    CyDelay(STEP_SETTLING_TIME);
            
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_SEEK_REPLY);
    send_reply(&r);    
}

static void handle_command(void)
{
    static uint8_t input_buffer[FRAME_SIZE];
    int length = USBFS_GetEPCount(FLUXENGINE_CMD_OUT_EP_NUM);
    USBFS_ReadOutEP(FLUXENGINE_CMD_OUT_EP_NUM, input_buffer, length);
    
    struct any_frame* f = (struct any_frame*) input_buffer;
    switch (f->f.type)
    {
        case F_FRAME_GET_VERSION_CMD:
            cmd_get_version(f);
            break;
            
        case F_FRAME_SEEK_CMD:
            cmd_seek((struct seek_frame*) f);
            break;
            
        default:
            send_error(F_ERROR_BAD_COMMAND);
    }
}

int main(void)
{
    CyGlobalIntEnable;
    CySysTickStart();
    CySysTickSetCallback(4, system_timer_cb);
    UART_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    //USBFS_CDC_Init();
    //DMA_FINISHED_IRQ_StartEx(&dma_finished_isr);
    //init_dma();
    
    CyWdtStart(CYWDT_1024_TICKS, CYWDT_LPMODE_DISABLED);
    
    UART_PutString("GO\r");
    LED_REG_Write(0);

    for (;;)
    {
        CyWdtClear();
        
        if (motor_on)
        {
            uint32_t time_on = clock - motor_on_time;
            if (time_on > MOTOR_ON_TIME)
            {
                MOTOR_REG_Write(0);
                motor_on = false;
                homed = false; /* for debugging */
            }
        }
        
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
            handle_command();
    }
}
