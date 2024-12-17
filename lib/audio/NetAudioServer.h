#ifndef NETAUDIOSERVER_H
#define NETAUDIOSERVER_H

#include <Audio.h>
#include <AudioSystemManager.h>
#include <QNEthernet.h>

using namespace qindesign::network;

class NetAudioServer final : public AudioStream
{
public:
    NetAudioServer();

    void connect();

    void send();

private:
    void update() override;

    static constexpr size_t k_TxBufferNumFrames{AUDIO_BLOCK_SAMPLES};
    static constexpr size_t k_PacketBufferSize{300};
    static constexpr NanoTime k_PacketReproductionOffsetNs{
        2 * ClockConstants::k_NanosecondsPerSecond / 10
    };

    int m_NumPacketsAvailable{0};
    EthernetUDP m_Socket;
    uint8_t m_TxPacketBuffer[sizeof(NanoTime) + (k_TxBufferNumFrames << 2)];
    AudioSystemManager::Packet m_Packet;
    AudioSystemManager::Packet m_PacketBuffer[k_PacketBufferSize];
    size_t m_PacketBufferTxIndex;
    size_t m_PacketBufferWriteIndex;
    uint8_t m_Connected{0};
};

#endif //NETAUDIOSERVER_H
