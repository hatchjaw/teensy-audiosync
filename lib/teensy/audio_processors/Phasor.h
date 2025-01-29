#ifndef PHASOR_H
#define PHASOR_H

#include <AudioProcessor.h>

class Phasor final
{
public:
    void prepare(uint sampleRate);

    void setFrequency(float freq);

    float getNextSample();

    float getPrevSample() const;

private:
    float phase{0}, prevPhase{-0}, delta{0};
    uint sampleRate{0};
};

#endif //PHASOR_H
