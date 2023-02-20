#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define AUDIO_DIN   22
#define AUDIO_LRCLK 23
#define AUDIO_BCLK  24

#define AUDIO_EN    25

void Audio_GetWaveInfo(const char* filename);

void Audio_Init(void);
void Audio_Play(void);
void Audio_Stop(void);