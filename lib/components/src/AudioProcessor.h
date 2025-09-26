#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>

#define CYCLES_TO_APPROX_PERCENT(cycles) (((float)((uint32_t)(cycles) * 6400u) * (float)(AUDIO_SAMPLE_RATE_EXACT / 128)) / (float)(F_CPU_ACTUAL))

class AudioProcessor : public Printable
{
public:
    virtual ~AudioProcessor() = default;

    virtual void prepare(uint sampleRate) = 0;

    virtual void processAudio(int16_t *buffer, const size_t numChannels, const size_t numSamples) final
    {
        uint32_t cycles = ARM_DWT_CYCCNT;

        processImpl(buffer, numChannels, numSamples);

        cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
        currentCycles = cycles;
        if (currentCycles > maxCycles) {
            maxCycles = currentCycles;
        }
    }

    size_t printTo(Print &p) const override
    {
        return p.printf("%% CPU: %f (max %f)",
                        CYCLES_TO_APPROX_PERCENT(currentCycles),
                        CYCLES_TO_APPROX_PERCENT(maxCycles)
        );
    }

protected:
    virtual void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) = 0;

private:
    uint16_t currentCycles{0}, maxCycles{0};
};

#endif //AUDIOPROCESSOR_H
