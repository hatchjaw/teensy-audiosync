#include "ConvolverCMSISDSPConv.h"

extern "C" uint8_t external_psram_size;

// Size: smallest multiple of 128 greater than kIRSize.
EXTMEM q15_t irBuffer[ConvolverCMSISDSPConv::kIRSize] __attribute__ ((aligned (4)));

ConvolverCMSISDSPConv::ConvolverCMSISDSPConv()
{
}

void ConvolverCMSISDSPConv::prepare(uint sampleRate)
{
    if (external_psram_size <= 0) {
        Serial.println("Convolver: no external PSRAM found; no processing will be applied.");
        return;
    }

    const float frequency = 22 * 24 / (float) (((CCM_CBCMR >> 29) & 7) + 1);
    Serial.printf("\nCCM_CBCMR=%08X (%.1f MHz)\n", CCM_CBCMR, frequency);

    Serial.printf("\nConvolver: IR buffer at address 0x%X\n",
                  irBuffer);

    // Initialise IR buffer.
    memset(irBuffer, 0, sizeof(irBuffer));
    irBuffer[0] = INT16_MAX * 1 / 2;
    irBuffer[kIRSize - 1] = INT16_MAX * -1 / 4;

    // Initialise state buffers.
    memset(stateL, 0, sizeof(stateL));
    memset(stateR, 0, sizeof(stateR));
    memset(leftChannel, 0, sizeof(leftChannel));
    memset(rightChannel, 0, sizeof(rightChannel));
}

void ConvolverCMSISDSPConv::processImpl(int16_t *buffer, size_t numChannels, size_t numSamples)
{
    if (external_psram_size <= 0) return;

    // Temporary buffers for each channel
    q15_t leftOutput[numSamples + kIRSize - 1];
    q15_t rightOutput[numSamples + kIRSize - 1];

    // De-interleave
    for (size_t i{0}; i < numSamples; i++) {
        leftChannel[i] = buffer[i * 2];
        rightChannel[i] = buffer[i * 2 + 1];
    }

    // Process each channel separately
    arm_conv_fast_q15(leftChannel, numSamples, irBuffer, kIRSize, leftOutput);
    arm_conv_fast_q15(rightChannel, numSamples, irBuffer, kIRSize, rightOutput);

    // Overlap-add and re-interleave results
    for (size_t i{0}; i < numSamples; i++) {
        buffer[i * 2] = (q15_t) __QADD16(leftOutput[i], stateL[i]);
        buffer[i * 2 + 1] = (q15_t) __QADD16(rightOutput[i], stateR[i]);
    }

    memcpy(stateL, leftOutput + numSamples, (kStateSize - numSamples) * sizeof(q15_t));
    memcpy(stateR, rightOutput + numSamples, (kStateSize - numSamples) * sizeof(q15_t));
}
