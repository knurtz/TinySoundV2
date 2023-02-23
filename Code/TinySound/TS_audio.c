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

#define BUFFER_SIZE 512                         // number of samples
#define AUDIO_SM    0
#define DMA_CHANNEL 0

int16_t audio_buffer[2][BUFFER_SIZE];           // 512x 16bit samples -> 1024 bytes -> 256x 32bit words

static bool buffer_empty = false;
uint8_t play_buffer_index = 0;

uint32_t file_offset;
uint32_t file_size;
bool playing = false;

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
    dma_channel_claim(DMA_CHANNEL);

    dma_channel_config dma_config = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, DREQ_PIO0_TX0 + AUDIO_SM);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);

    dma_channel_configure(DMA_CHANNEL,
                          &dma_config,
                          &pio0->txf[AUDIO_SM],                 // dest:  TX buffer of PIO0, statemachine 0
                          NULL,                                 // src:   will be defined later
                          BUFFER_SIZE / 4,                      // count: half of the buffer (counted in 32bit words)
                          false);                               // don't trigger immediately

    irq_add_shared_handler(DMA_IRQ_0, Audio_DMACallback, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    dma_irqn_set_channel_enabled(0, DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);
}


void __isr __time_critical_func(Audio_DMACallback)(void)
{
    if (dma_irqn_get_channel_status(0, DMA_CHANNEL))
    {
        dma_irqn_acknowledge_channel(0, DMA_CHANNEL);

        play_buffer_index = play_buffer_index ? 0 : 1;
        dma_channel_transfer_from_buffer_now(DMA_CHANNEL, audio_buffer[play_buffer_index], BUFFER_SIZE / 4);
        buffer_empty = true;
    }
}

void Audio_Play(const char* filename)
{   
    // Check minimum file size
    file_size = FAT_GetFilesize(filename);
    if (file_size < BUFFER_SIZE) return;

    // Load first chunk of audio file
    file_offset = FAT_ReadFileToBuffer(filename, 0, BUFFER_SIZE, (uint8_t*)audio_buffer[0]);

    // Re-interprate first 44 bytes of audio buffer as RIFF WAVE header
    FILEHeader* wave_header = (FILEHeader*) audio_buffer[0];

    // Basic checks for valid RIFF WAVE file
    if (wave_header->riff.id != RIFF ||
        wave_header->riff.type != WAVE ||
        wave_header->format.id != FMT ||
        wave_header->data.id != DATA)
    {
        xprintf("Error - Some header IDs not as expected.\n");
        return;
    }

    // TODO: check format details like sample rate, number of channels etc. and adjust I2S settings accordingly

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
    buffer_empty = true;        // so main rountine will immediately fill second half of the buffer

    dma_channel_transfer_from_buffer_now(DMA_CHANNEL, audio_buffer[play_buffer_index], BUFFER_SIZE / 4);
    pio_sm_set_enabled(pio0, AUDIO_SM, true);

    playing = true;
}


void Audio_CheckBuffer(void)
{
    if (playing && buffer_empty)
    {
        // Check for end of file
        if (file_size - file_offset < BUFFER_SIZE) 
        {
            Audio_Stop();
            return;
        }

        // Fill buffer
        uint8_t load_buffer = play_buffer_index ? 0 : 1;
        file_offset += FAT_ReadFileToBuffer(NULL, file_offset, BUFFER_SIZE, (uint8_t*)audio_buffer[load_buffer]);
        buffer_empty = false;
    }
}


void Audio_Stop(void)
{
    if (!playing) return;
    pio_sm_set_enabled(pio0, AUDIO_SM, false);
    playing = false;
}

bool Audio_IsPlaying(void)
{
    return playing;
}