#include <AudioSystemManager.h>
#include <AnanasClient.h>
#include <ControlDataListener.h>
#include <smalloc.h>
#include <wfs.h>
#include <program_components/ComponentManager.h>
#include <program_components/EthernetManager.h>
#include <program_components/PTPManager.h>
#include "audio_processors/DistributedWFSProcessor.h"

extern "C" uint8_t external_psram_size;

volatile bool ptpLock{false};
AudioSystemConfig config{
    ananas::Constants::AudioBlockFrames,
    ananas::Constants::AudioSamplingRate,
    ClockRole::Subscriber
};
AudioSystemManager audioSystemManager{config};
PTPManager ptpManager{ClockRole::Subscriber};
ananas::AudioClient ananasClient{ananas::Constants::AudioSocketParams};
ananas::WFS::ControlContext context;
ananas::WFS::ControlDataListener controlDataListener{context};
wfs wfs;
WFSModule wfsModule{ananasClient, wfs, context};
std::vector<NetworkProcessor *> networkProcessors{
    &ptpManager,
    &ananasClient,
    &controlDataListener
};
EthernetManager ethernetManager{"t41wfsmodule", networkProcessors};
std::vector<ProgramComponent *> programComponents{
    &ethernetManager,
    &ptpManager,
    &audioSystemManager,
    &ananasClient,
    &wfs,
    &wfsModule,
    &controlDataListener
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
    });

    context.moduleID.onChange = [](const int value)
    {
        wfs.setParamValue("moduleID", static_cast<float>(value));
    };
    context.speakerSpacing.onChange = [](const float value)
    {
        wfs.setParamValue("spacing", value);
    };
    // Set up source positions.
    char path[5];
    for (size_t i{0}; i < ananas::Constants::MaxChannels; ++i) {
        sprintf(path, "%d/x", i);
        context.sourcePositions.insert(ananas::WFS::SourcePositionsMap::value_type(
                path,
                ananas::SmoothedValue<float>{0., .975f})
        );
        sprintf(path, "%d/y", i);
        context.sourcePositions.insert(ananas::WFS::SourcePositionsMap::value_type(
                path,
                ananas::SmoothedValue<float>{0., .975f})
        );
    }
    for (auto &sp: context.sourcePositions) {
        // // If smoothing in Faust with si.smoo:
        // sp.second.onSet = [sp](const float value)
        // {
        //     // Serial.printf("Updating %s: %f\n", sp.first.c_str(), value);
        //     wfs.setParamValue(sp.first, value);
        // };

        // If smoothing outside of Faust:
        sp.second.onChange = [sp](float value)
        {
            // Serial.printf("%s changed: %.9f\n", sp.first.c_str(), value);
            value = ananas::Utils::clamp(value, -1.f, 1.f);
            wfs.setParamValue(sp.first, value);
        };
    }

    AudioSystemManager::setAudioProcessor(&wfsModule);
    componentManager.begin();
}

void loop()
{
    componentManager.run();

    ananasClient.setPercentCPU(wfsModule.getCurrentPercentCPU());
    ananasClient.setModuleID(static_cast<uint16_t>(wfs.getParamValue("moduleID")));
}
