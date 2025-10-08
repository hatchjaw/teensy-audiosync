#include "AudioSystemManager.h"
#include <QNEthernet.h>
#include <t41-ptp.h>
#include <AnanasUtils.h>

AudioSystemManager::AudioSystemManager(const AudioSystemConfig config)
    : config(config)
{
    sInterruptsPerSecond = static_cast<uint16_t>(2 * config.kSampleRate / config.kBufferSize);
}

FLASHMEM
bool AudioSystemManager::begin()
{
    // Calculate fundamental values for clock dividers.
    clockDividers.calculateCoarse(config.kSampleRate);
    Serial.print(clockDividers);

    // Set up the audio processor.
    sAudioProcessor->prepare(config.kSampleRate);

    cycPreReg = ARM_DWT_CYCCNT;

    //==========================================================================
    // Reset all registers
    //==========================================================================
    analogAudioPllControlRegister.begin();
    audioPllNumeratorRegister.begin();
    audioPllDenominatorRegister.begin();
    miscellaneousRegister2.begin();
    serialClockMultiplexerRegister1.begin();
    clockDividerRegister1.begin();
    clockGatingRegister5.begin();
    generalPurposeRegister1.begin();
    pin7SwMuxControlRegister.begin();
    pin20SwMuxControlRegister.begin();
    pin21SwMuxControlRegister.begin();
    pin23SwMuxControlRegister.begin();
    sai1TransmitMaskRegister.begin();
    sai1TransmitConfig1Register.begin();
    sai1TransmitConfig2Register.begin();
    sai1TransmitConfig3Register.begin();
    sai1TransmitConfig4Register.begin();
    sai1TransmitConfig5Register.begin();
    sai1TransmitControlRegister.begin();
    sai1ReceiveMaskRegister.begin();
    sai1ReceiveConfig1Register.begin();
    sai1ReceiveConfig2Register.begin();
    sai1ReceiveConfig3Register.begin();
    sai1ReceiveConfig4Register.begin();
    sai1ReceiveConfig5Register.begin();
    sai1ReceiveControlRegister.begin();

    //==========================================================================
    // Set mux modes for I2S pins.
    //==========================================================================
    // LRCLK1 on pin 20
    pin20SwMuxControlRegister.setMuxMode(Pin20SwMuxControlRegister::MuxMode::Sai1RxSync);
    // BCLK on pin 21
    pin21SwMuxControlRegister.setMuxMode(Pin21SwMuxControlRegister::MuxMode::Sai1RxBclk);
    // MCLK on pin 23
    pin23SwMuxControlRegister.setMuxMode(Pin23SwMuxControlRegister::MuxMode::Sai1Mclk);
    // Data on pin 7
    pin7SwMuxControlRegister.setMuxMode(Pin7SwMuxControlRegister::MuxMode::Sai1TxData00);

    //==========================================================================
    // Set audio PLL (PLL4) and SAI1 clock registers
    //==========================================================================
    // Enable SAI1 clock
    clockGatingRegister5.enableSai1Clock();

    serialClockMultiplexerRegister1.setSai1ClkSel(SerialClockMultiplexerRegister1::Sai1ClkSel::DeriveClockFromPll4);

    // Set audio divider registers
    miscellaneousRegister2.setAudioPostDiv(ClockDividers::kAudioPostDiv);

    clockDividerRegister1.setSai1ClkPred(clockDividers.sai1Pre);
    clockDividerRegister1.setSai1ClkPodf(clockDividers.sai1Post);

    generalPurposeRegister1.setSai1MclkDirection(GeneralPurposeRegister1::SignalDirection::Output);
    generalPurposeRegister1.setSai1MclkSource(GeneralPurposeRegister1::Sai1MclkSource::CcmSai1ClkRoot);

    analogAudioPllControlRegister.setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource::RefClk24M);
    analogAudioPllControlRegister.setPostDivSelect(ClockDividers::kPll4PostDiv);
    analogAudioPllControlRegister.setDivSelect(clockDividers.pll4Div);
    audioPllNumeratorRegister.set(clockDividers.pll4Num);
    audioPllDenominatorRegister.set(clockDividers.pll4Denom);

    //==========================================================================
    // Configure SAI1 (I2S)
    //==========================================================================
    // TODO: check whether transmit/receive are enabled
    sai1TransmitMaskRegister.setMask(0);
    sai1TransmitConfig1Register.setWatermarkLevel(1);
    sai1TransmitConfig2Register.setSyncMode(SAI1TransmitConfig2Register::SyncMode::SynchronousWithReceiver);
    sai1TransmitConfig2Register.setBitClockPolarity(SAI1TransmitConfig2Register::BitClockPolarity::ActiveLow);
    sai1TransmitConfig2Register.setBitClockDirection(SAI1TransmitConfig2Register::BitClockDirection::Internal);
    sai1TransmitConfig2Register.setBitClockDivide(1);
    sai1TransmitConfig2Register.selectMasterClock(SAI1TransmitConfig2Register::Clock::MasterClock1);
    sai1TransmitConfig3Register.enableTransmitChannel(1, true);
    sai1TransmitConfig4Register.setFrameSize(2);
    sai1TransmitConfig4Register.setSyncWidth(32);
    sai1TransmitConfig4Register.setEndianness(SAI1TransmitConfig4Register::Endianness::BigEndian);
    sai1TransmitConfig4Register.setFrameSyncDirection(SAI1TransmitConfig4Register::FrameSyncDirection::Internal);
    sai1TransmitConfig4Register.setFrameSyncEarly(SAI1TransmitConfig4Register::FrameSyncAssert::OneBitEarly);
    sai1TransmitConfig4Register.setFrameSyncPolarity(SAI1TransmitConfig4Register::FrameSyncPolarity::ActiveLow);
    sai1TransmitConfig5Register.setWordNWidth(32);
    sai1TransmitConfig5Register.setWord0Width(32);
    sai1TransmitConfig5Register.setFirstBitShift(32);
    sai1ReceiveMaskRegister.setMask(0);
    sai1ReceiveConfig1Register.setWatermarkLevel(1);
    sai1ReceiveConfig2Register.setSyncMode(SAI1ReceiveConfig2Register::SyncMode::Asynchronous);
    sai1ReceiveConfig2Register.setBitClockPolarity(SAI1ReceiveConfig2Register::BitClockPolarity::ActiveLow);
    sai1ReceiveConfig2Register.setBitClockDirection(SAI1ReceiveConfig2Register::BitClockDirection::Internal);
    sai1ReceiveConfig2Register.setBitClockDivide(1);
    sai1ReceiveConfig2Register.selectMasterClock(SAI1ReceiveConfig2Register::Clock::MasterClock1);
    sai1ReceiveConfig3Register.enableReceiveChannel(1, true);
    sai1ReceiveConfig4Register.setFrameSize(2);
    sai1ReceiveConfig4Register.setSyncWidth(32);
    sai1ReceiveConfig4Register.setEndianness(SAI1ReceiveConfig4Register::Endianness::BigEndian);
    sai1ReceiveConfig4Register.setFrameSyncDirection(SAI1ReceiveConfig4Register::FrameSyncDirection::Internal);
    sai1ReceiveConfig4Register.setFrameSyncEarly(SAI1ReceiveConfig4Register::FrameSyncAssert::OneBitEarly);
    sai1ReceiveConfig4Register.setFrameSyncPolarity(SAI1ReceiveConfig4Register::FrameSyncPolarity::ActiveLow);
    sai1ReceiveConfig5Register.setWordNWidth(32);
    sai1ReceiveConfig5Register.setWord0Width(32);
    sai1ReceiveConfig5Register.setFirstBitShift(32);

    //==========================================================================
    // Set up DMA
    //==========================================================================
    sDMA.begin(true);

    sDMA.TCD->SADDR = sI2sTxBuffer; //source address
    sDMA.TCD->SOFF = 2; // source buffer address increment per transfer in bytes
    sDMA.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1); // specifies 16 bit source and destination
    sDMA.TCD->NBYTES_MLNO = 2; // bytes to transfer for each service request///////////////////////////////////////////////////////////////////
    sDMA.TCD->SLAST = -sizeof(sI2sTxBuffer); // last source address adjustment
    sDMA.TCD->DOFF = 0; // increments at destination
    sDMA.TCD->CITER_ELINKNO = sizeof(sI2sTxBuffer) / 2;
    sDMA.TCD->DLASTSGA = 0; // destination address offset
    sDMA.TCD->BITER_ELINKNO = sizeof(sI2sTxBuffer) / 2;
    sDMA.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
    // interrupts
    sDMA.TCD->DADDR = (void *) ((uint32_t) &I2S1_TDR0 + 2); // I2S1 register DMA writes to
    sDMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX); // i2s channel that will trigger the DMA transfer when ready for data
    sDMA.enable();

    attachInterruptVector(IRQ_SOFTWARE, softwareISR);
    NVIC_SET_PRIORITY(IRQ_SOFTWARE, 208); // 255 = lowest priority
    NVIC_ENABLE_IRQ(IRQ_SOFTWARE);

    sDMA.attachInterrupt(isr);

    sNumInterrupts = -1;
    sFirstInterruptNS = 0;

    cycPostStop = ARM_DWT_CYCCNT;

    // With predictability of timing in mind, report setup here, rather than
    // between calls to set up registers.
    Serial.println(config);
    // Serial.printf("Audio interrupt priority: %d\n", NVIC_GET_PRIORITY(s_DMA.channel));

    Serial.printf(
        "=== Audio system setup took %" PRIu32 " cycles (%" PRIu32 " ns).\n",
        cycPostStop - cycPreReg, ananas::Utils::cyclesToNs(cycPostStop - cycPreReg));

    return true;
}

void AudioSystemManager::startClock()
{
    analogAudioPllControlRegister.setPowerDown(false);
    analogAudioPllControlRegister.setBypass(false);
    analogAudioPllControlRegister.setEnable(true);

    sai1TransmitControlRegister.setBitClockEnable(true);
    sai1TransmitControlRegister.setFIFORequestDMAEnable(true);
    sai1TransmitControlRegister.setTransmitterEnable(true);
    sai1ReceiveControlRegister.setBitClockEnable(true);
    sai1ReceiveControlRegister.setReceiverEnable(true);

    audioShield.enable();
    audioShield.volume(config.volume);

    sNumInterrupts = -1;
    sFirstInterruptNS = 0;
}

void AudioSystemManager::stopClock()
{
    audioShield.volume(0.f);

    analogAudioPllControlRegister.setEnable(false);
    analogAudioPllControlRegister.setPowerDown(true);
    analogAudioPllControlRegister.setBypass(true);

    audioShield.disable();
    audioShield.volume(0.f);

    sNumInterrupts = -1;
    sFirstInterruptNS = 0;
}

volatile bool AudioSystemManager::isClockRunning() const
{
    return analogAudioPllControlRegister.isClockRunning();
}

FLASHMEM
void AudioSystemManager::setAudioProcessor(AudioProcessor *processor)
{
    sAudioProcessor = processor;
}

size_t AudioSystemManager::printTo(Print &p) const
{
    return p.println(config)
           + p.print(clockDividers)
           + p.printf("\nAudio-PTP offset: %" PRId32 " ns", sAudioPTPOffset);
}

void AudioSystemManager::adjustClock(const double adjust)
{
    const double proportionalAdjustment{
        1. + (adjust + sAudioPTPOffset) * ClockConstants::Nanosecond
    };

    config.setExactSampleRate(proportionalAdjustment);
    if (!clockDividers.calculateFine(config.getExactSampleRate())) {
        if (invalidSamplingRateCallback != nullptr) {
            invalidSamplingRateCallback();
        }
        return;
    }

    // Tends to take on the order of 28 ns.
    // const auto cycles{ARM_DWT_CYCCNT};
    audioPllNumeratorRegister.set(clockDividers.pll4Num);
    // Serial.printf("Fs update took %" PRIu32 " ns\n", ananas::Utils::cyclesToNs(ARM_DWT_CYCCNT - cycles));
}

void AudioSystemManager::isr()
{
    if (sNumInterrupts == -1) {
        sNumInterrupts = 0;
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        const auto ns{ts.tv_sec * ClockConstants::NanosecondsPerSecond + ts.tv_nsec};
        Serial.print("First interrupt time: ");
        printTime(ns);
        Serial.println();
    }

    if (++sNumInterrupts >= sInterruptsPerSecond) {
        sNumInterrupts = 0;
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);

        // Get a reference nanosecond figure to use for comparison.
        if (sFirstInterruptNS == 0) {
            sFirstInterruptNS = ts.tv_nsec;
        }

        // Each second, compare the number of nanoseconds with the reference.
        // TODO: check for zero-wraparound, e.g. if first NS is 999,999,999, and
        //   a later timer read gives 1, this will break...
        // TODO: sometimes settles into an oscillating pattern with an amplitude
        //   of a couple a few hundred ns. Would a PI controller help?
        sAudioPTPOffset = ts.tv_nsec - sFirstInterruptNS;
    }

    int16_t *destination, *source;
    const auto sourceAddress = (uint32_t) sDMA.TCD->SADDR;
    sDMA.clearInterrupt();

    if (sourceAddress < reinterpret_cast<uint32_t>(sI2sTxBuffer) + sizeof(sI2sTxBuffer) / 2) {
        triggerAudioProcessing();
        // DMA is transmitting the first half of the buffer; fill the second half.
        destination = reinterpret_cast<int16_t *>(&sI2sTxBuffer[ananas::Constants::AudioBlockFrames / 2]);
        source = &sAudioBuffer[ananas::Constants::AudioBlockFrames];
    } else {
        // DMA is transmitting the second half of the buffer; fill the first half.
        destination = reinterpret_cast<int16_t *>(sI2sTxBuffer);
        source = sAudioBuffer;
    }

    memcpy(destination, source, ananas::Constants::AudioBlockFrames * sizeof(int16_t));

    arm_dcache_flush_delete(destination, sizeof(sI2sTxBuffer) / 2);
}

void AudioSystemManager::triggerAudioProcessing()
{
    NVIC_SET_PENDING(IRQ_SOFTWARE);
}

void AudioSystemManager::softwareISR()
{
    // sAudioProcessor->processAudio(sAudioBuffer, 2, ananas::Constants::AudioBlockFrames);
    sAudioProcessor->processAudioV2(ananas::Constants::AudioBlockFrames);
    sAudioProcessor->getOutputInterleaved(sAudioBuffer, 2, ananas::Constants::AudioBlockFrames);
}

FLASHMEM
void AudioSystemManager::ClockDividers::calculateCoarse(const uint32_t targetSampleRate)
{
    constexpr auto orderOfMagnitude{1e6};

    for (pll4Div = ClockConstants::Pll4DivMin; pll4Div <= ClockConstants::Pll4DivMax; ++pll4Div) {
        pll4Num = 0;
        pll4Denom = (uint32_t) orderOfMagnitude;
        sai1Pre = 1;
        sai1Post = 1;
        auto divExact{-1.};

        while (!isSai1PostFreqValid() && sai1Pre < ClockConstants::Sai1PreMax) {
            ++sai1Pre;
        }

        while ((uint8_t) floor(divExact / orderOfMagnitude) != pll4Div) {
            while ((uint32_t) getCurrentSampleRate() > targetSampleRate
                   && sai1Post < ClockConstants::Sai1PostMax) {
                ++sai1Post;
            }

            if (targetSampleRate > getCurrentMaxPossibleSampleRate() && sai1Pre < ClockConstants::Sai1PreMax) {
                ++sai1Pre;
                sai1Post = 1;
                continue;
            }

            divExact = (double) targetSampleRate * ClockConstants::AudioWordSize * sai1Pre * sai1Post / ClockConstants::OscMHz;

            if ((uint8_t) floor(divExact / orderOfMagnitude) != pll4Div) {
                if (sai1Pre < ClockConstants::Sai1PreMax) {
                    ++sai1Pre;
                    sai1Post = 1;
                    continue;
                } else {
                    break;
                }
            }

            pll4Num = (int32_t) floor(divExact - orderOfMagnitude * pll4Div);

            while (pll4Num * 10 < ClockConstants::Pll4NumMax && pll4Denom * 10 < ClockConstants::Pll4DenomMax) {
                pll4Num *= 10;
                pll4Denom *= 10;
            }

            // TODO: prefer high precision denom, i.e. where num < (1 << 29) - 1
            if (isPll4FreqValid()
                //                && getPll4Freq() > (ClockConstants::k_pll4FreqMin + ClockConstants::k_pll4FreqMax) / 2
                && pll4Denom == 1'000'000'000) {
                // Serial.println(*this);
                return;
            } else {
                break;
            }
        }
    }
}

bool AudioSystemManager::ClockDividers::calculateFine(const double targetSampleRate)
{
    constexpr auto orderOfMagnitude{1e6};

    const auto sai1WordOsc{(double) ClockConstants::AudioWordSize * sai1Pre * sai1Post / ClockConstants::OscMHz};
    const auto num{targetSampleRate * sai1WordOsc - orderOfMagnitude * pll4Div};
    const auto numInt{(int32_t) round(num * (pll4Denom / orderOfMagnitude))};

    if (numInt < 0 || (uint32_t) numInt > pll4Denom) {
        Serial.printf("Invalid PLL4 numerator %" PRId32 " "
                      "for target sample rate %f "
                      "and denominator %" PRIu32 "\n",
                      numInt, targetSampleRate, pll4Denom);
        return false;
    }

    // Serial.printf("PLL4 numerator: %23" PRId32 "\n"
    //               "PLL4 denominator: %21" PRIu32 "\n",
    //               numInt, pll4Denom);

    pll4Num = numInt;
    return true;
}

size_t AudioSystemManager::ClockDividers::printTo(Print &p) const
{
    return p.printf("PLL4 DIV: %" PRIu8
                    ", NUM: %" PRId32
                    ", DENOM: %" PRIu32
                    "; SAI1 PRED: %" PRIu8
                    ", PODF: %" PRIu8,
                    pll4Div,
                    pll4Num,
                    pll4Denom,
                    sai1Pre,
                    sai1Post);
}

FLASHMEM
bool AudioSystemManager::ClockDividers::isPll4FreqValid() const
{
    const auto pll4Freq{getPll4Freq()};
    //    Serial.printf("PLL4 Freq: %" PRIu32 "\n", pll4Freq);
    return pll4Freq > ClockConstants::Pll4FreqMin && pll4Freq < ClockConstants::Pll4FreqMax;
}

FLASHMEM
bool AudioSystemManager::ClockDividers::isSai1PostFreqValid() const
{
    const auto sai1PostFreq{(uint32_t) ((double) getPll4Freq() / sai1Pre)};
    return sai1PostFreq <= ClockConstants::Sai1PostMaxFreq;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getPll4Freq() const
{
    const auto result{ClockConstants::OscHz * getPLL4FractionalDivider()};
    return (uint32_t) result;
}

FLASHMEM
double AudioSystemManager::ClockDividers::getCurrentSampleRate() const
{
    return ClockConstants::CyclesPerWord * getPLL4FractionalDivider() / (sai1Pre * sai1Post);
}

FLASHMEM
double AudioSystemManager::ClockDividers::getPLL4FractionalDivider() const
{
    return pll4Div + (double) pll4Num / pll4Denom;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getCurrentMaxPossibleSampleRate() const
{
    const auto result{ClockConstants::CyclesPerWord * (pll4Div + 1) / (sai1Pre * sai1Post)};
    return (uint32_t) result;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getCurrentSai1ClkRootFreq() const
{
    const auto result{(ClockConstants::OscHz * getPLL4FractionalDivider() / (sai1Pre * sai1Post))};
    return (uint32_t) result;
}
