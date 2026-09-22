#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <cstdint>
#include "status_data.h"

class TcpClient
{
    public:
        TcpClient();
        ~TcpClient();

        bool Connect(const char* ip, uint16_t port);
        void Close();
        bool IsConnected() const;

        int Receive(uint8_t* buffer, int buffer_size);
        bool GetPacket(uint8_t* packet_buffer, int packet_size);
        bool GetStatusPacket(uint8_t* packet_buffer, int packet_buffer_size);
        bool ValidateStatusPayload(const uint8_t* packet, int packet_size) const; void ResetReceiveBuffer();
        
        bool DecodeStatusPayload(
            const uint8_t* packet,
            int packet_size,
            uint8_t* raw_data,
            int raw_data_size) const;

        bool ParseStatusData(
            const uint8_t* raw_data,
            int raw_data_size,
            StatusData& status_data) const;

        double ScaleAIValue(
            int channel,
            uint16_t raw_value) const;

        bool AppendTestData(const uint8_t* data, int length);

    private:
        uintptr_t socket_;
        static constexpr int RECEIVE_BUFFER_SIZE = 4096;

        uint8_t receive_buffer_[RECEIVE_BUFFER_SIZE];
        int receive_length_;
        bool ValidateStatusPacketCRC(const uint8_t* packet, int packet_size) const;
};

#endif
