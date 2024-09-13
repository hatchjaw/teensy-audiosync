#ifndef AUDIOCLOCKMANAGER_H
#define AUDIOCLOCKMANAGER_H

#include "ClockConstants.h"
#include "registers/MiscellaneousRegister2.h"
#include "registers/AnalogAudioPllControlRegister.h"

class AudioClockManager : public Printable
{
public:
    bool begin();

    void setClock(uint32_t targetSampleRate);

    void setClock(double targetSampleRate);

    size_t printTo(Print &p) const override;

    void startClock();

    void stopClock();

private:
    void calculateDividers(uint32_t targetSampleRate);

    void calculatePll4Numerator(double targetSampleRate);

    double getCurrentSampleRate() const;

    uint32_t getPll4Freq() const;

    double getPLL4FractionalDivider() const;

    bool isPll4FreqValid() const;

    bool isSai1PostFreqValid() const;

    uint32_t getCurrentMaxPossibleSampleRate() const;

    uint32_t getCurrentSai1ClkRootFreq() const;

    uint8_t m_pll4Div{ClockConstants::k_pll4DivMin};
    int32_t m_pll4Num{0};
    uint32_t m_pll4Denom{1};
    uint8_t m_sai1Pre{1};
    uint8_t m_sai1Post{1};
    const MiscellaneousRegister2::AudioPostDiv k_audioPostDiv{MiscellaneousRegister2::AudioPostDiv::kDivideBy1};
    const AnalogAudioPllControlRegister::PostDivSelect k_pll4PostDiv{AnalogAudioPllControlRegister::PostDivSelect::kDivideBy1};
};


#endif //AUDIOCLOCKMANAGER_H
