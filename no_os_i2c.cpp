#include <Wire.h>
#include <stdlib.h>
#include "no_os_i2c.h"

int32_t no_os_i2c_init(struct no_os_i2c_desc **desc,
                       const struct no_os_i2c_init_param *param)
{
    if (!desc || !param)
        return -1;

    Wire.begin(); // Start I2C

    *desc = (struct no_os_i2c_desc *)malloc(sizeof(**desc));
    if (!(*desc))
        return -1;

    (*desc)->device_id = param->device_id;
    (*desc)->max_speed_hz = param->max_speed_hz;
    (*desc)->slave_address = param->slave_address;
    (*desc)->platform_ops = param->platform_ops;
    (*desc)->extra = param->extra;

    return 0;
}

int32_t no_os_i2c_remove(struct no_os_i2c_desc *desc)
{
    if (desc)
        free(desc);
    return 0;
}

int32_t no_os_i2c_write(struct no_os_i2c_desc *desc,
                        uint8_t *data,
                        uint8_t bytes_number,
                        uint8_t stop_bit)
{
    if (!desc || !data)
        return -1;

    Wire.beginTransmission(desc->slave_address);
    Wire.write(data, bytes_number);
    return Wire.endTransmission(stop_bit);
}

int32_t no_os_i2c_read(struct no_os_i2c_desc *desc,
                       uint8_t *data,
                       uint8_t bytes_number,
                       uint8_t stop_bit)
{
    if (!desc || !data)
        return -1;

    Wire.requestFrom((int)desc->slave_address, (int)bytes_number, (bool)stop_bit);

    for (uint8_t i = 0; i < bytes_number && Wire.available(); i++) {
        data[i] = Wire.read();
    }

    return 0;
}

int32_t no_os_i2cbus_init(const struct no_os_i2c_init_param *param)
{
    // Stubbed for Arduino
    return 0;
}

void no_os_i2cbus_remove(uint32_t bus_number)
{
    // Stubbed for Arduino
}
