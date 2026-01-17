#include "AudioProcessor.h"
#include <AnanasUtils.h>

void AudioProcessor::processAudio(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames)
{
    uint32_t cycles = ARM_DWT_CYCCNT;

    processImpl(inputBuffer, outputBuffer, numFrames);

    cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
    currentCycles = cycles;
    if (currentCycles > maxCycles) {
        maxCycles = currentCycles;
    }
}

size_t AudioProcessor::printTo(Print &p) const
{
    return p.printf("%% CPU: %f (max %f)",
                    CYCLES_TO_APPROX_PERCENT(currentCycles),
                    CYCLES_TO_APPROX_PERCENT(maxCycles)
    );
}

float AudioProcessor::getCurrentPercentCPU() const
{
    return CYCLES_TO_APPROX_PERCENT(currentCycles);
}
