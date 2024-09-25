#ifndef SINEWAVEGENERATOR_H
#define SINEWAVEGENERATOR_H

#include <Arduino.h>

class SineWaveGenerator
{
public:
    void setFrequency(double freq);

    void setAmplitude(double amplitude);

    void generate(int16_t *buffer, size_t numChannels, size_t numSamples);

    void reset();

private:
    double m_Phase{0};
    double m_Amplitude{1};
    double m_PhaseIncrement{0};
};


#endif //SINEWAVEGENERATOR_H
