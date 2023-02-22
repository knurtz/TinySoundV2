#include "pico/stdlib.h"
#include "wave_header.h"

#define AUDIO_DIN   22
#define AUDIO_LRCLK 23
#define AUDIO_BCLK  24

#define AUDIO_EN    25


void Audio_Init(void);

void Audio_Play(const char* filename);
void Audio_Stop(void);

void Audio_CheckBuffer(void);

void __isr __time_critical_func(Audio_DMACallback)(void);