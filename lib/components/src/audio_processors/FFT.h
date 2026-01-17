#ifndef FFT_H
#define FFT_H

#include <arm_math.h>
#include <AudioProcessor.h>

// windows.c
extern "C" {
extern const int16_t AudioWindowHanning1024[];
extern const int16_t AudioWindowBartlett1024[];
extern const int16_t AudioWindowBlackman1024[];
extern const int16_t AudioWindowFlattop1024[];
extern const int16_t AudioWindowBlackmanHarris1024[];
extern const int16_t AudioWindowNuttall1024[];
extern const int16_t AudioWindowBlackmanNuttall1024[];
extern const int16_t AudioWindowWelch1024[];
extern const int16_t AudioWindowHamming1024[];
extern const int16_t AudioWindowCosine1024[];
extern const int16_t AudioWindowTukey1024[];
}

class FFT final : public AudioProcessor
{
public:
    FFT();

    void prepare(uint sampleRate) override;

    float readBin(uint bindex) const;

protected:
    void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

private:
    const int16_t *window{nullptr};
    /**
     * A container for enough consecutive buffers to calculate a full 1024-point
     * FFT. Assumes a buffer size of 128 (and that the aim is to compute 1024
     * points). TODO: get rid of all these hard-coded array sizes.
     */
    int16_t *bufferOfBuffers[8]{};
    /**
     * A buffer to hold input signal data, to be copied from the buffer of
     * buffers. Must be twice the FFT size. Aligned for use with ARM SIMD
     * operations.
     */
    int16_t fftBuffer[2048] __attribute__ ((aligned (4)));
    /**
     * An output buffer to store the magnitudes of the FFT bins (0-Nyquist).
     * Aligned... but I'm not certain that it needs to be.
     */
    uint16_t output[512] __attribute__ ((aligned (4)));
    /**
     * Processing state; it's going to be necessary to wait until enough samples
     * are present for a full 1024-point FFT.
     */
    uint8_t state{0};
    /**
     * Instance of a data structure for a fixed-point CFFT radix-4 algorithm.
     */
    arm_cfft_radix4_instance_q15 cfftInstance;
};


#endif //FFT_H
