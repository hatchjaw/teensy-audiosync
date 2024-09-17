#include "AudioSystemManager.h"
#include "registers/ClockDividerRegister1.h"
#include "registers/SerialClockMultiplexerRegister1.h"
#include "registers/ClockGatingRegister5.h"
#include "registers/GeneralPurposeRegister1.h"
#include "registers/SwMuxControlRegister.h"

extern AnalogAudioPllControlRegister &AnalogAudioPllControlRegister;
extern AudioPllNumeratorRegister &AudioPllNumeratorRegister;
extern AudioPllDenominatorRegister &AudioPllDenominatorRegister;
extern ClockDividerRegister1 &ClockDividerRegister1;
extern SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1;
extern MiscellaneousRegister2 &MiscellaneousRegister2;
extern ClockGatingRegister5 &ClockGatingRegister5;
extern GeneralPurposeRegister1 &GeneralPurposeRegister1;
extern Pin7SwMuxControlRegister &Pin7SwMuxControlRegister;
extern Pin20SwMuxControlRegister &Pin20SwMuxControlRegister;
extern Pin21SwMuxControlRegister &Pin21SwMuxControlRegister;
extern Pin23SwMuxControlRegister &Pin23SwMuxControlRegister;

AudioSystemManager::AudioSystemManager(uint32_t sampleRate, uint16_t blockSize)
    : m_SampleRate(sampleRate), m_SampleRateExact(sampleRate), m_BlockSize(blockSize)
{
}

size_t AudioSystemManager::printTo(Print &p) const
{
    return p.printf("Frames/Fs: %" PRIu16 "/%" PRIu32 " (%.16f)\n",
                    m_BlockSize, m_SampleRate, m_SampleRateExact);
}

FLASHMEM
bool AudioSystemManager::begin()
{
    AnalogAudioPllControlRegister.begin();
    AudioPllNumeratorRegister.begin();
    AudioPllDenominatorRegister.begin();
    MiscellaneousRegister2.begin();
    SerialClockMultiplexerRegister1.begin();
    ClockDividerRegister1.begin();
    ClockGatingRegister5.begin();
    GeneralPurposeRegister1.begin();
    Pin7SwMuxControlRegister.begin();
    Pin20SwMuxControlRegister.begin();
    Pin21SwMuxControlRegister.begin();
    Pin23SwMuxControlRegister.begin();

    setClock();

    setupPins();

    setupI2S();

    return true;
}

void AudioSystemManager::setupPins() const
{
    // LRCLK1 on pin 20
    Pin20SwMuxControlRegister.setMuxMode(Pin20SwMuxControlRegister::MuxMode::Sai1RxSync);
    // BCLK on pin 21
    Pin21SwMuxControlRegister.setMuxMode(Pin21SwMuxControlRegister::MuxMode::Sai1RxBclk);
    // MCLK on pin 23
    Pin23SwMuxControlRegister.setMuxMode(Pin23SwMuxControlRegister::MuxMode::Sai1Mclk);
    // Data on pin 7
    Pin7SwMuxControlRegister.setMuxMode(Pin7SwMuxControlRegister::MuxMode::Sai1TxData00);
}

void AudioSystemManager::setupI2S() const
{
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
    I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP // sync=0; rx is async;
                | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
    I2S1_RCR3 = I2S_RCR3_RCE;
    I2S1_RCR4 = I2S_RCR4_FRSZ((2 - 1)) | I2S_RCR4_SYWD((32 - 1)) | I2S_RCR4_MF
                | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
    I2S1_RCR5 = I2S_RCR5_WNW((32 - 1)) | I2S_RCR5_W0W((32 - 1)) | I2S_RCR5_FBT((32 - 1));
}

FLASHMEM
void AudioSystemManager::setClock()
{
    printTo(Serial);
    m_ClockDividers.calculateCoarse(m_SampleRate);
    Serial.print(m_ClockDividers);

    ClockGatingRegister5.enableSai1Clock();

    MiscellaneousRegister2.setAudioPostDiv(m_ClockDividers.k_AudioPostDiv);

    SerialClockMultiplexerRegister1.setSai1ClkSel(SerialClockMultiplexerRegister1::Sai1ClkSel::DeriveClockFromPll4);

    ClockDividerRegister1.setSai1ClkPred(m_ClockDividers.m_Sai1Pre);
    ClockDividerRegister1.setSai1ClkPodf(m_ClockDividers.m_Sai1Post);

    GeneralPurposeRegister1.setSai1MclkDirection(GeneralPurposeRegister1::SignalDirection::Output);
    GeneralPurposeRegister1.setSai1MclkSource(GeneralPurposeRegister1::Sai1MclkSource::CcmSai1ClkRoot);

    AnalogAudioPllControlRegister.setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource::RefClk24M);
    AnalogAudioPllControlRegister.setPostDivSelect(m_ClockDividers.k_Pll4PostDiv);
    AnalogAudioPllControlRegister.setDivSelect(m_ClockDividers.m_Pll4Div);
    AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
    AudioPllDenominatorRegister.set(m_ClockDividers.m_Pll4Denom);

    // These have to be present. Not necessarily in this order, but if not here
    // the audio subsystem appears not to work.
    AnalogAudioPllControlRegister.setEnable(true);
    AnalogAudioPllControlRegister.setPowerDown(false);
    AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(AnalogAudioPllControlRegister);
}

void AudioSystemManager::setSampleRate(double targetSampleRate)
{
    m_SampleRateExact = targetSampleRate;
    printTo(Serial);
    m_ClockDividers.calculateFine(m_SampleRateExact);
    AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
}

void AudioSystemManager::startClock()
{
    AnalogAudioPllControlRegister.setEnable(true);
    //    AnalogAudioPllControlRegister.setPowerDown(false);
    //    AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(AnalogAudioPllControlRegister);
}

void AudioSystemManager::stopClock()
{
    //    AnalogAudioPllControlRegister.setBypass(true);
    //    AnalogAudioPllControlRegister.setPowerDown(false);
    AnalogAudioPllControlRegister.setEnable(false);

    Serial.println(AnalogAudioPllControlRegister);
}

FLASHMEM
void AudioSystemManager::ClockDividers::calculateCoarse(const uint32_t targetSampleRate)
{
    auto orderOfMagnitude{1e6};

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

            auto pll4Num = floor(divExact - orderOfMagnitude * m_Pll4Div);
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
    auto orderOfMagnitude{1e6};

    auto sai1WordOsc{(double) ClockConstants::k_AudioWordSize * m_Sai1Pre * m_Sai1Post / ClockConstants::k_OscMHz};
    auto num{targetSampleRate * sai1WordOsc - orderOfMagnitude * m_Pll4Div};
    auto numInt{(int32_t) (num * (m_Pll4Denom / orderOfMagnitude))};

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
    auto pll4Freq{getPll4Freq()};
    //    Serial.printf("PLL4 Freq: %" PRIu32 "\n", pll4Freq);
    return pll4Freq > ClockConstants::k_Pll4FreqMin && pll4Freq < ClockConstants::k_Pll4FreqMax;
}

FLASHMEM
bool AudioSystemManager::ClockDividers::isSai1PostFreqValid() const
{
    auto sai1PostFreq{(uint32_t) ((double) getPll4Freq() / m_Sai1Pre)};
    return sai1PostFreq <= ClockConstants::k_Sai1PostMaxFreq;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getPll4Freq() const
{
    auto result{ClockConstants::k_OscHz * getPLL4FractionalDivider()};
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
    auto result{ClockConstants::k_CyclesPerWord * (m_Pll4Div + 1) / (m_Sai1Pre * m_Sai1Post)};
    return (uint32_t) result;
}

FLASHMEM
uint32_t AudioSystemManager::ClockDividers::getCurrentSai1ClkRootFreq() const
{
    auto result{(ClockConstants::k_OscHz * getPLL4FractionalDivider() / (m_Sai1Pre * m_Sai1Post))};
    return (uint32_t) result;
}
