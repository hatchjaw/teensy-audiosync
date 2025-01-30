#include <Audio.h>
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioSystemManager.h>
#include <AnanasClient.h>

static void interrupt_1588_timer();

volatile bool shouldEnableAudio{false};
AudioSystemConfig config{
    AUDIO_BLOCK_SAMPLES,
    AUDIO_SAMPLE_RATE_EXACT,
    AudioSystemConfig::ClockRole::Subscriber
};
AudioSystemManager audioSystemManager{config};
ananas::AudioClient ananasClient;

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};

bool connected{false};
bool p2p = false;

l2PTP ptp(
    config.k_ClockRole == AudioSystemConfig::ClockRole::Authority,
    config.k_ClockRole == AudioSystemConfig::ClockRole::Subscriber,
    p2p
);

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {}
#endif

    Serial.begin(2000000);
    pinMode(13, OUTPUT);

    // Setup networking
    qindesign::network::Ethernet.setHostname("t41ptpsubscriber");
    qindesign::network::Ethernet.macAddress(mac);
    staticIP[2] = mac[4];
    staticIP[3] = mac[5];
    qindesign::network::Ethernet.begin(staticIP, subnetMask, gateway);
    qindesign::network::EthernetIEEE1588.begin();

    qindesign::network::Ethernet.onLinkState([](bool state)
    {
        Serial.printf("[Ethernet] Link %dMbps %s\n", qindesign::network::Ethernet.linkSpeed(), state ? "ON" : "OFF");
        connected = state;
        if (state) {
            ptp.begin();
            ananasClient.connect();
        }
    });

    // PPS Out
    // peripherial: ENET_1588_EVENT1_OUT
    // IOMUX: ALT6
    // teensy pin: 24
    CORE_PIN24_CONFIG = 6;
    qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S - 60);
    qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare);
    qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25);

    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true);
    attachInterruptVector(IRQ_ENET_TIMER, interrupt_1588_timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling

    // Set up audio
    AudioSystemManager::setAudioProcessor(&ananasClient);
    audioSystemManager.begin();
    ananasClient.begin();

    // Report *after* setting everything up.
    Serial.println("Clock subscriber");
    Serial.printf("Mac address:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("IP:            ");
    Serial.println(qindesign::network::Ethernet.localIP());
    Serial.println();
}

NanoTime interrupt_s = 0;
NanoTime interrupt_ns = 0;
NanoTime pps_s = 0;
NanoTime pps_ns = 0;

int numRead{0};

void loop()
{
    ptp.update();

    // Six consecutive offsets below 100 ns sets pin 13 high to switch on the LED
    digitalWrite(13, ptp.getLockCount() > 5 ? HIGH : LOW);

    ananasClient.run();

    // if (started && !paused && elapsed > 10000) {
    //     audioSystemManager.stopClock();
    //     Serial.println("Subscriber stop audio clock");
    //     paused = true;
    // } else if (started && paused && elapsed > 10100) {
    //     audioSystemManager.startClock();
    //     elapsed = 0;
    //     Serial.println("Subscriber start audio clock");
    //     paused = false;
    // }
}

static void interrupt_1588_timer()
{
    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        // Allow write to complete so the interrupt doesn't fire twice
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

    // Serial.printf("Adjust: %f\n"
    //               "Accum: %d\n"
    //               "Drift: %f\n"
    //               "Delay: %" PRId64 "\n"
    //               "Offset: %" PRId64 "\n",
    //               ptp.getAdjust(),
    //               ptp.getAccumulatedOffset(),
    //               ptp.getDrift(),
    //               ptp.getDelay(),
    //               ptp.getOffset());

    audioSystemManager.adjustClock(ptp.getAdjust() + (double) ptp.getOffset());
    // audioSystemManager.adjustClock(ptp.getAdjust() - 5e-4 * ptp.getDrift());
    // audioSystemManager.adjustClock(ptp.getAdjust());

    // Start audio at t = 10s
    // Only works if started at *around the same time* as the clock authority.
    // For arbitrary start times, it'll be necessary to measure the initial
    // offset, i.e. first large time adjustment.
    // shouldEnableAudio = interrupt_s >= 10;
    // const auto offset{ptp.getOffset()};
    shouldEnableAudio = ptp.getLockCount() > 0;// ptp.getDelay() != 0 && offset < 1000 && offset > -1000;

    NanoTime now{interrupt_s * NS_PER_S + interrupt_ns};

    if (shouldEnableAudio && !audioSystemManager.isClockRunning()) {
        // audioSystemManager.begin();
        audioSystemManager.startClock();
        Serial.print("Subscriber start audio clock ");
        ananas::Utils::printTime(now);
        Serial.println();
    } else if (!shouldEnableAudio && audioSystemManager.isClockRunning()) {
        // audioSystemManager.stopClock();
        // ananas::Utils::printTime(now);
        // Serial.println();
    } else if (audioSystemManager.isClockRunning()) {
//        ananasClient.printStats();
        ananasClient.adjustBufferReadIndex(now);
    }

    // Allow write to complete so the interrupt doesn't fire twice
    __DSB();
}
