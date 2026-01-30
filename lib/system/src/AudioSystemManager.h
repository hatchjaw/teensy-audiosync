#ifndef AUDIOSYSTEMMANGER_H
#define AUDIOSYSTEMMANGER_H

#include <Arduino.h>
#include <AudioProcessor.h>
#include <AudioSystemConfig.h>
#include <DMAChannel.h>
#include <SGTL5000.h>
#include <AnanasUtils.h>
#include <ProgramComponent.h>
#include <registers/AnalogAudioPllControlRegister.h>
#include <registers/ClockDividerRegister1.h>
#include <registers/ClockGatingRegister5.h>
#include <registers/GeneralPurposeRegister1.h>
#include <registers/MiscellaneousRegister2.h>
#include <registers/SAI1.h>
#include <registers/SerialClockMultiplexerRegister1.h>
#include <registers/SwMuxControlRegister.h>

class AudioSystemManager final : public ProgramComponent
{
public:
    explicit AudioSystemManager(AudioSystemConfig &config);

protected:
    void beginImpl() override;

public:
    void run() override;

    void adjustClock(double adjust);

    void startClock();

    void stopClock();

    volatile bool isClockRunning() const;

    static void setAudioProcessor(AudioProcessor *processor);

    size_t printTo(Print &p) const override;

    void onInvalidSamplingRate(void (*callback)());

    void onAudioPtpOffsetChanged(void (*callback)(long));

private:
    struct ClockDividers final : Printable
    {
        uint8_t pll4Div{ClockConstants::Pll4DivMin};
        int32_t pll4Num{0};
        uint32_t pll4Denom{1};
        uint8_t sai1Pre{1};
        uint8_t sai1Post{1};
        constexpr static MiscellaneousRegister2::AudioPostDiv kAudioPostDiv{MiscellaneousRegister2::AudioPostDiv::DivideBy1};
        constexpr static AnalogAudioPllControlRegister::PostDivSelect kPll4PostDiv{AnalogAudioPllControlRegister::PostDivSelect::DivideBy1};

        size_t printTo(Print &p) const override;

        void calculateCoarse(uint32_t targetSamplingRate);

        bool calculateFine(double targetSamplingRate);

    private:
        double getCurrentSamplingRate() const;

        uint32_t getPll4Freq() const;

        double getPLL4FractionalDivider() const;

        bool isPll4FreqValid() const;

        bool isSai1PostFreqValid() const;

        uint32_t getCurrentMaxPossibleSamplingRate() const;

        uint32_t getCurrentSai1ClkRootFreq() const;
    };

    static void setupDMA();

    static void isr();

    static void triggerAudioProcessing();

    static void softwareISR();

private:
    uint32_t cycPreReg{0},
            cycPostStop{0};

    AudioSystemConfig &config;
    ClockDividers clockDividers;

    AnalogAudioPllControlRegister analogAudioPllControlRegister;
    AudioPllNumeratorRegister audioPllNumeratorRegister;
    AudioPllDenominatorRegister audioPllDenominatorRegister;
    ClockDividerRegister1 clockDividerRegister1;
    SerialClockMultiplexerRegister1 serialClockMultiplexerRegister1;
    MiscellaneousRegister2 miscellaneousRegister2;
    ClockGatingRegister5 clockGatingRegister5;
    GeneralPurposeRegister1 generalPurposeRegister1;
    Pin7SwMuxControlRegister pin7SwMuxControlRegister;
    Pin20SwMuxControlRegister pin20SwMuxControlRegister;
    Pin21SwMuxControlRegister pin21SwMuxControlRegister;
    Pin23SwMuxControlRegister pin23SwMuxControlRegister;
    SAI1TransmitControlRegister sai1TransmitControlRegister;
    SAI1TransmitConfig1Register sai1TransmitConfig1Register;
    SAI1TransmitConfig2Register sai1TransmitConfig2Register;
    SAI1TransmitConfig3Register sai1TransmitConfig3Register;
    SAI1TransmitConfig4Register sai1TransmitConfig4Register;
    SAI1TransmitConfig5Register sai1TransmitConfig5Register;
    SAI1TransmitMaskRegister sai1TransmitMaskRegister;
    SAI1ReceiveControlRegister sai1ReceiveControlRegister;
    SAI1ReceiveConfig1Register sai1ReceiveConfig1Register;
    SAI1ReceiveConfig2Register sai1ReceiveConfig2Register;
    SAI1ReceiveConfig3Register sai1ReceiveConfig3Register;
    SAI1ReceiveConfig4Register sai1ReceiveConfig4Register;
    SAI1ReceiveConfig5Register sai1ReceiveConfig5Register;
    SAI1ReceiveMaskRegister sai1ReceiveMaskRegister;

    SGTL5000 audioShield;

    void (*invalidSamplingRateCallback)(){nullptr};

    void (*updateAudioPtpOffsetCallback)(long){nullptr};

    inline static AudioProcessor *sAudioProcessor{nullptr};

    inline static uint16_t sInterruptsPerSecond{0};
    inline static int16_t sNumInterrupts{-1};
    inline static long sFirstInterruptNS{0};
    inline static long sAudioPTPOffset{0};
    inline static bool sAudioPTPOffsetChanged{false};
    inline static DMAChannel sDMA{false};

    DMAMEM inline static int16_t sInputBufferData[ananas::Constants::MaxChannels][ananas::Constants::AudioBlockFrames]{};
    DMAMEM inline static int16_t sOutputBufferData[ananas::Constants::MaxChannels][ananas::Constants::AudioBlockFrames]{};
    inline static int16_t *sInputBuffer[ananas::Constants::MaxChannels]{};
    inline static int16_t *sOutputBuffer[ananas::Constants::MaxChannels]{};

    inline static int16_t sAudioBuffer[ananas::Constants::AudioBlockFrames * ananas::Constants::NumOutputChannels]{};

    DMAMEM __attribute__((aligned(32))) inline static uint32_t sI2sTxBuffer[ananas::Constants::AudioBlockFrames]{};
};

#endif //AUDIOSYSTEMMANGER_H
