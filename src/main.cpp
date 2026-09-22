#include "tcp_client.h"
#include <cstdint>
#include <cstdio>
#include <windows.h>
#include "status_data.h"

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

    std::printf("[CLIENT] Connected\n");

    uint8_t receive_buffer[512];

    while (client.IsConnected())
    {
        int received = client.Receive(
            receive_buffer,
            sizeof(receive_buffer));

        if (received <= 0)
        {
            break;
        }

        std::printf("[RX] TCP bytes received = %d\n",
                    received);

        while (client.GetStatusPacket(
            receive_buffer,
            sizeof(receive_buffer)))
        {
            std::printf("[PACKET] Complete packet received\n");

            std::printf("[PACKET] HEX:");

            for (int i = 0; i < 151; i++)
            {
                std::printf(" %02X", receive_buffer[i]);
            }

            std::printf("\n");

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
                    std::printf("[STATUS] Controller Status = 0x%08X\n",
                                status_data.controller_status);

                    std::printf("[STATUS] CONNECT=%d "
                                "VOLTAGEON=%d "
                                "ISMOVING=%d "
                                "ISFA=%d "
                                "HOMINGEND=%d "
                                "ERROR=%d\n",
                                status_data.connect,
                                status_data.voltage_on,
                                status_data.is_moving,
                                status_data.is_fa,
                                status_data.homing_end,
                                status_data.error);

                    std::printf("[STATUS] AI00 Raw=%u Voltage=%.3f V\n",
                                status_data.ai[0],
                                status_data.ai_voltage[0]);

                    std::printf("[STATUS] AI01 Raw=%u\n",
                                status_data.ai[1]);

                    std::printf("[STATUS] AI02 Raw=%u\n",
                                status_data.ai[2]);

                    std::printf("[STATUS] AI03 Raw=%u\n",
                                status_data.ai[3]);

                    std::printf("[STATUS] AI04 Raw=%u Voltage=%.3f V\n",
                                status_data.ai[4],
                                status_data.ai_voltage[4]);

                    std::printf("[STATUS] AI05 Raw=%u Voltage=%.3f V\n",
                                status_data.ai[5],
                                status_data.ai_voltage[5]);

                    std::printf("[STATUS] AI06 Raw=%u Voltage=%.3f V\n",
                                status_data.ai[6],
                                status_data.ai_voltage[6]);

                    std::printf("[STATUS] AI07 Raw=%u Voltage=%.3f V\n",
                                status_data.ai[7],
                                status_data.ai_voltage[7]);

                    std::printf("[STATUS] Position "
                                "X=%.3f Y=%.3f Z=%.3f "
                                "RX=%.3f RY=%.3f RZ=%.3f\n",
                                status_data.x,
                                status_data.y,
                                status_data.z,
                                status_data.rx,
                                status_data.ry,
                                status_data.rz);

                    std::printf("\n");
                }
            }


        }
    }

}
