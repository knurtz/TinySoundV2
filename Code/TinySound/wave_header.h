#ifndef WAVE_H
#define WAVE_H

#include "pico/stdlib.h"

#define FORMAT_PCM      1
#define RIFF            0x46464952  // FFIR
#define WAVE            0x45564157  // EVAW 
#define DATA            0x61746164  // atad
#define FMT             0x20746D66  //  tmf

typedef struct _RIFFHeader      // Size: 12 bytes
{
	uint32_t    id;             // "RIFF"
    uint32_t    size;
	uint32_t    type;           // "WAVE"
} RIFFHeader;

typedef struct _WAVEHeader      // Size: 24 bytes
{
    uint32_t    id;             // "fmt "     
    uint32_t    size;
    uint16_t    audioFormat;    // 1 - PCM
    uint16_t    numChannels;    // 1 - mono, 2 - stereo
    uint32_t    sampleRate;
    uint32_t    byteRate;       // byteRate = SampleRate * BlockAlign
    uint16_t    blockAlign;     // BlockAlign = bitsPerSample / 8 * NumChannels
    uint16_t    bitsPerSample;
} WAVEHeader;

typedef struct _DATAHeader      // Size: 8 bytes
{
	uint32_t    id;             // "data"          
	uint32_t    size;
} DATAHeader;

typedef struct _FILEHeader
{
    RIFFHeader  riff;           // 12 bytes
    WAVEHeader  format;         // 24 bytes
    DATAHeader  data;           // 8 bytes + data
} FILEHeader;

#endif /* WAVE_H */
