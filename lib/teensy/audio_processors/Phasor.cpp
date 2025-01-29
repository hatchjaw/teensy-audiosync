#include "Phasor.h"

void Phasor::prepare(const uint sampleRate)
{
    this->sampleRate = sampleRate;
    phase = 0;
    prevPhase = -0;
    delta = 0;
}

void Phasor::setFrequency(const float freq)
{
    delta = freq / static_cast<float>(sampleRate);
}

float Phasor::getNextSample()
{
    prevPhase = phase;
    phase += delta;
    phase -= floorf(phase);
    return phase;
}

float Phasor::getPrevSample() const
{
    return prevPhase;
}
