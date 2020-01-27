#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "project.h"
#include "../protocol.h"
#include "../lib/common/crunch.h"

#define MOTOR_ON_TIME 5000 /* milliseconds */
#define STEP_INTERVAL_TIME 6 /* ms */
#define STEP_SETTLING_TIME 50 /* ms */

#define DISKSTATUS_WPT    1
#define DISKSTATUS_DSKCHG 2

#define STEP_TOWARDS0 1
#define STEP_AWAYFROM0 0

static volatile uint32_t clock = 0; /* ms */
static volatile bool index_irq = false;

static bool motor_on = false;
static uint32_t motor_on_time = 0;
static bool homed = false;
static int current_track = 0;
static struct set_drive_frame current_drive_flags;

#define BUFFER_COUNT 16
#define BUFFER_SIZE 64
static uint8_t td[BUFFER_COUNT];
static uint8_t dma_buffer[BUFFER_COUNT][BUFFER_SIZE] __attribute__((aligned()));
static uint8_t usb_buffer[BUFFER_SIZE] __attribute__((aligned()));
static uint8_t dma_channel;
#define NEXT_BUFFER(b) (((b)+1) % BUFFER_COUNT)

static volatile int dma_writing_to_td = 0;
static volatile int dma_reading_from_td = 0;
static volatile bool dma_underrun = false;

#define DECLARE_REPLY_FRAME(STRUCT, TYPE) \
    STRUCT r = {.f = { .type = TYPE, .size = sizeof(STRUCT) }}

static void system_timer_cb(void)
{
    CyGlobalIntDisable;
    clock++;
    
    static int counter300rpm = 0;
    counter300rpm++;
    if (counter300rpm == 200)
        counter300rpm = 0;
    
    static int counter360rpm = 0;
    counter360rpm++;
    if (counter360rpm == 167)
        counter360rpm = 0;
    
    FAKE_INDEX_GENERATOR_REG_Write(
        ((counter300rpm == 0) ? 1 : 0)
        | ((counter360rpm == 0) ? 2 : 0));
    
    CyGlobalIntEnable;
}

CY_ISR(index_irq_cb)
{
    index_irq = true;
    
    /* Stop writing the instant the index pulse comes along; it may take a few
     * moments for the main code to notice the pulse, and we don't want to overwrite
     * the beginning of the track. */
    ERASE_REG_Write(0);
}

CY_ISR(capture_dma_finished_irq_cb)
{
    dma_writing_to_td = NEXT_BUFFER(dma_writing_to_td);
    if (dma_writing_to_td == dma_reading_from_td)
        dma_underrun = true;
}

CY_ISR(replay_dma_finished_irq_cb)
{
    dma_reading_from_td = NEXT_BUFFER(dma_reading_from_td);
    if (dma_reading_from_td == dma_writing_to_td)
        dma_underrun = true;
}

static void print(const char* msg, ...)
{
    char buffer[64];
    
    va_list ap;
    va_start(ap, msg);
    vsnprintf(buffer, sizeof(buffer), msg, ap);
    va_end(ap);
    
    UART_PutString(buffer);
    UART_PutCRLF();
}

static void set_drive_flags(struct set_drive_frame* flags)
{
    if (current_drive_flags.drive != flags->drive)
        homed = false;
    
    current_drive_flags = *flags;
    DRIVESELECT_REG_Write(flags->drive ? 2 : 1); /* select drive 1 or 0 */
    DENSITY_REG_Write(flags->high_density); /* density bit */
    INDEX_REG_Write(flags->index_mode);
}

static void start_motor(void)
{
    if (!motor_on)
    {
        set_drive_flags(&current_drive_flags);
        MOTOR_REG_Write(1);
        CyDelay(1000);
        homed = false;
    }

    motor_on_time = clock;
    motor_on = true;
    CyWdtClear();
}

static void stop_motor(void)
{
    if (motor_on)
    {
        MOTOR_REG_Write(0);
        DRIVESELECT_REG_Write(0); /* deselect all drives */
        motor_on = false;
    }
}

static void wait_until_writeable(int ep)
{
    while (USBFS_GetEPState(ep) != USBFS_IN_BUFFER_EMPTY)
        ;
}

static void send_reply(struct any_frame* f)
{
    print("reply 0x%02x", f->f.type);
    wait_until_writeable(FLUXENGINE_CMD_IN_EP_NUM);
    USBFS_LoadInEP(FLUXENGINE_CMD_IN_EP_NUM, (uint8_t*) f, f->f.size);
}

static void send_error(int code)
{
    DECLARE_REPLY_FRAME(struct error_frame, F_FRAME_ERROR);
    r.error = code;
    send_reply((struct any_frame*) &r);
}

/* buffer must be big enough for a frame */
static int usb_read(int ep, uint8_t buffer[FRAME_SIZE])
{
    int length = USBFS_GetEPCount(ep);
    USBFS_ReadOutEP(ep, buffer, length);
    while (USBFS_GetEPState(ep) == USBFS_OUT_BUFFER_FULL)
        ;
    return length;
}

static void cmd_get_version(struct any_frame* f)
{
    DECLARE_REPLY_FRAME(struct version_frame, F_FRAME_GET_VERSION_REPLY);
    r.version = FLUXENGINE_VERSION;
    send_reply((struct any_frame*) &r);
}

static void step(int dir)
{
    STEP_REG_Write(dir); /* step high */
    CyDelayUs(6);
    STEP_REG_Write(dir | 2); /* step low */
    CyDelayUs(6);
    STEP_REG_Write(dir); /* step high again, drive moves now */
    CyDelay(STEP_INTERVAL_TIME);
}

static void home(void)
{
    for (int i=0; i<100; i++)
    {
        /* Don't keep stepping forever, because if a drive's
         * not connected bad things happen. */
        if (TRACK0_REG_Read())
            break;
        step(STEP_TOWARDS0);
    }
    
    /* Step to -1, which should be a nop, to reset the disk on disk change. */
    step(STEP_TOWARDS0);
}

static void seek_to(int track)
{
    start_motor();
    if (!homed || (track == 0))
    {
        print("homing");
        home();
        
        homed = true;
        current_track = 0;
        CyDelayUs(1); /* for direction change */
    }
    
    print("beginning seek from %d to %d", current_track, track);
    while (track != current_track)
    {
        if (TRACK0_REG_Read())
            current_track = 0;
        
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
        CyWdtClear();
    }
    CyDelay(STEP_SETTLING_TIME);
    print("finished seek");
}

static void cmd_seek(struct seek_frame* f)
{
    seek_to(f->track);
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_SEEK_REPLY);
    send_reply(&r);    
}

static void cmd_recalibrate(void)
{
    homed = false;
    seek_to(0);
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_RECALIBRATE_REPLY);
    send_reply(&r);    
}
    
static void cmd_measure_speed(struct any_frame* f)
{
    start_motor();
    
    index_irq = false;
    int start_clock = clock;
    int elapsed = 0;
    while (!index_irq)
    {
        elapsed = clock - start_clock;
        if (elapsed > 1000)
        {
            elapsed = 0;
            break;
        }
    }

    if (elapsed != 0)
    {
        index_irq = false;
        start_clock = clock;
        while (!index_irq)
            elapsed = clock - start_clock;
    }
    
    DECLARE_REPLY_FRAME(struct speed_frame, F_FRAME_MEASURE_SPEED_REPLY);
    r.period_ms = elapsed;
    send_reply((struct any_frame*) &r);    
}

static void cmd_bulk_test(struct any_frame* f)
{
    uint8_t buffer[64];
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    for (int x=0; x<64; x++)
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

static void deinit_dma(void)
{
    for (int i=0; i<BUFFER_COUNT; i++)
        CyDmaTdFree(td[i]);
}

static void init_capture_dma(void)
{
    dma_channel = SAMPLER_DMA_DmaInitialize(
        2 /* bytes */,
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
            CY_DMA_TD_INC_DST_ADR | SAMPLER_DMA__TD_TERMOUT_EN);
        CyDmaTdSetAddress(td[i], LO16((uint32)SAMPLER_FIFO_FIFO_PTR), LO16((uint32)&dma_buffer[i]));
    }    
}

static void cmd_read(struct read_frame* f)
{
    SIDE_REG_Write(f->side);
    seek_to(current_track);
    
    /* Do slow setup *before* we go into the real-time bit. */
    
    {
        uint8_t i = CyEnterCriticalSection();
        SAMPLER_FIFO_SET_LEVEL_MID;
        SAMPLER_FIFO_CLEAR;
        SAMPLER_FIFO_SINGLE_BUFFER_UNSET;
        CyExitCriticalSection(i);
    }
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    init_capture_dma();

    /* Wait for the beginning of a rotation, if requested. */
        
    if (f->synced)
    {
        index_irq = false;
        while (!index_irq)
            ;
        index_irq = false;
    }
    
    crunch_state_t cs = {};
    cs.outputptr = usb_buffer;
    cs.outputlen = BUFFER_SIZE;
    
    dma_writing_to_td = 0;
    dma_reading_from_td = -1;
    dma_underrun = false;
    int count = 0;
    CyDmaChSetInitialTd(dma_channel, td[dma_writing_to_td]);
    CyDmaClearPendingDrq(dma_channel);
    CyDmaChEnable(dma_channel, 1);

    /* Wait for the first DMA transfer to complete, after which we can start the
     * USB transfer. */

    while (dma_writing_to_td == 0)
        ;
    dma_reading_from_td = 0;
    
    /* Start transferring. */

    uint32_t start_time = clock;
    while (!dma_underrun)
    {
        CyWdtClear();

        /* Wait for the next block to be read. */
        while (dma_reading_from_td == dma_writing_to_td)
        {
            /* On an underrun, give up immediately. */
            if (dma_underrun)
                goto abort;
            
            /* Also finish if the sample session is over. */
            if ((clock - start_time) >= f->milliseconds)
                goto abort;
        }

        uint8_t dma_buffer_usage = 0;
        while (dma_buffer_usage < BUFFER_SIZE)
        {
            cs.inputptr = dma_buffer[dma_reading_from_td] + dma_buffer_usage;
            cs.inputlen = BUFFER_SIZE - dma_buffer_usage;
            crunch(&cs);
            dma_buffer_usage += BUFFER_SIZE - cs.inputlen;
            count++;
            
            /* If there is no available space in the output buffer, flush the buffer via
             * USB and go again. */
            if (cs.outputlen == 0)
            {
                wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
                USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, usb_buffer, BUFFER_SIZE);
                
                cs.outputptr = usb_buffer;
                cs.outputlen = BUFFER_SIZE;
            }
        }
        dma_reading_from_td = NEXT_BUFFER(dma_reading_from_td);
    }
abort:;
    bool saved_dma_underrun = dma_underrun;
    CyDmaChSetRequest(dma_channel, CY_DMA_CPU_TERM_CHAIN);
    while (CyDmaChGetRequest(dma_channel))
        ;

    donecrunch(&cs);
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    /* If there's a complete packet waiting, send it. */
    if (cs.outputlen != BUFFER_SIZE)
    {
        USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, usb_buffer, BUFFER_SIZE);
        wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    }
    if ((cs.outputlen != 0) && (cs.outputlen != BUFFER_SIZE))
    {
        /* If there's a partial packet waiting, send it; this will also terminate the transfer. */
        USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, usb_buffer, BUFFER_SIZE-cs.outputlen);
    }
    else
    {
        /* Otherwise just terminate the transfer. */
        USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, NULL, 0);
    }
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    deinit_dma();

    if (saved_dma_underrun)
    {
        print("underrun after %d packets");
        send_error(F_ERROR_UNDERRUN);
    }
    else
    {
        DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_READ_REPLY);
        send_reply(&r);
    }
    print("count=%d i=%d d=%d", count, index_irq, dma_underrun);
}

static void init_replay_dma(void)
{
    dma_channel = SEQUENCER_DMA_DmaInitialize(
        1 /* bytes */,
        true /* request per burst */, 
        HI16(CYDEV_SRAM_BASE),
        HI16(CYDEV_PERIPH_BASE));
    
    for (int i=0; i<BUFFER_COUNT; i++)
        td[i] = CyDmaTdAllocate(); 
    for (int i=0; i<BUFFER_COUNT; i++)
    {
        int nexti = i+1;
        if (nexti == BUFFER_COUNT)
            nexti = 0;

        CyDmaTdSetConfiguration(td[i], BUFFER_SIZE, td[nexti],
            CY_DMA_TD_INC_SRC_ADR | SEQUENCER_DMA__TD_TERMOUT_EN);
        CyDmaTdSetAddress(td[i], LO16((uint32)&dma_buffer[i]), LO16((uint32)REPLAY_FIFO_FIFO_PTR));
    }    
}

static void cmd_write(struct write_frame* f)
{
    print("cmd_write");
    
    if (f->bytes_to_write % FRAME_SIZE)
    {
        send_error(F_ERROR_INVALID_VALUE);
        return;
    }
    
    SEQUENCER_CONTROL_Write(1); /* put the sequencer into reset */

    SIDE_REG_Write(f->side);
    {
        uint8_t i = CyEnterCriticalSection();        
        REPLAY_FIFO_SET_LEVEL_NORMAL;
        REPLAY_FIFO_CLEAR;
        REPLAY_FIFO_SINGLE_BUFFER_UNSET;
        CyExitCriticalSection(i);
    }
    seek_to(current_track);    

    init_replay_dma();
    bool writing = false; /* to the disk */
    bool finished = false;
    int packets = f->bytes_to_write / FRAME_SIZE;
    int count_written = 0;
    int count_read = 0;
    dma_writing_to_td = 0;
    dma_reading_from_td = -1;
    dma_underrun = false;

    crunch_state_t cs = {};
    cs.outputlen = BUFFER_SIZE;
    USBFS_EnableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);
    
    int old_reading_from_td = -1;
    for (;;)
    {
        CyWdtClear();

        /* Read data from USB into the buffers. */
        
        if (NEXT_BUFFER(dma_writing_to_td) != dma_reading_from_td)
        {
            if (writing && (dma_underrun || index_irq))
                goto abort;
            
            /* Read crunched data, if necessary. */
            
            if (cs.inputlen == 0)
            {
                if (finished)
                {
                    /* There's no more data to read, so fake some. */
                    
                    for (int i=0; i<BUFFER_SIZE; i++)
                        usb_buffer[i+0] = 0x7f;
                    cs.inputptr = usb_buffer;
                    cs.inputlen = BUFFER_SIZE;
                }
                else
                {
                    while (USBFS_GetEPState(FLUXENGINE_DATA_OUT_EP_NUM) != USBFS_OUT_BUFFER_FULL)
                    {
                        if (writing && (dma_underrun || index_irq))
                            goto abort;
                    }

                    int length = usb_read(FLUXENGINE_DATA_OUT_EP_NUM, usb_buffer);
                    cs.inputptr = usb_buffer;
                    cs.inputlen = length;
                    USBFS_EnableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);

                    count_read++;
                    if ((length < FRAME_SIZE) || (count_read == packets))
                        finished = true;
                }
            }
            
            /* If there *is* data waiting in the buffer, uncrunch it. */
            
            if (cs.inputlen != 0)
            {
                cs.outputptr = dma_buffer[dma_writing_to_td] + BUFFER_SIZE - cs.outputlen;
                uncrunch(&cs);
                if (cs.outputlen == 0)
                {
                    /* Completed a DMA buffer; queue it for writing. */
                    
                    dma_writing_to_td = NEXT_BUFFER(dma_writing_to_td);
                    cs.outputlen = BUFFER_SIZE;
                }
            }
            
            /* If we have a full buffer, start writing. */
            if ((dma_reading_from_td == -1) && (dma_writing_to_td == BUFFER_COUNT-1))
            {
                dma_reading_from_td = old_reading_from_td = 0;
                
                /* Start the DMA engine. */
                
                SEQUENCER_DMA_FINISHED_IRQ_Enable();
                dma_underrun = false;
                CyDmaChSetInitialTd(dma_channel, td[dma_reading_from_td]);
                CyDmaClearPendingDrq(dma_channel);
                CyDmaChEnable(dma_channel, 1);

                /* Wait for the index marker. While this happens, the DMA engine
                 * will prime the FIFO. */
                
                index_irq = false;
                while (!index_irq)
                    ;
                index_irq = false;

                writing = true;
                ERASE_REG_Write(1); /* start erasing! */
                SEQUENCER_CONTROL_Write(0); /* start writing! */
            }
        }
        
        if (writing && (dma_underrun || index_irq))
            goto abort;

        if (dma_reading_from_td != old_reading_from_td)
        {
            count_written++;
            old_reading_from_td = dma_reading_from_td;
        }
    }
abort:
    SEQUENCER_DMA_FINISHED_IRQ_Disable();

    SEQUENCER_CONTROL_Write(1); /* reset */
    if (writing)
    {
        ERASE_REG_Write(0);
        CyDmaChSetRequest(dma_channel, CY_DMA_CPU_TERM_CHAIN);
        while (CyDmaChGetRequest(dma_channel))
            ;
        CyDmaChDisable(dma_channel);
    }
    
    print("p=%d cr=%d cw=%d f=%d w=%d index=%d underrun=%d", packets, count_read, count_written, finished, writing, index_irq, dma_underrun);
    if (!finished)
    {
        while (count_read < packets)
        {
            if (USBFS_GetEPState(FLUXENGINE_DATA_OUT_EP_NUM) == USBFS_OUT_BUFFER_FULL)
            {
                int length = usb_read(FLUXENGINE_DATA_OUT_EP_NUM, usb_buffer);
                if (length < FRAME_SIZE)
                    break;
                USBFS_EnableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);
                count_read++;
            }
        }
        USBFS_DisableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);
    }
    
    deinit_dma();
    print("write finished");
    
    if (dma_underrun)
    {
        send_error(F_ERROR_UNDERRUN);
        return;
    }

    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_WRITE_REPLY);
    send_reply((struct any_frame*) &r);
}

static void cmd_erase(struct erase_frame* f)
{
    SIDE_REG_Write(f->side);
    seek_to(current_track);    
    /* Disk is now spinning. */
    
    print("start erasing");
    index_irq = false;
    while (!index_irq)
        ;
    ERASE_REG_Write(1);
    index_irq = false;
    while (!index_irq)
        ;
    ERASE_REG_Write(0);
    print("stop erasing");

    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_ERASE_REPLY);
    send_reply((struct any_frame*) &r);
}

static void cmd_set_drive(struct set_drive_frame* f)
{
    set_drive_flags(f);
    
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_SET_DRIVE_REPLY);
    send_reply((struct any_frame*) &r);
}

static uint16_t read_output_voltage_mv(void)
{
    OUTPUT_VOLTAGE_ADC_StartConvert();
    OUTPUT_VOLTAGE_ADC_IsEndConversion(OUTPUT_VOLTAGE_ADC_WAIT_FOR_RESULT);
    uint16_t samples = OUTPUT_VOLTAGE_ADC_GetResult16();
    return OUTPUT_VOLTAGE_ADC_CountsTo_mVolts(samples);
}

static void read_output_voltages(struct voltages* v)
{
    SIDE_REG_Write(1); /* set DIR to low (remember this is inverted) */
    CyDelay(100);
    v->logic0_mv = read_output_voltage_mv();

    SIDE_REG_Write(0);
    CyDelay(100);
    v->logic1_mv = read_output_voltage_mv();
}

static uint16_t read_input_voltage_mv(void)
{
    INPUT_VOLTAGE_ADC_StartConvert();
    INPUT_VOLTAGE_ADC_IsEndConversion(INPUT_VOLTAGE_ADC_WAIT_FOR_RESULT);
    uint16_t samples = INPUT_VOLTAGE_ADC_GetResult16();
    return INPUT_VOLTAGE_ADC_CountsTo_mVolts(samples);
}

static void read_input_voltages(struct voltages* v)
{
    home();
    CyDelay(50);
    v->logic0_mv = read_input_voltage_mv();
    
    step(STEP_AWAYFROM0);
    CyDelay(50);
    v->logic1_mv = read_input_voltage_mv();
}

static void cmd_measure_voltages(void)
{
    stop_motor();
    INPUT_VOLTAGE_ADC_Start();
    INPUT_VOLTAGE_ADC_SetPower(INPUT_VOLTAGE_ADC__HIGHPOWER);
    OUTPUT_VOLTAGE_ADC_Start();
    OUTPUT_VOLTAGE_ADC_SetPower(OUTPUT_VOLTAGE_ADC__HIGHPOWER);
    
    DECLARE_REPLY_FRAME(struct voltages_frame, F_FRAME_MEASURE_VOLTAGES_REPLY);
    
    CyWdtClear();
    MOTOR_REG_Write(0); /* should be ignored anyway */
    DRIVESELECT_REG_Write(0); /* deselect both drives */
    CyDelay(200); /* wait for things to settle */
    read_output_voltages(&r.output_both_off);
    read_input_voltages(&r.input_both_off);

    CyWdtClear();
    DRIVESELECT_REG_Write(1); /* select drive 0 */
    CyDelay(50);
    read_output_voltages(&r.output_drive_0_selected);
    read_input_voltages(&r.input_drive_0_selected);
    MOTOR_REG_Write(1);
    CyDelay(300);
    CyWdtClear();
    read_output_voltages(&r.output_drive_0_running);
    read_input_voltages(&r.input_drive_0_running);
    MOTOR_REG_Write(0);
    CyDelay(300);
    
    CyWdtClear();
    DRIVESELECT_REG_Write(2); /* select drive 1 */
    CyDelay(50);
    read_output_voltages(&r.output_drive_1_selected);
    read_input_voltages(&r.input_drive_1_selected);
    MOTOR_REG_Write(1);
    CyDelay(300);
    CyWdtClear();
    read_output_voltages(&r.output_drive_1_running);
    read_input_voltages(&r.input_drive_1_running);
    MOTOR_REG_Write(0);
    CyDelay(300);

    CyWdtClear();
    DRIVESELECT_REG_Write(0);
    homed = false;
    INPUT_VOLTAGE_ADC_Stop();
    OUTPUT_VOLTAGE_ADC_Stop();
    send_reply((struct any_frame*) &r);
}

static void handle_command(void)
{
    static uint8_t input_buffer[FRAME_SIZE];
    (void) usb_read(FLUXENGINE_CMD_OUT_EP_NUM, input_buffer);

    struct any_frame* f = (struct any_frame*) input_buffer;
    print("command 0x%02x", f->f.type);
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
        
        case F_FRAME_WRITE_CMD:
            cmd_write((struct write_frame*) f);
            break;
            
        case F_FRAME_ERASE_CMD:
            cmd_erase((struct erase_frame*) f);
            break;
        
        case F_FRAME_RECALIBRATE_CMD:
            cmd_recalibrate();
            break;
            
        case F_FRAME_SET_DRIVE_CMD:
            cmd_set_drive((struct set_drive_frame*) f);
            break;
        
        case F_FRAME_MEASURE_VOLTAGES_CMD:
            cmd_measure_voltages();
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
    SAMPLER_DMA_FINISHED_IRQ_StartEx(&capture_dma_finished_irq_cb);
    SEQUENCER_DMA_FINISHED_IRQ_StartEx(&replay_dma_finished_irq_cb);
    INPUT_VOLTAGE_ADC_Stop();
    OUTPUT_VOLTAGE_ADC_Stop();
    DRIVESELECT_REG_Write(0);
    UART_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    
    CyWdtStart(CYWDT_1024_TICKS, CYWDT_LPMODE_DISABLED);
    
    /* UART_PutString("GO\r"); */

    for (;;)
    {
        CyWdtClear();
        
        if (motor_on)
        {
            uint32_t time_on = clock - motor_on_time;
            if (time_on > MOTOR_ON_TIME)
                stop_motor();
        }
        
        if (!USBFS_GetConfiguration() || USBFS_IsConfigurationChanged())
        {
            print("Waiting for USB...");
            while (!USBFS_GetConfiguration())
                ;
            print("USB ready");
            USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP_NUM);
        }
        
        if (USBFS_GetEPState(FLUXENGINE_CMD_OUT_EP_NUM) == USBFS_OUT_BUFFER_FULL)
        {
            set_drive_flags(&current_drive_flags);
            handle_command();
            USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP_NUM);
            print("idle");
        }
    }
}
