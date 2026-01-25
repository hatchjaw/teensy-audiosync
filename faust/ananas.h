#ifndef FAUSTARCHITECTURE_H
#define FAUSTARCHITECTURE_H

#include <string>
#include <AudioProcessor.h>
#include <AnanasUtils.h>

#define fprintf(X, Y, Z) Serial.printf(Y, Z)

class dsp;
class MapUI;

#if MIDICTRL
class MidiUI;
class ananas_midi;
#endif

class AudioFaust : public AudioProcessor
{
public:
    AudioFaust();

    ~AudioFaust();

    void prepare(uint sampleRate) override;

    size_t printTo(Print &p) const override;

    void setParamValue(const std::string &path, float value) const;

    float getParamValue(const std::string &path) const;

    [[nodiscard]] size_t getNumInputs() const override;

    [[nodiscard]] size_t getNumOutputs() const override;

protected:
    void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

private:
//    DMAMEM inline static float fInChannelData[ananas::Constants::MaxChannels][ananas::Constants::AudioBlockFrames]{};
//    DMAMEM inline static float fOutChannelData[ananas::Constants::NumOutputChannels][ananas::Constants::AudioBlockFrames]{};
//    inline static float *fInChannel[ananas::Constants::MaxChannels]{};
//    inline static float *fOutChannel[ananas::Constants::NumOutputChannels]{};
    float **fInChannel;
    float **fOutChannel;
    MapUI *fUI;
#if MIDICTRL
        ananas_midi* fMIDIHandler;
        MidiUI* fMIDIInterface;
#endif
    dsp *fDSP;
};

#endif //FAUSTARCHITECTURE_H
