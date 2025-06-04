#ifndef NETWORKAURALISER_H
#define NETWORKAURALISER_H

#include <audio_processors/Convolver.h>

class NetworkAuraliser final : public AudioProcessor
{
public:
    NetworkAuraliser(ananas::AudioClient &client, Convolver &convolver)
        : client(client),
          convolver(convolver)
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
        convolver.processAudio(buffer, numChannels, numSamples);

        cycles = (ARM_DWT_CYCCNT - cycles) >> 6;
        currentCycles = cycles;
        if (currentCycles > maxCycles) {
            maxCycles = currentCycles;
        }
    }

private:
    ananas::AudioClient &client;
    Convolver &convolver;
};


#endif //NETWORKAURALISER_H
