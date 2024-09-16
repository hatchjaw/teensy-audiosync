#include "AudioClockManager.h"
#include "registers/ClockDividerRegister1.h"
#include "registers/SerialClockMultiplexerRegister1.h"
#include "registers/ClockGatingRegister5.h"
#include "registers/GeneralPurposeRegister1.h"

extern AnalogAudioPllControlRegister &AnalogAudioPllControlRegister;
extern AudioPllNumeratorRegister &AudioPllNumeratorRegister;
extern AudioPllDenominatorRegister &AudioPllDenominatorRegister;
extern ClockDividerRegister1 &ClockDividerRegister1;
extern SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1;
extern MiscellaneousRegister2 &MiscellaneousRegister2;
extern ClockGatingRegister5 &ClockGatingRegister5;
extern GeneralPurposeRegister1 &GeneralPurposeRegister1;

FLASHMEM
bool AudioClockManager::begin()
{
    AnalogAudioPllControlRegister.begin();
    AudioPllNumeratorRegister.begin();
    AudioPllDenominatorRegister.begin();
    MiscellaneousRegister2.begin();
    SerialClockMultiplexerRegister1.begin();
    ClockDividerRegister1.begin();
    ClockGatingRegister5.begin();
    GeneralPurposeRegister1.begin();
    return true;
}

FLASHMEM
void AudioClockManager::setClock(uint32_t targetSampleRate)
{
    calculateDividers(targetSampleRate);

    printTo(Serial);

    ClockGatingRegister5.enableSai1Clock();

    MiscellaneousRegister2.setAudioPostDiv(k_audioPostDiv);

    SerialClockMultiplexerRegister1.setSai1ClkSel(SerialClockMultiplexerRegister1::Sai1ClkSel::kDeriveClockFromPll4);

    ClockDividerRegister1.setSai1ClkPred(m_sai1Pre);
    ClockDividerRegister1.setSai1ClkPodf(m_sai1Post);

    GeneralPurposeRegister1.setSai1MclkDirection(GeneralPurposeRegister1::SignalDirection::kOutput);
    GeneralPurposeRegister1.setSai1MclkSource(GeneralPurposeRegister1::Sai1MclkSource::kCcmSsi1ClkRoot);

    AnalogAudioPllControlRegister.setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource::kRefClk24M);
    AnalogAudioPllControlRegister.setPostDivSelect(k_pll4PostDiv);
    AnalogAudioPllControlRegister.setDivSelect(m_pll4Div);
    AudioPllNumeratorRegister.set(m_pll4Num);
    AudioPllDenominatorRegister.set(m_pll4Denom);

    // These have to be present. Not necessarily in this order, but if not here
    // the audio subsystem appears not to work.
    AnalogAudioPllControlRegister.setEnable(true);
    AnalogAudioPllControlRegister.setPowerDown(false);
    AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(AnalogAudioPllControlRegister);
}

FLASHMEM
void AudioClockManager::setClock(double targetSampleRate)
{
    calculatePll4Numerator(targetSampleRate);
    AudioPllNumeratorRegister.set(m_pll4Num);
}

void AudioClockManager::startClock()
{
    AnalogAudioPllControlRegister.setEnable(true);
//    AnalogAudioPllControlRegister.setPowerDown(false);
//    AnalogAudioPllControlRegister.setBypass(false);

    Serial.println(AnalogAudioPllControlRegister);
}

void AudioClockManager::stopClock()
{
//    AnalogAudioPllControlRegister.setBypass(true);
//    AnalogAudioPllControlRegister.setPowerDown(false);
    AnalogAudioPllControlRegister.setEnable(false);

    Serial.println(AnalogAudioPllControlRegister);
}

FLASHMEM
void AudioClockManager::calculateDividers(uint32_t targetSampleRate)
{
    auto orderOfMagnitude{1e6};

    for (m_pll4Div = ClockConstants::k_pll4DivMin; m_pll4Div <= ClockConstants::k_pll4DivMax; ++m_pll4Div) {
        m_pll4Num = 0;
        m_pll4Denom = (uint32_t) orderOfMagnitude;
        m_sai1Pre = 1;
        m_sai1Post = 1;
        auto divExact{-1.};

        while (!isSai1PostFreqValid() && m_sai1Pre < ClockConstants::k_sai1PreMax) {
            ++m_sai1Pre;
        }

        while ((uint8_t) floor(divExact / orderOfMagnitude) != m_pll4Div) {
            while ((uint32_t) getCurrentSampleRate() > targetSampleRate
                   && m_sai1Post < ClockConstants::k_sai1PostMax) {
                ++m_sai1Post;
            }

            if (targetSampleRate > getCurrentMaxPossibleSampleRate() && m_sai1Pre < ClockConstants::k_sai1PreMax) {
                ++m_sai1Pre;
                m_sai1Post = 1;
                continue;
            }

            divExact = (double) targetSampleRate * ClockConstants::k_audioWordSize * m_sai1Pre * m_sai1Post / ClockConstants::k_oscMHz;

            if ((uint8_t) floor(divExact / orderOfMagnitude) != m_pll4Div) {
                if (m_sai1Pre < ClockConstants::k_sai1PreMax) {
                    ++m_sai1Pre;
                    m_sai1Post = 1;
                    continue;
                } else {
                    break;
                }
            }

            auto pll4Num = floor(divExact - orderOfMagnitude * m_pll4Div);
            m_pll4Num = (int32_t) pll4Num;

            while (m_pll4Num * 10 < ClockConstants::k_pll4NumMax && m_pll4Denom * 10 < ClockConstants::k_pll4DenomMax) {
                m_pll4Num *= 10;
                m_pll4Denom *= 10;
            }

            // TODO: prefer high precision denom, i.e. where num < (1 << 29) - 1
            if (isPll4FreqValid()
//                && getPll4Freq() > (ClockConstants::k_pll4FreqMin + ClockConstants::k_pll4FreqMax) / 2
                && m_pll4Denom == 1'000'000'000) {
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
void AudioClockManager::calculatePll4Numerator(double targetSampleRate)
{
    auto orderOfMagnitude{1e6};

    auto sai1WordOsc{(double) ClockConstants::k_audioWordSize * m_sai1Pre * m_sai1Post / ClockConstants::k_oscMHz};
    auto num{targetSampleRate * sai1WordOsc - orderOfMagnitude * m_pll4Div};
    auto numInt{(int32_t) (num * (m_pll4Denom / orderOfMagnitude))};

    if (numInt < 0 || (uint32_t) numInt > m_pll4Denom) {
        Serial.printf("Invalid PLL4 numerator %" PRId32 " "
                      "for target sample rate %f "
                      "and denominator %" PRIu32 "\n",
                      numInt, targetSampleRate, m_pll4Denom);
        return;
    }

    Serial.printf("PLL4 numerator:   %16" PRId32 "\n"
                  "PLL4 denominator: %16" PRIu32 "\n",
                  numInt, m_pll4Denom);

    m_pll4Num = numInt;
}

size_t AudioClockManager::printTo(Print &p) const
{
    return p.printf("PLL4 DIV: %" PRIu8 "\n"
                    "PLL4 NUM: %" PRId32 "\n"
                    "PLL4 DENOM: %" PRIu32 "\n"
                    "SAI1 PRED: %" PRIu8 "\n"
                    "SAI1 PODF: %" PRIu8 "\n",
                    m_pll4Div,
                    m_pll4Num,
                    m_pll4Denom,
                    m_sai1Pre,
                    m_sai1Post);
}

FLASHMEM
bool AudioClockManager::isPll4FreqValid() const
{
    auto pll4Freq{getPll4Freq()};
//    Serial.printf("PLL4 Freq: %" PRIu32 "\n", pll4Freq);
    return pll4Freq > ClockConstants::k_pll4FreqMin && pll4Freq < ClockConstants::k_pll4FreqMax;
}

FLASHMEM
bool AudioClockManager::isSai1PostFreqValid() const
{
    auto sai1PostFreq{(uint32_t) ((double) getPll4Freq() / m_sai1Pre)};
    return sai1PostFreq <= ClockConstants::k_sai1PostMaxFreq;
}

FLASHMEM
uint32_t AudioClockManager::getPll4Freq() const
{
    auto result{ClockConstants::k_oscHz * getPLL4FractionalDivider()};
    return (uint32_t) result;
}

FLASHMEM
double AudioClockManager::getCurrentSampleRate() const
{
    return ClockConstants::k_cyclesPerWord * getPLL4FractionalDivider() / (m_sai1Pre * m_sai1Post);
}

FLASHMEM
double AudioClockManager::getPLL4FractionalDivider() const
{
    return m_pll4Div + (double) m_pll4Num / m_pll4Denom;
}

FLASHMEM
uint32_t AudioClockManager::getCurrentMaxPossibleSampleRate() const
{
    auto result{ClockConstants::k_cyclesPerWord * (m_pll4Div + 1) / (m_sai1Pre * m_sai1Post)};
    return (uint32_t) result;
}

FLASHMEM
uint32_t AudioClockManager::getCurrentSai1ClkRootFreq() const
{
    auto result{(ClockConstants::k_oscHz * getPLL4FractionalDivider() / (m_sai1Pre * m_sai1Post))};
    return (uint32_t) result;
}
