#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "project.h"
#include "../protocol.h"

#define MOTOR_ON_TIME 5000 /* milliseconds */
#define STEP_INTERVAL_TIME 6 /* ms */
#define STEP_SETTLING_TIME 50 /* ms */

#define DISKSTATUS_WPT    1
#define DISKSTATUS_DSKCHG 2

#define STEP_TOWARDS0 0
#define STEP_AWAYFROM0 1

static bool drive0_present;
static bool drive1_present;

static volatile uint32_t clock = 0; /* ms */
static volatile bool index_irq = false;
/* Duration in ms. 0 causes every pulse to be an index pulse. Durations since
 * last pulse greater than this value imply sector pulse. Otherwise is an index
 * pulse. */
static volatile uint32_t hardsec_index_threshold = 0;

static bool motor_on = false;
static uint32_t motor_on_time = 0;
static bool homed = false;
static int current_track = 0;
static struct set_drive_frame current_drive_flags;

#define BUFFER_COUNT 64 /* the maximum */
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

static void stop_motor(void);

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
    /* Hard sectored media has sector pulses at the beginning of every sector
     * and the index pulse is an extra pulse in the middle of the last sector.
     * When the extra pulse is seen, the next sector pulse is also the start of
     * the track. */
    static bool hardsec_index_irq_primed = false;
    static uint32_t hardsec_last_pulse_time = 0;
    uint32_t index_pulse_duration = clock - hardsec_last_pulse_time;

    if (!hardsec_index_threshold)
    {
        index_irq = true;
        hardsec_index_irq_primed = false;
    }
    else
    {
        /* It's only an index pulse if the previous pulse is less than
         * the threshold.
         */
        index_irq = (index_pulse_duration <= hardsec_index_threshold) ?
            hardsec_index_irq_primed : false;

        if (index_irq)
            hardsec_index_irq_primed = false;
        else
            hardsec_index_irq_primed =
                index_pulse_duration <= hardsec_index_threshold;

        hardsec_last_pulse_time = clock;
    }
    
    /* Stop writing the instant the index pulse comes along; it may take a few
     * moments for the main code to notice the pulse, and we don't want to overwrite
     * the beginning of the track. */
    if (index_irq)
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
    {
        stop_motor();
        homed = false;
    }
    
    current_drive_flags = *flags;
    DRIVESELECT_REG_Write(flags->drive ? 2 : 1); /* select drive 1 or 0 */
    DENSITY_REG_Write(!flags->high_density); /* double density bit */
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

static void wait_until_readable(int ep)
{
    while (USBFS_GetEPState(ep) != USBFS_OUT_BUFFER_FULL)
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
    if (USBFS_GetEPState(ep) != USBFS_OUT_BUFFER_FULL)
    {
        USBFS_EnableOutEP(ep);
        wait_until_readable(ep);
    }

    int length = USBFS_GetEPCount(ep);
    USBFS_ReadOutEP(ep, buffer, length);
    while (USBFS_GetEPState(ep) != USBFS_OUT_BUFFER_EMPTY)
        ;
    return length;
}

static void cmd_get_version(struct any_frame* f)
{
    DECLARE_REPLY_FRAME(struct version_frame, F_FRAME_GET_VERSION_REPLY);
    r.version = FLUXENGINE_PROTOCOL_VERSION;
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

/* returns true if it looks like a drive is attached */
static bool home(void)
{
    for (int i=0; i<100; i++)
    {
        /* Don't keep stepping forever, because if a drive's
         * not connected bad things happen. */
        if (TRACK0_REG_Read())
            return true;
        step(STEP_TOWARDS0);
    }
    
    return false;
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
    TK43_REG_Write(track < 43); /* high if 0..42, low if 43 or up */
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
    
static void cmd_measure_speed(struct measurespeed_frame* f)
{
    start_motor();
    
    index_irq = false;
    int start_clock = clock;
    int elapsed = 0;
    while (!index_irq)
    {
        elapsed = clock - start_clock;
        if (elapsed > 1500)
        {
            elapsed = 0;
            break;
        }
    }

    if (elapsed != 0)
    {
        int target_pulse_count = f->hard_sector_count + 1;
        start_clock = clock;
        for (int x=0; x<target_pulse_count; x++)
        {
            index_irq = false;
            while (!index_irq)
                elapsed = clock - start_clock;
        }
    }
    
    DECLARE_REPLY_FRAME(struct speed_frame, F_FRAME_MEASURE_SPEED_REPLY);
    r.period_ms = elapsed;
    send_reply((struct any_frame*) &r);    
}

static void cmd_bulk_write_test(struct any_frame* f)
{
    uint8_t buffer[64];
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    for (int x=0; x<64; x++)
    {
        CyWdtClear();
        for (int y=0; y<256; y++)
        {
            for (unsigned z=0; z<sizeof(buffer); z++)
                buffer[z] = x+y+z;
            
            wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
            USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, buffer, sizeof(buffer));
        }
    }
    
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_BULK_WRITE_TEST_REPLY);
    send_reply(&r);
}

static void cmd_bulk_read_test(struct any_frame* f)
{
    uint8_t buffer[64];
    
    bool passed = true;
    for (int x=0; x<64; x++)
    {
        CyWdtClear();
        for (int y=0; y<256; y++)
        {
            usb_read(FLUXENGINE_DATA_OUT_EP_NUM, buffer);
            for (unsigned z=0; z<sizeof(buffer); z++)
            {
                if (buffer[z] != (uint8)(x+y+z))
                {
                    print("fail %d+%d+%d == %d, not %d", x, y, z, buffer[z], (uint8)(x+y+z));
                    passed = false;
                }
            }
        }
    }

    print("passed=%d", passed);
    if (passed)
    {
        DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_BULK_READ_TEST_REPLY);
        send_reply(&r);
    }
    else
        send_error(F_ERROR_INVALID_VALUE);
}

static void deinit_dma(void)
{
    for (int i=0; i<BUFFER_COUNT; i++)
        CyDmaTdFree(td[i]);
}

static void init_capture_dma(void)
{
    dma_channel = SAMPLER_DMA_DmaInitialize(
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
            CY_DMA_TD_INC_DST_ADR | SAMPLER_DMA__TD_TERMOUT_EN);
        CyDmaTdSetAddress(td[i], LO16((uint32)SAMPLER_FIFO_FIFO_PTR), LO16((uint32)&dma_buffer[i]));
    }    
}

static void cmd_read(struct read_frame* f)
{
    seek_to(current_track);
    SIDE_REG_Write(f->side);
    STEP_REG_Write(f->side); /* for drives which multiplex SIDE and DIR */
    /* Do slow setup *before* we go into the real-time bit. */
    
    {
        uint8_t i = CyEnterCriticalSection();
        SAMPLER_FIFO_SET_LEVEL_NORMAL;
        SAMPLER_FIFO_CLEAR;
        SAMPLER_FIFO_SINGLE_BUFFER_UNSET;
        CyExitCriticalSection(i);
    }
    
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    init_capture_dma();

    /* Wait for the beginning of a rotation, if requested. */
        
    if (f->synced)
    {
        hardsec_index_threshold = f->hardsec_threshold_ms;
        index_irq = false;
        while (!index_irq)
            ;
        index_irq = false;
        hardsec_index_threshold = 0;
    }
    
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
    bool dma_running = true;
    
    /* Start transferring. */

    uint32_t start_time = clock;
    for (;;)
    {
        CyWdtClear();

        /* If the sample session is over, stop reading but continue processing until
         * the DMA chain is empty. */
        
        if ((clock - start_time) >= f->milliseconds)
        {
            if (dma_running)
            {
                CyDmaChSetRequest(dma_channel, CY_DMA_CPU_TERM_CHAIN);
                while (CyDmaChGetRequest(dma_channel))
                    ;
                dma_running = false;
                dma_underrun = false;
            }
        }
        
        /* If there's an underrun event, stop immediately. */
        
        if (dma_underrun)
            goto abort;
        
        /* If there are no more blocks to be read, check to see if we've finished. */
        
        if (dma_reading_from_td == dma_writing_to_td)
        {
            /* Also if we've run out of blocks to send. */
            
            if (!dma_running)
                goto abort;
        }
        else
        {
            /* Otherwise, there's a block waiting, so attempt to send it. */
            
            wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
            USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, dma_buffer[dma_reading_from_td], BUFFER_SIZE);
            count++;
            dma_reading_from_td = NEXT_BUFFER(dma_reading_from_td);
        }
    }
abort:;
    bool saved_dma_underrun = dma_underrun;

    /* Terminate the transfer (all transfers are an exact number of fragments). */
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    USBFS_LoadInEP(FLUXENGINE_DATA_IN_EP_NUM, NULL, 0);
    wait_until_writeable(FLUXENGINE_DATA_IN_EP_NUM);
    deinit_dma();

    STEP_REG_Write(0);
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
    
    seek_to(current_track);    
    SIDE_REG_Write(f->side);
    STEP_REG_Write(f->side); /* for drives which multiplex SIDE and DIR */
    SEQUENCER_CONTROL_Write(1); /* put the sequencer into reset */
    {
        uint8_t i = CyEnterCriticalSection();        
        REPLAY_FIFO_SET_LEVEL_MID;
        REPLAY_FIFO_CLEAR;
        REPLAY_FIFO_SINGLE_BUFFER_UNSET;
        CyExitCriticalSection(i);
    }

    init_replay_dma();
    bool writing = false; /* to the disk */
    int packets = f->bytes_to_write / FRAME_SIZE;
    bool finished = (packets == 0);
    int count_written = 0;
    int count_read = 0;
    dma_writing_to_td = 0;
    dma_reading_from_td = -1;
    dma_underrun = false;
    
    int old_reading_from_td = -1;
    for (;;)
    {
        //CyWdtClear();

        /* Read data from USB into the buffers. */
        
        if (NEXT_BUFFER(dma_writing_to_td) != dma_reading_from_td)
        {
            if (writing && (dma_underrun || index_irq))
                goto abort;
            
            uint8_t* buffer = dma_buffer[dma_writing_to_td];
            if (finished)
            {
                /* There's no more data to read, so fake some. */
                
                memset(buffer, 0x3f, BUFFER_SIZE);
            }
            else
            {
                (void) usb_read(FLUXENGINE_DATA_OUT_EP_NUM, buffer);
                count_read++;
                
                if (count_read == packets)
                    finished = true;
            }
            dma_writing_to_td = NEXT_BUFFER(dma_writing_to_td);
            
            /* Once all the buffers are full, start writing. */
            
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
                hardsec_index_threshold = f->hardsec_threshold_ms;
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
    hardsec_index_threshold = 0;
    if (!finished)
    {
        /* There's still some data to read, so just read and blackhole it ---
         * easier than trying to terminate the connection. */
        while (count_read != packets)
        {
            (void) usb_read(FLUXENGINE_DATA_OUT_EP_NUM, usb_buffer);
            count_read++;
        }
    }
    
    deinit_dma();
    
    STEP_REG_Write(0);
    if (dma_underrun)
    {
        print("underrun!");
        send_error(F_ERROR_UNDERRUN);
        return;
    }

    print("success");
    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_WRITE_REPLY);
    send_reply((struct any_frame*) &r);
}

static void cmd_erase(struct erase_frame* f)
{
    SIDE_REG_Write(f->side);
    seek_to(current_track);
    /* Disk is now spinning. */
    
    print("start erasing");
    hardsec_index_threshold = f->hardsec_threshold_ms;
    index_irq = false;
    while (!index_irq)
        ;
    ERASE_REG_Write(1);
    index_irq = false;
    while (!index_irq)
        ;
    ERASE_REG_Write(0);
    hardsec_index_threshold = 0;
    print("stop erasing");

    DECLARE_REPLY_FRAME(struct any_frame, F_FRAME_ERASE_REPLY);
    send_reply((struct any_frame*) &r);
}

static void cmd_set_drive(struct set_drive_frame* f)
{
    if (drive0_present && !drive1_present)
        f->drive = 0;
    if (drive1_present && !drive0_present)
        f->drive = 1;
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
            cmd_measure_speed((struct measurespeed_frame*) f);
            break;
            
        case F_FRAME_BULK_WRITE_TEST_CMD:
            cmd_bulk_write_test(f);
            break;
            
        case F_FRAME_BULK_READ_TEST_CMD:
            cmd_bulk_read_test(f);
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

static void detect_drives(void)
{
    current_drive_flags.drive = 0;
    start_motor();
    drive0_present = home();
    stop_motor();
    
    current_drive_flags.drive = 1;
    start_motor();
    drive1_present = home();
    stop_motor();
    
    print("drive 0: %s drive 1: %s", drive0_present ? "yes" : "no", drive1_present ? "yes" : "no");
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
    USBFS_DisableOutEP(FLUXENGINE_DATA_OUT_EP_NUM);
    
    CyWdtStart(CYWDT_1024_TICKS, CYWDT_LPMODE_DISABLED);
    
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
                CyWdtClear();
            print("USB ready");
            USBFS_EnableOutEP(FLUXENGINE_CMD_OUT_EP_NUM);
            print("Scanning drives...");
            detect_drives();
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

const uint8_t USBFS_MSOS_CONFIGURATION_DESCR[USBFS_MSOS_CONF_DESCR_LENGTH] = {
/*  Length of the descriptor 4 bytes       */   0x28u, 0x00u, 0x00u, 0x00u,
/*  Version of the descriptor 2 bytes      */   0x00u, 0x01u,
/*  wIndex - Fixed:INDEX_CONFIG_DESCRIPTOR */   0x04u, 0x00u,
/*  bCount - Count of device functions.    */   0x01u,
/*  Reserved : 7 bytes                     */   0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
/*  bFirstInterfaceNumber                  */   0x00u,
/*  Reserved                               */   0x01u,
/*  compatibleId - "WINUSB\0\0"            */   'W', 'I', 'N', 'U', 'S', 'B', 0, 0,
/*  subcompatibleID - "00001\0\0"          */   '0', '0', '0', '0', '1',
                                                0x00u, 0x00u, 0x00u,
/*  Reserved : 6 bytes                     */   0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
};

const uint8_t USBFS_MSOS_EXTENDED_PROPERTIES_DESCR[224] = {
    /* Length; 4 bytes       */   224, 0, 0, 0,
    /* Version; 2 bytes      */   0x00, 0x01, /* 1.0 */
    /* wIndex                */   0x05, 0x00,
    /* Number of sections    */   0x01, 0x00,
    /* Property section size */   214, 0, 0, 0,
    /* Property data type    */   0x07, 0x00, 0x00, 0x00, /* 7 = REG_MULTI_SZ Unicode */
    /* Property name length  */   42, 0,
    /* Property name         */   'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
                                  'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0,
                                  'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0,
                                  'D', 0, 's', 0, 0,   0,
    /* Property data length  */   158, 0, 0, 0,
    /* GUID #1 data          */   '{', 0, '3', 0, 'd', 0, '2', 0, '7', 0, '5', 0,
                                  'c', 0, 'f', 0, 'e', 0, '-', 0, '5', 0, '4', 0,
                                  '3', 0, '5', 0, '-', 0, '4', 0, 'd', 0, 'd', 0,
                                  '5', 0, '-', 0, 'a', 0, 'c', 0, 'c', 0, 'a', 0,
                                  '-', 0, '9', 0, 'f', 0, 'b', 0, '9', 0, '9', 0,
                                  '5', 0, 'e', 0, '2', 0, 'f', 0, '6', 0, '3', 0,
                                  '8', 0, '}', 0, '\0', 0,
    /* GUID #2 data          */   '{', 0, '3', 0, 'd', 0, '2', 0, '7', 0, '5', 0,
                                  'c', 0, 'f', 0, 'e', 0, '-', 0, '5', 0, '4', 0,
                                  '3', 0, '5', 0, '-', 0, '4', 0, 'd', 0, 'd', 0,
                                  '5', 0, '-', 0, 'a', 0, 'c', 0, 'c', 0, 'a', 0,
                                  '-', 0, '9', 0, 'f', 0, 'b', 0, '9', 0, '9', 0,
                                  '5', 0, 'e', 0, '2', 0, 'f', 0, '6', 0, '3', 0,
                                  '8', 0, '}', 0, '\0', 0, '\0', 0
};

uint8 USBFS_HandleVendorRqst(void)
{
    if (!(USBFS_bmRequestTypeReg & USBFS_RQST_DIR_D2H))
        return false;

    switch (USBFS_bRequestReg)
    {
        case USBFS_GET_EXTENDED_CONFIG_DESCRIPTOR:
            switch (USBFS_wIndexLoReg)
            {
                case 4:
                    USBFS_currentTD.pData = (volatile uint8 *) &USBFS_MSOS_CONFIGURATION_DESCR[0u];
                    USBFS_currentTD.count = USBFS_MSOS_CONFIGURATION_DESCR[0u];
                    return USBFS_InitControlRead();
                    
                case 5:
                    USBFS_currentTD.pData = (volatile uint8 *) &USBFS_MSOS_EXTENDED_PROPERTIES_DESCR[0u];
                    USBFS_currentTD.count = USBFS_MSOS_EXTENDED_PROPERTIES_DESCR[0u];
                    return USBFS_InitControlRead();
            }

        default:
            break;
    }

    return true;
}

