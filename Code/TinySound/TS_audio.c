#include "TS_audio.h"
#include "TS_sine.h"
#include "TS_shell.h"
#include "TS_fat.h"

#include "wave_header.h"

#include "ff.h"
#include "pico/audio_i2s.h"
#include "hardware/gpio.h"

// Global audio producer pool (collection of audio buffers)
struct audio_buffer_pool *ap;

void Audio_GetWaveInfo(const char* filename)
{
    uint8_t buf[sizeof(FILEHeader)];
    FAT_ReadFileToBuffer(filename, 0, sizeof(FILEHeader), buf);

    FILEHeader* wave_header = (FILEHeader*) &buf;

    if (wave_header->riff.id != RIFF ||
        wave_header->riff.type != WAVE ||
        wave_header->format.id != FMT ||
        wave_header->data.id != DATA)
    {
        xprintf("Some header IDs not as expected.\n");
        return;
    }

    xprintf("Header IDs ok.\n");

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
   
   Audio_Init();
   xprintf("Audio fully initialized.\n");

   Audio_Play();
}


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

void Audio_Init(void)
{
    gpio_init(AUDIO_EN);
    gpio_set_dir(AUDIO_EN, GPIO_OUT);
    gpio_put(AUDIO_EN, 1);
    sleep_ms(100);

    ap = init_audio();
}

void Audio_Play(void)
{    
    uint32_t file_offset = 44;

    while (true) 
    {        
        // Get next slot in producer pool (second parameter enables blocking)
        audio_buffer_t *buffer = take_audio_buffer(ap, false);
        //xprintf("Got buffer.\n");

        if (buffer)
        {
            // Fill buffer with next audio data
            file_offset += FAT_ReadFileToBuffer(NULL, file_offset, buffer->max_sample_count, buffer->buffer->bytes);
            
            // Modify volume

            
            // Update sample count and return to producer pool
            buffer->sample_count = buffer->max_sample_count;
            give_audio_buffer(ap, buffer);

            //xprintf("Filled buffer offset %d.\n", file_offset);
        }
    }
}


void Audio_Stop(void)
{
    
}