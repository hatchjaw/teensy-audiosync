#include "FFT.h"
#include <sqrt_integer.h>
#include <utility/dspinst.h>

FFT::FFT() : window(AudioWindowHanning1024)
{
    // Initialise the CFFT instance. Seems to be necessary to do this here in
    // the constructor.
    arm_cfft_radix4_init_q15(&cfftInstance, 1024, 0, 1);
}

static void copy_to_fft_buffer(void *destination, const void *source, size_t numSamples)
{
    const auto *src = (const uint16_t *) source;
    auto *dst = (uint32_t *) destination;

    for (int i = 0; i < numSamples; i++) {
        *dst++ = *src++; // real sample plus a zero for imaginary
    }
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
    auto buf = (int16_t *) buffer;
    const auto *win = (int16_t *) window;;

    for (int i = 0; i < 1024; i++) {
        int32_t val = *buf * *win++;
        // *buf = signed_saturate_rshift(val, 16, 15);
        *buf = val >> 15;
        buf += 2;
    }
}

void FFT::prepare(uint sampleRate)
{
}

float FFT::readBin(const uint bindex) const
{
    if (bindex > 511) return 0.f;
    return (float) (output[bindex]) * (1.f / 16384.f);
}

void FFT::processAudio(int16_t *buffer, size_t numChannels, const size_t numSamples)
{
    uint32_t cycles = ARM_DWT_CYCCNT;

    switch (state) {
        case 0:
            bufferOfBuffers[0] = buffer;
            state = 1;
            break;
        case 1:
            bufferOfBuffers[1] = buffer;
            state = 2;
            break;
        case 2:
            bufferOfBuffers[2] = buffer;
            state = 3;
            break;
        case 3:
            bufferOfBuffers[3] = buffer;
            state = 4;
            break;
        case 4:
            bufferOfBuffers[4] = buffer;
            copy_to_fft_buffer(fftBuffer + 0x000, bufferOfBuffers[0], numSamples);
            copy_to_fft_buffer(fftBuffer + 0x400, bufferOfBuffers[4], numSamples);
            state = 5;
            break;
        case 5:
            bufferOfBuffers[5] = buffer;
            copy_to_fft_buffer(fftBuffer + 0x100, bufferOfBuffers[1], numSamples);
            copy_to_fft_buffer(fftBuffer + 0x500, bufferOfBuffers[5], numSamples);
            state = 6;
            break;
        case 6:
            bufferOfBuffers[6] = buffer;
            copy_to_fft_buffer(fftBuffer + 0x200, bufferOfBuffers[2], numSamples);
            copy_to_fft_buffer(fftBuffer + 0x600, bufferOfBuffers[6], numSamples);
            state = 7;
            break;
        case 7:
            bufferOfBuffers[7] = buffer;
            copy_to_fft_buffer(fftBuffer + 0x300, bufferOfBuffers[3], numSamples);
            copy_to_fft_buffer(fftBuffer + 0x700, bufferOfBuffers[7], numSamples);

            if (window) {
                apply_window_to_fft_buffer(fftBuffer, window);
            }

            arm_cfft_radix4_q15(&cfftInstance, fftBuffer);

        // TODO: support averaging multiple copies
            for (int i = 0; i < 512; i++) {
                uint32_t tmp = *((uint32_t *) fftBuffer + i); // real & imag
                uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
                output[i] = sqrt_uint32_approx(magsq);
            }

            bufferOfBuffers[0] = bufferOfBuffers[4];
            bufferOfBuffers[1] = bufferOfBuffers[5];
            bufferOfBuffers[2] = bufferOfBuffers[6];
            bufferOfBuffers[3] = bufferOfBuffers[7];
            state = 4;

        // Serial.print("FFT: ");
        // for (auto i{0}; i < 40; i++) {
        //     auto n{readBin(i)};
        //     if (n >= 0.01) {
        //         Serial.print(n);
        //         Serial.print(" ");
        //     } else {
        //         Serial.print("  -  "); // don't print "0.00"
        //     }
        // }
        // Serial.println();

            break;
    }

    cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
    currentCycles = cycles;
    if (currentCycles > maxCycles) {
        maxCycles = currentCycles;
    }
}
