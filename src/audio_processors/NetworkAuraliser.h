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

    void processAudio(int16_t *buffer, const size_t numChannels, const size_t numSamples) override
    {
        uint32_t cycles = ARM_DWT_CYCCNT;

        client.processAudio(buffer, numChannels, numSamples);
        // convolver.processAudio(buffer, numChannels, numSamples);
        fft.processAudio(buffer, numChannels, numSamples);

        cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
        currentCycles = cycles;
        if (currentCycles > maxCycles) {
            maxCycles = currentCycles;
        }
    }

private:
    AudioProcessor &client;
    AudioProcessor &convolver;
    AudioProcessor &fft;
};


#endif //NETWORKAURALISER_H
