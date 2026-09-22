#include "tcp_client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <cstdio>

TcpClient::TcpClient()
    : socket_(INVALID_SOCKET),
      receive_length_(0)
{
}

TcpClient::~TcpClient()
{
Close();
}

bool TcpClient::Connect(const char* ip, uint16_t port)
{
WSADATA wsa_data;

if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
{
    std::printf("[CLIENT] WSAStartup failed\n");
    return false;
}

SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

if (sock == INVALID_SOCKET)
{
    std::printf("[CLIENT] socket() failed\n");
    WSACleanup();
    return false;
}

sockaddr_in server_address{};
server_address.sin_family = AF_INET;
server_address.sin_port = htons(port);

if (inet_pton(AF_INET, ip, &server_address.sin_addr) != 1)
{
    std::printf("[CLIENT] Invalid IP address\n");
    closesocket(sock);
    WSACleanup();
    return false;
}

std::printf("[CLIENT] Connecting to %s:%u\n",
            ip,
            static_cast<unsigned>(port));

if (connect(sock,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)) == SOCKET_ERROR)
{
    std::printf("[CLIENT] connect() failed, error=%d\n",
                WSAGetLastError());

    closesocket(sock);
    WSACleanup();
    return false;
}

socket_ = static_cast<uintptr_t>(sock);

receive_length_ = 0;

std::printf("[CLIENT] Connected\n");

return true;

}

void TcpClient::Close()
{
    if (socket_ != static_cast<uintptr_t>(INVALID_SOCKET))
    {
        closesocket(static_cast<SOCKET>(socket_));
        socket_ = INVALID_SOCKET;
        WSACleanup();
    }
}

bool TcpClient::IsConnected() const
{
    return socket_ != static_cast<uintptr_t>(INVALID_SOCKET);
}

int TcpClient::Receive(uint8_t* buffer, int buffer_size)
{
    (void)buffer;
    (void)buffer_size;

    if (!IsConnected())
    {
        return -1;
    }

    int received = recv(
        static_cast<SOCKET>(socket_),
        reinterpret_cast<char*>(
            receive_buffer_ + receive_length_),
        RECEIVE_BUFFER_SIZE - receive_length_,
        0);

    if (received == 0)
    {
        std::printf("[CLIENT] Server disconnected\n");
        Close();
        return 0;
    }

    if (received == SOCKET_ERROR)
    {
        std::printf("[CLIENT] recv() failed, error=%d\n",
                    WSAGetLastError());

        Close();
        return -1;
    }

    receive_length_ += received;

    return received;
}

bool TcpClient::GetPacket(uint8_t* packet_buffer, int packet_size)
{
    if (packet_size <= 0)
    {
        return false;
    }

    if (receive_length_ < packet_size)
    {
        return false;
    }

    std::memcpy(
        packet_buffer,
        receive_buffer_,
        packet_size);

    receive_length_ -= packet_size;

    if (receive_length_ > 0)
    {
        std::memmove(
            receive_buffer_,
            receive_buffer_ + packet_size,
            receive_length_);
    }

    return true;
}


bool TcpClient::ValidateStatusPacketCRC(
    const uint8_t* packet,
    int packet_size) const
{
    if (packet == nullptr || packet_size < 2)
    {
        return false;
    }

    uint16_t crc = 0xFFFF;

    for (int i = 0; i < packet_size - 2; ++i)
    {
        crc ^= packet[i];

        for (int bit = 0; bit < 8; ++bit)
        {
            if ((crc & 0x0001U) != 0)
            {
                crc = static_cast<uint16_t>(
                    (crc >> 1U) ^ 0xA001U);
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    const uint16_t received_crc =
        static_cast<uint16_t>(packet[packet_size - 2]) |
        (static_cast<uint16_t>(packet[packet_size - 1]) << 8U);

    return crc == received_crc;
}


bool TcpClient::ValidateStatusPayload(
    const uint8_t* packet,
    int packet_size) const
{
    constexpr int HEADER_SIZE = 10;
    constexpr int PAYLOAD_SIZE = 139;

    if (packet == nullptr ||
        packet_size != HEADER_SIZE + PAYLOAD_SIZE + 2)
    {
        return false;
    }

    const uint8_t* payload =
        packet + HEADER_SIZE;

    if (payload[0] != '>')
    {
        return false;
    }

    for (int i = 1; i <= 136; ++i)
    {
        const uint8_t value = payload[i];

        const bool is_hex =
            (value >= '0' && value <= '9') ||
            (value >= 'A' && value <= 'F') ||
            (value >= 'a' && value <= 'f');

        if (!is_hex)
        {
            return false;
        }
    }

    if (payload[137] != '\r' ||
        payload[138] != '\n')
    {
        return false;
    }

    return true;
}


bool TcpClient::DecodeStatusPayload(
    const uint8_t* packet,
    int packet_size,
    uint8_t* raw_data,
    int raw_data_size) const
{
    constexpr int HEADER_SIZE = 10;
    constexpr int PAYLOAD_SIZE = 139;
    constexpr int RAW_DATA_SIZE = 68;

    if (packet == nullptr ||
        raw_data == nullptr ||
        packet_size != HEADER_SIZE + PAYLOAD_SIZE + 2 ||
        raw_data_size < RAW_DATA_SIZE)
    {
        return false;
    }

    if (!ValidateStatusPayload(
            packet,
            packet_size))
    {
        return false;
    }

    const uint8_t* payload =
        packet + HEADER_SIZE;

    auto HexToValue = [](uint8_t value) -> int
    {
        if (value >= '0' && value <= '9')
        {
            return value - '0';
        }

        if (value >= 'A' && value <= 'F')
        {
            return value - 'A' + 10;
        }

        if (value >= 'a' && value <= 'f')
        {
            return value - 'a' + 10;
        }

        return -1;
    };

    for (int i = 0; i < RAW_DATA_SIZE; ++i)
    {
        const int high =
            HexToValue(payload[1 + i * 2]);

        const int low =
            HexToValue(payload[1 + i * 2 + 1]);

        if (high < 0 || low < 0)
        {
            return false;
        }

        raw_data[i] =
            static_cast<uint8_t>((high << 4) | low);
    }

    return true;
}


double TcpClient::ScaleAIValue(
    int channel,
    uint16_t raw_value) const
{
    switch (channel)
    {
        case 0:
            // AI00: Load Cell, -10V ~ +10V
            return
                (static_cast<double>(raw_value) / 65535.0) * 20.0
                - 10.0;

        case 4:
        case 5:
        case 6:
        case 7:
            // AI04~AI07: PM Ch0~Ch3, 0V ~ 10V
            return
                (static_cast<double>(raw_value) / 65535.0) * 10.0;

        default:
            // AI01~AI03: scaling not defined by document
            return 0.0;
    }
}


bool TcpClient::ParseStatusData(
    const uint8_t* raw_data,
    int raw_data_size,
    StatusData& status_data) const
{
    constexpr int RAW_DATA_SIZE = 68;

    if (raw_data == nullptr ||
        raw_data_size < RAW_DATA_SIZE)
    {
        return false;
    }

    status_data.controller_status =
        (static_cast<uint32_t>(raw_data[0]) << 24U) |
        (static_cast<uint32_t>(raw_data[1]) << 16U) |
        (static_cast<uint32_t>(raw_data[2]) << 8U) |
        static_cast<uint32_t>(raw_data[3]);


    status_data.connect =
        (status_data.controller_status & (1U << 0U)) != 0U;

    status_data.voltage_on =
        (status_data.controller_status & (1U << 1U)) != 0U;

    status_data.is_moving =
        (status_data.controller_status & (1U << 3U)) != 0U;

    status_data.is_fa =
        (status_data.controller_status & (1U << 4U)) != 0U;

    status_data.homing_end =
        (status_data.controller_status & (1U << 5U)) != 0U;

    status_data.error =
        (status_data.controller_status & (1U << 6U)) != 0U;


    for (int i = 0; i < 8; ++i)
    {
        const int offset = 4 + i * 2;

        status_data.ai[i] =
            static_cast<uint16_t>(
                (static_cast<uint16_t>(raw_data[offset]) << 8U) |
                static_cast<uint16_t>(raw_data[offset + 1]));

        status_data.ai_voltage[i] =
            ScaleAIValue(i, status_data.ai[i]);

    }

    uint64_t double_bits[6] = {};

    for (int axis = 0; axis < 6; ++axis)
    {
        const int offset = 20 + axis * 8;

        for (int i = 0; i < 8; ++i)
        {
            double_bits[axis] =
                (double_bits[axis] << 8U) |
                static_cast<uint64_t>(raw_data[offset + i]);
        }
    }

    std::memcpy(
        &status_data.x,
        &double_bits[0],
        sizeof(double));

    std::memcpy(
        &status_data.y,
        &double_bits[1],
        sizeof(double));

    std::memcpy(
        &status_data.z,
        &double_bits[2],
        sizeof(double));

    std::memcpy(
        &status_data.rx,
        &double_bits[3],
        sizeof(double));

    std::memcpy(
        &status_data.ry,
        &double_bits[4],
        sizeof(double));

    std::memcpy(
        &status_data.rz,
        &double_bits[5],
        sizeof(double));

    return true;
}


bool TcpClient::GetStatusPacket(
    uint8_t* packet_buffer,
    int packet_buffer_size)
{
    constexpr uint8_t MAGIC_0 = 0x47;
    constexpr uint8_t MAGIC_1 = 0x53;
    constexpr int HEADER_SIZE = 10;

    if (packet_buffer == nullptr ||
        packet_buffer_size < HEADER_SIZE)
    {
        return false;
    }

    if (receive_length_ < 2)
    {
        return false;
    }

    int magic_index = -1;

    for (int i = 0; i <= receive_length_ - 2; ++i)
    {
        if (receive_buffer_[i] == MAGIC_0 &&
            receive_buffer_[i + 1] == MAGIC_1)
        {
            magic_index = i;
            break;
        }
    }

    if (magic_index < 0)
    {
        return false;
    }

    if (magic_index > 0)
    {
        std::memmove(
            receive_buffer_,
            receive_buffer_ + magic_index,
            receive_length_ - magic_index);

        receive_length_ -= magic_index;
    }


    if (receive_length_ < HEADER_SIZE)
    {
        return false;
    }

    constexpr uint8_t STATUS_VERSION = 0x01;
    constexpr uint8_t STATUS_TYPE = 0x01;

    if (receive_buffer_[2] != STATUS_VERSION ||
        receive_buffer_[3] != STATUS_TYPE)
    {
        receive_length_ -= 2;

        if (receive_length_ > 0)
        {
            std::memmove(
                receive_buffer_,
                receive_buffer_ + 2,
                receive_length_);
        }

        return false;
    }

    constexpr uint16_t STATUS_PAYLOAD_LENGTH = 139;

    const uint16_t payload_length =
        static_cast<uint16_t>(receive_buffer_[8]) |
        (static_cast<uint16_t>(receive_buffer_[9]) << 8U);

    if (payload_length != STATUS_PAYLOAD_LENGTH)
    {
        receive_length_ -= 2;

        if (receive_length_ > 0)
        {
            std::memmove(
                receive_buffer_,
                receive_buffer_ + 2,
                receive_length_);
        }

        return false;
    }

    constexpr int CRC_SIZE = 2;


    const int packet_size =
        HEADER_SIZE +
        static_cast<int>(payload_length) +
        CRC_SIZE;

    if (packet_size > packet_buffer_size)
    {
        return false;
    }

    if (receive_length_ < packet_size)
    {
        return false;
    }

    if (!ValidateStatusPayload(
            receive_buffer_,
            packet_size))
    {
        receive_length_ -= 2;

        if (receive_length_ > 0)
        {
            std::memmove(
                receive_buffer_,
                receive_buffer_ + 2,
                receive_length_);
        }

        return false;
    }

    if (!ValidateStatusPacketCRC(
            receive_buffer_,
            packet_size))
    {
        receive_length_ -= 2;

        if (receive_length_ > 0)
        {
            std::memmove(
                receive_buffer_,
                receive_buffer_ + 2,
                receive_length_);
        }

        return false;
    }

    std::memcpy(
        packet_buffer,
        receive_buffer_,
        packet_size);


    receive_length_ -= packet_size;


    if (receive_length_ > 0)
    {
        std::memmove(
            receive_buffer_,
            receive_buffer_ + packet_size,
            receive_length_);
    }

    return true;
}

void TcpClient::ResetReceiveBuffer()
{
    receive_length_ = 0;
}

bool TcpClient::AppendTestData(
    const uint8_t* data,
    int length)
{
    if (data == nullptr || length <= 0)
    {
        return false;
    }

    if (receive_length_ + length >
        static_cast<int>(sizeof(receive_buffer_)))
    {
        return false;
    }

    std::memcpy(
        receive_buffer_ + receive_length_,
        data,
        length);

    receive_length_ += length;

    return true;
}
