#include "NetAudioServer.h"

NetAudioServer::NetAudioServer() : AudioStream(2, new audio_block_t *[2])
{
}

void NetAudioServer::connect()
{
    m_Connected = m_Socket.beginMulticast({224, 4, 224, 4}, 49152);
}

void NetAudioServer::send()
{
    if (!m_Connected || m_NumPacketsAvailable == 0) return;

    // Read from packet buffer.
    __disable_irq();
    memcpy(m_TxPacketBuffer, &m_PacketBuffer[m_PacketBufferTxIndex], sizeof(NanoTime) + (k_BufferSizeFrames << 2));
    ++m_PacketBufferTxIndex;
    if (m_PacketBufferTxIndex >= k_PacketBufferSize) {
        m_PacketBufferTxIndex = 0;
    }
    --m_NumPacketsAvailable;
    __enable_irq();

    m_Socket.beginPacket({224, 4, 224, 4}, 49152);
    m_Socket.write(m_TxPacketBuffer, sizeof(AudioSystemManager::Packet));
    m_Socket.endPacket();
}

void NetAudioServer::update()
{
    if (!m_Connected) return;

    // Get the current ethernet time.
    timespec ts;
    EthernetIEEE1588.readTimer(ts);
    NanoTime now{ts.tv_sec * NS_PER_S + ts.tv_nsec};

    // Set packet time some way in the future.
    m_Packet.time = now + k_PacketReproductionOffsetNs;

    audio_block_t *inBlock[num_inputs];
    const auto audioData = reinterpret_cast<int16_t *>(m_Packet.data);

    for (int channel = 0; channel < num_inputs; channel++) {
        inBlock[channel] = receiveReadOnly(channel);
        // Only proceed if a block was returned, i.e. something is connected
        // to one of this object's input channels.
        if (inBlock[channel]) {
            // Interleave samples into the packet.
            for (size_t frame{0}; frame < k_BufferSizeFrames; ++frame) {
                audioData[(frame * num_inputs + channel)] = inBlock[channel]->data[frame];
            }

            release(inBlock[channel]);
        } else {
            for (size_t frame{0}; frame < k_BufferSizeFrames; ++frame) {
                m_Packet.data[frame * num_inputs + channel] = 0;
            }
        }
    }

    // Write packet to packet buffer;
    __disable_irq();
    memcpy(&m_PacketBuffer[m_PacketBufferWriteIndex], &m_Packet, sizeof(NanoTime) + (k_BufferSizeFrames << 2));
    ++m_PacketBufferWriteIndex;
    if (m_PacketBufferWriteIndex >= k_PacketBufferSize) {
        m_PacketBufferWriteIndex = 0;
    }
    ++m_NumPacketsAvailable;
    __enable_irq();
}
