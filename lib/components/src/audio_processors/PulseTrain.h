#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include <AudioProcessor.h>

#include "Phasor.h"

class PulseTrain : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    void setFrequency(float freq);

    void setAmplitude(float amplitude);

    void setWidth(uint16_t width);

protected:
    void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

private:
    Phasor phasor;
    float amplitude{1.f};
    int width{1};
    int remainingWidth{-1};
};

#endif //PULSEGENERATOR_H
