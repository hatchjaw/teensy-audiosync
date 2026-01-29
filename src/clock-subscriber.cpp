#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioSystemManager.h>
#include <AnanasClient.h>
#include <arm_math.h>

extern "C" uint8_t external_psram_size;

static void interrupt_1588_timer();

volatile bool ptpLock{false};
AudioSystemConfig config{
    ananas::Constants::AudioBlockFrames,
    ananas::Constants::AudioSamplingRate,
    AudioSystemConfig::ClockRole::Subscriber
};
AudioSystemManager audioSystemManager{config};
ananas::AudioClient ananasClient;

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};

bool p2p = false;

l3PTP ptp(
    config.kClockRole == AudioSystemConfig::ClockRole::Authority,
    config.kClockRole == AudioSystemConfig::ClockRole::Subscriber,
    p2p
);

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {
    }
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

    qindesign::network::Ethernet.onLinkState([](const bool state)
    {
        Serial.printf("\n[Ethernet] Link %dMbps %s\n", qindesign::network::Ethernet.linkSpeed(), state ? "ON" : "OFF");
        if (state) {
            ptp.begin();
            ananasClient.connect();
        }
    });

    ptp.onControllerUpdated([](const double adjust)
    {
        audioSystemManager.adjustClock(adjust);
        ananasClient.setReportedSamplingRate(config.getExactSamplingRate());
    });

    audioSystemManager.onInvalidSamplingRate([]
    {
        Serial.printf("Resetting PTP and stopping audio.\n");
        ptp.reset();
        audioSystemManager.stopClock();
    });

    audioSystemManager.onAudioPtpOffsetChanged([](const long offset)
    {
        ananasClient.setAudioPtpOffsetNs(offset);
    });

    // PPS Out
    // peripherial: ENET_1588_EVENT1_OUT
    // IOMUX: ALT6
    // teensy pin: 24
    CORE_PIN24_CONFIG = 6;
    qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S);
    qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare);
    qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25);
    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true);

    attachInterruptVector(IRQ_ENET_TIMER, interrupt_1588_timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling

    // Set up audio
    AudioSystemManager::setAudioProcessor(&ananasClient);
    // AudioSystemManager::setAudioProcessor(&networkAuraliser);
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

elapsedMillis elapsed;
static constexpr int reportInterval{1000};

void loop()
{
    ptp.update();

    // Six consecutive offsets below 100 ns sets pin 13 high to switch on the LED
    digitalWrite(13, ptp.getLockCount() > 5 ? HIGH : LOW);

    audioSystemManager.run();
    ananasClient.run();

    if (elapsed > reportInterval) {
        elapsed = 0;
        Serial.print("\n"
            "=============================================================================="
            "\nIP: ");
        Serial.print(qindesign::network::Ethernet.localIP());
        Serial.printf(" | MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.printf(" | SN: %" PRIu32, ananasClient.getSerialNumber());
        Serial.printf(external_psram_size ? " | PSRAM: %" PRIu8 " MB\n" : "\n", external_psram_size);
        Serial.println(audioSystemManager);
        Serial.println(ananasClient);
        Serial.println("==============================================================================");
    }
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

    t %= NS_PER_S;

    timespec ts{};
    qindesign::network::EthernetIEEE1588.readTimer(ts);

    // The channel compare value may be close to, but not exactly, 1e9.
    // If it's just less than 1e9, the tv_sec part of the timespec read from the
    // timer will be 1 second too great.
    // What circumstances exist where t < 9e8 and tv_nsec > 1e8 I don't know;
    // perhaps when using an external PPS source.
    // The subscriber probably doesn't need to perform this check.
    if (ts.tv_nsec < 100 * 1000 * 1000 && t > 900 * 1000 * 1000) {
        interrupt_s = ts.tv_sec - 1;
        // Serial.printf("t (channel compare): %" PRIu32 ", ts.tv_nsec: %" PRId32 "\n", t, ts.tv_nsec);
        // Serial.printf("So interrupt_s = %" PRId64 " = %" PRId32 " - 1\n", interrupt_s, ts.tv_sec);
    } else {
        interrupt_s = ts.tv_sec;
        // Serial.printf("t (channel compare): %" PRIu32 ", ts.tv_nsec: %" PRId32 "\n", t, ts.tv_nsec);
    }

    interrupt_ns = t;

    // Start audio the first a PTP lock (offset < 100 ns) is reported.
    ptpLock = ptp.getLockCount() > 0;
    ananasClient.setIsPtpLocked(ptpLock);

    const NanoTime enetCompareTime{interrupt_s * NS_PER_S + interrupt_ns},
            now{ts.tv_sec * NS_PER_S + ts.tv_nsec};

    if (ptpLock && !audioSystemManager.isClockRunning()) {
        audioSystemManager.startClock();
        Serial.print("Subscriber start audio clock ");
        printTime(enetCompareTime);
        Serial.println();
    } else if (audioSystemManager.isClockRunning()) {
        ananasClient.adjustBufferReadIndex(now);
    }

    // Allow write to complete so the interrupt doesn't fire twice
    __DSB();
}
