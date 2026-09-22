#ifndef STATUS_DATA_H
#define STATUS_DATA_H

#include <cstdint>

struct StatusData
{
    uint32_t controller_status;

    bool connect;
    bool voltage_on;
    bool is_moving;
    bool is_fa;
    bool homing_end;
    bool error;

    uint16_t ai[8];
    double ai_voltage[8];

    double x;
    double y;
    double z;
    double rx;
    double ry;
    double rz;
};

#endif