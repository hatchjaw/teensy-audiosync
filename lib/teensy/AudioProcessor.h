#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>

class AudioProcessor
{
public:
    virtual ~AudioProcessor() = default;

    virtual void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) = 0;
};

#endif //AUDIOPROCESSOR_H
