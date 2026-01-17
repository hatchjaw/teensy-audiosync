#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <Arduino.h>

class AudioProcessor : public Printable
{
public:
    virtual void prepare(uint sampleRate) = 0;

    virtual void processAudio(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) final;

    size_t printTo(Print &p) const override;

    [[nodiscard]] float getCurrentPercentCPU() const;

    [[nodiscard]] virtual size_t getNumInputs() const = 0;

    [[nodiscard]] virtual size_t getNumOutputs() const = 0;

protected:
    virtual void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) = 0;

private:
    uint16_t currentCycles{0}, maxCycles{0};
};

#endif //AUDIOPROCESSOR_H
