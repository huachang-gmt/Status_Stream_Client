#include "tcp_client.h"
#include <cstdint>
#include <cstdio>
#include <windows.h>
#include "status_data.h"
#include <cstdlib>

int main()
{
    constexpr const char* SERVER_IP = "192.168.137.10";
    constexpr uint16_t SERVER_PORT = 8888;

    std::printf("========================================\n");
    std::printf(" GMT Status Stream Client\n");
    std::printf("========================================\n\n");

    TcpClient client;

    if (!client.Connect(SERVER_IP, SERVER_PORT))
    {
        std::printf("[CLIENT] Connection failed\n");
        return 1;
    }

    uint8_t receive_buffer[512];
    uint32_t current_sequence = 0;
    uint32_t packet_loss = 0;
    uint32_t latest_missing_sequence = 0;
    bool has_missing_sequence = false;

    while (client.IsConnected())
    {
        int received = client.Receive(
            receive_buffer,
            sizeof(receive_buffer));

        if (received <= 0)
        {
            break;
        }

        while (client.GetStatusPacket(
            receive_buffer,
            sizeof(receive_buffer)))
        {

            current_sequence =
                static_cast<uint32_t>(receive_buffer[4]) |
                (static_cast<uint32_t>(receive_buffer[5]) << 8U) |
                (static_cast<uint32_t>(receive_buffer[6]) << 16U) |
                (static_cast<uint32_t>(receive_buffer[7]) << 24U);

            uint8_t raw_data[68];
            StatusData status_data{};

            if (client.DecodeStatusPayload(
                    receive_buffer,
                    151,
                    raw_data,
                    sizeof(raw_data)))
            {
                if (client.ParseStatusData(
                        raw_data,
                        sizeof(raw_data),
                        status_data))
                {

                    std::system("cls");

                    std::printf("============================================================\n");
                    std::printf(" GMT Status Stream Client\n");
                    std::printf("============================================================\n\n");

                    std::printf(" Connection\n");
                    std::printf("   Server                    : 192.168.137.10:8888\n");
                    std::printf("   Status                    : CONNECTED\n\n");

                    std::printf(" Packet\n");
                    std::printf("   Current Sequence          : %u\n",
                                current_sequence);
                    std::printf("   Packet Loss               : %u\n",
                                packet_loss);

                    if (has_missing_sequence)
                    {
                        std::printf("   Latest Missing Sequence   : %u\n",
                                    latest_missing_sequence);
                    }
                    else
                    {
                        std::printf("   Latest Missing Sequence   : N/A\n");
                    }

                    std::printf("\n");

                    std::printf(" Controller\n");
                    std::printf("   CONNECT                   : %s\n",
                                status_data.connect ? "ON" : "OFF");
                    std::printf("   VOLTAGEON                 : %s\n",
                                status_data.voltage_on ? "ON" : "OFF");
                    std::printf("   ISMOVING                  : %s\n",
                                status_data.is_moving ? "ON" : "OFF");
                    std::printf("   ISFA                      : %s\n",
                                status_data.is_fa ? "ON" : "OFF");
                    std::printf("   HOMINGEND                 : %s\n",
                                status_data.homing_end ? "ON" : "OFF");
                    std::printf("   ERROR                     : %s\n",
                                status_data.error ? "ON" : "OFF");

                    std::printf("\n");

                    std::printf(" Analog Input\n");
                    std::printf("   AI00                      : %.3f V\n",
                                status_data.ai_voltage[0]);
                    std::printf("   AI01                      : RAW %u\n",
                                status_data.ai[1]);
                    std::printf("   AI02                      : RAW %u\n",
                                status_data.ai[2]);
                    std::printf("   AI03                      : RAW %u\n",
                                status_data.ai[3]);
                    std::printf("   AI04                      : %.3f V\n",
                                status_data.ai_voltage[4]);
                    std::printf("   AI05                      : %.3f V\n",
                                status_data.ai_voltage[5]);
                    std::printf("   AI06                      : %.3f V\n",
                                status_data.ai_voltage[6]);
                    std::printf("   AI07                      : %.3f V\n",
                                status_data.ai_voltage[7]);

                    std::printf("\n");

                    std::printf(" Position\n");
                    std::printf("   X                         : %.3f\n",
                                status_data.x);
                    std::printf("   Y                         : %.3f\n",
                                status_data.y);
                    std::printf("   Z                         : %.3f\n",
                                status_data.z);
                    std::printf("   RX                        : %.3f\n",
                                status_data.rx);
                    std::printf("   RY                        : %.3f\n",
                                status_data.ry);
                    std::printf("   RZ                        : %.3f\n",
                                status_data.rz);

                    std::printf("\n");
                    std::printf("============================================================\n");

                    std::printf("\n");
                }
            }


        }






















    }

}
