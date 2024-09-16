#include <Audio.h>
#include <t41-ptp.h>
#include <QNEthernet.h>
#include <TimeLib.h>
#include <AudioClockManager.h>

void syncInterrupt();

void announceInterrupt();

static void interrupt_1588_timer();

void initDAC();

void initI2S();

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
bool shouldEnableAudio{false};
bool audioEnabled{false};
boolean readyForNewSample = true; // Push new data on every second call to the ISR
bool doImpulse{false};
uint32_t counter{0};
AudioClockManager audioClockManager;

byte mac[6];
IPAddress staticIP{192, 168, 10, 255};
IPAddress subnetMask{255, 255, 255, 0};

IPAddress gateway{192, 168, 10, 1};
IntervalTimer syncTimer;
IntervalTimer announceTimer;

bool connected{false};
bool master = true;
bool slave = false;

l3PTP ptp(master, slave, false);
//l2PTP ptp(master, slave, false);

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
    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12 = 6;
    qindesign::network::EthernetIEEE1588.setChannelCompareValue(1, NS_PER_S - 60);
    qindesign::network::EthernetIEEE1588.setChannelMode(1, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kPulseHighOnCompare);
    qindesign::network::EthernetIEEE1588.setChannelOutputPulseWidth(1, 25);

    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(1, true);


    // PPS-IN
    // peripherial: ENET_1588_EVENT2_IN
    // IOMUX: ALT4
    // teensy pin: 15
    attachInterruptVector(IRQ_ENET_TIMER, interrupt_1588_timer); //Configure Interrupt Handler
    NVIC_ENABLE_IRQ(IRQ_ENET_TIMER); //Enable Interrupt Handling
//    IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03 = 4;
//    qindesign::network::EthernetIEEE1588.setChannelMode(2, qindesign::network::EthernetIEEE1588Class::TimerChannelModes::kCaptureOnRising); //enable Channel2
//    // rising edge trigger
//    qindesign::network::EthernetIEEE1588.setChannelInterruptEnable(2, true); //Configure Interrupt generation

    // Set up audio
    audioClockManager.begin();
    initDAC();
    audioShield.enable();
    audioShield.volume(0.5);
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

    // Only needed if running with GPS
//    pps = true;
//    noPPSCount = 0;

//    shouldEnableAudio = ts.tv_sec == 5;
//
//    if (shouldEnableAudio && !audioEnabled) {
//        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
//        Serial.println("Leader: Starting Audio");
//        audioClockManager.start();
//        counter = 0;
//        audioEnabled = true;
//    }

    shouldEnableAudio = ts.tv_sec % 10 != 9;

    if (shouldEnableAudio && !audioEnabled) {
        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
        Serial.println("Leader: Starting Audio");
        audioClockManager.startClock();
        counter = 0;
        audioEnabled = true;
    } else if (!shouldEnableAudio && audioEnabled) {
        displayTime((ts.tv_sec * NS_PER_S) + ts.tv_nsec);
        Serial.println("Leader: Stopping Audio");
        audioClockManager.stopClock();
//        counter = 0;
        audioEnabled = false;
    }

    asm("dsb"); // allow write to complete so the interrupt doesn't fire twice
}

void initDAC()
{
    dma.begin(true); // Allocate the DMA channel first
    initI2S();
    CORE_PIN7_CONFIG = 3;  //1:TX_DATA0 pin 7 on uP
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

void initI2S()
{
    uint32_t fs{AUDIO_SAMPLE_RATE_EXACT};
    audioClockManager.setClock(fs);

    CORE_PIN23_CONFIG = 3;  //1:MCLK
    CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
    CORE_PIN20_CONFIG = 3;  //1:RX_SYNC

    int rsync = 0;
    int tsync = 1;

    I2S1_TMR = 0; // no masking
    //I2S1_TCSR = (1<<25); //Reset
    I2S1_TCR1 = I2S_TCR1_RFW(1);
    I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP // sync=0; tx is async;
                | (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
    I2S1_TCR3 = I2S_TCR3_TCE;
    I2S1_TCR4 = I2S_TCR4_FRSZ((2 - 1)) | I2S_TCR4_SYWD((32 - 1)) | I2S_TCR4_MF
                | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
    I2S1_TCR5 = I2S_TCR5_WNW((32 - 1)) | I2S_TCR5_W0W((32 - 1)) | I2S_TCR5_FBT((32 - 1));

    I2S1_RMR = 0;
    //I2S1_RCSR = (1<<25); //Reset
    I2S1_RCR1 = I2S_RCR1_RFW(1);
    I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP  // sync=0; rx is async;
                | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
    I2S1_RCR3 = I2S_RCR3_RCE;
    I2S1_RCR4 = I2S_RCR4_FRSZ((2 - 1)) | I2S_RCR4_SYWD((32 - 1)) | I2S_RCR4_MF
                | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
    I2S1_RCR5 = I2S_RCR5_WNW((32 - 1)) | I2S_RCR5_W0W((32 - 1)) | I2S_RCR5_FBT((32 - 1));
}

// DMA interrupt, called twice per sample (buffer?)
// Audio data is pushed into the DMA channel source array on every second call.
void audioISR(void)
{
    ++counter;

    if (counter >= 1 && counter < 10) {
        doImpulse = true;
    } else if (counter == 2 * static_cast<int>(AUDIO_SAMPLE_RATE_EXACT)) {
        counter = 0;
    }

    if (readyForNewSample) {
//        if (doImpulse) {
//            doImpulse = false;
//
//            uint16_t impulse = (1 << 15) - 1;
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
            i2s_tx_buffer[1] = (1 << 15) - 1;
        } else {
            i2s_tx_buffer[0] = 0;
            i2s_tx_buffer[1] = 0;
        }



        // Flush buffer and clear interrupt
        arm_dcache_flush_delete(i2s_tx_buffer, sizeof(i2s_tx_buffer));
        dma.clearInterrupt();
    }
    readyForNewSample = 1 - readyForNewSample;
}
