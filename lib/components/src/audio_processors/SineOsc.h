#ifndef SINEOSC_H
#define SINEOSC_H

#include <AudioProcessor.h>

#include "Phasor.h"

class SineOsc : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    void setFrequency(float freq);

    void setAmplitude(float amplitude);

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override;

private:
    Phasor phasor;
    float amplitude{1.f};
};


#endif //SINEOSC_H
