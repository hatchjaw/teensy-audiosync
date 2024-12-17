#include "PulseGenerator.h"

void PulseGenerator::setWidth(const uint16_t width)
{
    m_Width = width;
}

void PulseGenerator::setFrequency(const double freq)
{
    m_PhaseIncrement = freq / (double) AUDIO_SAMPLE_RATE_EXACT;
}

void PulseGenerator::setAmplitude(const double amplitude)
{
    m_Amplitude = amplitude;
}

void PulseGenerator::generate(int16_t *buffer, size_t numChannels, size_t numSamples)
{
    for (size_t i{0}; i < numSamples; ++i) {
        auto sample{0.0};
        if (m_RemainingWidth > 0) {
            sample = m_Amplitude * ((1 << 15) - 1);
            m_RemainingWidth--;
        }
        for (size_t channel = 0; channel < numChannels; ++channel) {
            buffer[numChannels * i + channel] = (int16_t) sample;
        }
        m_Phase += m_PhaseIncrement;
        if (m_Phase >= 1.0) {
            m_RemainingWidth = m_Width;
        }
        m_Phase = fmod(m_Phase, 1);
    }
}

void PulseGenerator::reset()
{
    m_Phase = 0;
}
