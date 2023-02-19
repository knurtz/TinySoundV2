#include "TS_audio.h"
#include "pico/audio_i2s.h"
#include "TS_sine.h"

#include "ff.h"


void Audio_Play(void)
{

}


struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .sample_freq = 48000,
        .channel_count = 2
    };

    static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 4
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 1, sizeof(dac_buffer));
    
    struct audio_i2s_config i2s_config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    const struct audio_format *output_format;
    output_format = audio_i2s_setup(&audio_format, &i2s_config);
    
    if (!output_format) {
        printf("PicoAudio: Unable to open audio device.\n");
        return;
    }

    audio_i2s_connect(producer_pool);
    audio_i2s_set_enabled(true);

    return producer_pool;
}

void Audio_Init(void)
{
    gpio_put(AUDIO_EN, 1);
    sleep_ms(100);

    uint32_t step = 0x200000;
    uint32_t pos = 0;
    uint32_t pos_max = 0x10000 * sizeof(dac_buffer);
    uint vol = 128;

    struct audio_buffer_pool *ap = init_audio();
}

void Audio_Play(void)
{
    while (true) {
        struct audio_buffer *buffer = take_audio_buffer(ap, true);
        int16_t *samples = (int16_t *) buffer->buffer->bytes;
        
        for (uint i = 0; i < buffer->max_sample_count; i++) {
            samples[i] = (vol * sine_wave_table[pos >> 16u]) >> 8u;
            pos += step;
            if (pos >= pos_max) pos -= pos_max;
        }
        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
    }
}

void Audio_Stop(void)
{
    
}


    

    