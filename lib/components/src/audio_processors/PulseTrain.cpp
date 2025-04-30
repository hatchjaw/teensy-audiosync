#include "PulseTrain.h"

void PulseTrain::prepare(const uint sampleRate)
{
    phasor.prepare(sampleRate);
    remainingWidth = -1;
}

void PulseTrain::processAudio(int16_t *buffer, const size_t numChannels, const size_t numSamples)
{
    for (size_t n{0}; n < numSamples; ++n) {
        if (phasor.getNextSample() < phasor.getPrevSample()) {
            remainingWidth = width;
        }

        auto sample{0.f};
        if (remainingWidth >= 1) {
            sample = amplitude * INT16_MAX;
            --remainingWidth;
        }

        for (size_t ch{0}; ch < numChannels; ++ch) {
            buffer[n * numChannels + ch] = static_cast<int16_t>(sample);
        }
    }
}

void PulseTrain::setFrequency(const float freq)
{
    phasor.setFrequency(freq);
}

void PulseTrain::setAmplitude(const float amplitude)
{
    this->amplitude = amplitude;
}

void PulseTrain::setWidth(const uint16_t width)
{
    this->width = static_cast<int>(width);
}
