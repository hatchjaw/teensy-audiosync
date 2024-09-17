#include <Audio.h>
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioSystemManager.h>

static void interrupt_1588_timer();

void initDAC();

void audioISR();

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

DMAMEM __attribute__((aligned(32))) static uint16_t i2s_tx_buffer[2] = {0, 0};
static DMAChannel dma;
AudioControlSGTL5000 audioShield;
volatile bool shouldEnableAudio{false};
bool audioEnabled{false};
boolean readyForNewSample = true; // Push new data on every second call to the ISR
bool doImpulse{false};
uint32_t counter{0};
AudioSystemManager audioSystemManager{AUDIO_SAMPLE_RATE_EXACT, AUDIO_BLOCK_SAMPLES};

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 10, 1};

bool connected{false};
bool p2p = false;
bool master = false;
bool slave = true;

l3PTP ptp(master, slave, p2p);
//l2PTP ptp(master, slave, p2p);

void setup()
{
#ifdef WAIT_FOR_SERIAL
    while (!Serial) {}
#endif

    Serial.begin(2000000);
    pinMode(13, OUTPUT);

    // Setup networking
    qindesign::network::Ethernet.setHostname("t41ptpslave");
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
        }
    });

    Serial.println("Clock subscriber");
    Serial.printf("Mac address:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("IP:            ");
    Serial.println(qindesign::network::Ethernet.localIP());
    Serial.println();

    // PPS Out
    // peripherial: ENET_1588_EVENT1_OUT
    // IOMUX: ALT6
    // teensy pin: 24
    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12 = 6;
    qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S - 60);
    qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare);
    qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25);

    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true);
    attachInterruptVector(IRQ_ENET_TIMER, interrupt_1588_timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling

    // Set up audio
    audioSystemManager.begin();
    initDAC();
    audioShield.enable();
    audioShield.volume(0.5);
}

NanoTime interrupt_s = 0;
NanoTime interrupt_ns = 0;
NanoTime pps_s = 0;
NanoTime pps_ns = 0;

void loop()
{
    ptp.update();

    // Six consecutive offsets below 100 ns sets pin 13 high to switch on the LED
    digitalWrite(13, ptp.getLockCount() > 5 ? HIGH : LOW);
}

static void interrupt_1588_timer()
{
    uint32_t t;
    if (!qindesign::network::EthernetIEEE1588.getAndClearChannelStatus(1)) {
        asm("dsb"); // allow write to complete so the interrupt doesn't fire twice
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

    //    if (audioEnabled) {
    auto adjust{ptp.getAdjust()};

    Serial.printf("Adjust (nsps): %f\n", adjust);

    double proportionalAdjustment{1. + (adjust / NS_PER_S)};

    Serial.printf("Proportional adjustment: %.*f\n", 10, proportionalAdjustment);

    double targetFs{AUDIO_SAMPLE_RATE_EXACT * proportionalAdjustment};

    // Serial.printf("Target sample rate: %.*f\n", 10, targetFs);

    audioSystemManager.setSampleRate(targetFs);
    //    }

    //    auto offset{ptp.getOffset()};
    //    shouldEnableAudio = offset != 0 && !(offset > 100 || offset < -100);
    //
    //    if (shouldEnableAudio && !audioEnabled) {
    //        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
    //        Serial.println("Follower: Starting Audio");
    //        audioSystemManager.start();
    //        counter = 0;
    //        audioEnabled = true;
    //    }

    shouldEnableAudio = ts.tv_sec % 10 != 9;

    if (shouldEnableAudio && !audioEnabled) {
        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
        Serial.println("Follower: Starting Audio");
        audioSystemManager.startClock();
        counter = 0;
        audioEnabled = true;
    } else if (!shouldEnableAudio && audioEnabled) {
        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
        Serial.println("Follower: Stopping Audio");
        audioSystemManager.stopClock();
        //        counter = 0;
        audioEnabled = false;
    }

    asm("dsb"); // allow write to complete so the interrupt doesn't fire twice
}

void initDAC()
{
    dma.begin(true); // Allocate the DMA channel first


    dma.TCD->SADDR = i2s_tx_buffer; //source address
    dma.TCD->SOFF = 2; // source buffer address increment per transfer in bytes
    dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1); // specifies 16 bit source and destination
    dma.TCD->NBYTES_MLNO = 2; // bytes to transfer for each service request///////////////////////////////////////////////////////////////////
    dma.TCD->SLAST = -sizeof(i2s_tx_buffer); // last source address adjustment
    dma.TCD->DOFF = 0; // increments at destination
    dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
    dma.TCD->DLASTSGA = 0; // destination address offset
    dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
    dma.TCD->CSR = DMA_TCD_CSR_INTHALF; //| DMA_TCD_CSR_INTMAJOR; // enables interrupt when transfers half complete. SET TO 0 to disable DMA interrupts
    dma.TCD->DADDR = (void *) ((uint32_t) &I2S1_TDR0 + 2); // I2S1 register DMA writes to
    dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX); // i2s channel that will trigger the DMA transfer when ready for data
    dma.enable();
    I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
    I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
    dma.attachInterrupt(audioISR);
}

// DMA interrupt, called twice per sample (buffer?)
// Audio data is pushed into the DMA channel source array on every second call.
void audioISR(void)
{
    ++counter;
    //    if (counter > 2 * static_cast<int>(AUDIO_SAMPLE_RATE_EXACT) - 10) {
    //        doImpulse = true;
    //    }
    if (counter >= 1 && counter < 5) {
        doImpulse = true;
    } else if (counter == 2 * AUDIO_SAMPLE_RATE_EXACT) {
        counter = 0;
    }

    if (readyForNewSample) {
        //        if (doImpulse) {
        //            doImpulse = false;
        //
        //            int16_t impulse = 32767;
        //
        //            // Pass current sample to L+R audio buffers
        //            i2s_tx_buffer[0] = impulse; // Left Channel
        //            i2s_tx_buffer[1] = 0; // Right Channel
        //        } else {
        //            i2s_tx_buffer[0] = 0; // Left Channel
        //            i2s_tx_buffer[1] = 0; // Right Channel
        //        }

        if (doImpulse) {
            doImpulse = false;
            i2s_tx_buffer[0] = (1 << 15) - 1;
        } else {
            i2s_tx_buffer[0] = 0;
        }
        //        else if (counter > 1.666 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 2) - 1) << 7);
        //        } else if (counter > 1.333 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 4) - 1) << 6);
        //        } else if (counter > AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 6) - 1) << 5);
        //        } else if (counter > .666 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 8) - 1) << 4);
        //        } else if (counter > .333 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 10) - 1) << 3);
        //        } else {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 12) - 1) << 2);
        //        }

        i2s_tx_buffer[1] = 0;

        // Flush buffer and clear interrupt
        arm_dcache_flush_delete(i2s_tx_buffer, sizeof(i2s_tx_buffer));
        dma.clearInterrupt();
    }
    readyForNewSample = 1 - readyForNewSample;
}
