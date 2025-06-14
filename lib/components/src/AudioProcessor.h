#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>

#define CYCLES_TO_APPROX_PERCENT(cycles) (((float)((uint32_t)(cycles) * 6400u) * (float)(AUDIO_SAMPLE_RATE_EXACT / 128)) / (float)(F_CPU_ACTUAL))

class AudioProcessor : public Printable
{
public:
    virtual ~AudioProcessor() = default;

    virtual void prepare(uint sampleRate) = 0;

    virtual void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) = 0;

    size_t printTo(Print &p) const override
    {
        return p.printf("%% CPU: %f (max %f)",
                        CYCLES_TO_APPROX_PERCENT(currentCycles),
                        CYCLES_TO_APPROX_PERCENT(maxCycles)
        );
    }

protected:
    uint16_t currentCycles{0}, maxCycles{0};
};

#endif //AUDIOPROCESSOR_H
