#ifndef DISTRIBUTEDWFSPROCESSOR_H
#define DISTRIBUTEDWFSPROCESSOR_H

class WFSModule final : public AudioProcessor
{
public:
    WFSModule(AudioProcessor &ananasClient, AudioProcessor &faustWFS)
        : AudioProcessor(0, 2), client(ananasClient), wfs(faustWFS)
    {
    }

    void prepare(const uint sampleRate) override
    {
        client.prepare(sampleRate);
        wfs.prepare(sampleRate);
    }

    size_t printTo(Print &p) const override
    {
        return p.print("Module:            ") + AudioProcessor::printTo(p);
    }

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numFrames) override
    {
    }

    void processImplV2(const size_t numFrames) override
    {
        client.processAudioV2(numFrames);
        client.getOutput(wfs.getInput(), wfs.getNumInputs(), numFrames);
        wfs.processAudioV2(numFrames);
        wfs.getOutput(outputBuffer, wfs.getNumOutputs(), numFrames);
    }

private:
    AudioProcessor &client;
    AudioProcessor &wfs;
};

#endif //DISTRIBUTEDWFSPROCESSOR_H
