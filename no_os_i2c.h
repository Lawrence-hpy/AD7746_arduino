#ifndef NO_OS_I2C_H
#define NO_OS_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C initialization structure
 */
struct no_os_i2c_init_param {
    uint32_t device_id;
    uint32_t max_speed_hz;
    uint8_t slave_address;
    const void* platform_ops;
    void* extra;
};

/**
 * @brief I2C descriptor
 */
struct no_os_i2c_desc {
    uint32_t device_id;
    uint32_t max_speed_hz;
    uint8_t slave_address;
    const void* platform_ops;
    void* extra;
};

// Initialize the I2C peripheral
int32_t no_os_i2c_init(struct no_os_i2c_desc **desc,
                       const struct no_os_i2c_init_param *param);

// Remove/frees the I2C descriptor
int32_t no_os_i2c_remove(struct no_os_i2c_desc *desc);

// Write bytes to I2C slave
int32_t no_os_i2c_write(struct no_os_i2c_desc *desc,
                        uint8_t *data,
                        uint8_t bytes_number,
                        uint8_t stop_bit);

// Read bytes from I2C slave
int32_t no_os_i2c_read(struct no_os_i2c_desc *desc,
                       uint8_t *data,
                       uint8_t bytes_number,
                       uint8_t stop_bit);

// Not used for Arduino but included for compatibility
int32_t no_os_i2cbus_init(const struct no_os_i2c_init_param *param);
void no_os_i2cbus_remove(uint32_t bus_number);

#ifdef __cplusplus
}
#endif

#endif // NO_OS_I2C_H
