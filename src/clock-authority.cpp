#include <AnanasServer.h>
#include <Audio.h>
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioSystemManager.h>

void syncInterrupt();

void announceInterrupt();

static void interrupt_1588_timer();

void displayTime(const NanoTime t)
{
    NanoTime x = t;
    const int ns = x % 1000;
    x /= 1000;
    const int us = x % 1000;
    x /= 1000;
    const int ms = x % 1000;
    x /= 1000;

    tmElements_t tme;
    breakTime((time_t) x, tme);

    Serial.printf("TIME: %02d.%02d.%04d %02d:%02d:%02d::%03d:%03d:%03d\n", tme.Day, tme.Month, 1970 + tme.Year, tme.Hour, tme.Minute, tme.Second, ms, us, ns);
}

bool shouldEnableAudio{false};

AudioSystemConfig config{
    AUDIO_BLOCK_SAMPLES,
    AUDIO_SAMPLE_RATE_EXACT,
    AudioSystemConfig::ClockRole::Authority
};
AudioSystemManager audioSystemManager{config};
ananas::AudioServer ananasServer;

int16_t txBuffer[128 * 2];
uint8_t txPacketBuffer[sizeof(NanoTime) + (128 << 2)];
qindesign::network::EthernetUDP socket;
auto sineFreq{60.};

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};
IntervalTimer syncTimer;
IntervalTimer announceTimer;

bool connected{false};

l2PTP ptp(
    config.k_ClockRole == AudioSystemConfig::ClockRole::Authority,
    config.k_ClockRole == AudioSystemConfig::ClockRole::Subscriber,
    false
);

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {}
#endif

    Serial.begin(2000000);
    pinMode(13, OUTPUT);

    // Setup networking
    qindesign::network::Ethernet.setHostname("t41ptpmaster");
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
            syncTimer.begin(syncInterrupt, 1000000);
            announceTimer.begin(announceInterrupt, 1000000);
            // Valid, ad-hoc multicast IP, and valid dynamic port.
            // socket.beginMulticast({224, 4, 224, 4}, 49152);
            ananasServer.connect();
        }
    });

    Serial.println("Clock authority");
    Serial.printf("Mac address:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("IP:            ");
    Serial.println(qindesign::network::Ethernet.localIP());
    Serial.println();

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
    // PPS-IN
    // peripherial: ENET_1588_EVENT2_IN
    // IOMUX: ALT4
    // teensy pin: 15
    //    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03 = 4;
    //    qindesign::network::EthernetIEEE1588.setChannelMode(2, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kCaptureOnRising); //enable Channel2
    //    // rising edge trigger
    //    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(2, true); //Configure Interrupt generation

    ananasServer.begin();
    // Set up audio
    audioSystemManager.begin();
}

bool sync = false;
bool announce = false;
bool pps = false;
int noPPSCount = 0;

NanoTime interrupt_s = 0;
NanoTime interrupt_ns = 0;
NanoTime pps_s = 0;
NanoTime pps_ns = 0;

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
    if (pps) {
        pps = false;
        ptp.ppsInterruptTriggered((pps_s * NS_PER_S) + pps_ns, (interrupt_s * NS_PER_S) + interrupt_ns);
        if (ptp.getLockCount() > 5) {
            sync = true;
        }
    }
    ptp.update();

    digitalWrite(13, ptp.getLockCount() > 5 && noPPSCount < 5 ? HIGH : LOW);

    // if (connected && AudioSystemManager::getNumPacketsAvailable() >= 1) {
    //     AudioSystemManager::readFromPacketBuffer(txPacketBuffer);
    //     socket.beginPacket({224, 4, 224, 4}, 49152);
    //     socket.write(txPacketBuffer, sizeof(AudioSystemManager::Packet));
    //     socket.endPacket();
    // }

    ananasServer.run();
}

void syncInterrupt()
{
    if (noPPSCount > 5) {
        sync = true;
    } else {
        noPPSCount++;
    }
}

void announceInterrupt()
{
    announce = true;
}

static void interrupt_1588_timer()
{
    //    Serial.println("IRQ_ENET_TIMER!");

    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        //        asm("dsb"); // allow write to complete so the interrupt doesn't fire twice
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

    // Only needed if running with GPS input
    //    pps = true;
    //    noPPSCount = 0;

    //    shouldEnableAudio = ts.tv_sec == 5;
    //
    //    if (shouldEnableAudio && !audioEnabled) {
    //        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
    //        Serial.println("Leader: Starting Audio");
    //        audioSystemManager.start();
    //        counter = 0;
    //        audioEnabled = true;
    //    }

    // Start audio at t = 10s
    shouldEnableAudio = interrupt_s >= 10; // % 10 != 9;

    NanoTime now{interrupt_s * NS_PER_S + interrupt_ns};

    if (shouldEnableAudio && !audioSystemManager.isClockRunning()) {
        Serial.print("Authority start audio clock ");
        displayTime(now);
        audioSystemManager.startClock();
    } else if (!shouldEnableAudio && audioSystemManager.isClockRunning()) {
        displayTime(now);
        audioSystemManager.stopClock();
    } else if (audioSystemManager.isClockRunning()) {
        AudioSystemManager::adjustPacketBufferReadIndex(now);
    }

    // Change frequency once per second.
    sineFreq += 60;
    sineFreq = sineFreq > 1000 ? 60 : sineFreq;
    AudioSystemManager::s_SineWaveGenerator.setFrequency(sineFreq); //interrupt_s % 2 == 0 ? 480 : 240);

    //    asm("dsb"); // allow write to complete so the interrupt doesn't fire twice
    __DSB();
}
