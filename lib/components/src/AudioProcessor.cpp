#include "AudioProcessor.h"
#include <AnanasUtils.h>

#define CYCLES_TO_APPROX_PERCENT(cycles) (((float)((uint32_t)(cycles) * 6400u) * (float)(AUDIO_SAMPLE_RATE_EXACT / AUDIO_BLOCK_SAMPLES)) / (float)(F_CPU_ACTUAL))

AudioProcessor::AudioProcessor(const size_t nInputs, const size_t nOutputs) : numInputs(nInputs), numOutputs(nOutputs)
{
    if (numInputs > 0) {
        inputBuffer = new int16_t *[numInputs];
        for (size_t i{0}; i < numInputs; i++) {
            inputBuffer[i] = new int16_t[ananas::Constants::AudioBlockFrames];
        }
    } else {
        inputBuffer = nullptr;
    }

    if (numOutputs > 0) {
        outputBuffer = new int16_t *[numOutputs];
        for (size_t i{0}; i < numOutputs; i++) {
            outputBuffer[i] = new int16_t[ananas::Constants::AudioBlockFrames];
        }
    } else {
        outputBuffer = nullptr;
    }
}

AudioProcessor::~AudioProcessor()
{
    for (size_t i{0}; i < numInputs; i++) {
        delete[] inputBuffer[i];
    }
    delete[] inputBuffer;
    for (size_t i{0}; i < numOutputs; i++) {
        delete[] outputBuffer[i];
    }
    delete[] outputBuffer;
}

void AudioProcessor::processAudio(int16_t *buffer, const size_t numChannels, const size_t numFrames)
{
    uint32_t cycles = ARM_DWT_CYCCNT;

    processImpl(buffer, numChannels, numFrames);

    cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
    currentCycles = cycles;
    if (currentCycles > maxCycles) {
        maxCycles = currentCycles;
    }
}

void AudioProcessor::processAudioV2(const size_t numFrames)
{
    uint32_t cycles = ARM_DWT_CYCCNT;

    processImplV2(numFrames);

    cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
    currentCycles = cycles;
    if (currentCycles > maxCycles) {
        maxCycles = currentCycles;
    }
}

void AudioProcessor::getOutput(int16_t **dest, size_t numChannels, size_t numFrames) const
{
}

void AudioProcessor::getOutputInterleaved(int16_t *dest, const size_t numChannels, const size_t numFrames) const
{
    for (size_t frame{0}; frame < numFrames; frame++) {
        for (size_t channel{0}; channel < numChannels; channel++) {
            dest[frame * numChannels + channel] = outputBuffer[channel][frame];
        }
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
