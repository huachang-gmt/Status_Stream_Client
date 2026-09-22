#include "tcp_client.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>

int main()
{
    std::printf("========================================\n");
    std::printf(" Status Stream Packet Assembly Test\n");
    std::printf("========================================\n\n");

    TcpClient client;

    uint8_t test_packet[151];
    
    test_packet[0] = 0x47;
    test_packet[1] = 0x53;
    test_packet[2] = 0x01;
    test_packet[3] = 0x01;

    // Sequence = 0x00000001
    test_packet[4] = 0x01;
    test_packet[5] = 0x00;
    test_packet[6] = 0x00;
    test_packet[7] = 0x00;

    // Payload Length = 139
    test_packet[8] = 0x8B;
    test_packet[9] = 0x00;

    // Payload
    test_packet[10] = '>';

    for (int i = 11; i < 147; ++i)
    {
        test_packet[i] = '0';
    }


    test_packet[147] = '\r';
    test_packet[148] = '\n';

    uint16_t test_crc = 0xFFFF;

    for (int i = 0; i < 149; ++i)
    {
        test_crc ^= test_packet[i];

        for (int bit = 0; bit < 8; ++bit)
        {
            if ((test_crc & 0x0001U) != 0)
            {
                test_crc = static_cast<uint16_t>(
                    (test_crc >> 1U) ^ 0xA001U);
            }
            else
            {
                test_crc >>= 1U;
            }
        }
    }

    test_packet[149] =
        static_cast<uint8_t>(test_crc & 0xFFU);

    test_packet[150] =
        static_cast<uint8_t>((test_crc >> 8U) & 0xFFU);

    uint8_t output[151];

    std::printf("[TEST 1] Fragmented packet: 80 + 71\n");

    client.ResetReceiveBuffer();

    client.AppendTestData(test_packet, 80);

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet assembled too early\n");
        return 1;
    }

    client.AppendTestData(
        test_packet + 80,
        71);

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet was not assembled\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Packet data mismatch\n");
        return 1;
    }

    std::printf("[PASS] 80 + 71 -> 151 bytes\n\n");


    std::printf("[TEST 2] Two packets: 302 bytes\n");

    client.ResetReceiveBuffer();

    uint8_t two_packets[302];

    std::memcpy(
        two_packets,
        test_packet,
        151);

    std::memcpy(
        two_packets + 151,
        test_packet,
        151);

    client.AppendTestData(
        two_packets,
        302);

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] First packet was not extracted\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] First packet mismatch\n");
        return 1;
    }

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Second packet was not extracted\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Second packet mismatch\n");
        return 1;
    }

    std::printf("[PASS] 302 bytes -> 151 + 151\n\n");


    std::printf("[TEST 3] Fragmented packet: 50 + 50 + 51\n");

    client.ResetReceiveBuffer();

    client.AppendTestData(
        test_packet,
        50);

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet assembled too early after 50 bytes\n");
        return 1;
    }

    client.AppendTestData(
        test_packet + 50,
        50);

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet assembled too early after 100 bytes\n");
        return 1;
    }

    client.AppendTestData(
        test_packet + 100,
        51);

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet was not assembled after 50 + 50 + 51\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Fragmented packet data mismatch\n");
        return 1;
    }

    std::printf("[PASS] 50 + 50 + 51 -> 151 bytes\n\n");

    
    std::printf("[TEST 4] Noise before Magic\n");

    client.ResetReceiveBuffer();

    uint8_t noise_and_packet[155];

    noise_and_packet[0] = 0xAA;
    noise_and_packet[1] = 0xBB;
    noise_and_packet[2] = 0xCC;
    noise_and_packet[3] = 0x11;

    std::memcpy(
        noise_and_packet + 4,
        test_packet,
        151);

    client.AppendTestData(
        noise_and_packet,
        sizeof(noise_and_packet));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet was not found after noise\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Packet mismatch after noise\n");
        return 1;
    }

    std::printf("[PASS] Noise + packet -> 151 bytes\n\n");


    std::printf("[TEST 5] Magic split across fragments\n");

    client.ResetReceiveBuffer();

    uint8_t first_fragment[4] =
    {
        0xAA,
        0xBB,
        0xCC,
        0x47
    };

    client.AppendTestData(
        first_fragment,
        sizeof(first_fragment));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet assembled too early\n");
        return 1;
    }

    uint8_t second_fragment[150];

    second_fragment[0] = 0x53;

    std::memcpy(
        second_fragment + 1,
        test_packet + 2,
        149);

    client.AppendTestData(
        second_fragment,
        sizeof(second_fragment));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Packet was not assembled after split Magic\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Packet mismatch after split Magic\n");
        return 1;
    }

    std::printf("[PASS] Magic 47 + 53 across fragments -> 151 bytes\n\n");


    std::printf("[TEST 6] Invalid Version\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_version_packet[151];

    std::memcpy(
        invalid_version_packet,
        test_packet,
        sizeof(test_packet));

    invalid_version_packet[2] = 0x02;

    client.AppendTestData(
        invalid_version_packet,
        sizeof(invalid_version_packet));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid Version was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Version rejected\n\n");


    std::printf("[TEST 7] Invalid Type\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_type_packet[151];

    std::memcpy(
        invalid_type_packet,
        test_packet,
        sizeof(test_packet));

    invalid_type_packet[3] = 0x02;

    client.AppendTestData(
        invalid_type_packet,
        sizeof(invalid_type_packet));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid Type was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Type rejected\n\n");


    std::printf("[TEST 8] Valid Version and Type\n");

    client.ResetReceiveBuffer();

    client.AppendTestData(
        test_packet,
        sizeof(test_packet));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Valid Version and Type were rejected\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Valid packet data mismatch\n");
        return 1;
    }

    std::printf("[PASS] Valid Version and Type accepted\n\n");


    std::printf("[TEST 9] Invalid Payload Length: 138\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_length_138_packet[151];

    std::memcpy(
        invalid_length_138_packet,
        test_packet,
        sizeof(test_packet));

    invalid_length_138_packet[8] = 0x8A;
    invalid_length_138_packet[9] = 0x00;

    client.AppendTestData(
        invalid_length_138_packet,
        sizeof(invalid_length_138_packet));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Payload Length 138 was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Payload Length 138 rejected\n\n");


    std::printf("[TEST 10] Invalid Payload Length: 140\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_length_140_packet[151];

    std::memcpy(
        invalid_length_140_packet,
        test_packet,
        sizeof(test_packet));

    invalid_length_140_packet[8] = 0x8C;
    invalid_length_140_packet[9] = 0x00;

    client.AppendTestData(
        invalid_length_140_packet,
        sizeof(invalid_length_140_packet));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Payload Length 140 was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Payload Length 140 rejected\n\n");


    std::printf("[TEST 11] Valid Payload Length: 139\n");

    client.ResetReceiveBuffer();

    client.AppendTestData(
        test_packet,
        sizeof(test_packet));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Payload Length 139 was rejected\n");
        return 1;
    }

    if (std::memcmp(
            output,
            test_packet,
            sizeof(test_packet)) != 0)
    {
        std::printf("[FAIL] Valid Payload Length packet mismatch\n");
        return 1;
    }

    std::printf("[PASS] Valid Payload Length 139 accepted\n\n");


    auto CalculateCRC16 = [](const uint8_t* data, int length) -> uint16_t
    {
        uint16_t crc = 0xFFFF;

        for (int i = 0; i < length; ++i)
        {
            crc ^= data[i];

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

        return crc;
    };


    std::printf("[TEST 12] Calculate valid CRC\n");

    uint16_t valid_crc =
        CalculateCRC16(test_packet, 149);

    std::printf("[TEST 12] Calculated CRC = %04X\n",
                valid_crc);

    test_packet[149] =
        static_cast<uint8_t>(valid_crc & 0xFFU);

    test_packet[150] =
        static_cast<uint8_t>((valid_crc >> 8U) & 0xFFU);

    uint16_t verify_crc =
        CalculateCRC16(test_packet, 149);

    uint16_t packet_crc =
        static_cast<uint16_t>(test_packet[149]) |
        (static_cast<uint16_t>(test_packet[150]) << 8U);

    if (verify_crc != packet_crc)
    {
        std::printf("[FAIL] CRC calculation mismatch\n");
        return 1;
    }

    std::printf("[PASS] Valid CRC generated and verified\n\n");


    std::printf("[TEST 13] Invalid CRC\n");

    test_packet[149] ^= 0x01U;

    uint16_t invalid_packet_crc =
        static_cast<uint16_t>(test_packet[149]) |
        (static_cast<uint16_t>(test_packet[150]) << 8U);

    if (CalculateCRC16(test_packet, 149) ==
        invalid_packet_crc)
    {
        std::printf("[FAIL] Invalid CRC was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid CRC detected\n\n");


    std::printf("[TEST 14] Valid CRC packet accepted\n");

    client.ResetReceiveBuffer();

    uint8_t valid_crc_packet[151];

    std::memcpy(
        valid_crc_packet,
        test_packet,
        sizeof(test_packet));

    uint16_t crc14 =
        CalculateCRC16(valid_crc_packet, 149);

    valid_crc_packet[149] =
        static_cast<uint8_t>(crc14 & 0xFFU);

    valid_crc_packet[150] =
        static_cast<uint8_t>((crc14 >> 8U) & 0xFFU);

    client.AppendTestData(
        valid_crc_packet,
        sizeof(valid_crc_packet));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Valid CRC packet was rejected\n");
        return 1;
    }

    if (std::memcmp(
            output,
            valid_crc_packet,
            sizeof(valid_crc_packet)) != 0)
    {
        std::printf("[FAIL] Valid CRC packet data mismatch\n");
        return 1;
    }

    std::printf("[PASS] Valid CRC packet accepted\n\n");


    std::printf("[TEST 15] Invalid CRC packet rejected\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_crc_packet[151];

    std::memcpy(
        invalid_crc_packet,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_crc_packet[149] ^= 0x01U;

    client.AppendTestData(
        invalid_crc_packet,
        sizeof(invalid_crc_packet));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid CRC packet was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid CRC packet rejected\n\n");


    std::printf("[TEST 16] Invalid CRC followed by valid packet\n");

    client.ResetReceiveBuffer();

    uint8_t two_crc_packets[302];

    std::memcpy(
        two_crc_packets,
        invalid_crc_packet,
        sizeof(invalid_crc_packet));

    std::memcpy(
        two_crc_packets + 151,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    client.AppendTestData(
        two_crc_packets,
        sizeof(two_crc_packets));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid CRC packet was accepted\n");
        return 1;
    }

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Valid packet after invalid CRC was not found\n");
        return 1;
    }

    if (std::memcmp(
            output,
            valid_crc_packet,
            sizeof(valid_crc_packet)) != 0)
    {
        std::printf("[FAIL] Valid packet after invalid CRC mismatch\n");
        return 1;
    }

    std::printf(
        "[PASS] Invalid CRC discarded, next valid packet accepted\n\n");


    std::printf("[TEST 17] Valid Payload Format\n");

    if (!client.ValidateStatusPayload(
            valid_crc_packet,
            sizeof(valid_crc_packet)))
    {
        std::printf("[FAIL] Valid Payload Format was rejected\n");
        return 1;
    }

    std::printf("[PASS] Valid Payload Format accepted\n\n");


    std::printf("[TEST 18] Invalid Payload Start\n");

    uint8_t invalid_start_packet[151];

    std::memcpy(
        invalid_start_packet,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_start_packet[10] = '<';

    if (client.ValidateStatusPayload(
            invalid_start_packet,
            sizeof(invalid_start_packet)))
    {
        std::printf("[FAIL] Invalid Payload Start was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Payload Start rejected\n\n");


    std::printf("[TEST 19] Invalid HEX Character\n");

    uint8_t invalid_hex_packet[151];

    std::memcpy(
        invalid_hex_packet,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_hex_packet[20] = 'G';

    if (client.ValidateStatusPayload(
            invalid_hex_packet,
            sizeof(invalid_hex_packet)))
    {
        std::printf("[FAIL] Invalid HEX Character was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid HEX Character rejected\n\n");


    std::printf("[TEST 20] Invalid Payload Terminator\n");

    uint8_t invalid_terminator_packet[151];

    std::memcpy(
        invalid_terminator_packet,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_terminator_packet[147] = '\n';
    invalid_terminator_packet[148] = '\r';

    if (client.ValidateStatusPayload(
            invalid_terminator_packet,
            sizeof(invalid_terminator_packet)))
    {
        std::printf("[FAIL] Invalid Payload Terminator was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Payload Terminator rejected\n\n");


    std::printf("[TEST 21] Valid Payload through GetStatusPacket\n");

    client.ResetReceiveBuffer();

    client.AppendTestData(
        valid_crc_packet,
        sizeof(valid_crc_packet));

    if (!client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Valid Payload was rejected\n");
        return 1;
    }

    if (std::memcmp(
            output,
            valid_crc_packet,
            sizeof(valid_crc_packet)) != 0)
    {
        std::printf("[FAIL] Valid Payload packet mismatch\n");
        return 1;
    }

    std::printf("[PASS] Valid Payload accepted by GetStatusPacket\n\n");


    std::printf("[TEST 22] Invalid Payload Start through GetStatusPacket\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_start_packet_22[151];

    std::memcpy(
        invalid_start_packet_22,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_start_packet_22[10] = '<';

    client.AppendTestData(
        invalid_start_packet_22,
        sizeof(invalid_start_packet_22));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid Payload Start was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid Payload Start rejected by GetStatusPacket\n\n");


    std::printf("[TEST 23] Invalid HEX through GetStatusPacket\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_hex_packet_23[151];

    std::memcpy(
        invalid_hex_packet_23,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_hex_packet_23[20] = 'G';

    client.AppendTestData(
        invalid_hex_packet_23,
        sizeof(invalid_hex_packet_23));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid HEX was accepted\n");
        return 1;
    }

    std::printf("[PASS] Invalid HEX rejected by GetStatusPacket\n\n");


    std::printf("[TEST 24] Invalid Terminator through GetStatusPacket\n");

    client.ResetReceiveBuffer();

    uint8_t invalid_terminator_packet_24[151];

    std::memcpy(
        invalid_terminator_packet_24,
        valid_crc_packet,
        sizeof(valid_crc_packet));

    invalid_terminator_packet_24[147] = '\n';
    invalid_terminator_packet_24[148] = '\r';

    client.AppendTestData(
        invalid_terminator_packet_24,
        sizeof(invalid_terminator_packet_24));

    if (client.GetStatusPacket(
            output,
            sizeof(output)))
    {
        std::printf("[FAIL] Invalid Payload Terminator was accepted\n");
        return 1;
    }

    std::printf(
        "[PASS] Invalid Payload Terminator rejected by GetStatusPacket\n\n");


    std::printf("[TEST 25] HEX Payload Decode\n");

    uint8_t decode_test_packet[151];

    std::memcpy(
        decode_test_packet,
        valid_crc_packet,
        sizeof(decode_test_packet));

    // Use known raw data: 00 01 02 ... 43
    for (int i = 0; i < 68; ++i)
    {
        const uint8_t value =
            static_cast<uint8_t>(i);

        const char hex[] = "0123456789ABCDEF";

        decode_test_packet[11 + i * 2] =
            hex[(value >> 4U) & 0x0FU];

        decode_test_packet[11 + i * 2 + 1] =
            hex[value & 0x0FU];
    }

    uint16_t decode_test_crc = 0xFFFF;

    for (int i = 0; i < 149; ++i)
    {
        decode_test_crc ^= decode_test_packet[i];

        for (int bit = 0; bit < 8; ++bit)
        {
            if ((decode_test_crc & 0x0001U) != 0)
            {
                decode_test_crc = static_cast<uint16_t>(
                    (decode_test_crc >> 1U) ^ 0xA001U);
            }
            else
            {
                decode_test_crc >>= 1U;
            }
        }
    }

    decode_test_packet[149] =
        static_cast<uint8_t>(decode_test_crc & 0xFFU);

    decode_test_packet[150] =
        static_cast<uint8_t>(
            (decode_test_crc >> 8U) & 0xFFU);

    uint8_t decoded_raw_data[68] = {};

    if (!client.DecodeStatusPayload(
            decode_test_packet,
            sizeof(decode_test_packet),
            decoded_raw_data,
            sizeof(decoded_raw_data)))
    {
        std::printf(
            "[FAIL] DecodeStatusPayload returned false\n");

        return 1;
    }

    bool decode_ok = true;

    for (int i = 0; i < 68; ++i)
    {
        if (decoded_raw_data[i] !=
            static_cast<uint8_t>(i))
        {
            decode_ok = false;
            break;
        }
    }

    if (!decode_ok)
    {
        std::printf(
            "[FAIL] Decoded raw data does not match expected data\n");

        return 1;
    }

    std::printf(
        "[PASS] 136 HEX characters decoded to 68 raw bytes\n");


    std::printf("\n[TEST 26] Raw Status Data Parsing\n");

    uint8_t parse_test_raw_data[68] = {};

    // Controller Status = 0x00000053
    parse_test_raw_data[0] = 0x00;
    parse_test_raw_data[1] = 0x00;
    parse_test_raw_data[2] = 0x00;
    parse_test_raw_data[3] = 0x53;

    // AI00 ~ AI07
    const uint16_t expected_ai[8] =
    {
        1000,
        2000,
        3000,
        4000,
        5000,
        6000,
        7000,
        8000
    };

    for (int i = 0; i < 8; ++i)
    {
        const int offset = 4 + i * 2;

        parse_test_raw_data[offset] =
            static_cast<uint8_t>(
                (expected_ai[i] >> 8U) & 0xFFU);

        parse_test_raw_data[offset + 1] =
            static_cast<uint8_t>(
                expected_ai[i] & 0xFFU);
    }

    // X, Y, Z, RX, RY, RZ
    const double expected_axis[6] =
    {
        1.25,
        -2.50,
        100.0,
        0.5,
        -0.75,
        3.14159
    };

    for (int axis = 0; axis < 6; ++axis)
    {
        uint64_t bits = 0;

        std::memcpy(
            &bits,
            &expected_axis[axis],
            sizeof(bits));

        const int offset = 20 + axis * 8;

        for (int i = 0; i < 8; ++i)
        {
            parse_test_raw_data[offset + i] =
                static_cast<uint8_t>(
                    (bits >> (56U - i * 8U)) & 0xFFU);
        }
    }

    StatusData parsed_status = {};

    if (!client.ParseStatusData(
            parse_test_raw_data,
            sizeof(parse_test_raw_data),
            parsed_status))
    {
        std::printf(
            "[FAIL] ParseStatusData returned false\n");

        return 1;
    }

    if (parsed_status.controller_status != 0x00000053U)
    {
        std::printf(
            "[FAIL] Controller Status mismatch: %08X\n",
            parsed_status.controller_status);

        return 1;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (parsed_status.ai[i] != expected_ai[i])
        {
            std::printf(
                "[FAIL] AI%d mismatch: expected=%u actual=%u\n",
                i,
                expected_ai[i],
                parsed_status.ai[i]);

            return 1;
        }
    }

    const double parsed_axis[6] =
    {
        parsed_status.x,
        parsed_status.y,
        parsed_status.z,
        parsed_status.rx,
        parsed_status.ry,
        parsed_status.rz
    };

    for (int axis = 0; axis < 6; ++axis)
    {
        if (parsed_axis[axis] != expected_axis[axis])
        {
            std::printf(
                "[FAIL] Axis %d mismatch: expected=%f actual=%f\n",
                axis,
                expected_axis[axis],
                parsed_axis[axis]);

            return 1;
        }
    }

    std::printf(
        "[PASS] Controller Status, AI00-AI07 and 6 DOUBLE fields parsed correctly\n");


    std::printf("\n[TEST 27] Complete Status Packet to StatusData\n");

    uint8_t integration_packet[151];

    std::memcpy(
        integration_packet,
        valid_crc_packet,
        sizeof(integration_packet));

    // Controller Status = 0x00000053
    integration_packet[11] = '0';
    integration_packet[12] = '0';
    integration_packet[13] = '0';
    integration_packet[14] = '0';
    integration_packet[15] = '0';
    integration_packet[16] = '0';
    integration_packet[17] = '5';
    integration_packet[18] = '3';

    // AI00 ~ AI07
    const uint16_t integration_ai[8] =
    {
        1000,
        2000,
        3000,
        4000,
        5000,
        6000,
        7000,
        8000
    };

    const char integration_hex[] =
        "0123456789ABCDEF";

    for (int i = 0; i < 8; ++i)
    {
        const int payload_hex_offset =
            19 + i * 4;

        const uint16_t value =
            integration_ai[i];

        integration_packet[payload_hex_offset] =
            integration_hex[(value >> 12U) & 0x0FU];

        integration_packet[payload_hex_offset + 1] =
            integration_hex[(value >> 8U) & 0x0FU];

        integration_packet[payload_hex_offset + 2] =
            integration_hex[(value >> 4U) & 0x0FU];

        integration_packet[payload_hex_offset + 3] =
            integration_hex[value & 0x0FU];
    }

    // X, Y, Z, RX, RY, RZ
    const double integration_axis[6] =
    {
        1.25,
        -2.50,
        100.0,
        0.5,
        -0.75,
        3.14159
    };

    for (int axis = 0; axis < 6; ++axis)
    {
        uint64_t bits = 0;

        std::memcpy(
            &bits,
            &integration_axis[axis],
            sizeof(bits));

        const int payload_hex_offset =
            51 + axis * 16;

        for (int i = 0; i < 8; ++i)
        {
            const uint8_t byte =
                static_cast<uint8_t>(
                    (bits >> (56U - i * 8U)) & 0xFFU);

            integration_packet[payload_hex_offset + i * 2] =
                integration_hex[(byte >> 4U) & 0x0FU];

            integration_packet[payload_hex_offset + i * 2 + 1] =
                integration_hex[byte & 0x0FU];
        }
    }

    uint16_t integration_crc = 0xFFFF;

    for (int i = 0; i < 149; ++i)
    {
        integration_crc ^= integration_packet[i];

        for (int bit = 0; bit < 8; ++bit)
        {
            if ((integration_crc & 0x0001U) != 0)
            {
                integration_crc = static_cast<uint16_t>(
                    (integration_crc >> 1U) ^ 0xA001U);
            }
            else
            {
                integration_crc >>= 1U;
            }
        }
    }

    integration_packet[149] =
        static_cast<uint8_t>(
            integration_crc & 0xFFU);

    integration_packet[150] =
        static_cast<uint8_t>(
            (integration_crc >> 8U) & 0xFFU);

    client.ResetReceiveBuffer();

    if (!client.AppendTestData(
            integration_packet,
            sizeof(integration_packet)))
    {
        std::printf(
            "[FAIL] Failed to append integration packet\n");

        return 1;
    }

    uint8_t received_packet[151] = {};

    if (!client.GetStatusPacket(
            received_packet,
            sizeof(received_packet)))
    {
        std::printf(
            "[FAIL] GetStatusPacket rejected valid integration packet\n");

        return 1;
    }

    uint8_t integration_raw_data[68] = {};

    if (!client.DecodeStatusPayload(
            received_packet,
            sizeof(received_packet),
            integration_raw_data,
            sizeof(integration_raw_data)))
    {
        std::printf(
            "[FAIL] DecodeStatusPayload failed\n");

        return 1;
    }

    StatusData integration_status = {};

    if (!client.ParseStatusData(
            integration_raw_data,
            sizeof(integration_raw_data),
            integration_status))
    {
        std::printf(
            "[FAIL] ParseStatusData failed\n");

        return 1;
    }

    if (integration_status.controller_status !=
        0x00000053U)
    {
        std::printf(
            "[FAIL] Integration Controller Status mismatch\n");

        return 1;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (integration_status.ai[i] !=
            integration_ai[i])
        {
            std::printf(
                "[FAIL] Integration AI%d mismatch\n",
                i);

            return 1;
        }
    }

    const double integration_parsed_axis[6] =
    {
        integration_status.x,
        integration_status.y,
        integration_status.z,
        integration_status.rx,
        integration_status.ry,
        integration_status.rz
    };

    for (int axis = 0; axis < 6; ++axis)
    {
        if (integration_parsed_axis[axis] !=
            integration_axis[axis])
        {
            std::printf(
                "[FAIL] Integration Axis %d mismatch\n",
                axis);

            return 1;
        }
    }

    std::printf(
        "[PASS] 151-byte packet decoded to complete StatusData\n");

    
        
    std::printf("\n[TEST 28] Controller Status Bit Parsing\n");

    uint8_t status_bit_test_raw_data[68] = {};

    // Set bits: CONNECT, VOLTAGEON, ISFA, HOMINGEND, ERROR
    const uint32_t test_controller_status =
        (1U << 0U) |
        (1U << 1U) |
        (1U << 4U) |
        (1U << 5U) |
        (1U << 6U);

    status_bit_test_raw_data[0] =
        static_cast<uint8_t>(
            (test_controller_status >> 24U) & 0xFFU);

    status_bit_test_raw_data[1] =
        static_cast<uint8_t>(
            (test_controller_status >> 16U) & 0xFFU);

    status_bit_test_raw_data[2] =
        static_cast<uint8_t>(
            (test_controller_status >> 8U) & 0xFFU);

    status_bit_test_raw_data[3] =
        static_cast<uint8_t>(
            test_controller_status & 0xFFU);

    StatusData status_bit_test = {};

    if (!client.ParseStatusData(
            status_bit_test_raw_data,
            sizeof(status_bit_test_raw_data),
            status_bit_test))
    {
        std::printf(
            "[FAIL] ParseStatusData returned false\n");

        return 1;
    }

    if (status_bit_test.controller_status !=
        test_controller_status)
    {
        std::printf(
            "[FAIL] Controller Status value mismatch\n");

        return 1;
    }

    if (!status_bit_test.connect)
    {
        std::printf(
            "[FAIL] CONNECT bit not parsed correctly\n");

        return 1;
    }

    if (!status_bit_test.voltage_on)
    {
        std::printf(
            "[FAIL] VOLTAGEON bit not parsed correctly\n");

        return 1;
    }

    if (status_bit_test.is_moving)
    {
        std::printf(
            "[FAIL] ISMOVING bit should be false\n");

        return 1;
    }

    if (!status_bit_test.is_fa)
    {
        std::printf(
            "[FAIL] ISFA bit not parsed correctly\n");

        return 1;
    }

    if (!status_bit_test.homing_end)
    {
        std::printf(
            "[FAIL] HOMINGEND bit not parsed correctly\n");

        return 1;
    }

    if (!status_bit_test.error)
    {
        std::printf(
            "[FAIL] ERROR bit not parsed correctly\n");

        return 1;
    }

    std::printf(
        "[PASS] Controller Status bits parsed correctly\n");


    std::printf("\n[TEST 29] AI Scaling Formula\n");

    const double ai00_raw_0 =
        client.ScaleAIValue(0, 0);

    const double ai00_raw_max =
        client.ScaleAIValue(0, 65535);

    if (ai00_raw_0 != -10.0)
    {
        std::printf(
            "[FAIL] AI00 Raw=0: expected=-10.0 actual=%f\n",
            ai00_raw_0);

        return 1;
    }

    if (ai00_raw_max != 10.0)
    {
        std::printf(
            "[FAIL] AI00 Raw=65535: expected=10.0 actual=%f\n",
            ai00_raw_max);

        return 1;
    }


    const double ai04_raw_0 =
        client.ScaleAIValue(4, 0);

    const double ai04_raw_max =
        client.ScaleAIValue(4, 65535);

    if (ai04_raw_0 != 0.0)
    {
        std::printf(
            "[FAIL] AI04 Raw=0: expected=0.0 actual=%f\n",
            ai04_raw_0);

        return 1;
    }

    if (ai04_raw_max != 10.0)
    {
        std::printf(
            "[FAIL] AI04 Raw=65535: expected=10.0 actual=%f\n",
            ai04_raw_max);

        return 1;
    }


    const double ai05_raw_max =
        client.ScaleAIValue(5, 65535);

    const double ai06_raw_max =
        client.ScaleAIValue(6, 65535);

    const double ai07_raw_max =
        client.ScaleAIValue(7, 65535);

    if (ai05_raw_max != 10.0 ||
        ai06_raw_max != 10.0 ||
        ai07_raw_max != 10.0)
    {
        std::printf(
            "[FAIL] AI05~AI07 maximum scaling mismatch\n");

        return 1;
    }


    const double ai01_value =
        client.ScaleAIValue(1, 12345);

    const double ai02_value =
        client.ScaleAIValue(2, 12345);

    const double ai03_value =
        client.ScaleAIValue(3, 12345);

    if (ai01_value != 0.0 ||
        ai02_value != 0.0 ||
        ai03_value != 0.0)
    {
        std::printf(
            "[FAIL] AI01~AI03 undefined scaling handling mismatch\n");

        return 1;
    }

    std::printf(
        "[PASS] AI00 and AI04~AI07 scaling formulas verified\n");

    std::printf(
        "[PASS] AI01~AI03 remain without defined scaling\n");


    std::printf("\n[TEST 30] AI Scaling Intermediate Values\n");

    const double ai00_mid_low =
        client.ScaleAIValue(0, 32767);

    const double ai00_mid_high =
        client.ScaleAIValue(0, 32768);

    const double ai04_mid_low =
        client.ScaleAIValue(4, 32767);

    const double ai04_mid_high =
        client.ScaleAIValue(4, 32768);

    const double expected_ai00_mid_low =
        (32767.0 / 65535.0) * 20.0 - 10.0;

    const double expected_ai00_mid_high =
        (32768.0 / 65535.0) * 20.0 - 10.0;

    const double expected_ai04_mid_low =
        (32767.0 / 65535.0) * 10.0;

    const double expected_ai04_mid_high =
        (32768.0 / 65535.0) * 10.0;

    const double tolerance = 1e-9;

    if (std::fabs(ai00_mid_low - expected_ai00_mid_low) > tolerance ||
        std::fabs(ai00_mid_high - expected_ai00_mid_high) > tolerance)
    {
        std::printf(
            "[FAIL] AI00 intermediate scaling mismatch\n");

        return 1;
    }

    if (std::fabs(ai04_mid_low - expected_ai04_mid_low) > tolerance ||
        std::fabs(ai04_mid_high - expected_ai04_mid_high) > tolerance)
    {
        std::printf(
            "[FAIL] AI04 intermediate scaling mismatch\n");

        return 1;
    }

    std::printf(
        "[PASS] AI00 intermediate scaling verified\n");

    std::printf(
        "[PASS] AI04 intermediate scaling verified\n");


    std::printf("\n[TEST 31] AI Raw and Scaled Integration\n");

    uint8_t ai_test_raw[68] = {};

    ai_test_raw[0] = 0x00;
    ai_test_raw[1] = 0x00;
    ai_test_raw[2] = 0x00;
    ai_test_raw[3] = 0x01;

    /* AI00 = Raw 0 -> -10V */
    ai_test_raw[4] = 0x00;
    ai_test_raw[5] = 0x00;

    /* AI01 = Reserved */
    ai_test_raw[6] = 0x30;
    ai_test_raw[7] = 0x39;

    /* AI02 = I2C0 */
    ai_test_raw[8] = 0x12;
    ai_test_raw[9] = 0x34;

    /* AI03 = I2C3 */
    ai_test_raw[10] = 0x56;
    ai_test_raw[11] = 0x78;

    /* AI04 = Raw 65535 -> 10V */
    ai_test_raw[12] = 0xFF;
    ai_test_raw[13] = 0xFF;

    /* AI05 = Raw 32768 */
    ai_test_raw[14] = 0x80;
    ai_test_raw[15] = 0x00;

    /* AI06 = Raw 32767 */
    ai_test_raw[16] = 0x7F;
    ai_test_raw[17] = 0xFF;

    /* AI07 = Raw 0 */
    ai_test_raw[18] = 0x00;
    ai_test_raw[19] = 0x00;

    StatusData ai_test_status{};

    if (!client.ParseStatusData(
            ai_test_raw,
            sizeof(ai_test_raw),
            ai_test_status))
    {
        std::printf(
            "[FAIL] ParseStatusData rejected AI test data\n");

        return 1;
    }

    const double expected_ai05 =
        (32768.0 / 65535.0) * 10.0;

    const double expected_ai06 =
        (32767.0 / 65535.0) * 10.0;

    const double ai_integration_tolerance = 1e-9;

    if (ai_test_status.ai[0] != 0 ||
        ai_test_status.ai[4] != 65535 ||
        ai_test_status.ai[5] != 32768 ||
        ai_test_status.ai[6] != 32767 ||
        ai_test_status.ai[7] != 0)
    {
        std::printf(
            "[FAIL] AI raw values were not parsed correctly\n");

        return 1;
    }

    if (std::fabs(ai_test_status.ai_voltage[0] - (-10.0)) > ai_integration_tolerance ||
        std::fabs(ai_test_status.ai_voltage[4] - 10.0) > ai_integration_tolerance ||
        std::fabs(ai_test_status.ai_voltage[5] - expected_ai05) > ai_integration_tolerance ||
        std::fabs(ai_test_status.ai_voltage[6] - expected_ai06) > ai_integration_tolerance ||
        std::fabs(ai_test_status.ai_voltage[7] - 0.0) > ai_integration_tolerance)
    {
        std::printf(
            "[FAIL] AI scaled voltage values were not parsed correctly\n");

        return 1;
    }

    if (ai_test_status.ai_voltage[1] != 0.0 ||
        ai_test_status.ai_voltage[2] != 0.0 ||
        ai_test_status.ai_voltage[3] != 0.0)
    {
        std::printf(
            "[FAIL] AI01~AI03 undefined scaling handling changed\n");

        return 1;
    }

    std::printf(
        "[PASS] AI raw values preserved correctly\n");

    std::printf(
        "[PASS] AI scaled voltage values parsed correctly\n");

    std::printf(
        "[PASS] AI01~AI03 remain without defined scaling\n");









    std::printf("========================================\n");
    std::printf(" ALL ASSEMBLY TESTS PASSED\n");
    std::printf("========================================\n");

    return 0;
}
