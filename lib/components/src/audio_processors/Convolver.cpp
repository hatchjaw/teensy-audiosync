#include "Convolver.h"

extern "C" uint8_t external_psram_size;

// It's more efficient to convert to float, calculate, then convert back to
// int16 than it is to cast to int32, calculate, then cast back to int16.
EXTMEM float convBuffer[Convolver::kBufferSize][Convolver::kNumChannels]{};
EXTMEM float irBuffer[Convolver::kBufferSize]{};

void Convolver::prepare(uint sampleRate)
{
    if (external_psram_size <= 0) {
        Serial.println("Convolver: no external PSRAM found; no processing will be applied.");
        return;
    }

    const float frequency = 22 * 24 / (float) (((CCM_CBCMR >> 29) & 7) + 1);
    Serial.printf("\nCCM_CBCMR=%08X (%.1f MHz)\n", CCM_CBCMR, frequency);

    Serial.printf("\nConvolver: IR buffer at address 0x%X\n"
                  "Convolver: input buffer at address 0x%X\n",
                  irBuffer, convBuffer);

    memset(convBuffer, 0, sizeof(convBuffer));

    fifo.prepare();

    memset(irBuffer, 0, sizeof(irBuffer));
    for (size_t i{0}; i < kBufferSize; i++) {
        irBuffer[i] = 0.f;
    }
    irBuffer[0] = 1.f;
    irBuffer[kBufferSize - 1] = .9f;
}

void Convolver::processImpl(int16_t *buffer, const size_t numChannels, const size_t numSamples)
{
    if (external_psram_size <= 0) return;

    // Do naïve time-domain convolution

    // Each output sample for each channel needs to be the sum of the product
    // of that sample with the first sample of the IR, the previous sample with
    // the second sample of the IR, etc.
    //
    // y[n] = s[n]h[0] + s[n-1]h[1] + s[n-2]h[2] + ... + s[n - (M - 1)]h[M - 1]
    //
    for (size_t n{0}; n < numSamples; n++) {
        // Get the write index to the input/convolution buffer.
        const auto writeIndex{fifo.getNextWriteIndex()};

        // Set the read index equal to the write index; counting backwards will
        // yield delayed input samples.
        fifo.prepareReadIndex();

        // Write the current sample for each channel to the input buffer.
        // It's *so much more efficient* to avoid iterating.
        convBuffer[writeIndex][0] = buffer[n * numChannels] / static_cast<float>(INT16_MAX);
        convBuffer[writeIndex][1] = buffer[n * numChannels + 1] / static_cast<float>(INT16_MAX);

        auto sampleL{0.f}, sampleR{0.f};
        for (size_t i{0}; i < kBufferSize; i++) {
            const auto readIndex = fifo.getNextReadIndex();
            const auto irSample{irBuffer[i]};
            sampleL += irSample * convBuffer[readIndex][0];
            sampleR += irSample * convBuffer[readIndex][1];
        }
        buffer[n * numChannels] = static_cast<int16_t>(INT16_MAX * sampleL);
        buffer[n * numChannels + 1] = static_cast<int16_t>(INT16_MAX * sampleR);
    }
}

void Convolver::FIFO::prepare()
{
    readIndex = 0;
    writeIndex = 0;
}

size_t Convolver::FIFO::getNextReadIndex()
{
    const auto current{readIndex};
    if (readIndex == 0) {
        readIndex = kBufferSize;
    }
    --readIndex;
    return current;
}

size_t Convolver::FIFO::getNextWriteIndex()
{
    const auto current{writeIndex};
    ++writeIndex;
    if (writeIndex > kBufferSize - 1) {
        writeIndex = 0;
    }
    return current;
}

void Convolver::FIFO::prepareReadIndex()
{
    readIndex = writeIndex == 0 ? kBufferSize - 1 : writeIndex - 1;
}
