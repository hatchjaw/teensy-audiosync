#include "PulseTrain.h"

void PulseTrain::prepare(const uint sampleRate)
{
    phasor.prepare(sampleRate);
    remainingWidth = -1;
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

void PulseTrain::processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames)
{
    for (size_t n{0}; n < numFrames; ++n) {
        if (phasor.getNextSample() < phasor.getPrevSample()) {
            remainingWidth = width;
        }

        auto sample{0.f};
        if (remainingWidth >= 1) {
            sample = amplitude * INT16_MAX;
            --remainingWidth;
        }

        for (size_t ch{0}; ch < getNumOutputs(); ++ch) {
            outputBuffer[ch][n] = static_cast<int16_t>(sample);
        }
    }
}
