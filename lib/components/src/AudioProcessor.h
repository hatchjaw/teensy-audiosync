#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>

class AudioProcessor : public Printable
{
public:
    AudioProcessor(size_t nInputs, size_t nOutputs);

    ~AudioProcessor();

    virtual void prepare(uint sampleRate) = 0;

    virtual void processAudio(int16_t *buffer, size_t numChannels, size_t numFrames) final;

    virtual void processAudioV2(size_t numFrames) final;

    void getOutput(int16_t **dest, size_t numChannels, size_t numFrames) const;

    void getOutputInterleaved(int16_t *dest, size_t numChannels, size_t numFrames) const;

    size_t printTo(Print &p) const override;

protected:
    virtual void processImpl(int16_t *buffer, size_t numChannels, size_t numFrames) = 0;

    virtual void processImplV2(size_t numFrames) {}

    size_t numInputs, numOutputs;
    int16_t **inputBuffer, **outputBuffer;

private:
    uint16_t currentCycles{0}, maxCycles{0};
};

#endif //AUDIOPROCESSOR_H
