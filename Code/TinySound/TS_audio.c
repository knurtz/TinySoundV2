#include "TS_audio.h"
#include "TS_sine.h"
#include "TS_shell.h"
#include "TS_fat.h"

#include "audio_i2s.pio.h"

#include "ff.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "pico/sync.h"

#include <string.h>

#define BUFFER_SIZE 512         // number of samples
#define AUDIO_SM    0

// Global audio producer pool (collection of audio buffers)
//struct audio_buffer_pool *ap;

/*
struct audio_buffer_pool *init_audio(void)
{
    // This might change depending on the WAVE file
    static audio_format_t audio_format = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .sample_freq = 44100,
        .channel_count = 2
    };

    // This stays the same unless we decide to do more than double buffering
    static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 4        // Is this related to double buffering?
    };

    // Creates a new producer pool with the given audio format, this will be returned by this function
    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 2, 512);

    xprintf("Producer pool created.\n");
    
    // COnfigure the I2S module
    struct audio_i2s_config i2s_config = {
            .data_pin = AUDIO_DIN,
            .clock_pin_base = AUDIO_LRCLK,
            .dma_channel = 0,
            .pio_sm = 0
    };
    
    // Setup I2S module
    const struct audio_format *output_format;
    output_format = audio_i2s_setup(&audio_format, &i2s_config);
    
    if (!output_format) {
        xprintf("PicoAudio: Unable to open audio device.\n");
        return NULL;
    }

    xprintf("I2S setup finished.\n");

    // Link producer pool to I2S module and enable I2S
    audio_i2s_connect(producer_pool);    
    xprintf("Producer pool connected.\n");

    audio_i2s_set_enabled(true);
    xprintf("I2S enabled.\n");

    // Producer pool will be needed to feed audio data
    return producer_pool;
}
*/

int16_t audio_buffer[2][BUFFER_SIZE];           // 512x 16bit samples -> 1024 bytes -> 256x 32bit words

static bool buffer_empty = false;
uint8_t play_buffer_index = 0;

uint32_t file_offset;

static void Audio_UpdateFrequency(uint32_t sample_freq)
{
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / sample_freq;
    pio_sm_set_clkdiv_int_frac(pio0, AUDIO_SM, divider >> 8u, divider & 0xffu);
}

void Audio_Init(void)
{
    // Enable MAX98360
    gpio_init(AUDIO_EN);
    gpio_set_dir(AUDIO_EN, GPIO_OUT);
    gpio_put(AUDIO_EN, 1);
    sleep_ms(10);

    // Setup I2S pins
    gpio_set_function(AUDIO_DIN, GPIO_FUNC_PIO0);
    gpio_set_function(AUDIO_LRCLK, GPIO_FUNC_PIO0);
    gpio_set_function(AUDIO_BCLK, GPIO_FUNC_PIO0);

    // Setup PIO0 statemachine 0 with I2S program
    pio_sm_claim(pio0, AUDIO_SM);
    uint offset = pio_add_program(pio0, &audio_i2s_program);
    audio_i2s_program_init(pio0, AUDIO_SM, offset, AUDIO_DIN, AUDIO_LRCLK);
    __mem_fence_release();

    // Setup DMA channel
    uint8_t dma_channel = 0;
    dma_channel_claim(dma_channel);

    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, DREQ_PIO0_TX0 + AUDIO_SM);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);

    dma_channel_configure(dma_channel,
                          &dma_config,
                          &pio0->txf[AUDIO_SM],                 // dest:  TX buffer of PIO0, statemachine 0
                          NULL,                                 // src:   will be defined later
                          BUFFER_SIZE / 4,                      // count: half of the buffer (counted in 32bit words)
                          false);                               // don't trigger immediately

    irq_add_shared_handler(DMA_IRQ_0, Audio_DMACallback, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    dma_irqn_set_channel_enabled(0, dma_channel, true);
    irq_set_enabled(DMA_IRQ_0, true);
}


void __isr __time_critical_func(Audio_DMACallback)(void)
{
    if (dma_irqn_get_channel_status(0, 0))
    {
        dma_irqn_acknowledge_channel(0, 0);

        play_buffer_index = play_buffer_index ? 0 : 1;
        dma_channel_transfer_from_buffer_now(0, audio_buffer[play_buffer_index], BUFFER_SIZE / 4);
        buffer_empty = true;        
    }
}

void Audio_Play(const char* filename)
{
    file_offset = FAT_ReadFileToBuffer(filename, 0, 512, (uint8_t*)audio_buffer[0]);

    FILEHeader* wave_header = (FILEHeader*) audio_buffer[0];

    if (wave_header->riff.id != RIFF ||
        wave_header->riff.type != WAVE ||
        wave_header->format.id != FMT ||
        wave_header->data.id != DATA)
    {
        xprintf("Error - Some header IDs not as expected.\n");
        return;
    }

    /*
    xprintf("Wave info:\n  audioFormat:   %d\n  numChannels:   %d\n",
            wave_header->format.audioFormat,
            wave_header->format.numChannels);
    
    xprintf("  sampleRate:    %d\n  byteRate:      %d\n",
            wave_header->format.sampleRate,
            wave_header->format.byteRate);

    xprintf("  blockAlign:    %d\n  bitsPerSample: %d",       
            wave_header->format.blockAlign,
            wave_header->format.bitsPerSample);
    */

    // Overwrite wave header with silence
    memset(audio_buffer, 0, 44);
   
    // Set correct I2S frequency
    Audio_UpdateFrequency(44100);
    __mem_fence_release();

    // Connect buffer
    play_buffer_index = 0;
    buffer_empty = true;        // so main rountine will also fill second half of the buffer

    dma_channel_transfer_from_buffer_now(0, audio_buffer[0], BUFFER_SIZE / 4);
    pio_sm_set_enabled(pio0, AUDIO_SM, true);

}


void Audio_CheckBuffer(void)
{
    if (buffer_empty)
    {
        uint8_t load_buffer = play_buffer_index ? 0 : 1;
        file_offset += FAT_ReadFileToBuffer(NULL, file_offset, 512, (uint8_t*)audio_buffer[load_buffer]);
        buffer_empty = false;
    }
}


void Audio_Stop(void)
{
    
}