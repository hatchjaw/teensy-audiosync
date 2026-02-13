#include <AudioSystemManager.h>
#include <AnanasClient.h>
#include <program_components/ComponentManager.h>
#include <program_components/EthernetManager.h>
#include <program_components/PTPManager.h>

extern "C" uint8_t external_psram_size;

AudioSystemConfig config{
    ananas::Constants::AudioBlockFrames,
    ananas::Constants::AudioSamplingRate,
    ClockRole::Subscriber
};
AudioSystemManager audioSystemManager{config};
PTPManager ptpManager{ClockRole::Subscriber};
ananas::AudioClient ananasClient{ananas::Constants::AudioSocketParams};
std::vector<NetworkProcessor *> networkProcessors{
    &ptpManager,
    &ananasClient
};
EthernetManager ethernetManager{"t41clocksubscriber", networkProcessors};
std::vector<ProgramComponent *> programComponents{
    &ethernetManager,
    &ptpManager,
    &audioSystemManager,
    &ananasClient
};
ComponentManager componentManager{programComponents};

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {
    }
#endif

    Serial.begin(2000000);

    ptpManager.onPTPLock([](const bool isLocked, const NanoTime compare, const NanoTime now)
    {
        ananasClient.setIsPtpLocked(isLocked);

        if (isLocked && !audioSystemManager.isClockRunning()) {
            audioSystemManager.startClock();
            Serial.print("Subscriber start audio clock ");
            printTime(compare);
            Serial.println();
        } else if (audioSystemManager.isClockRunning()) {
            ananasClient.adjustBufferReadIndex(now);
        }
    });

    ptpManager.onPTPControllerUpdated([](const double adjust)
    {
        audioSystemManager.adjustClock(adjust);
        ananasClient.setReportedSamplingRate(config.getExactSamplingRate());
    });

    audioSystemManager.onInvalidSamplingRate([]
    {
        Serial.printf("Resetting PTP and stopping audio.\n");
        ptpManager.resetPTP();
        audioSystemManager.stopClock();
    });

    audioSystemManager.onAudioPtpOffsetChanged([](const long offset)
    {
        ananasClient.setAudioPtpOffsetNs(offset);
        // TODO: update PLL4 NUM more frequently?
    });

    AudioSystemManager::setAudioProcessor(&ananasClient);
    componentManager.begin();
}

elapsedMillis elapsed;
static constexpr int reportInterval{1000};

void loop()
{
    componentManager.run();
}
