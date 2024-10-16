#include "AudioSystemManager.h"
#include <QNEthernet.h>
#include <TimeLib.h>

DMAChannel AudioSystemManager::s_DMA{false};
bool AudioSystemManager::s_FirstInterrupt{false};
SineWaveGenerator AudioSystemManager::s_SineWaveGenerator;
int16_t AudioSystemManager::s_AudioTxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
int16_t AudioSystemManager::s_AudioRxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
uint16_t AudioSystemManager::s_ReadIndexTx{0};
uint16_t AudioSystemManager::s_WriteIndexTx{0};
size_t AudioSystemManager::s_NumTxFramesAvailable{0};
uint16_t AudioSystemManager::s_ReadIndexRx{k_AudioBufferFrames};
uint16_t AudioSystemManager::s_WriteIndexRx{0};
DMAMEM __attribute__((aligned(32))) uint32_t AudioSystemManager::s_I2sTxBuffer[k_BufferSize];

AudioSystemManager::AudioSystemManager(const AudioSystemConfig config)
    : m_Config(config)
{
}

FLASHMEM
bool AudioSystemManager::begin()
{
    Serial.println(m_Config);

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

    setupPins();

    // The order of these calls is important.
    setClock();

    setupI2S();

    setupDMA();

    // This call must follow the above, and features some significant delays.
    m_AudioShield.enable();
    // TODO: either mute the audio shield till clock startup, or output zeros.
    m_AudioShield.volume(m_Config.k_Volume);

    s_SineWaveGenerator.setFrequency(240);

    // Stop the audio clock till PTP settles down...
    stopClock();

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

void AudioSystemManager::setupDMA() const
{
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

    I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
    I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

    s_DMA.attachInterrupt(
        m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? clockAuthorityISR : clockSubscriberISR,
        80 // default, apparently
    );

    Serial.printf("Audio interrupt priority: %d\n", NVIC_GET_PRIORITY(s_DMA.channel));
}

void AudioSystemManager::isr()
{
    if (s_FirstInterrupt) {
        s_FirstInterrupt = false;

        auto printTime = [](const int64_t t)
        {
            int64_t x = t;
            const int ns = x % 1000;
            x /= 1000;
            const int us = x % 1000;
            x /= 1000;
            const int ms = x % 1000;
            x /= 1000;

            tmElements_t tme;
            breakTime((time_t) x, tme);

            Serial.printf(
                "First interrupt TIME:"
                " %02" PRIu8
                ".%02" PRIu8
                ".%04" PRIu16
                " %02" PRIu8
                ":%02" PRIu8
                ":%02" PRIu8
                "::%03" PRIu32
                ":%03" PRIu32
                ":%03" PRIu32 "\n",
                tme.Day, tme.Month, 1970 + tme.Year, tme.Hour, tme.Minute, tme.Second, ms, us, ns);
        };

        timespec ts;
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        Serial.printf("%" PRId64 " s %" PRId32 " ns\n", ts.tv_sec, ts.tv_nsec);

        auto ns{ts.tv_sec * ClockConstants::k_NanosecondsPerSecond + ts.tv_nsec};
        printTime(ns);

        s_WriteIndexTx = 0;
        // s_ReadIndex = k_AudioBufferFrames;
    }

    int16_t *destination;
    const auto sourceAddress = (uint32_t) s_DMA.TCD->SADDR;
    s_DMA.clearInterrupt();

    if (sourceAddress < (uint32_t) s_I2sTxBuffer + sizeof(s_I2sTxBuffer) / 2) {
        // DMA is transmitting the first half of the buffer
        // so we must fill the second half
        destination = (int16_t *) &s_I2sTxBuffer[k_BufferSize / 2];
    } else {
        // DMA is transmitting the second half of the buffer
        // so we must fill the first half
        destination = (int16_t *) s_I2sTxBuffer;
    }

    if (false) {
        s_SineWaveGenerator.generate(destination, 2, k_BufferSize / 2);
    } else {
        const auto start{2 * s_WriteIndexTx};
        s_WriteIndexTx += k_BufferSize / 2;
        // Generate a signal; write it to the outgoing audio buffer
        if (s_WriteIndexTx >= k_AudioBufferFrames) {
            uint16_t length2{(uint16_t) (s_WriteIndexTx - k_AudioBufferFrames)},
                    length1{(uint16_t) ((k_BufferSize / 2) - length2)};
            s_SineWaveGenerator.generate(s_AudioTxBuffer + start, 2, length1);
            s_SineWaveGenerator.generate(s_AudioTxBuffer, 2, length2);
            s_WriteIndexTx -= k_AudioBufferFrames;
        } else {
            s_SineWaveGenerator.generate(s_AudioTxBuffer + start, 2, k_BufferSize / 2);
        }
        s_NumTxFramesAvailable += k_BufferSize / 2;

        // Read some samples. For now, do so from the outgoing audio buffer.
        // This should be replaced with samples from the incoming audio buffer.
        for (size_t i = 0; i < k_BufferSize / 2; ++i) {
            for (size_t channel = 0; channel < k_AudioBufferChannels; ++channel) {
                destination[k_AudioBufferChannels * i + channel] = s_AudioTxBuffer[k_AudioBufferChannels * s_ReadIndexRx + channel];
            }
            ++s_ReadIndexRx;
            if (s_ReadIndexRx >= k_AudioBufferFrames) {
                s_ReadIndexRx = 0;
            }
        }
    }

    arm_dcache_flush_delete(destination, sizeof(s_I2sTxBuffer) / 2);
}

void AudioSystemManager::clockAuthorityISR()
{
    if (s_FirstInterrupt) {
        s_ReadIndexTx = k_AudioBufferFrames;
    }
    isr();

    // Generate audio.
    // Write to (packet/audio) buffer.
    // If enough new samples are available, signal readiness to transmit packet.
    // Read (n-k)th packet or (n-kT)th -> (n-k(T-1))th samples (T, buffer size).
    // Copy samples to destination.
}

void AudioSystemManager::clockSubscriberISR()
{
    if (s_FirstInterrupt) {
        // Reproduce pi/2 radians out of phase (50 samples @ F0 = 240 Hz @ Fs = 48 kHz)
        // s_ReadIndexTx = k_AudioBufferFrames - 50;
        s_ReadIndexTx = k_AudioBufferFrames;
    }
    isr();

    // Read from (packet/audio) buffer.
    // Compare reproduction time with current time.
    // If time matches (!..), copy samples to destination.
}

FLASHMEM
void AudioSystemManager::setClock()
{
    m_ClockDividers.calculateCoarse(m_Config.k_SampleRate);
    Serial.print(m_ClockDividers);

    m_ClockGatingRegister5.enableSai1Clock();

    m_MiscellaneousRegister2.setAudioPostDiv(m_ClockDividers.k_AudioPostDiv);

    m_SerialClockMultiplexerRegister1.setSai1ClkSel(SerialClockMultiplexerRegister1::Sai1ClkSel::DeriveClockFromPll4);

    m_ClockDividerRegister1.setSai1ClkPred(m_ClockDividers.m_Sai1Pre);
    m_ClockDividerRegister1.setSai1ClkPodf(m_ClockDividers.m_Sai1Post);

    m_GeneralPurposeRegister1.setSai1MclkDirection(GeneralPurposeRegister1::SignalDirection::Output);
    m_GeneralPurposeRegister1.setSai1MclkSource(GeneralPurposeRegister1::Sai1MclkSource::CcmSai1ClkRoot);

    m_AnalogAudioPllControlRegister.setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource::RefClk24M);
    m_AnalogAudioPllControlRegister.setPostDivSelect(m_ClockDividers.k_Pll4PostDiv);
    m_AnalogAudioPllControlRegister.setDivSelect(m_ClockDividers.m_Pll4Div);
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
    m_AudioPllDenominatorRegister.set(m_ClockDividers.m_Pll4Denom);

    // These have to be present. Not necessarily in this order, but if not here
    // the audio subsystem appears to work, but not the audio shield.
    m_AnalogAudioPllControlRegister.setEnable(true);
    m_AnalogAudioPllControlRegister.setPowerDown(false);
    m_AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(m_AnalogAudioPllControlRegister);
}

void AudioSystemManager::setSampleRate(const double targetSampleRate)
{
    m_Config.setExactSampleRate(targetSampleRate);
    Serial.println(m_Config);
    m_ClockDividers.calculateFine(m_Config.getExactSampleRate());
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
}

void AudioSystemManager::startClock() const
{
    s_FirstInterrupt = true;
    s_SineWaveGenerator.reset();

    m_AnalogAudioPllControlRegister.setEnable(true);
    // m_AnalogAudioPllControlRegister.setPowerDown(false);
    // m_AnalogAudioPllControlRegister.setBypass(false);

    // Serial.printf("Clock %s starting audio clock\n",
    //               m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? "authority" : "subscriber");
    // Serial.println(m_AnalogAudioPllControlRegister);
}

void AudioSystemManager::stopClock() const
{
    // m_AnalogAudioPllControlRegister.setBypass(true);
    // m_AnalogAudioPllControlRegister.setPowerDown(false);
    m_AnalogAudioPllControlRegister.setEnable(false);

    // Serial.printf("Clock %s stopping audio clock\n",
    //               m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? "authority" : "subscriber");
    // Serial.println(m_AnalogAudioPllControlRegister);

    s_FirstInterrupt = true;
    s_SineWaveGenerator.reset();
}

volatile bool AudioSystemManager::isClockRunning() const
{
    return m_AnalogAudioPllControlRegister.isClockRunning();
}

void AudioSystemManager::adjustClock(const double nspsDiscrepancy)
{
    // Serial.printf("Skew (nsps): %*.*f\n", 26, 8, nspsDiscrepancy);

    const double proportionalAdjustment{1. + (nspsDiscrepancy / ClockConstants::k_NanosecondsPerSecond)};

    // Serial.printf("Proportional adjustment: %*.*f\n", 14, 8, proportionalAdjustment);

    m_Config.setExactSampleRate(proportionalAdjustment);
    // Serial.println(m_Config);
    m_ClockDividers.calculateFine(m_Config.getExactSampleRate());
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
}

size_t AudioSystemManager::getNumTxFramesAvailable()
{
    return s_NumTxFramesAvailable;
}

void AudioSystemManager::readFromTxAudioBuffer(int16_t *dest, const size_t numChannels, const size_t numSamples)
{
    __disable_irq();
    for (size_t n{0}; n < numSamples; ++n) {
        for (size_t ch{0}; ch < numChannels; ++ch) {
            dest[n * numChannels + ch] = s_AudioTxBuffer[numChannels * s_ReadIndexTx + ch];
        }
        ++s_ReadIndexTx;
        if (s_ReadIndexTx >= k_AudioBufferFrames) {
            s_ReadIndexTx = 0;
        }
        --s_NumTxFramesAvailable;
    }
    __enable_irq();
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
                return;
                printTo(Serial);
                Serial.println();
                break;
            } else {
                break;
            }
        }
    }
}

FLASHMEM
void AudioSystemManager::ClockDividers::calculateFine(const double targetSampleRate)
{
    constexpr auto orderOfMagnitude{1e6};

    const auto sai1WordOsc{(double) ClockConstants::k_AudioWordSize * m_Sai1Pre * m_Sai1Post / ClockConstants::k_OscMHz};
    const auto num{targetSampleRate * sai1WordOsc - orderOfMagnitude * m_Pll4Div};
    const auto numInt{(int32_t) (num * (m_Pll4Denom / orderOfMagnitude))};

    if (numInt < 0 || (uint32_t) numInt > m_Pll4Denom) {
        Serial.printf("Invalid PLL4 numerator %" PRId32 " "
                      "for target sample rate %f "
                      "and denominator %" PRIu32 "\n",
                      numInt, targetSampleRate, m_Pll4Denom);
        return;
    }

    // Serial.printf("PLL4 numerator: %23" PRId32 "\n"
    //               "PLL4 denominator: %21" PRIu32 "\n",
    //               numInt, m_Pll4Denom);

    m_Pll4Num = numInt;
}

size_t AudioSystemManager::ClockDividers::printTo(Print &p) const
{
    return p.printf("PLL4 DIV: %" PRIu8 "\n"
                    "PLL4 NUM: %" PRId32 "\n"
                    "PLL4 DENOM: %" PRIu32 "\n"
                    "SAI1 PRED: %" PRIu8 "\n"
                    "SAI1 PODF: %" PRIu8 "\n",
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
