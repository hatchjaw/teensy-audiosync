#include "AudioSystemManager.h"

DMAChannel AudioSystemManager::s_DMA{false};
bool AudioSystemManager::s_ReadyForNewSample{false};
bool AudioSystemManager::s_DoImpulse{false};
uint32_t AudioSystemManager::s_Counter{0};
DMAMEM __attribute__((aligned(32))) static uint16_t i2sTxBuffer[2];

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

    setClock();

    setupPins();

    setupI2S();

    setupDMA();

    m_AudioShield.enable();
    m_AudioShield.volume(m_Config.k_Volume);

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

    s_DMA.TCD->SADDR = i2sTxBuffer; //source address
    s_DMA.TCD->SOFF = 2; // source buffer address increment per transfer in bytes
    s_DMA.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1); // specifies 16 bit source and destination
    s_DMA.TCD->NBYTES_MLNO = 2; // bytes to transfer for each service request///////////////////////////////////////////////////////////////////
    s_DMA.TCD->SLAST = -sizeof(i2sTxBuffer); // last source address adjustment
    s_DMA.TCD->DOFF = 0; // increments at destination
    s_DMA.TCD->CITER_ELINKNO = sizeof(i2sTxBuffer) / 2;
    s_DMA.TCD->DLASTSGA = 0; // destination address offset
    s_DMA.TCD->BITER_ELINKNO = sizeof(i2sTxBuffer) / 2;
    s_DMA.TCD->CSR = DMA_TCD_CSR_INTHALF; //| DMA_TCD_CSR_INTMAJOR; // enables interrupt when transfers half complete. SET TO 0 to disable DMA interrupts
    s_DMA.TCD->DADDR = (void *) ((uint32_t) &I2S1_TDR0 + 2); // I2S1 register DMA writes to
    s_DMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX); // i2s channel that will trigger the DMA transfer when ready for data
    s_DMA.enable();
    I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
    I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
    s_DMA.attachInterrupt(
        m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? clockAuthorityISR : clockSubscriberISR
    );
}

void AudioSystemManager::clockAuthorityISR()
{
    ++s_Counter;

    if (s_Counter >= 1 && s_Counter < 5) {
        s_DoImpulse = true;
    } else if (s_Counter == 2 * AUDIO_SAMPLE_RATE_EXACT) {
        s_Counter = 0;
    }

    if (s_ReadyForNewSample) {
        //        if (s_DoImpulse) {
        //            s_DoImpulse = false;
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

        if (s_DoImpulse) {
            s_DoImpulse = false;
            i2sTxBuffer[0] = (1 << 15) - 1;
            i2sTxBuffer[1] = (1 << 15) - 1;
        } else {
            i2sTxBuffer[0] = 0;
            i2sTxBuffer[1] = 0;
        }


        // Flush buffer and clear interrupt
        arm_dcache_flush_delete(i2sTxBuffer, sizeof(i2sTxBuffer));
        s_DMA.clearInterrupt();
    }
    s_ReadyForNewSample = 1 - s_ReadyForNewSample;
}

void AudioSystemManager::clockSubscriberISR()
{
    ++s_Counter;
    //    if (s_Counter > 2 * static_cast<int>(AUDIO_SAMPLE_RATE_EXACT) - 10) {
    //        s_DoImpulse = true;
    //    }
    if (s_Counter >= 1 && s_Counter < 5) {
        s_DoImpulse = true;
    } else if (s_Counter == 2 * AUDIO_SAMPLE_RATE_EXACT) {
        s_Counter = 0;
    }

    if (s_ReadyForNewSample) {
        //        if (s_DoImpulse) {
        //            s_DoImpulse = false;
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

        if (s_DoImpulse) {
            s_DoImpulse = false;
            i2sTxBuffer[0] = (1 << 15) - 1;
        } else {
            i2sTxBuffer[0] = 0;
        }
        //        else if (s_Counter > 1.666 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 2) - 1) << 7);
        //        } else if (s_Counter > 1.333 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 4) - 1) << 6);
        //        } else if (s_Counter > AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 6) - 1) << 5);
        //        } else if (s_Counter > .666 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 8) - 1) << 4);
        //        } else if (s_Counter > .333 * AUDIO_SAMPLE_RATE_EXACT) {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 10) - 1) << 3);
        //        } else {
        //            i2s_tx_buffer[0] = ((1 << 16) - 1) ^ (((1 << 12) - 1) << 2);
        //        }

        i2sTxBuffer[1] = 0;

        // Flush buffer and clear interrupt
        arm_dcache_flush_delete(i2sTxBuffer, sizeof(i2sTxBuffer));
        s_DMA.clearInterrupt();
    }
    s_ReadyForNewSample = 1 - s_ReadyForNewSample;
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
    // the audio subsystem appears not to work.
    m_AnalogAudioPllControlRegister.setEnable(true);
    m_AnalogAudioPllControlRegister.setPowerDown(false);
    m_AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(m_AnalogAudioPllControlRegister);
}

void AudioSystemManager::setSampleRate(const double targetSampleRate)
{
    m_Config.m_SampleRateExact = targetSampleRate;
    Serial.println(m_Config);
    m_ClockDividers.calculateFine(m_Config.m_SampleRateExact);
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
}

void AudioSystemManager::startClock() const
{
    m_AnalogAudioPllControlRegister.setEnable(true);
    //    AnalogAudioPllControlRegister.setPowerDown(false);
    //    AnalogAudioPllControlRegister.setBypass(false)

    Serial.printf("Clock %s starting audio clock\n",
                  m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? "authority" : "subscriber");
    Serial.println(m_AnalogAudioPllControlRegister);

    s_Counter = 0;
}

void AudioSystemManager::stopClock() const
{
    //    AnalogAudioPllControlRegister.setBypass(true);
    //    AnalogAudioPllControlRegister.setPowerDown(false);
    m_AnalogAudioPllControlRegister.setEnable(false);

    Serial.printf("Clock %s stopping audio clock\n",
                  m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? "authority" : "subscriber");
    Serial.println(m_AnalogAudioPllControlRegister);
}

volatile bool AudioSystemManager::isClockRunning() const
{
    return m_AnalogAudioPllControlRegister.isClockRunning();
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

    Serial.printf("PLL4 numerator:   %16" PRId32 "\n"
                  "PLL4 denominator: %16" PRIu32 "\n",
                  numInt, m_Pll4Denom);

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
