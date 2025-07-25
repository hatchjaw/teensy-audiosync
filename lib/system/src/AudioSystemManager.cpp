#include "AudioSystemManager.h"
#include <QNEthernet.h>
#include <TimeLib.h>
#include <Utils.h>

DMAChannel AudioSystemManager::s_DMA{false};
bool AudioSystemManager::s_FirstInterrupt{false};
uint16_t AudioSystemManager::s_NumInterrupts{0};
long AudioSystemManager::s_FirstInterruptNS{0};
long AudioSystemManager::s_AudioPTPOffset{0};
AudioProcessor *AudioSystemManager::s_AudioProcessor = nullptr;
DMAMEM __attribute__((aligned(32))) uint32_t AudioSystemManager::s_I2sTxBuffer[k_I2sBufferSizeFrames];

AudioSystemManager::AudioSystemManager(const AudioSystemConfig config)
    : m_Config(config)
{
    srand(static_cast<unsigned>(time(nullptr)));
}

FLASHMEM
bool AudioSystemManager::begin()
{
    // Calculate fundamental values for clock dividers.
    m_ClockDividers.calculateCoarse(m_Config.k_SampleRate);
    Serial.print(m_ClockDividers);

    // Set up the audio processor.
    s_AudioProcessor->prepare(m_Config.k_SampleRate);

    cycPreReg = ARM_DWT_CYCCNT;

    //==========================================================================
    // Reset all registers
    //==========================================================================
    m_AnalogAudioPllControlRegister.begin();
    m_AudioPllNumeratorRegister.begin();
    m_AudioPllDenominatorRegister.begin();
    m_MiscellaneousRegister2.begin();
    m_SerialClockMultiplexerRegister1.begin();
    m_ClockDividerRegister1.begin();
    m_ClockGatingRegister5.begin();
    m_GeneralPurposeRegister1.begin();
    m_Pin7SwMuxControlRegister.begin();
    m_Pin20SwMuxControlRegister.begin();
    m_Pin21SwMuxControlRegister.begin();
    m_Pin23SwMuxControlRegister.begin();
    m_SAI1TransmitMaskRegister.begin();
    m_SAI1TransmitConfig1Register.begin();
    m_SAI1TransmitConfig2Register.begin();
    m_SAI1TransmitConfig3Register.begin();
    m_SAI1TransmitConfig4Register.begin();
    m_SAI1TransmitConfig5Register.begin();
    m_SAI1TransmitControlRegister.begin();
    m_SAI1ReceiveMaskRegister.begin();
    m_SAI1ReceiveConfig1Register.begin();
    m_SAI1ReceiveConfig2Register.begin();
    m_SAI1ReceiveConfig3Register.begin();
    m_SAI1ReceiveConfig4Register.begin();
    m_SAI1ReceiveConfig5Register.begin();
    m_SAI1ReceiveControlRegister.begin();

    // m_AudioShield.begin();

    cycPostReg = ARM_DWT_CYCCNT;

    // setupPins();

    //==========================================================================
    // Set mux modes for I2S pins.
    //==========================================================================
    // LRCLK1 on pin 20
    m_Pin20SwMuxControlRegister.setMuxMode(Pin20SwMuxControlRegister::MuxMode::Sai1RxSync);
    // BCLK on pin 21
    m_Pin21SwMuxControlRegister.setMuxMode(Pin21SwMuxControlRegister::MuxMode::Sai1RxBclk);
    // MCLK on pin 23
    m_Pin23SwMuxControlRegister.setMuxMode(Pin23SwMuxControlRegister::MuxMode::Sai1Mclk);
    // Data on pin 7
    m_Pin7SwMuxControlRegister.setMuxMode(Pin7SwMuxControlRegister::MuxMode::Sai1TxData00);

    cycPostPin = ARM_DWT_CYCCNT;

    //==========================================================================
    // Set audio PLL (PLL4) and SAI1 clock registers
    //==========================================================================
    // Enable SAI1 clock
    m_ClockGatingRegister5.enableSai1Clock();

    m_SerialClockMultiplexerRegister1.setSai1ClkSel(SerialClockMultiplexerRegister1::Sai1ClkSel::DeriveClockFromPll4);

    // Set audio divider registers
    m_MiscellaneousRegister2.setAudioPostDiv(m_ClockDividers.k_AudioPostDiv);

    m_ClockDividerRegister1.setSai1ClkPred(m_ClockDividers.m_Sai1Pre);
    m_ClockDividerRegister1.setSai1ClkPodf(m_ClockDividers.m_Sai1Post);

    m_GeneralPurposeRegister1.setSai1MclkDirection(GeneralPurposeRegister1::SignalDirection::Output);
    m_GeneralPurposeRegister1.setSai1MclkSource(GeneralPurposeRegister1::Sai1MclkSource::CcmSai1ClkRoot);

    m_AnalogAudioPllControlRegister.setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource::RefClk24M);
    m_AnalogAudioPllControlRegister.setPostDivSelect(m_ClockDividers.k_Pll4PostDiv);
    m_AnalogAudioPllControlRegister.setDivSelect(m_ClockDividers.m_Pll4Div);
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
    m_AudioPllDenominatorRegister.set(m_ClockDividers.m_Pll4Denom);

    //==========================================================================
    // Activate the audio PLL
    //==========================================================================
    // These have to be present. Not necessarily in this order, but if not here
    // the audio subsystem appears to work, but not the audio shield.
    // m_AnalogAudioPllControlRegister.setEnable(true);
    // m_AnalogAudioPllControlRegister.setPowerDown(false);
    // m_AnalogAudioPllControlRegister.setBypass(false);

    cycPostClk = ARM_DWT_CYCCNT;

    //==========================================================================
    // Configure SAI1 (I2S)
    //==========================================================================
    // TODO: check whether transmit/receive are enabled
    m_SAI1TransmitMaskRegister.setMask(0);
    m_SAI1TransmitConfig1Register.setWatermarkLevel(1);
    m_SAI1TransmitConfig2Register.setSyncMode(SAI1TransmitConfig2Register::SyncMode::SynchronousWithReceiver);
    m_SAI1TransmitConfig2Register.setBitClockPolarity(SAI1TransmitConfig2Register::BitClockPolarity::ActiveLow);
    m_SAI1TransmitConfig2Register.setBitClockDirection(SAI1TransmitConfig2Register::BitClockDirection::Internal);
    m_SAI1TransmitConfig2Register.setBitClockDivide(1);
    m_SAI1TransmitConfig2Register.selectMasterClock(SAI1TransmitConfig2Register::Clock::MasterClock1);
    m_SAI1TransmitConfig3Register.enableTransmitChannel(1, true);
    m_SAI1TransmitConfig4Register.setFrameSize(2);
    m_SAI1TransmitConfig4Register.setSyncWidth(32);
    m_SAI1TransmitConfig4Register.setEndianness(SAI1TransmitConfig4Register::Endianness::BigEndian);
    m_SAI1TransmitConfig4Register.setFrameSyncDirection(SAI1TransmitConfig4Register::FrameSyncDirection::Internal);
    m_SAI1TransmitConfig4Register.setFrameSyncEarly(SAI1TransmitConfig4Register::FrameSyncAssert::OneBitEarly);
    m_SAI1TransmitConfig4Register.setFrameSyncPolarity(SAI1TransmitConfig4Register::FrameSyncPolarity::ActiveLow);
    m_SAI1TransmitConfig5Register.setWordNWidth(32);
    m_SAI1TransmitConfig5Register.setWord0Width(32);
    m_SAI1TransmitConfig5Register.setFirstBitShift(32);
    m_SAI1ReceiveMaskRegister.setMask(0);
    m_SAI1ReceiveConfig1Register.setWatermarkLevel(1);
    m_SAI1ReceiveConfig2Register.setSyncMode(SAI1ReceiveConfig2Register::SyncMode::Asynchronous);
    m_SAI1ReceiveConfig2Register.setBitClockPolarity(SAI1ReceiveConfig2Register::BitClockPolarity::ActiveLow);
    m_SAI1ReceiveConfig2Register.setBitClockDirection(SAI1ReceiveConfig2Register::BitClockDirection::Internal);
    m_SAI1ReceiveConfig2Register.setBitClockDivide(1);
    m_SAI1ReceiveConfig2Register.selectMasterClock(SAI1ReceiveConfig2Register::Clock::MasterClock1);
    m_SAI1ReceiveConfig3Register.enableReceiveChannel(1, true);
    m_SAI1ReceiveConfig4Register.setFrameSize(2);
    m_SAI1ReceiveConfig4Register.setSyncWidth(32);
    m_SAI1ReceiveConfig4Register.setEndianness(SAI1ReceiveConfig4Register::Endianness::BigEndian);
    m_SAI1ReceiveConfig4Register.setFrameSyncDirection(SAI1ReceiveConfig4Register::FrameSyncDirection::Internal);
    m_SAI1ReceiveConfig4Register.setFrameSyncEarly(SAI1ReceiveConfig4Register::FrameSyncAssert::OneBitEarly);
    m_SAI1ReceiveConfig4Register.setFrameSyncPolarity(SAI1ReceiveConfig4Register::FrameSyncPolarity::ActiveLow);
    m_SAI1ReceiveConfig5Register.setWordNWidth(32);
    m_SAI1ReceiveConfig5Register.setWord0Width(32);
    m_SAI1ReceiveConfig5Register.setFirstBitShift(32);

    cycPostI2S = ARM_DWT_CYCCNT;

    //==========================================================================
    // Set up DMA
    //==========================================================================
    s_DMA.begin(true);

    s_DMA.TCD->SADDR = s_I2sTxBuffer; //source address
    s_DMA.TCD->SOFF = 2; // source buffer address increment per transfer in bytes
    s_DMA.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1); // specifies 16 bit source and destination
    s_DMA.TCD->NBYTES_MLNO = 2; // bytes to transfer for each service request///////////////////////////////////////////////////////////////////
    s_DMA.TCD->SLAST = -sizeof(s_I2sTxBuffer); // last source address adjustment
    s_DMA.TCD->DOFF = 0; // increments at destination
    s_DMA.TCD->CITER_ELINKNO = sizeof(s_I2sTxBuffer) / 2;
    s_DMA.TCD->DLASTSGA = 0; // destination address offset
    s_DMA.TCD->BITER_ELINKNO = sizeof(s_I2sTxBuffer) / 2;
    s_DMA.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
    // interrupts
    s_DMA.TCD->DADDR = (void *) ((uint32_t) &I2S1_TDR0 + 2); // I2S1 register DMA writes to
    s_DMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX); // i2s channel that will trigger the DMA transfer when ready for data
    s_DMA.enable();

    // m_SAI1TransmitControlRegister.setTransmitterEnable(true);
    // m_SAI1TransmitControlRegister.setBitClockEnable(true);
    // m_SAI1TransmitControlRegister.setFIFORequestDMAEnable(true);
    // m_SAI1ReceiveControlRegister.setReceiverEnable(true);
    // m_SAI1ReceiveControlRegister.setBitClockEnable(true);
    //
    s_DMA.attachInterrupt(isr);

    s_FirstInterrupt = true;
    s_NumInterrupts = 0;
    s_FirstInterruptNS = 0;

    cycPostDMA = ARM_DWT_CYCCNT;

    //==========================================================================
    // Enable the audio shield
    //==========================================================================
    // This call must follow the above, and features some significant delays.
    // m_AudioShield.enable();
    // m_AudioShield.volume(m_Config.k_Volume);

    //==========================================================================

    cycPostSGTL = ARM_DWT_CYCCNT;

    // Stop the audio clock (originally till PTP settles down...)
    // stopClock();

    cycPostStop = ARM_DWT_CYCCNT;

    // With predictability of timing in mind, report setup here, rather than
    // between calls to set up registers.
    Serial.println(m_Config);
    // Serial.printf("Audio interrupt priority: %d\n", NVIC_GET_PRIORITY(s_DMA.channel));

    Serial.printf(
        // "- Register setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- Pin setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- Clock setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- I2S setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- DMA setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- Audio shield setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Begin took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Init took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Analog power took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Digital power took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Line out took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Clock took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "  - Setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        // "- Audio stop took %" PRIu32 " cycles (%" PRIu32 " ns).\n"
        "=== Audio system setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n",
        // cycPostReg - cycPreReg, ananas::Utils::cyclesToNs(cycPostReg - cycPreReg),
        // cycPostPin - cycPostReg, ananas::Utils::cyclesToNs(cycPostPin - cycPostReg),
        // cycPostClk - cycPostPin, ananas::Utils::cyclesToNs(cycPostClk - cycPostPin),
        // cycPostI2S - cycPostClk, ananas::Utils::cyclesToNs(cycPostI2S - cycPostClk),
        // cycPostDMA - cycPostI2S, ananas::Utils::cyclesToNs(cycPostDMA - cycPostI2S),
        // cycPostSGTL - cycPostDMA, ananas::Utils::cyclesToNs(cycPostSGTL - cycPostDMA),
        // m_AudioShield.cycPostBegin - m_AudioShield.cycStart, ananas::Utils::cyclesToNs(m_AudioShield.cycPostBegin - m_AudioShield.cycStart),
        // m_AudioShield.cycPostInit - m_AudioShield.cycPostBegin, ananas::Utils::cyclesToNs(m_AudioShield.cycPostInit - m_AudioShield.cycPostBegin),
        // m_AudioShield.cycPostAnPwr - m_AudioShield.cycPostInit, ananas::Utils::cyclesToNs(m_AudioShield.cycPostAnPwr - m_AudioShield.cycPostInit),
        // m_AudioShield.cycPostDgPwr - m_AudioShield.cycPostAnPwr, ananas::Utils::cyclesToNs(m_AudioShield.cycPostDgPwr - m_AudioShield.cycPostAnPwr),
        // m_AudioShield.cycPostLnOut - m_AudioShield.cycPostDgPwr, ananas::Utils::cyclesToNs(m_AudioShield.cycPostLnOut - m_AudioShield.cycPostDgPwr),
        // m_AudioShield.cycPostClock - m_AudioShield.cycPostLnOut, ananas::Utils::cyclesToNs(m_AudioShield.cycPostClock - m_AudioShield.cycPostLnOut),
        // m_AudioShield.cycPostSetup - m_AudioShield.cycPostClock, ananas::Utils::cyclesToNs(m_AudioShield.cycPostSetup - m_AudioShield.cycPostClock),
        // cycPostStop - cycPostSGTL, ananas::Utils::cyclesToNs(cycPostStop - cycPostSGTL),
        cycPostStop - cycPreReg, ananas::Utils::cyclesToNs(cycPostStop - cycPreReg));

    // Check SAI1/I2S config
    // TMR 0 | TCR1 1 | TCR2 47000001 | TCR3 10000 | TCR4 11F1B | TCR5 1F1F1F00
    // RMR 0 | RCR1 1 | RCR2 7000001 | RCR3 10000 | RCR4 11F1B | RCR5 1F1F1F00
    // TCSR 90100001 | RCSR 90170000
    // Serial.printf("TMR %X | TCR1 %X | TCR2 %X | TCR3 %X | TCR4 %X | TCR5 %X\n"
    //               "RMR %X | RCR1 %X | RCR2 %X | RCR3 %X | RCR4 %X | RCR5 %X\n"
    //               "TCSR %X | RCSR %X\n",
    //               I2S1_TMR, I2S1_TCR1, I2S1_TCR2, I2S1_TCR3, I2S1_TCR4, I2S1_TCR5,
    //               I2S1_RMR, I2S1_RCR1, I2S1_RCR2, I2S1_RCR3, I2S1_RCR4, I2S1_RCR5,
    //               I2S1_TCSR, I2S1_RCSR);

    return true;
}

FLASHMEM
void AudioSystemManager::setupPins() const
{
    // LRCLK1 on pin 20
    m_Pin20SwMuxControlRegister.setMuxMode(Pin20SwMuxControlRegister::MuxMode::Sai1RxSync);
    // BCLK on pin 21
    m_Pin21SwMuxControlRegister.setMuxMode(Pin21SwMuxControlRegister::MuxMode::Sai1RxBclk);
    // MCLK on pin 23
    m_Pin23SwMuxControlRegister.setMuxMode(Pin23SwMuxControlRegister::MuxMode::Sai1Mclk);
    // Data on pin 7
    m_Pin7SwMuxControlRegister.setMuxMode(Pin7SwMuxControlRegister::MuxMode::Sai1TxData00);
}

FLASHMEM
void AudioSystemManager::setupI2S() const
{
    int rsync = 0;
    int tsync = 1;

    // if either transmitter or receiver is enabled, do nothing
    if (I2S1_TCSR & I2S_TCSR_TE || I2S1_RCSR & I2S_RCSR_RE) {
        Serial.println("I2S transmitter/receiver already enabled. "
            "Aborting I2S setup.");
        return;
    }

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
    I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP // sync=0; rx is async;
                | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
    I2S1_RCR3 = I2S_RCR3_RCE;
    I2S1_RCR4 = I2S_RCR4_FRSZ((2 - 1)) | I2S_RCR4_SYWD((32 - 1)) | I2S_RCR4_MF
                | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
    I2S1_RCR5 = I2S_RCR5_WNW((32 - 1)) | I2S_RCR5_W0W((32 - 1)) | I2S_RCR5_FBT((32 - 1));
}

FLASHMEM
void AudioSystemManager::setupDMA() const
{
    s_DMA.begin(true);

    // TODO: why do two interrupts per buffer?
    s_DMA.TCD->SADDR = s_I2sTxBuffer; //source address
    s_DMA.TCD->SOFF = 2; // source buffer address increment per transfer in bytes
    s_DMA.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1); // 16 bit source and destination
    s_DMA.TCD->NBYTES_MLNO = 2; // bytes to transfer for each service request
    s_DMA.TCD->SLAST = -sizeof(s_I2sTxBuffer); // last source address adjustment
    s_DMA.TCD->DOFF = 0; // increments at destination
    s_DMA.TCD->CITER_ELINKNO = sizeof(s_I2sTxBuffer) / 2;
    s_DMA.TCD->DLASTSGA = 0; // destination address offset
    s_DMA.TCD->BITER_ELINKNO = sizeof(s_I2sTxBuffer) / 2;
    s_DMA.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
    // interrupts
    s_DMA.TCD->DADDR = (void *) ((uint32_t) &I2S1_TDR0 + 2); // I2S1 register DMA writes to
    s_DMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX); // i2s channel that will trigger the DMA transfer when ready for data
    s_DMA.enable();

    I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
    I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

    s_DMA.attachInterrupt(isr);
    // isr,
    // 64 // 128 is the default.
    // );
}

void AudioSystemManager::startClock()
{
    m_AnalogAudioPllControlRegister.setPowerDown(false);
    m_AnalogAudioPllControlRegister.setBypass(false);
    m_AnalogAudioPllControlRegister.setEnable(true);

    m_SAI1TransmitControlRegister.setBitClockEnable(true);
    m_SAI1TransmitControlRegister.setFIFORequestDMAEnable(true);
    m_SAI1TransmitControlRegister.setTransmitterEnable(true);
    m_SAI1ReceiveControlRegister.setBitClockEnable(true);
    m_SAI1ReceiveControlRegister.setReceiverEnable(true);

    m_AudioShield.enable();
    m_AudioShield.volume(m_Config.k_Volume);

    s_FirstInterrupt = true;
    s_NumInterrupts = 0;
    s_FirstInterruptNS = 0;
}

void AudioSystemManager::stopClock()
{
    m_AudioShield.volume(0.f);

    m_AnalogAudioPllControlRegister.setEnable(false);
    m_AnalogAudioPllControlRegister.setPowerDown(true);
    m_AnalogAudioPllControlRegister.setBypass(true);

    s_FirstInterrupt = true;
    s_NumInterrupts = 0;
    s_FirstInterruptNS = 0;
}

volatile bool AudioSystemManager::isClockRunning() const
{
    return m_AnalogAudioPllControlRegister.isClockRunning();
}

FLASHMEM
void AudioSystemManager::setAudioProcessor(AudioProcessor *processor)
{
    s_AudioProcessor = processor;
}

size_t AudioSystemManager::printTo(Print &p) const
{
    return p.println(m_Config)
           + p.print(m_ClockDividers)
           + p.printf("\nAudio-PTP offset: %" PRId32 " ns", s_AudioPTPOffset);
}

void AudioSystemManager::adjustClock(const double adjust, const double drift)
{
    const double proportionalAdjustment{
        1. + (adjust + s_AudioPTPOffset) * ClockConstants::k_Nanosecond
    };

    m_Config.setExactSampleRate(proportionalAdjustment);
    m_ClockDividers.calculateFine(m_Config.getExactSampleRate());

    // Tends to take on the order of 28 ns.
    // const auto cycles{ARM_DWT_CYCCNT};
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
    // Serial.printf("Fs update took %" PRIu32 " ns\n", ananas::Utils::cyclesToNs(ARM_DWT_CYCCNT - cycles));
}

// TODO: move these into the class.
static int16_t buff[128 * 2]; // audio block frames * numChannels
static uint count{0};

void AudioSystemManager::isr()
{
    if (s_FirstInterrupt) {
        s_FirstInterrupt = false;
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        const auto ns{ts.tv_sec * ClockConstants::k_NanosecondsPerSecond + ts.tv_nsec};
        Serial.print("First interrupt time: ");
        ananas::Utils::printTime(ns);
        Serial.println();
    }

    // TODO: replace with 2 * Fs/blockSize.
    if (++s_NumInterrupts >= 750) {
        s_NumInterrupts = 0;
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);

        // Get a reference nanosecond figure to use for comparison.
        if (s_FirstInterruptNS == 0) {
            s_FirstInterruptNS = ts.tv_nsec;
        }

        // Each second, compare the number of nanoseconds with the reference.
        // TODO: check for zero-wraparound, e.g. if first NS is 999,999,999, and
        //   a later timer read gives 1, this will break...
        s_AudioPTPOffset = ts.tv_nsec - s_FirstInterruptNS;
    }

    int16_t *destination, *source;
    const auto sourceAddress = (uint32_t) s_DMA.TCD->SADDR;
    s_DMA.clearInterrupt();

    // Bloody hell, I'm not sure why this works. Fill the second half of the I2S
    // buffer with the first half of the source buffer?..
    if (sourceAddress < (uint32_t) s_I2sTxBuffer + sizeof(s_I2sTxBuffer) / 2) {
        // TODO: Is it necessary to proces the whole buffer here?
        //   That's what AudioOutputI2S::isr does... sort of, it delegates to a software ISR, no?
        s_AudioProcessor->processAudio(buff, 2, k_I2sBufferSizeFrames);
        // DMA is transmitting the first half of the buffer; fill the second half.
        destination = (int16_t *) &s_I2sTxBuffer[k_I2sBufferSizeFrames / 2];
        // source = &buff[k_I2sBufferSizeFrames]; // That's the midpoint.
        source = buff;

        // if (++count % 1000 <= 1) {
        //     Serial.println("Output buffer:");
        //     ananas::Utils::hexDump(reinterpret_cast<uint8_t *>(buff), sizeof(int16_t) * 2 * k_I2sBufferSizeFrames);
        // }
    } else {
        // DMA is transmitting the second half of the buffer; fill the first half.
        destination = (int16_t *) s_I2sTxBuffer;
        // source = buff;
        source = &buff[k_I2sBufferSizeFrames]; // That's the midpoint.
    }

    memcpy(destination, source, k_I2sBufferSizeBytes); // Actually number of bytes over two?

    arm_dcache_flush_delete(destination, sizeof(s_I2sTxBuffer) / 2);
}

FLASHMEM
void AudioSystemManager::ClockDividers::calculateCoarse(const uint32_t targetSampleRate)
{
    constexpr auto orderOfMagnitude{1e6};

    for (m_Pll4Div = ClockConstants::k_Pll4DivMin; m_Pll4Div <= ClockConstants::k_Pll4DivMax; ++m_Pll4Div) {
        m_Pll4Num = 0;
        m_Pll4Denom = (uint32_t) orderOfMagnitude;
        m_Sai1Pre = 1;
        m_Sai1Post = 1;
        auto divExact{-1.};

        while (!isSai1PostFreqValid() && m_Sai1Pre < ClockConstants::k_Sai1PreMax) {
            ++m_Sai1Pre;
        }

        while ((uint8_t) floor(divExact / orderOfMagnitude) != m_Pll4Div) {
            while ((uint32_t) getCurrentSampleRate() > targetSampleRate
                   && m_Sai1Post < ClockConstants::k_Sai1PostMax) {
                ++m_Sai1Post;
            }

            if (targetSampleRate > getCurrentMaxPossibleSampleRate() && m_Sai1Pre < ClockConstants::k_Sai1PreMax) {
                ++m_Sai1Pre;
                m_Sai1Post = 1;
                continue;
            }

            divExact = (double) targetSampleRate * ClockConstants::k_AudioWordSize * m_Sai1Pre * m_Sai1Post / ClockConstants::k_OscMHz;

            if ((uint8_t) floor(divExact / orderOfMagnitude) != m_Pll4Div) {
                if (m_Sai1Pre < ClockConstants::k_Sai1PreMax) {
                    ++m_Sai1Pre;
                    m_Sai1Post = 1;
                    continue;
                } else {
                    break;
                }
            }

            const auto pll4Num = floor(divExact - orderOfMagnitude * m_Pll4Div);
            m_Pll4Num = (int32_t) pll4Num;

            while (m_Pll4Num * 10 < ClockConstants::k_Pll4NumMax && m_Pll4Denom * 10 < ClockConstants::k_Pll4DenomMax) {
                m_Pll4Num *= 10;
                m_Pll4Denom *= 10;
            }

            // TODO: prefer high precision denom, i.e. where num < (1 << 29) - 1
            if (isPll4FreqValid()
                //                && getPll4Freq() > (ClockConstants::k_pll4FreqMin + ClockConstants::k_pll4FreqMax) / 2
                && m_Pll4Denom == 1'000'000'000) {
                // Serial.println(*this);
                return;
            } else {
                break;
            }
        }
    }
}

void AudioSystemManager::ClockDividers::calculateFine(const double targetSampleRate)
{
    constexpr auto orderOfMagnitude{1e6};

    const auto sai1WordOsc{(double) ClockConstants::k_AudioWordSize * m_Sai1Pre * m_Sai1Post / ClockConstants::k_OscMHz};
    const auto num{targetSampleRate * sai1WordOsc - orderOfMagnitude * m_Pll4Div};
    const auto numInt{(int32_t) round(num * (m_Pll4Denom / orderOfMagnitude))};

    if (numInt < 0 || (uint32_t) numInt > m_Pll4Denom) {
        Serial.printf("Invalid PLL4 numerator %" PRId32 " "
                      "for target sample rate %f "
                      "and denominator %" PRIu32 "\n",
                      numInt, targetSampleRate, m_Pll4Denom);
        // TODO: (Stop audio and) reset PTP. (Maybe) complicated by the need to restart the SGTL5000 too.
        // TODO: Actually, this should be doable by checking targetSampleRate before we even get here.
        return;
    }

    // Serial.printf("PLL4 numerator: %23" PRId32 "\n"
    //               "PLL4 denominator: %21" PRIu32 "\n",
    //               numInt, m_Pll4Denom);

    m_Pll4Num = numInt;
}

size_t AudioSystemManager::ClockDividers::printTo(Print &p) const
{
    return p.printf("PLL4 DIV: %" PRIu8
                    ", NUM: %" PRId32
                    ", DENOM: %" PRIu32
                    "; SAI1 PRED: %" PRIu8
                    ", PODF: %" PRIu8,
                    m_Pll4Div,
                    m_Pll4Num,
                    m_Pll4Denom,
                    m_Sai1Pre,
                    m_Sai1Post);
}

FLASHMEM
bool AudioSystemManager::ClockDividers::isPll4FreqValid() const
{
    const auto pll4Freq{getPll4Freq()};
    //    Serial.printf("PLL4 Freq: %" PRIu32 "\n", pll4Freq);
    return pll4Freq > ClockConstants::k_Pll4FreqMin && pll4Freq < ClockConstants::k_Pll4FreqMax;
}

FLASHMEM
bool AudioSystemManager::ClockDividers::isSai1PostFreqValid() const
{
    const auto sai1PostFreq{(uint32_t) ((double) getPll4Freq() / m_Sai1Pre)};
    return sai1PostFreq <= ClockConstants::k_Sai1PostMaxFreq;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getPll4Freq() const
{
    const auto result{ClockConstants::k_OscHz * getPLL4FractionalDivider()};
    return (uint32_t) result;
}

FLASHMEM
double AudioSystemManager::ClockDividers::getCurrentSampleRate() const
{
    return ClockConstants::k_CyclesPerWord * getPLL4FractionalDivider() / (m_Sai1Pre * m_Sai1Post);
}

FLASHMEM
double AudioSystemManager::ClockDividers::getPLL4FractionalDivider() const
{
    return m_Pll4Div + (double) m_Pll4Num / m_Pll4Denom;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getCurrentMaxPossibleSampleRate() const
{
    const auto result{ClockConstants::k_CyclesPerWord * (m_Pll4Div + 1) / (m_Sai1Pre * m_Sai1Post)};
    return (uint32_t) result;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getCurrentSai1ClkRootFreq() const
{
    const auto result{(ClockConstants::k_OscHz * getPLL4FractionalDivider() / (m_Sai1Pre * m_Sai1Post))};
    return (uint32_t) result;
}
