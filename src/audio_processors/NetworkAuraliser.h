#ifndef NETWORKAURALISER_H
#define NETWORKAURALISER_H

class NetworkAuraliser final : public AudioProcessor
{
public:
    NetworkAuraliser(AudioProcessor &client, AudioProcessor &convolver, AudioProcessor &fft)
        : client(client),
          convolver(convolver),
          fft(fft)
    {
    }

    void prepare(const uint sampleRate) override
    {
        client.prepare(sampleRate);
        convolver.prepare(sampleRate);
    }

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override
    {
        client.processAudio(buffer, numChannels, numSamples);
        // convolver.processAudio(buffer, numChannels, numSamples);
        fft.processAudio(buffer, numChannels, numSamples);
    };

private:
    AudioProcessor &client;
    AudioProcessor &convolver;
    AudioProcessor &fft;
};


#endif //NETWORKAURALISER_H
