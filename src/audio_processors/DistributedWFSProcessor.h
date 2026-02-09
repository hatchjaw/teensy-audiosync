#ifndef DISTRIBUTEDWFSPROCESSOR_H
#define DISTRIBUTEDWFSPROCESSOR_H


class WFSModule final : public AudioProcessor, public ProgramComponent
{
public:
    WFSModule(AudioProcessor &ananasClient, AudioProcessor &faustWFS, ananas::WFS::ControlContext &controlContext)
        : client(ananasClient), wfs(faustWFS), context(controlContext)
    {
    }

    void prepare(const uint sampleRate) override
    {
        client.prepare(sampleRate);
        wfs.prepare(sampleRate);
    }

    void run() override
    {
    }

    size_t printTo(Print &p) const override
    {
        return p.print("Module:            ") + AudioProcessor::printTo(p);
    }

    [[nodiscard]] size_t getNumInputs() const override { return client.getNumInputs(); }

    [[nodiscard]] size_t getNumOutputs() const override { return wfs.getNumOutputs(); }

protected:
    void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t const numFrames) override
    {
        // Advance smoothed parameters, i.e. source positions
        for (auto &sp: context.sourcePositions) {
            sp.second.getNext();
        }
        client.processAudio(inputBuffer, outputBuffer, numFrames);
        wfs.processAudio(outputBuffer, outputBuffer, numFrames);
    }

    void beginImpl() override
    {
    };

private:
    AudioProcessor &client;
    AudioProcessor &wfs;
    ananas::WFS::ControlContext &context;
};

#endif //DISTRIBUTEDWFSPROCESSOR_H
