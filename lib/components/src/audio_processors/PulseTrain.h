#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include <AudioProcessor.h>

#include "Phasor.h"

class PulseTrain : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) override;

    void setFrequency(float freq);

    void setAmplitude(float amplitude);

    void setWidth(uint16_t width);

private:
    Phasor phasor;
    float amplitude{1.f};
    int width{1};
    int remainingWidth{-1};
};

#endif //PULSEGENERATOR_H
