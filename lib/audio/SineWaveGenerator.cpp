#include "SineWaveGenerator.h"

void SineWaveGenerator::setFrequency(const double freq)
{
    m_PhaseIncrement = freq / (double) AUDIO_SAMPLE_RATE_EXACT;
}

void SineWaveGenerator::setAmplitude(const double amplitude)
{
    m_Amplitude = amplitude;
}

void SineWaveGenerator::generate(int16_t *buffer, const size_t numChannels, const size_t numSamples)
{
    for (size_t i{0}; i < numSamples; ++i) {
        auto sample{m_Amplitude * ((1 << 15) - 1) * sin(2 * M_PI * m_Phase)};
        if (sample > 32751) {
            // For ease of logic analysis...
            sample = 32767;
        }
        for (size_t channel = 0; channel < numChannels; ++channel) {
            buffer[numChannels * i + channel] = (int16_t) sample;
        }
        m_Phase += m_PhaseIncrement;
    }
}

void SineWaveGenerator::reset()
{
    m_Phase = 0;
}
