#ifndef AUDIOSYSTEMMANGER_H
#define AUDIOSYSTEMMANGER_H

#include <Arduino.h>
#include <control_sgtl5000.h>
#include <DMAChannel.h>
#include <ptp/ptp-base.h>

#include "Config.h"
#include "registers/MiscellaneousRegister2.h"
#include "registers/AnalogAudioPllControlRegister.h"
#include "registers/ClockDividerRegister1.h"
#include "registers/SerialClockMultiplexerRegister1.h"
#include "registers/ClockGatingRegister5.h"
#include "registers/GeneralPurposeRegister1.h"
#include "registers/SwMuxControlRegister.h"
#include "SineWaveGenerator.h"

class AudioSystemManager
{
public:
    struct Packet
    {
        NanoTime time;
        uint8_t data[128 << 2];
    };

    explicit AudioSystemManager(AudioSystemConfig config);

    bool begin();

    void setSampleRate(double targetSampleRate);

    void adjustClock(double nspsDiscrepancy);

    void startClock() const;

    void stopClock() const;

    volatile bool isClockRunning() const;

    static void adjustPacketBufferReadIndex(NanoTime now);

    static size_t getNumTxFramesAvailable();

    static size_t getNumPacketsAvailable();

    /**
    * Read from the TX buffer into an external buffer.
    */
    static void readFromTxAudioBuffer(int16_t *dest, size_t numChannels, size_t numSamples);

    static void readFromPacketBuffer(uint8_t *dest);

    static void writeToPacketBuffer(uint8_t *src);

    /**
    * Write to the RX buffer from an external buffer.
    */
    static void writeToRxAudioBuffer(const int16_t *src, size_t numChannels, size_t numSamples);

    static SineWaveGenerator s_SineWaveGenerator;

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

    void setupDMA() const;

    static void isr();

    static void clockAuthorityISR();

    static void clockSubscriberISR();

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

    AudioControlSGTL5000 m_AudioShield;

    static bool s_FirstInterrupt;
    static DMAChannel s_DMA;
    // Let's start with a buffer of 1/10 s
    static constexpr uint16_t k_AudioBufferFrames{4800}, k_AudioBufferChannels{2};
    static int16_t s_AudioTxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
    static size_t s_NumTxFramesAvailable;
    static int16_t s_AudioRxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
    static uint16_t s_ReadIndexTx, s_WriteIndexTx, s_ReadIndexRx, s_WriteIndexRx;
    // 150 packets @ 128 frames @ 48 kHz = 0.4 s.
    static constexpr size_t k_PacketBufferSize{150};
    static Packet s_PacketBuffer[k_PacketBufferSize];
    static Packet s_Packet;
    static size_t s_NumPacketsAvailable;
    static size_t s_PacketBufferTxIndex, s_PacketBufferReadIndex, s_PacketBufferWriteIndex;

    static constexpr NanoTime k_PacketReproductionOffsetNs{ClockConstants::k_NanosecondsPerSecond / 10};

    static constexpr uint16_t k_I2sBufferSize{AUDIO_BLOCK_SAMPLES};
    static constexpr size_t k_I2sBufferSizeBytes{k_I2sBufferSize * sizeof(int16_t)};
    DMAMEM __attribute__((aligned(32))) static uint32_t s_I2sTxBuffer[k_I2sBufferSize];
};

#endif //AUDIOSYSTEMMANGER_H
