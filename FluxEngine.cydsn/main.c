#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>
#include "project.h"
#include "../protocol.h"

#define MOTOR_ON_TIME 5000 /* milliseconds */
#define STEP_INTERVAL_TIME 3 /* ms */
#define STEP_SETTLING_TIME 20 /* ms */

#define STEP_TOWARDS0 1
#define STEP_AWAYFROM0 0

static volatile uint32_t clock = 0;
static volatile bool index_irq = false;
static volatile bool capture_dma_finished_irq = false;

static bool motor_on = false;
static uint32_t motor_on_time = 0;
static bool homed = false;
static int current_track = 0;

#define BUFFER_COUNT 16
#define BUFFER_SIZE 64
static uint8_t td[BUFFER_COUNT];
static uint8_t dma_buffer[BUFFER_COUNT][BUFFER_SIZE] __attribute__((aligned()));
static uint8_t dma_channel;

static volatile int dma_writing_to_td = 0;
static volatile int dma_reading_from_td = 0;
static volatile bool dma_underrun = false;

#define DECLARE_REPLY_FRAME(STRUCT, TYPE) \
    STRUCT r = {.f = { .type = TYPE, .size = sizeof(STRUCT) }}

static void system_timer_cb(void)
{
    CyGlobalIntDisable;
    clock++;
    CyGlobalIntEnable;
}

CY_ISR(index_irq_cb)
{
    index_irq = true;
}

CY_ISR(capture_dma_finished_irq_cb)
{
    dma_writing_to_td = (dma_writing_to_td+1) % BUFFER_COUNT;
    if (dma_writing_to_td == dma_reading_from_td)
        dma_underrun = true;
}

static void print(const char* s)
{
    UART_PutString(s);
}

static void printi(int i)
{
    char buffer[16];
    sprintf(buffer, "%d", i);
    print(buffer);
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
    CyWdtClear();
}

static void wait_until_writeable(int ep)
{
    while (USBFS_GetEPState(ep) != USBFS_IN_BUFFER_EMPTY)
        ;
}

static void send_reply(struct any_frame* f)
{
    wait_until_writeable(FLUXENGINE_CMD_IN_EP_NUM);
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

static void seek_to(int track)
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
    
    while (track != current_track)
    {
        if (track > current_track)
        {
            step(STEP_AWAYFROM0);
            current_track++;
        }
        else if (track < current_track)
        {
            step(STEP_TOWARDS0);
            current_track--;
        }
    }
    CyDelay(STEP_SETTLING_TIME);
}

static void cmd_seek(struct seek_frame* f)
{
    seek_to(f->track);
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_SEEK_REPLY);
    send_reply(&r);    
}

static void cmd_measure_speed(struct any_frame* f)
{
    start_motor();
    
    index_irq = false;
    while (!index_irq)
        ;
    index_irq = false;
    int start_clock = clock;
    while (!index_irq)
        ;
    int end_clock = clock;
    
    DECLARE_REPLY_FRAME(struct speed_frame, F_FRAME_MEASURE_SPEED_REPLY);
    r.period_ms = end_clock - start_clock;
    send_reply((struct any_frame*) &r);    
}

static void cmd_bulk_test(struct any_frame* f)
{
    uint8_t buffer[64];
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    for (int x=0; x<16; x++)
        for (int y=0; y<256; y++)
        {
            for (unsigned z=0; z<sizeof(buffer); z++)
                buffer[z] = x+y+z;
            
            wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
            USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, buffer, sizeof(buffer));
        }
    
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_BULK_TEST_REPLY);
    send_reply(&r);
}

static void init_dma(void)
{
    dma_channel = CAPTURE_DMA_DmaInitialize(
        1 /* bytes */,
        true /* request per burst */, 
        HI16(CYDEV_PERIPH_BASE),
        HI16(CYDEV_SRAM_BASE));
    
    for (int i=0; i<BUFFER_COUNT; i++)
        td[i] = CyDmaTdAllocate(); 
    for (int i=0; i<BUFFER_COUNT; i++)
    {
        int nexti = i+1;
        if (nexti == BUFFER_COUNT)
            nexti = 0;

        CyDmaTdSetConfiguration(td[i], BUFFER_SIZE, td[nexti],   
            CY_DMA_TD_INC_DST_ADR | CAPTURE_DMA__TD_TERMOUT_EN);
        CyDmaTdSetAddress(td[i], LO16((uint32)CAPTURE_REG_Status_PTR), LO16((uint32)&dma_buffer[i]));
    }    
}

static void cmd_read(struct read_frame* f)
{
    seek_to(current_track);    
    SIDE_REG_Write(f->side1);
    
    /* Do slow setup *before* we go into the real-time bit. */
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    print("starting transfer\r");

    /* Wait for the beginning of a rotation. */
        
    index_irq = false;
    while (!index_irq)
        ;
    index_irq = false;
    
    dma_writing_to_td = 0;
    dma_reading_from_td = -1;
    dma_underrun = false;
    int count = 0;
    CyDmaChSetInitialTd(dma_channel, td[0]);
    CyDmaClearPendingDrq(dma_channel);
    CyDmaChEnable(dma_channel, 1);

    /* Wait for the fir0st DMA transfer to complete, after which we can start the
     * USB transfer. */

    while ((dma_writing_to_td == 0) && !index_irq)
        ;
    dma_reading_from_td = 0;
    
    /* Start transferring. */

    while (!index_irq && !dma_underrun)
    {
        /* Wait for the next block to be read. */
        while (dma_reading_from_td == dma_writing_to_td)
        {
            /* On an underrun, give up immediately. */
            if (dma_underrun)
                goto abort;
        }

        while (USBFS_GetEPState(FLUXENGINE_DATA_IN_EP_NUM) != USBFS_IN_BUFFER_EMPTY)
        {
            if (index_irq || dma_underrun)
                goto abort;
        }

        USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, dma_buffer[dma_reading_from_td], BUFFER_SIZE);
        dma_reading_from_td = (dma_reading_from_td+1) % BUFFER_COUNT;
        count++;
    }
abort:
    CyDmaChSetRequest(dma_channel, CY_DMA_CPU_TERM_CHAIN);
    while (CyDmaChGetRequest(dma_channel))
        ;

    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, NULL, 0);
    print("bulk transfer terminated\r");

    if (dma_underrun)
    {
        print("underrun after ");
        printi(count);
        print(" packets\r");
        send_error(F_ERROR_UNDERRUN);
    }
    else
    {
        DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_READ_REPLY);
        send_reply(&r);
    }
}

static void handle_command(void)
{
    static uint8_t input_buffer[FRAME_SIZE];
    int length = USBFS_GetEPCount(FLUXENGINE_CMD_OUT_EP_NUM);
    USBFS_ReadOutEP(FLUXENGINE_CMD_OUT_EP_NUM, input_buffer, length);
    while (USBFS_GetEPState(FLUXENGINE_CMD_OUT_EP_NUM) == USBFS_OUT_BUFFER_FULL)
        ;
    USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP_NUM);

    struct any_frame* f = (struct any_frame*) input_buffer;
    switch (f->f.type)
    {
        case F_FRAME_GET_VERSION_CMD:
            cmd_get_version(f);
            break;
            
        case F_FRAME_SEEK_CMD:
            cmd_seek((struct seek_frame*) f);
            break;
        
        case F_FRAME_MEASURE_SPEED_CMD:
            cmd_measure_speed(f);
            break;
            
        case F_FRAME_BULK_TEST_CMD:
            cmd_bulk_test(f);
            break;
            
        case F_FRAME_READ_CMD:
            cmd_read((struct read_frame*) f);
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
    INDEX_IRQ_StartEx(&index_irq_cb);
    CAPTURE_DMA_FINISHED_IRQ_StartEx(&capture_dma_finished_irq_cb);
    UART_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    init_dma();
    
    CyWdtStart(CYWDT_1024_TICKS, CYWDT_LPMODE_DISABLED);
    
    UART_PutString("GO\r");

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
            USBFS_EnableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);
        }
        
        if (USBFS_GetEPState(FLUXENGINE_CMD_OUT_EP_NUM) == USBFS_OUT_BUFFER_FULL)
        {
            handle_command();
            USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP);
            UART_PutString("idle\r");
        }
    }
}
