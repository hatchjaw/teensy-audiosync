/**
 * This sketch describes a Teensy running as a USB audio device *and* as a PTP
 * clock authority. If this works as intended, a Teensy running this sketch
 * should serve as an authoritative source of time for:
 *
 * - audio interrupts on a general purpose machine acting as a multicast
 * networked audio server.
 * - a network of Teensies running as PTP clock subscribers *and* as recipients
 * for data transmitted by the multicast audio server.
 */

#include <AnanasPacket.h>
#include <Audio.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <Announcer.h>
#include <program_components/ComponentManager.h>
#include <program_components/EthernetManager.h>
#include <program_components/PTPManager.h>

class PulsePerSecond : public AudioStream
{
public:
    PulsePerSecond() : AudioStream(0, nullptr)
    {
    }

private:
    void update() override
    {
        if (++numBuffers == kBuffersPerSec) {
            numBuffers = 0;
            if (auto *block{allocate()}) {
                memset(block->data, 0, sizeof(block->data));
                block->data[0] = INT16_MAX - 1; // Trigger logic analyser at value 0x7FFE
                transmit(block, 0);
                release(block);
            }
        }
    }

    static constexpr uint kBuffersPerSec{AUDIO_SAMPLE_RATE_EXACT / AUDIO_BLOCK_SAMPLES};
    uint numBuffers{0};
};

AudioControlSGTL5000 audioShield;
AudioInputUSB usbIn;
AudioOutputI2S out;
PulsePerSecond pps;
AudioConnection c0(pps, 0, out, 0);
AudioConnection c2(usbIn, 1, out, 1);

PTPManager ptpManager{ClockRole::Authority};
ananas::Announcer<ananas::AuthorityAnnouncePacket> authorityAnnouncer{
    ananas::Constants::AuthorityAnnounceSocketParams
};
std::vector<NetworkProcessor *> networkProcessors{
    &ptpManager,
    &authorityAnnouncer
};
EthernetManager ethernetManager{"t41clocksubscriber", networkProcessors};
std::vector<ProgramComponent *> programComponents{
    &ethernetManager,
    &ptpManager,
    &authorityAnnouncer
};
ComponentManager componentManager{programComponents};

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {}
#endif

    Serial.begin(0);

    AudioMemory(10);
    audioShield.enable();
    audioShield.volume(.75f);

    componentManager.begin();
}

// void interrupt_1588_timer()
// {
//     uint32_t t;
//     if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
//         __DSB();
//         return;
//     }
//     qindesign::network::EthernetIEEE1588.getChannelCompareValue(1, t);
//
//     t = ((NanoTime) t + NS_PER_S - 60) % NS_PER_S;
//
//     timespec ts;
//     qindesign::network::EthernetIEEE1588.readTimer(ts);
//
//     if (ts.tv_nsec < 100 * 1000 * 1000 && t > 900 * 1000 * 1000) {
//         pps_s = ts.tv_sec;
//         interrupt_s = ts.tv_sec - 1;
//     } else {
//         interrupt_s = ts.tv_sec;
//         if (ts.tv_nsec < 500 * 1000 * 1000) {
//             pps_s = ts.tv_sec;
//         } else {
//             pps_s = ts.tv_sec + 1;
//         }
//     }
//
//     interrupt_ns = t;
//     pps_ns = 0;
//
//     __DSB();
// }

void loop()
{
    componentManager.run();

    authorityAnnouncer.txPacket.numUnderruns = AudioInputUSB::getNumUnderruns();
    authorityAnnouncer.txPacket.numOverflows = AudioInputUSB::getNumOverflows();
    authorityAnnouncer.txPacket.usbFeedbackAccumulator = AudioInputUSB::getFeedbackAccumulator();
}
