#include "SineOsc.h"

void SineOsc::setFrequency(const float freq)
{
    phasor.setFrequency(freq);
}

void SineOsc::setAmplitude(const float amplitude)
{
    this->amplitude = amplitude;
}

// void SineWaveGenerator::generate(int16_t *buffer, const size_t numChannels, const size_t numSamples)
// {
//     for (size_t i{0}; i < numSamples; ++i) {
//         auto sample{m_Amplitude * INT16_MAX * sin(2 * M_PI * m_Phase)};
//         if (sample > 32751) {
//             // For ease of logic analysis...
//             sample = 32767;
//         }
//         for (size_t channel = 0; channel < numChannels; ++channel) {
//             buffer[numChannels * i + channel] = (int16_t) sample;
//         }
//         m_Phase += m_PhaseIncrement;
//     }
// }

void SineOsc::processAudio(int16_t *buffer, const size_t numChannels, const size_t numSamples)
{
    for (size_t n{0}; n < numSamples; ++n) {
        const auto phase{phasor.getNextSample()},
          sample{amplitude * INT16_MAX * sinf(M_PI * 2.f * phase)};
        for (size_t ch{0}; ch < numChannels; ++ch) {
            buffer[n * numChannels + ch] = static_cast<int16_t>(sample);
        }
    }
}

void SineOsc::prepare(const uint sampleRate)
{
    phasor.prepare(sampleRate);
}
