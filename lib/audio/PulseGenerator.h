#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include <Arduino.h>

class PulseGenerator {
public:
    void setWidth(uint16_t width);

    void setFrequency(double freq);

    void setAmplitude(double amplitude);

    void generate(int16_t *buffer, size_t numChannels, size_t numSamples);

    void reset();

private:
    double m_Phase{0};
    double m_Amplitude{1};
    double m_PhaseIncrement{0};
    uint16_t m_Width{1};
    uint16_t m_RemainingWidth{0};
};

#endif //PULSEGENERATOR_H
