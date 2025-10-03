#include <AnanasServer.h>
#include <Audio.h>
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioSystemManager.h>
#include <audio_processors/PulseTrain.h>
#include <audio_processors/SineOsc.h>

void syncInterrupt();

void announceInterrupt();

static void interrupt_1588_timer();

bool shouldEnableAudio{false};

AudioSystemConfig config{
    AUDIO_BLOCK_SAMPLES,
    AUDIO_SAMPLE_RATE_EXACT,
    AudioSystemConfig::ClockRole::Authority
};
AudioSystemManager audioSystemManager{config};

class Proc1 final : public AudioProcessor
{
public:
    Proc1(SineOsc &sine, ananas::AudioServer &server)
        : sine(sine),
          server(server)
    {
    }

    void prepare(const uint sampleRate) override
    {
        sine.prepare(sampleRate);
        server.prepare(sampleRate);
    }

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override
    {
        sine.processAudio(buffer, numChannels, numSamples);
        server.processAudio(buffer, numChannels, numSamples);
    }

private:
    SineOsc &sine;
    ananas::AudioServer &server;
};

class Proc2 final : public AudioProcessor
{
public:
    Proc2(PulseTrain &pulseTrain, ananas::AudioServer &server)
        : pulseTrain(pulseTrain),
          server(server)
    {
    }

    void prepare(const uint sampleRate) override
    {
        pulseTrain.prepare(sampleRate);
        server.prepare(sampleRate);
    }

protected:
    void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override
    {
        pulseTrain.processAudio(buffer, numChannels, numSamples);
        server.processAudio(buffer, numChannels, numSamples);
    }

private:
    PulseTrain &pulseTrain;
    ananas::AudioServer &server;
};

SineOsc sine;
PulseTrain pulseTrain;
ananas::AudioServer ananasServer;
Proc1 p1{sine, ananasServer};
Proc2 p2{pulseTrain, ananasServer};

auto sineFreq{60.f};

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};
IntervalTimer syncTimer;
IntervalTimer announceTimer;

bool connected{false};

l2PTP ptp(
    config.kClockRole == AudioSystemConfig::ClockRole::Authority,
    config.kClockRole == AudioSystemConfig::ClockRole::Subscriber,
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
    qindesign::network::Ethernet.setHostname("t41ptpauthority");
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
            const timespec t{1745940240, 0};
            qindesign::network::EthernetIEEE1588.writeTimer(t);
            syncTimer.begin(syncInterrupt, 1000000);
            announceTimer.begin(announceInterrupt, 1000000);
            ananasServer.connect();
        }
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
    // PPS-IN
    // peripherial: ENET_1588_EVENT2_IN
    // IOMUX: ALT4
    // teensy pin: 15
    //    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03 = 4;
    //    qindesign::network::EthernetIEEE1588.setChannelMode(2, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kCaptureOnRising); //enable Channel2
    //    // rising edge trigger
    //    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(2, true); //Configure Interrupt generation

    // Set up audio
    AudioSystemManager::setAudioProcessor(&p2);
    audioSystemManager.begin();
    ananasServer.begin();
    sine.setAmplitude(.5f);
    // pulseTrain.setWidth(4);
    pulseTrain.setFrequency(60.f);

    Serial.println("Clock authority");
    Serial.printf("Mac address:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("IP:            ");
    Serial.println(qindesign::network::Ethernet.localIP());
    Serial.println();
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
    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        // allow write to complete so the interrupt doesn't fire twice
        __DSB();
        return;
    }
    qindesign::network::EthernetIEEE1588.getChannelCompareValue(1, t);

    t %= NS_PER_S;

    timespec ts{};
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

    // Start audio at t = 10s
    shouldEnableAudio = interrupt_s >= 10;

    const NanoTime enetCompareTime{interrupt_s * NS_PER_S + interrupt_ns},
            now{ts.tv_sec * NS_PER_S + ts.tv_nsec};

    // ananas::Utils::printTime(now); Serial.println();

    if (shouldEnableAudio && !audioSystemManager.isClockRunning()) {
        audioSystemManager.startClock();
        Serial.print("Authority start audio clock ");
        ananas::Utils::printTime(enetCompareTime);
        Serial.println();
    } else if (!shouldEnableAudio && audioSystemManager.isClockRunning()) {
        // audioSystemManager.stopClock();
        // ananas::Utils::printTime(now);
        // Serial.println();
    } else if (audioSystemManager.isClockRunning()) {
        ananasServer.adjustBufferReadIndex(now);
    }

    // Change frequency once per second.
    // sineFreq += 60.f;
    // sineFreq = sineFreq > 1000.f ? 60.f : sineFreq;
    // sine.setFrequency(interrupt_s % 2 == 0 ? 480.f : 240.f);

    // allow write to complete so the interrupt doesn't fire twice
    __DSB();
}
