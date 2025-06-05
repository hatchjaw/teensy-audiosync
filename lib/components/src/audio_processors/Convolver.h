#ifndef CONVOLVER_H
#define CONVOLVER_H

#include <AudioProcessor.h>

class Convolver final : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) override;

    static constexpr size_t kBufferSize{750};
    static constexpr size_t kNumChannels{2};

private:
    class FIFO
    {
    public:
        void prepare();

        size_t getNextReadIndex();

        size_t getNextWriteIndex();

        void prepareReadIndex();

    private:
        size_t readIndex{0}, writeIndex{0};
    };

    FIFO fifo;
    uint sampleCount{0};

    // float irBuffer[Convolver::kBufferSize]{};
    // float convBuffer[Convolver::kBufferSize][Convolver::kNumChannels]{};
};

#endif //CONVOLVER_H
