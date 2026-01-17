#ifndef FAUSTARCHITECTURE_H
#define FAUSTARCHITECTURE_H

#include <string>
#include <AudioProcessor.h>

#define fprintf(X, Y, Z) Serial.printf(Y, Z)

class dsp;
class MapUI;

#if MIDICTRL
class MidiUI;
class ananas_midi;
#endif

class wfs : public AudioProcessor
{
public:
    wfs();

    ~wfs();

    void prepare(uint sampleRate) override;

    size_t printTo(Print &p) const override;

    void setParamValue(const std::string &path, float value);

    float getParamValue(const std::string &path);

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numFrames) override;

    void processImplV2(size_t numFrames) override;

private:
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
