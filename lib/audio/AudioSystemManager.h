#ifndef AUDIOCLOCKMANAGER_H
#define AUDIOCLOCKMANAGER_H

#include "ClockConstants.h"
#include "registers/MiscellaneousRegister2.h"
#include "registers/AnalogAudioPllControlRegister.h"

class AudioSystemManager : public Printable
{
public:
    AudioSystemManager(uint32_t sampleRate, uint16_t blockSize);

    size_t printTo(Print &p) const override;

    bool begin();

    void setSampleRate(double targetSampleRate);

    void startClock();

    void stopClock();

private:
    struct ClockDividers : Printable
    {
        uint8_t m_Pll4Div{ClockConstants::k_Pll4DivMin};
        int32_t m_Pll4Num{0};
        uint32_t m_Pll4Denom{1};
        uint8_t m_Sai1Pre{1};
        uint8_t m_Sai1Post{1};
        const MiscellaneousRegister2::AudioPostDiv k_AudioPostDiv{MiscellaneousRegister2::AudioPostDiv::DivideBy1};
        const AnalogAudioPllControlRegister::PostDivSelect k_Pll4PostDiv{AnalogAudioPllControlRegister::PostDivSelect::DivideBy1};

        size_t printTo(Print &p) const override;

        void calculateCoarse(uint32_t targetSampleRate);

        void calculateFine(double targetSampleRate);

    private:
        double getCurrentSampleRate() const;

        uint32_t getPll4Freq() const;

        double getPLL4FractionalDivider() const;

        bool isPll4FreqValid() const;

        bool isSai1PostFreqValid() const;

        uint32_t getCurrentMaxPossibleSampleRate() const;

        uint32_t getCurrentSai1ClkRootFreq() const;
    };

    void setClock();

    void setupPins() const;

    void setupI2S() const;

    uint32_t m_SampleRate;
    double m_SampleRateExact;
    uint16_t m_BlockSize;
    ClockDividers m_ClockDividers;
};


#endif //AUDIOCLOCKMANAGER_H
