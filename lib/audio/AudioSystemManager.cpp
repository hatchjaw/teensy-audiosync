#include "AudioSystemManager.h"
#include <QNEthernet.h>
#include <TimeLib.h>

DMAChannel AudioSystemManager::s_DMA{false};
bool AudioSystemManager::s_FirstInterrupt{false};
SineWaveGenerator AudioSystemManager::s_SineWaveGenerator;
PulseGenerator AudioSystemManager::s_PulseGenerator;
int16_t AudioSystemManager::s_AudioTxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
int16_t AudioSystemManager::s_AudioRxBuffer[k_AudioBufferChannels * k_AudioBufferFrames];
AudioSystemManager::Packet AudioSystemManager::s_PacketBuffer[k_PacketBufferSize];
AudioSystemManager::Packet AudioSystemManager::s_Packet;
size_t AudioSystemManager::s_NumPacketsAvailable{0};
size_t AudioSystemManager::s_PacketBufferTxIndex{0};
size_t AudioSystemManager::s_PacketBufferReadIndex{74};
size_t AudioSystemManager::s_PacketBufferWriteIndex{0};
uint16_t AudioSystemManager::s_ReadIndexTx{0};
uint16_t AudioSystemManager::s_WriteIndexTx{0};
size_t AudioSystemManager::s_NumTxFramesAvailable{0};
uint16_t AudioSystemManager::s_ReadIndexRx{0};
uint16_t AudioSystemManager::s_WriteIndexRx{0};
AudioProcessor *AudioSystemManager::s_AudioProcessor = nullptr;
DMAMEM __attribute__((aligned(32))) uint32_t AudioSystemManager::s_I2sTxBuffer[k_I2sBufferSizeFrames];

void showTime(const NanoTime t)
{
    NanoTime x = t;
    const int ns = x % 1000;
    x /= 1000;
    const int us = x % 1000;
    x /= 1000;
    const int ms = x % 1000;
    x /= 1000;

    tmElements_t tme;
    breakTime((time_t) x, tme);

    Serial.printf("%02d.%02d.%04d %02d:%02d:%02d::%03d:%03d:%03d", tme.Day, tme.Month, 1970 + tme.Year, tme.Hour, tme.Minute, tme.Second, ms, us, ns);
}

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
    s_PulseGenerator.setWidth(1);
    s_PulseGenerator.setFrequency(1);

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
        isr, //m_Config.k_ClockRole == AudioSystemConfig::ClockRole::Authority ? clockAuthorityISR : clockSubscriberISR,
        80 // default, apparently (source?..)
    );

    Serial.printf("Audio interrupt priority: %d\n", NVIC_GET_PRIORITY(s_DMA.channel));
}

static int16_t buff[128 * 2]; // audio block frames * numChannels
static uint count{0};

void AudioSystemManager::isr()
{
    int16_t *destination, *source;
    const auto sourceAddress = (uint32_t) s_DMA.TCD->SADDR;
    s_DMA.clearInterrupt();

    // Bloody hell, I'm not sure why this works. Fill the second half of the I2S
    // buffer with the first half of the source buffer?..
    if (sourceAddress < (uint32_t) s_I2sTxBuffer + sizeof(s_I2sTxBuffer) / 2) {
        s_AudioProcessor->processAudio(buff, 2, k_I2sBufferSizeFrames);
        // DMA is transmitting the first half of the buffer; fill the second half.
        destination = (int16_t *) &s_I2sTxBuffer[k_I2sBufferSizeFrames / 2];
        // source = &buff[k_I2sBufferSizeFrames]; // That's the midpoint.
        source = buff;

        // if (++count % 1000 <= 1) {
        //     Serial.println("Buff:");
        //     ananas::Utils::hexDump(reinterpret_cast<uint8_t *>(buff), sizeof(int16_t) * k_AudioBufferChannels * k_I2sBufferSizeFrames);
        // }
    } else {
        // DMA is transmitting the second half of the buffer; fill the first half.
        destination = (int16_t *) s_I2sTxBuffer;
        // source = buff;
        source = &buff[k_I2sBufferSizeFrames]; // That's the midpoint.
    }

    memcpy(destination, source, k_I2sBufferSizeBytes); // Actually number of bytes over two?

    arm_dcache_flush_delete(destination, sizeof(s_I2sTxBuffer) / 2);
}

void AudioSystemManager::clockAuthorityISR()
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
        s_ReadIndexTx = k_AudioBufferFrames;
    }

    const auto sourceAddress = (uint32_t) s_DMA.TCD->SADDR;
    s_DMA.clearInterrupt();

    // Generate some audio. //
    const auto start{2 * s_WriteIndexTx};
    s_WriteIndexTx += k_I2sBufferSizeFrames / 2;
    // Generate a signal; write it to the outgoing audio buffer
    if (s_WriteIndexTx >= k_AudioBufferFrames) {
        uint16_t length2{(uint16_t) (s_WriteIndexTx - k_AudioBufferFrames)},
                length1{(uint16_t) ((k_I2sBufferSizeFrames / 2) - length2)};
        // s_SineWaveGenerator.generate(s_AudioTxBuffer + start, 2, length1);
        s_PulseGenerator.generate(s_AudioTxBuffer + start, 2, length1);
        // s_SineWaveGenerator.generate(s_AudioTxBuffer, 2, length2);
        s_PulseGenerator.generate(s_AudioTxBuffer, 2, length2);
        s_WriteIndexTx -= k_AudioBufferFrames;
    } else {
        // s_SineWaveGenerator.generate(s_AudioTxBuffer + start, 2, k_I2sBufferSizeFrames / 2);
        s_PulseGenerator.generate(s_AudioTxBuffer + start, 2, k_I2sBufferSizeFrames / 2);
    }
    s_NumTxFramesAvailable += k_I2sBufferSizeFrames / 2;

    // Check if there are enough frames available to fill a packet.
    if (s_NumTxFramesAvailable >= k_I2sBufferSizeFrames) {
        // Write a packet to the packet buffer. //

        // Get the current ethernet time.
        timespec ts;
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        NanoTime now{ts.tv_sec * NS_PER_S + ts.tv_nsec};

        // Set packet time some way in the future.
        s_Packet.time = now + k_PacketReproductionOffsetNs;

        // Copy audio data to the packet
        if (s_ReadIndexTx + k_I2sBufferSizeFrames > k_AudioBufferFrames) {
            size_t len1{static_cast<size_t>(k_AudioBufferFrames - s_ReadIndexTx) << 2},
                    len2{(k_I2sBufferSizeFrames << 2) - len1};
            memcpy(s_Packet.data, &s_AudioTxBuffer[2 * s_ReadIndexTx], len1);
            memcpy(s_Packet.data + len1, s_AudioTxBuffer, len2);
        } else {
            memcpy(s_Packet.data, &s_AudioTxBuffer[2 * s_ReadIndexTx], k_I2sBufferSizeFrames << 2);
        }
        // Write the packet to the packet buffer.
        writeToPacketBuffer(reinterpret_cast<uint8_t *>(&s_Packet));

        // Update TX frame buffer.
        s_NumTxFramesAvailable -= k_I2sBufferSizeFrames;
        s_ReadIndexTx += k_I2sBufferSizeFrames;
        if (s_ReadIndexTx >= k_AudioBufferFrames) {
            s_ReadIndexTx -= k_AudioBufferFrames;
        }
    }

    int16_t *destination;
    uint8_t *source;

    // Read the current packet.
    Packet packet{s_PacketBuffer[s_PacketBufferReadIndex]};

    if (sourceAddress < (uint32_t) s_I2sTxBuffer + sizeof(s_I2sTxBuffer) / 2) {
        // DMA is transmitting the first half of the buffer
        // so we must fill the second half
        destination = (int16_t *) &s_I2sTxBuffer[k_I2sBufferSizeFrames / 2];
        source = &packet.data[k_I2sBufferSizeBytes];

        // Increment the packet read index.
        ++s_PacketBufferReadIndex;
        if (s_PacketBufferReadIndex >= k_PacketBufferSize) {
            s_PacketBufferReadIndex = 0;
        }
    } else {
        // DMA is transmitting the second half of the buffer
        // so we must fill the first half
        destination = (int16_t *) s_I2sTxBuffer;
        source = packet.data;
    }

    // Copy samples to destination. //
    memcpy(destination, source, k_I2sBufferSizeBytes);

    arm_dcache_flush_delete(destination, sizeof(s_I2sTxBuffer) / 2);
}

void AudioSystemManager::clockSubscriberISR()
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
        // Reproduce pi/2 radians out of phase (50 samples @ F0 = 240 Hz @ Fs = 48 kHz)
        // s_ReadIndexTx = k_AudioBufferFrames - 50;
        s_ReadIndexTx = k_AudioBufferFrames;
    }

    int16_t *destination;
    // int16_t *source;
    uint8_t *source;
    const auto sourceAddress = (uint32_t) s_DMA.TCD->SADDR;
    s_DMA.clearInterrupt();

    // Read from (packet/audio) buffer. //
    Packet packet{s_PacketBuffer[s_PacketBufferReadIndex]};

    if (sourceAddress < (uint32_t) s_I2sTxBuffer + sizeof(s_I2sTxBuffer) / 2) {
        // DMA is transmitting the first half of the buffer
        // so we must fill the second half
        destination = (int16_t *) &s_I2sTxBuffer[k_I2sBufferSizeFrames / 2];

        // source = &audioData[k_I2sBufferSize];
        source = &packet.data[k_I2sBufferSizeBytes];

        ++s_PacketBufferReadIndex;
        if (s_PacketBufferReadIndex >= k_PacketBufferSize) {
            s_PacketBufferReadIndex = 0;
        }
    } else {
        // DMA is transmitting the second half of the buffer
        // so we must fill the first half
        destination = (int16_t *) s_I2sTxBuffer;

        // source = audioData;
        source = packet.data;
    }

    memcpy(destination, source, k_I2sBufferSizeBytes);

    arm_dcache_flush_delete(destination, sizeof(s_I2sTxBuffer) / 2);
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
    s_PulseGenerator.reset();

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
    s_PulseGenerator.reset();
}

volatile bool AudioSystemManager::isClockRunning() const
{
    return m_AnalogAudioPllControlRegister.isClockRunning();
}

void AudioSystemManager::setAudioProcessor(AudioProcessor *processor)
{
    s_AudioProcessor = processor;
}

// Compare reproduction time with current time. //
// If times are close (!..) set the packet buffer read index. //
// Oh boy... this (kind of) works for 48 kHz @ 128-frames because
// 48000/128 = 375, but 44100/128 = 344.53125; if subscribers start up during
// different seconds there's no way they're going to synchronise.
void AudioSystemManager::adjustPacketBufferReadIndex(NanoTime now)
{
    auto initialPacketReadIndex{s_PacketBufferReadIndex == 0 ? k_PacketBufferSize - 1 : s_PacketBufferReadIndex - 1};
    Packet packet{s_PacketBuffer[s_PacketBufferReadIndex]};
    constexpr NanoTime acceptableOffset{
        ClockConstants::k_NanosecondsPerSecond * (int64_t) AUDIO_BLOCK_SAMPLES /
        (int64_t) AUDIO_SAMPLE_RATE_EXACT
        // 1'500'000
    };

    // // Serial.printf("!Current time: %" PRId64 ", packet time: %" PRId64 ", diff: %" PRId64 "\n", now, packet.time, now - packet.time);
    // Serial.print(":Current time: ");
    // showTime(now);
    // Serial.printf(", Read index: %" PRIu32 "\n:Packet time:  ", s_PacketBufferReadIndex);
    // showTime(packet.time);
    // Serial.printf(", diff: %" PRId64 "\n", now - packet.time);

    while (abs(now - packet.time) > acceptableOffset && s_PacketBufferReadIndex != initialPacketReadIndex) {
        ++s_PacketBufferReadIndex;
        if (s_PacketBufferReadIndex >= k_PacketBufferSize) {
            s_PacketBufferReadIndex = 0;
        }
        // if (s_PacketBufferReadIndex == 0) {
        //     s_PacketBufferReadIndex = k_PacketBufferSize;
        // }
        // --s_PacketBufferReadIndex;
        packet = s_PacketBuffer[s_PacketBufferReadIndex];

        //now < packet.time && s_PacketBufferReadIndex != initialPacketReadIndex) {
        // Serial.printf("Current time: %" PRId64 ", packet time: %" PRId64 ", diff: %" PRId64 "\n", now, packet.time, now - packet.time);
        Serial.print("Current time: ");
        showTime(now);
        Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", s_PacketBufferReadIndex);
        showTime(packet.time);
        Serial.printf(", diff: %" PRId64 "\n", now - packet.time);
    }
}

void AudioSystemManager::reportBufferFillLevel()
{
    Serial.printf("Read index: %" PRIu32 ", Write index: %" PRIu32 ", Num packets available: %" PRIu32 "\n",
                  s_PacketBufferReadIndex,
                  s_PacketBufferWriteIndex,
                  s_PacketBufferWriteIndex > s_PacketBufferReadIndex
                      ? s_PacketBufferWriteIndex - s_PacketBufferReadIndex
                      : s_PacketBufferWriteIndex + k_PacketBufferSize - s_PacketBufferReadIndex
    );
}

void AudioSystemManager::adjustClock(const double nspsDiscrepancy)
{
    Serial.printf("Skew (nsps): %*.*f\n", 26, 8, nspsDiscrepancy);

    const double proportionalAdjustment{
        1. + nspsDiscrepancy /
        (double) ClockConstants::k_NanosecondsPerSecond
    };

    Serial.printf("Proportional adjustment: %*.*f\n", 14, 8, proportionalAdjustment);

    m_Config.setExactSampleRate(proportionalAdjustment);
    Serial.println(m_Config);
    m_ClockDividers.calculateFine(m_Config.getExactSampleRate());
    m_AudioPllNumeratorRegister.set(m_ClockDividers.m_Pll4Num);
}

size_t AudioSystemManager::getNumPacketsAvailable()
{
    return s_NumPacketsAvailable;
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

// Only used by clock authority, to get the next packet to transmit.
void AudioSystemManager::readFromPacketBuffer(uint8_t *dest)
{
    __disable_irq();
    memcpy(dest, &s_PacketBuffer[s_PacketBufferTxIndex], sizeof(NanoTime) + (k_I2sBufferSizeFrames << 2));
    ++s_PacketBufferTxIndex;
    if (s_PacketBufferTxIndex >= k_PacketBufferSize) {
        s_PacketBufferTxIndex = 0;
    }
    --s_NumPacketsAvailable;
    __enable_irq();
}

void AudioSystemManager::writeToPacketBuffer(uint8_t *src)
{
    __disable_irq();
    memcpy(&s_PacketBuffer[s_PacketBufferWriteIndex], src, sizeof(NanoTime) + (k_I2sBufferSizeFrames << 2));
    ++s_PacketBufferWriteIndex;
    if (s_PacketBufferWriteIndex >= k_PacketBufferSize) {
        s_PacketBufferWriteIndex = 0;
    }
    // Signal that a packet is available for transmission.
    ++s_NumPacketsAvailable;
    __enable_irq();
}

void AudioSystemManager::writeToRxAudioBuffer(const int16_t *src, size_t numChannels, size_t numSamples)
{
    __disable_irq();
    for (size_t n{0}; n < numSamples; ++n) {
        for (size_t ch{0}; ch < numChannels; ++ch) {
            s_AudioRxBuffer[numChannels * s_WriteIndexRx + ch] = src[n * numChannels + ch];
        }
        ++s_WriteIndexRx;
        if (s_WriteIndexRx >= k_AudioBufferFrames) {
            s_WriteIndexRx = 0;
        }
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
                printTo(Serial);
                Serial.println();
                return;
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

    Serial.printf("PLL4 numerator: %23" PRId32 "\n"
                  "PLL4 denominator: %21" PRIu32 "\n",
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
