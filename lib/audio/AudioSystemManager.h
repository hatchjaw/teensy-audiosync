#ifndef AUDIOCLOCKMANAGER_H
#define AUDIOCLOCKMANAGER_H

#include "ClockConstants.h"
#include "registers/MiscellaneousRegister2.h"
#include "registers/AnalogAudioPllControlRegister.h"
#include "registers/ClockDividerRegister1.h"
#include "registers/SerialClockMultiplexerRegister1.h"
#include "registers/ClockGatingRegister5.h"
#include "registers/GeneralPurposeRegister1.h"
#include "registers/SwMuxControlRegister.h"

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

    AnalogAudioPllControlRegister m_AnalogAudioPllControlRegister;
    AudioPllNumeratorRegister m_AudioPllNumeratorRegister;
    AudioPllDenominatorRegister m_AudioPllDenominatorRegister;
    ClockDividerRegister1 m_ClockDividerRegister1;
    SerialClockMultiplexerRegister1 m_SerialClockMultiplexerRegister1;
    MiscellaneousRegister2 m_MiscellaneousRegister2;
    ClockGatingRegister5 m_ClockGatingRegister5;
    GeneralPurposeRegister1 m_GeneralPurposeRegister1;
    Pin7SwMuxControlRegister m_Pin7SwMuxControlRegister;
    Pin20SwMuxControlRegister m_Pin20SwMuxControlRegister;
    Pin21SwMuxControlRegister m_Pin21SwMuxControlRegister;
    Pin23SwMuxControlRegister m_Pin23SwMuxControlRegister;
};


#endif //AUDIOCLOCKMANAGER_H
