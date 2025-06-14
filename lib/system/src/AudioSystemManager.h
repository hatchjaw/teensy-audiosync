#ifndef AUDIOSYSTEMMANGER_H
#define AUDIOSYSTEMMANGER_H

#include <Arduino.h>
#include <AudioProcessor.h>
#include <AudioSystemConfig.h>
#include <DMAChannel.h>
#include <SGTL5000.h>
#include <Utils.h>
#include <registers/AnalogAudioPllControlRegister.h>
#include <registers/ClockDividerRegister1.h>
#include <registers/ClockGatingRegister5.h>
#include <registers/GeneralPurposeRegister1.h>
#include <registers/MiscellaneousRegister2.h>
#include <registers/SAI1.h>
#include <registers/SerialClockMultiplexerRegister1.h>
#include <registers/SwMuxControlRegister.h>

class AudioSystemManager : public Printable
{
public:
    explicit AudioSystemManager(AudioSystemConfig config);

    bool begin();

    void adjustClock(double adjust, double drift);

    void startClock();

    void stopClock();

    volatile bool isClockRunning() const;

    static void setAudioProcessor(AudioProcessor *processor);

    size_t printTo(Print &p) const override;

private:
    struct ClockDividers final : Printable
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

    void setupPins() const;

    void setupI2S() const;

    void setupDMA() const;

    static void isr();

    uint32_t cycPreReg{0},
            cycPostReg{0},
            cycPostPin{0},
            cycPostClk{0},
            cycPostI2S{0},
            cycPostDMA{0},
            cycPostSGTL{0},
            cycPostStop{0};

    AudioSystemConfig m_Config;
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
    SAI1TransmitControlRegister m_SAI1TransmitControlRegister;
    SAI1TransmitConfig1Register m_SAI1TransmitConfig1Register;
    SAI1TransmitConfig2Register m_SAI1TransmitConfig2Register;
    SAI1TransmitConfig3Register m_SAI1TransmitConfig3Register;
    SAI1TransmitConfig4Register m_SAI1TransmitConfig4Register;
    SAI1TransmitConfig5Register m_SAI1TransmitConfig5Register;
    SAI1TransmitMaskRegister m_SAI1TransmitMaskRegister;
    SAI1ReceiveControlRegister m_SAI1ReceiveControlRegister;
    SAI1ReceiveConfig1Register m_SAI1ReceiveConfig1Register;
    SAI1ReceiveConfig2Register m_SAI1ReceiveConfig2Register;
    SAI1ReceiveConfig3Register m_SAI1ReceiveConfig3Register;
    SAI1ReceiveConfig4Register m_SAI1ReceiveConfig4Register;
    SAI1ReceiveConfig5Register m_SAI1ReceiveConfig5Register;
    SAI1ReceiveMaskRegister m_SAI1ReceiveMaskRegister;

    SGTL5000 m_AudioShield;

    static AudioProcessor *s_AudioProcessor;

    static bool s_FirstInterrupt;
    static DMAChannel s_DMA;

    static constexpr uint16_t k_I2sBufferSizeFrames{ananas::Constants::kAudioBlockFrames};
    static constexpr size_t k_I2sBufferSizeBytes{k_I2sBufferSizeFrames * sizeof(int16_t)};
    DMAMEM __attribute__((aligned(32))) static uint32_t s_I2sTxBuffer[k_I2sBufferSizeFrames];
};

#endif //AUDIOSYSTEMMANGER_H
