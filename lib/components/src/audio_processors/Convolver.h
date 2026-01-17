#ifndef CONVOLVER_H
#define CONVOLVER_H

#include <AudioProcessor.h>

class Convolver final : public AudioProcessor
{
public:
    void prepare(uint sampleRate) override;

    static constexpr size_t kBufferSize{750};
    static constexpr size_t kNumChannels{2};

protected:
    void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

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
};

#endif //CONVOLVER_H
