#ifndef SINEOSC_H
#define SINEOSC_H

#include <AudioProcessor.h>

#include "Phasor.h"

class SineOsc : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) override;

    void setFrequency(float freq);

    void setAmplitude(float amplitude);

private:
    Phasor phasor;
    float amplitude{1.f};
};


#endif //SINEOSC_H
