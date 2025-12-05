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
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <Announcer.h>
#include <ClientListener.h>

void ptpSyncInterrupt();

void ptpAnnounceInterrupt();

static void interrupt_1588_timer();

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
// AudioOutputUSB usbOut;
AudioOutputI2S out;
PulsePerSecond pps;
AudioConnection c0(pps, 0, out, 0);
// AudioConnection c1(usb, 0, out, 0);
AudioConnection c2(usbIn, 1, out, 1);
float prevVol{0.f};

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};
IntervalTimer ptpSyncTimer;
IntervalTimer ptpAnnounceTimer;
bool clockAuthority{true}, clockSubscriber{false}, p2p{false};
l3PTP ptp{clockAuthority, clockSubscriber, p2p};
qindesign::network::EthernetUDP socket;

ananas::Announcer<ananas::AuthorityAnnouncePacket> authorityAnnouncer{
    ananas::Constants::AuthorityAnnounceMulticastIP,
    ananas::Constants::AuthorityAnnouncePort,
    ananas::Constants::AuthorityAnnounceIntervalMs
};

ananas::ClientListener clientListener;
elapsedMillis rateAdjustInterval;

bool sync = false;
bool announce = false;
int noPPSCount = 0;

NanoTime interrupt_s = 0;
NanoTime interrupt_ns = 0;
NanoTime pps_s = 0;
NanoTime pps_ns = 0;

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {}
#endif

    Serial.begin(0);

    // Setup networking
    qindesign::network::Ethernet.setHostname("t41ptpauthority");
    qindesign::network::Ethernet.macAddress(mac);
    staticIP[2] = mac[4];
    staticIP[3] = mac[5];
    qindesign::network::Ethernet.begin(staticIP, subnetMask, gateway);
    qindesign::network::EthernetIEEE1588.begin();

    qindesign::network::Ethernet.onLinkState([](const bool state)
    {
        Serial.printf("[Ethernet] Link %dMbps %s\n", qindesign::network::Ethernet.linkSpeed(), state ? "ON" : "OFF");
        if (state) {
            ptp.begin();
            ptpSyncTimer.begin(ptpSyncInterrupt, 1000000);
            ptpAnnounceTimer.begin(ptpAnnounceInterrupt, 1000000);
            authorityAnnouncer.connect();
            clientListener.connect();
        }
    });

    CORE_PIN24_CONFIG = 6;
    qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S - 60);
    qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare);
    qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25);

    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true);
    attachInterruptVector(IRQ_ENET_TIMER, interrupt_1588_timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling

    AudioMemory(10);
    audioShield.enable();
    audioShield.volume(.75f);

    authorityAnnouncer.begin();
    clientListener.begin();
}

void ptpSyncInterrupt()
{
    if (noPPSCount > 5) {
        sync = true;
    } else {
        noPPSCount++;
    }
}

void ptpAnnounceInterrupt()
{
    announce = true;
}

void interrupt_1588_timer()
{
    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        __DSB();
        return;
    }
    qindesign::network::EthernetIEEE1588.getChannelCompareValue(1, t);

    t = ((NanoTime) t + NS_PER_S - 60) % NS_PER_S;

    timespec ts;
    qindesign::network::EthernetIEEE1588.readTimer(ts);

    if (ts.tv_nsec < 100 * 1000 * 1000 && t > 900 * 1000 * 1000) {
        pps_s = ts.tv_sec;
        interrupt_s = ts.tv_sec - 1;
    } else {
        interrupt_s = ts.tv_sec;
        if (ts.tv_nsec < 500 * 1000 * 1000) {
            pps_s = ts.tv_sec;
        } else {
            pps_s = ts.tv_sec + 1;
        }
    }

    interrupt_ns = t;
    pps_ns = 0;

    __DSB();
}

void loop()
{
    if (announce) {
        announce = false;
        ptp.announceMessage();
    }
    if (sync) {
        sync = false;
        ptp.syncMessage();
    }
    ptp.update();

    authorityAnnouncer.run();
    clientListener.run();

    authorityAnnouncer.txPacket.numClients = clientListener.getNumClients(); // TODO: replace with callback f'n
    authorityAnnouncer.txPacket.avgBufferFillPercent = clientListener.getAvgBufferFill();

    // if (rateAdjustInterval > 1000) {
    //     rateAdjustInterval = 0;
    //
    //     if (clientListener.getAvgBufferFill() > 52) {
    //         usbIn.decreaseFeedbackAccumulator();
    //     } else if (clientListener.getAvgBufferFill() < 48) {
    //         usbIn.increaseFeedbackAccumulator();
    //     }
    // }

    authorityAnnouncer.txPacket.numUnderruns = AudioInputUSB::getNumUnderruns();
    authorityAnnouncer.txPacket.numOverflows = AudioInputUSB::getNumOverflows();
    authorityAnnouncer.txPacket.usbFeedbackAccumulator = AudioInputUSB::getFeedbackAccumulator();
}
