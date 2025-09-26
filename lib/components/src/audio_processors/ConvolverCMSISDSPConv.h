#ifndef CONVOLVERCMSISDSPCONV_H
#define CONVOLVERCMSISDSPCONV_H


#include <arm_math.h>
#include <AudioProcessor.h>

class ConvolverCMSISDSPConv final : public AudioProcessor
{
public:
    ConvolverCMSISDSPConv();

    void prepare(uint sampleRate) override;

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override;

public:
    /**
     * It doesn't appear to be possible to go any higher than 256. I tried
     * convolving by blocks (of 128 and 256) but to no avail. 10% CPU time @256.
     */
    static constexpr size_t kIRSize{256};
    // /**
    //  * Set the size of the IR buffer to the smallest multiple of 128 greater
    //  * than or equal to the size of the IR.
    //  */
    // static constexpr size_t kIRBufferSize{kIRSize + 127 & ~127};
    static constexpr size_t kNumChannels{2};

private:
    static constexpr size_t kStateSize{128 + kIRSize - 1};
    q15_t leftChannel[128] __attribute__((aligned(4)));
    q15_t rightChannel[128] __attribute__((aligned(4)));
    q15_t stateL[kStateSize]{}, stateR[kStateSize]{};
};


#endif //CONVOLVERCMSISDSPCONV_H
