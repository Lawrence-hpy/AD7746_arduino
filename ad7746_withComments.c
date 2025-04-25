#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Declare C-linkage debug functions provided by debug_print.cpp
void debug_print(const char* msg);
void debug_print_hex(uint32_t val);
void debug_print_time(float input_time);

// Replace macros
#define DEBUG_PRINT(msg)        debug_print(msg)
#define DEBUG_PRINT_HEX(val)    debug_print_hex(val)
#define DEBUG_PRINT_TIME(input_time)    debug_print_time(input_time)




#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// #include <Arduino.h>
#define DEBUG_PRINT(msg)        Serial.println(F(msg))
#define DEBUG_PRINT_HEX(val)    do { Serial.print(F("0x")); Serial.println(val, HEX); } while (0)
#define DEBUG_PRINT_TIME(input_time)    Serial.println(F(input_time))
#else
#define DEBUG_PRINT(msg)
#define DEBUG_PRINT_HEX(val)
#endif




extern void debug_log(const char*, uint8_t);
// #include <math.h>   // For round()

// /**
//  * @brief Convert a pF value to the sensor's long integer offset format(Peiyu generated with help of Deepseek).
//  * 
//  * @param pF Capacitance value in pF (0 to 21 pF).
//  * @param[out] error Error flag (0 = success, -1 = invalid input).
//  * @return long Scaled integer for the sensor.
//  */
// long pF_to_sensor_offset(float pF, int *error) {
//     const float MAX_PF = 21.0f;
//     const long SCALE_FACTOR = 2048000L; // 21pF → 43,008,000

//     if (pF < 0 || pF > MAX_PF) {
//         if (error) *error = -1; // Signal invalid input
//         return 0;
//     }

//     if (error) *error = 0; // Signal success
//     return (long)round(pF * SCALE_FACTOR);
// }

/***************************************************************************//**
 *   @file   ad7746.c
 *   @brief  Implementation of AD7746 Driver.
 *   @author Dragos Bogdan (dragos.bogdan@analog.com)
 *   @author Darius Berghe (darius.berghe@analog.com)
********************************************************************************
 * Copyright 2021(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 #include <stdlib.h>
 #include <string.h>
 #include "no_os_error.h"
 #include "no_os_delay.h"
 #include "no_os_alloc.h"
 #include "ad7746.h"
 
 /***************************************************************************//**
  * @brief Initialize the AD7746 device structure.
  *
  * Performs memory allocation of the device structure and initializes the device.
  *
  * @param device     - Pointer to location of device structure to write.
  * @param init_param - Pointer to the configuration of the driver.
  * @return ret - return code.
  *         Example: -ENOMEM - Memory allocation error.
  *                  -EIO - I2C communication error.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_init(struct ad7746_dev **device,
             struct ad7746_init_param *init_param)
 {
     int32_t ret;
     struct ad7746_dev *dev;
 
     // Allocate memory for the device structure
     dev = (struct ad7746_dev *)no_os_calloc(1, sizeof(struct ad7746_dev));
     if (!dev)
         return -ENOMEM; // Return error if memory allocation fails
 
     // Initialize I2C communication
     ret = no_os_i2c_init(&dev->i2c_dev, &init_param->i2c_init);
     if (ret < 0)
         goto error_1; // Jump to error handling if I2C initialization fails
 
     // Set the device ID
     dev->id = init_param->id;
 
     // Reset the AD7746 device
     ret = ad7746_reset(dev);
     if (ret < 0)
         goto error_2; // Jump to error handling if reset fails
 
     // Wait for the device to stabilize after reset (200 microseconds)
     no_os_udelay(200);
 
     // Configure capacitive setup
     ret = ad7746_set_cap(dev, init_param->setup.cap);
     if (ret < 0)
         goto error_2;
 
     // Configure voltage/temperature setup
     ret = ad7746_set_vt(dev, init_param->setup.vt);
     if (ret < 0)
         goto error_2;
 
     // Configure excitation setup
     ret = ad7746_set_exc(dev, init_param->setup.exc);
     if (ret < 0)
         goto error_2;
 
     // Configure the device's main configuration register
     ret = ad7746_set_config(dev, init_param->setup.config);
     if (ret < 0)
         goto error_2;
 
     // Return the initialized device structure
     *device = dev;
     return 0;
 
 error_2:
     // Clean up I2C if an error occurs
     no_os_i2c_remove(dev->i2c_dev);
 error_1:
     // Free allocated memory if an error occurs
     no_os_free(dev);
     return ret;
 }
 
 /***************************************************************************//**
  * @brief Writes data into AD7746 registers, starting from the selected
  *        register address pointer.
  *
  * @param dev - Device descriptor pointer.
  * @param reg - The selected register address.
  * @param data - Pointer to data to transmit.
  * @param bytes_number - Number of bytes to send (typically 1, other values when using the address auto-increment).
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_reg_write(struct ad7746_dev *dev, uint8_t reg,
              uint8_t* data, uint16_t bytes_number)
 {
     // Validate input parameters
     if (!data || bytes_number > AD7746_NUM_REGISTERS || reg >= AD7746_NUM_REGISTERS)
         return -EINVAL; // Return error if inputs are invalid
 
     // Prepare the buffer: register address followed by data
     dev->buf[0] = reg;
     memcpy(&dev->buf[1], data, bytes_number);
 
     // Write the data to the device via I2C
     return no_os_i2c_write(dev->i2c_dev, dev->buf, bytes_number + 1, 1);
 }
 
 /***************************************************************************//**
  * @brief Reads data from AD7746 registers, starting from the selected
  *        register address pointer.
  *
  * @param dev - Device descriptor pointer.
  * @param reg - The selected register address pointer.
  * @param data - Pointer to a buffer that will store the received data.
  * @param bytes_number - Number of bytes to read (typically 1, other values when using the address auto-increment).
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_reg_read(struct ad7746_dev *dev,
             uint8_t reg,
             uint8_t* data,
             uint16_t bytes_number)
 {
     int32_t ret;
 
     // Validate input parameters
     if (!data || bytes_number > AD7746_NUM_REGISTERS || reg >= AD7746_NUM_REGISTERS)
         return -EINVAL; // Return error if inputs are invalid
 
     // Write the register address to the device
     ret = no_os_i2c_write(dev->i2c_dev, &reg, 1, 0);
     if (ret < 0)
         return ret; // Return error if I2C write fails
 
     // Read the data from the device
     return no_os_i2c_read(dev->i2c_dev, data, bytes_number, 1);
 }
 
 /***************************************************************************//**
  * @brief Resets the AD7746.
  *
  * @param dev - Device descriptor pointer.
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_reset(struct ad7746_dev *dev)
 {
     uint8_t cmd = AD7746_RESET_CMD;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Send the reset command to the device
     return no_os_i2c_write(dev->i2c_dev, &cmd, 1, 1);
 }
 
 /***************************************************************************//**
  * @brief Deinitialize the AD7746 driver and free all allocated resources.
  *
  * @param dev - Device descriptor pointer.
  * @return 0
 *******************************************************************************/
 int32_t ad7746_remove(struct ad7746_dev *dev)
 {
     // Validate input parameters
     if (!dev)
         return 0; // Return success if device pointer is already NULL
 
     // Clean up I2C communication
     no_os_i2c_remove(dev->i2c_dev);
     dev->i2c_dev = NULL;
 
     // Free the device structure
     no_os_free(dev);
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Configure the capacitive setup register.
  *
  * @param dev - Device descriptor pointer.
  * @param cap - Capacitive setup settings.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_cap(struct ad7746_dev *dev, struct ad7746_cap cap)
 {
     int32_t ret;
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the capacitive setup register value
     reg = no_os_field_prep(AD7746_CAPSETUP_CAPEN_MSK, cap.capen) |
           no_os_field_prep(AD7746_CAPSETUP_CIN2_MSK, cap.cin2) |
           no_os_field_prep(AD7746_CAPSETUP_CAPDIFF_MSK, cap.capdiff) |
           no_os_field_prep(AD7746_CAPSETUP_CAPCHOP_MSK, cap.capchop);
 
     // Write the capacitive setup register
     ret = ad7746_reg_write(dev, AD7746_REG_CAP_SETUP, &reg, 1);
     if (ret < 0)
         return ret; // Return error if write fails
 
     // Update the device's capacitive setup structure
     dev->setup.cap = cap;
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Configure the voltage/temperature setup register.
  *
  * @param dev - Device descriptor pointer.
  * @param vt - Voltage/Temperature setup settings.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_vt(struct ad7746_dev *dev, struct ad7746_vt vt)
 {
     int32_t ret;
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the voltage/temperature setup register value
     reg = no_os_field_prep(AD7746_VTSETUP_VTEN_MSK, vt.vten) |
           no_os_field_prep(AD7746_VTSETUP_VTMD_MSK, vt.vtmd) |
           no_os_field_prep(AD7746_VTSETUP_EXTREF_MSK, vt.extref) |
           no_os_field_prep(AD7746_VTSETUP_VTSHORT_MSK, vt.vtshort) |
           no_os_field_prep(AD7746_VTSETUP_VTCHOP_MSK, vt.vtchop);
 
     // Write the voltage/temperature setup register
     ret = ad7746_reg_write(dev, AD7746_REG_VT_SETUP, &reg, 1);
     if (ret < 0)
         return ret; // Return error if write fails
 
     // Update the device's voltage/temperature setup structure
     dev->setup.vt = vt;
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Configure the excitation setup register.
  *
  * @param dev - Device descriptor pointer.
  * @param exc - Excitation setup settings.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_exc(struct ad7746_dev *dev, struct ad7746_exc exc)
 {
     int32_t ret;
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the excitation setup register value
     reg = no_os_field_prep(AD7746_EXCSETUP_CLKCTRL_MSK, exc.clkctrl) |
           no_os_field_prep(AD7746_EXCSETUP_EXCON_MSK, exc.excon) |
           no_os_field_prep(AD7746_EXCSETUP_EXCB_MSK, exc.excb) |
           no_os_field_prep(AD7746_EXCSETUP_EXCA_MSK, exc.exca) |
           no_os_field_prep(AD7746_EXCSETUP_EXCLVL_MSK, exc.exclvl);
 
     // Write the excitation setup register
     ret = ad7746_reg_write(dev, AD7746_REG_EXC_SETUP, &reg, 1);
     if (ret < 0)
         return ret; // Return error if write fails
 
     // Update the device's excitation setup structure
     dev->setup.exc = exc;
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Set the configuration register.
  *
  * @param dev - Device descriptor pointer.
  * @param config - Configuration register settings.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_config(struct ad7746_dev *dev, struct ad7746_config config)
 {
     int32_t ret;
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the configuration register value
     reg = no_os_field_prep(AD7746_CONF_VTF_MSK, config.vtf) |
           no_os_field_prep(AD7746_CONF_CAPF_MSK, config.capf) |
           no_os_field_prep(AD7746_CONF_MD_MSK, config.md);
 
     // Write the configuration register
     ret = ad7746_reg_write(dev, AD7746_REG_CFG, &reg, 1);
     if (ret < 0)
         return ret; // Return error if write fails
 
     // Update the device's configuration structure
     dev->setup.config = config;
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Set the DAC code and enable state for EXCA.
  *
  * @param dev - Device descriptor pointer.
  * @param enable - DAC_A Enable.
  * @param code - DAC_A register code.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_cap_dac_a(struct ad7746_dev *dev, bool enable, uint8_t code)
 {
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the DAC A register value, Official
    //  reg = no_os_field_prep(AD7746_CAPDAC_DACEN_MSK, enable) |
    //        no_os_field_prep(AD7746_CAPDAC_DACP_MSK, code);

    // temporary fix
    reg = (enable ? 0x80 : 0x00) | (code & 0x7F);


    // temporaty debug print
    debug_print("[DEBUG] Writing CAPDAC A:");
    debug_print_hex(reg);
 
     // Write the DAC A register
     return ad7746_reg_write(dev, AD7746_REG_CAPDACA, &reg, 1);
 }
 
 /***************************************************************************//**
  * @brief Set the DAC code and enable state for EXCB.
  *
  * @param dev - Device descriptor pointer.
  * @param enable - DAC_B Enable.
  * @param code - DAC_B register code.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_cap_dac_b(struct ad7746_dev *dev, bool enable, uint8_t code)
 {
     uint8_t reg;
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the DAC B register value
     reg = no_os_field_prep(AD7746_CAPDAC_DACEN_MSK, enable) |
           no_os_field_prep(AD7746_CAPDAC_DACP_MSK, code);
 
     // Write the DAC B register
     return ad7746_reg_write(dev, AD7746_REG_CAPDACB, &reg, 1);
 }
 
 /***************************************************************************//**
  * @brief Helper function to write a 2-byte value to a register.
  *
  * @param dev - Device descriptor pointer.
  * @param reg - The selected register address.
  * @param val - The 2-byte value to write.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 static inline int32_t _ad7746_write_2byte(struct ad7746_dev *dev, uint8_t reg,
         uint16_t val)
 {
     uint8_t buf[2];
 
     // Validate input parameters
     if (!dev)
         return -EINVAL; // Return error if device pointer is invalid
 
     // Prepare the 2-byte buffer
     buf[0] = (uint8_t)(val >> 8); // High byte
     buf[1] = (uint8_t)val;        // Low byte
 
     // Write the 2-byte buffer to the device
     return ad7746_reg_write(dev, reg, buf, 2);
 }
 
 /***************************************************************************//**
  * @brief Set the capacitive offset.
  *
  * @param dev - Device descriptor pointer.
  * @param offset - Offset as raw code.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_cap_offset(struct ad7746_dev *dev, uint16_t offset)
 {
     // Write the capacitive offset value to the device
     return _ad7746_write_2byte(dev, AD7746_REG_CAP_OFFH, offset);
 }
 
 /***************************************************************************//**
  * @brief Set the capacitive gain.
  *
  * @param dev - Device descriptor pointer.
  * @param gain - Gain as raw code.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_cap_gain(struct ad7746_dev *dev, uint16_t gain)
 {
     // Write the capacitive gain value to the device
     return _ad7746_write_2byte(dev, AD7746_REG_CAP_GAINH, gain);
 }
 
 /***************************************************************************//**
  * @brief Set the voltage gain.
  *
  * @param dev - Device descriptor pointer.
  * @param gain - Gain as raw code.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_set_volt_gain(struct ad7746_dev *dev, uint16_t gain)
 {
     // Write the voltage gain value to the device
     return _ad7746_write_2byte(dev, AD7746_REG_VOLT_GAINH, gain);
 }
 
 /***************************************************************************//**
  * @brief Waits until a conversion on a voltage/temperature channel has been
  *        finished and returns the output data.
  *
  * @param dev - Device descriptor pointer.
  * @param vt_data - The content of the VT Data register.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  -EIO - I2C Communication error.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_get_vt_data(struct ad7746_dev *dev, uint32_t *vt_data)
 {
     int32_t ret;
 
     // Validate input parameters
     if (!dev || !vt_data)
         return -EINVAL; // Return error if device pointer or data pointer is invalid
 
     // Clear the buffer
     memset(dev->buf, 0, 3);
 
     // Wait until the voltage/temperature data is ready
     dev->buf[0] = AD7746_STATUS_RDYVT_MSK;
     while (dev->buf[0] & AD7746_STATUS_RDYVT_MSK) {
         ret = ad7746_reg_read(dev, AD7746_REG_STATUS, dev->buf, 1);
         if (ret < 0)
             return ret; // Return error if read fails
     }
 
     // Read the voltage/temperature data
     ret = ad7746_reg_read(dev, AD7746_REG_VT_DATA_HIGH, dev->buf, 3);
     if (ret < 0)
         return ret; // Return error if read fails
 
     // Combine the 3-byte data into a 32-bit value
     *vt_data = ((uint32_t)dev->buf[0] << 16) |
            ((uint32_t)dev->buf[1] << 8) |
            dev->buf[0];
 
     // Reset the mode to idle if in single conversion mode
     if (dev->setup.config.md == AD7746_MODE_SINGLE)
         dev->setup.config.md = AD7746_MODE_IDLE;
 
     return 0;
 }
 
 /***************************************************************************//**
  * @brief Waits until a conversion on the capacitive channel has been
  *        finished and returns the output data.
  *
  * @param dev - Device descriptor pointer.
  * @param cap_data - The content of the Capacitive Data register.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  -EIO - I2C Communication error.
  *                  0 - No errors encountered.
 *******************************************************************************/
//  int32_t ad7746_get_cap_data(struct ad7746_dev *dev, uint32_t *cap_data)
//  {
//      int32_t ret;

//     //  unsigned long t = micros();
//     //  float t0 = t / 1e6;
 
//      // Validate input parameters
//      if (!dev || !cap_data)
//          return -EINVAL; // Return error if device pointer or data pointer is invalid

//     //  t = micros();
//     //  float t1 = t / 1e6;
 
//      // Clear the buffer
//      memset(dev->buf, 0, 3);
//     //  t = micros();
//     //  float t2 = t / 1e6;
 
//      // Wait until the capacitive data is ready, Peiyu Commented trying to reduce time
//      dev->buf[0] = AD7746_STATUS_RDYCAP_MSK;
//      while (dev->buf[0] & AD7746_STATUS_RDYCAP_MSK) {
//          ret = ad7746_reg_read(dev, AD7746_REG_STATUS,	dev->buf, 1);
//          if (ret < 0)
//              return ret; // Return error if read fails
//      }
//     //  t = micros();
//     //  float t3 = t / 1e6;

//      // Read the capacitive data
//      ret = ad7746_reg_read(dev, AD7746_REG_CAP_DATA_HIGH, dev->buf, 3);
//      if (ret < 0)
//          return ret; // Return error if read fails
//     //  t = micros();
//     //  float t4 = t / 1e6;

//      // Combine the 3-byte data into a 32-bit value
//      *cap_data = ((uint32_t)dev->buf[0] << 16) |
//              ((uint32_t)dev->buf[1] << 8) |
//              dev->buf[0];
//     //  t = micros();
//     //  float t5 = t / 1e6;
 
//      // Reset the mode to idle if in single conversion mode
//      if (dev->setup.config.md == AD7746_MODE_SINGLE)
//          dev->setup.config.md = AD7746_MODE_IDLE;

//     //  t = micros();
//     //  float t6 = t / 1e6;

//     //  Print out time
//     // DEBUG_PRINT("T1:");
//     // DEBUG_PRINT_TIME(t1);
//     // DEBUG_PRINT("T2:");
//     // DEBUG_PRINT(t2);
//     // DEBUG_PRINT("T3:");
//     // DEBUG_PRINT(t3);
//     // DEBUG_PRINT("T4:");
//     // DEBUG_PRINT(t4);
//     // DEBUG_PRINT("T5:");
//     // DEBUG_PRINT(t5);
    
//     // char buf[64];
//     // char time_str[16];
//     // dtostrf(t0, 6, 6, time_str); snprintf(buf, sizeof(buf), "T0: %s", time_str); debug_print(buf);
//     // dtostrf(t1, 6, 6, time_str); snprintf(buf, sizeof(buf), "T1: %s", time_str); debug_print(buf);
//     // dtostrf(t2, 6, 6, time_str); snprintf(buf, sizeof(buf), "T2: %s", time_str); debug_print(buf);
//     // dtostrf(t3, 6, 6, time_str); snprintf(buf, sizeof(buf), "T3: %s", time_str); debug_print(buf);
//     // dtostrf(t4, 6, 6, time_str); snprintf(buf, sizeof(buf), "T4: %s", time_str); debug_print(buf);
//     // dtostrf(t5, 6, 6, time_str); snprintf(buf, sizeof(buf), "T5: %s", time_str); debug_print(buf);
//     // dtostrf(t6, 6, 6, time_str); snprintf(buf, sizeof(buf), "T6: %s", time_str); debug_print(buf);
    
 
//      return 0;
//  }
// int32_t ad7746_get_cap_data(struct ad7746_dev *dev, uint32_t *cap_data)
// {
//     int32_t ret;

//     // Capture absolute time before any operation
//     unsigned long t0 = micros();

//     // Validate input parameters
//     if (!dev || !cap_data)
//         return -EINVAL;

//     unsigned long t1 = micros();

//     // Clear the buffer
//     memset(dev->buf, 0, 3);
//     unsigned long t2 = micros();

//     // Wait until the capacitive data is ready 
//     dev->buf[0] = AD7746_STATUS_RDYCAP_MSK;
//     while (dev->buf[0] & AD7746_STATUS_RDYCAP_MSK) {
//         ret = ad7746_reg_read(dev, AD7746_REG_STATUS, dev->buf, 1);
//         if (ret < 0)
//             return ret;
//     }
//     unsigned long t3 = micros();

//     // Read the capacitive data
//     ret = ad7746_reg_read(dev, AD7746_REG_CAP_DATA_HIGH, dev->buf, 3);
//     if (ret < 0)
//         return ret;
//     unsigned long t4 = micros();

//     // Combine the 3-byte data into a 32-bit value
//     *cap_data = ((uint32_t)dev->buf[0] << 16) |
//                 ((uint32_t)dev->buf[1] << 8) |
//                 dev->buf[2];
//     unsigned long t5 = micros();

//     // Reset the mode to idle if in single conversion mode
//     if (dev->setup.config.md == AD7746_MODE_SINGLE)
//         dev->setup.config.md = AD7746_MODE_IDLE;

//     unsigned long t6 = micros();

//     // Debug timing output
//     char buf[64];
//     snprintf(buf, sizeof(buf), "T0: %lu us", t0); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T1: %lu us", t1); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T2: %lu us", t2); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T3: %lu us", t3); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T4: %lu us", t4); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T5: %lu us", t5); debug_print(buf);
//     snprintf(buf, sizeof(buf), "T6: %lu us", t6); debug_print(buf);

//     // Optional: show deltas for analysis
//     snprintf(buf, sizeof(buf), "Δt_status_polling: %lu us", t3 - t2); debug_print(buf);
//     snprintf(buf, sizeof(buf), "Δt_data_read: %lu us", t4 - t3); debug_print(buf);
//     snprintf(buf, sizeof(buf), "Δt_combine: %lu us", t5 - t4); debug_print(buf);
//     snprintf(buf, sizeof(buf), "Δt_total: %lu us", t6 - t0); debug_print(buf);

//     return 0;
// }


/***************************************************************************//**
  * @brief Waits until a conversion on the capacitive channel has been
  *        finished and returns the output data. Peiyu's code for debugging
  *
  * @param dev - Device descriptor pointer.
  * @param cap_data - The content of the Capacitive Data register.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  -EIO - I2C Communication error.
  *                  0 - No errors encountered.
 *******************************************************************************/
int32_t ad7746_get_cap_data(struct ad7746_dev *dev, uint32_t *cap_data)
{
    int32_t ret;

    // debug_print("[CHECKPOINT 1] Entering get_cap_data");

    if (!dev || !cap_data) {
        debug_print("[ERROR] Null pointer in get_cap_data");
        return -EINVAL;
    }

    memset(dev->buf, 0, 3);

    // debug_print("[CHECKPOINT 2] Starting STATUS polling");

    dev->buf[0] = AD7746_STATUS_RDYCAP_MSK;
    uint32_t timeout = 0;
    while (dev->buf[0] & AD7746_STATUS_RDYCAP_MSK) {
        ret = ad7746_reg_read(dev, AD7746_REG_STATUS, dev->buf, 1);
        if (ret < 0) {
            debug_print("[ERROR] Failed to read STATUS register");
            return ret;
        }

        timeout++;
        if (timeout > 100000) {
            debug_print("[ERROR] Timeout waiting for RDYCAP to clear");
            return -EIO;
        }
    }

    // debug_print("[CHECKPOINT 3] RDYCAP cleared. Reading data...");

    ret = ad7746_reg_read(dev, AD7746_REG_CAP_DATA_HIGH, dev->buf, 3);
    if (ret < 0) {
        debug_print("[ERROR] Failed to read CAP_DATA");
        return ret;
    }

    *cap_data = ((uint32_t)dev->buf[0] << 16) |
                ((uint32_t)dev->buf[1] << 8) |
                dev->buf[2];

    // debug_print("[CHECKPOINT 4] Data read successfully");

    return 0;
}




 
 /***************************************************************************//**
  * @brief Perform offset/gain calibration
  *
  * @param dev - Device descriptor pointer.
  * @param md - AD7746 calibration mode specifier.
  *
  * @return return code.
  *         Example: -EINVAL - Wrong input values.
  *                  -EIO - I2C Communication error.
  *                  0 - No errors encountered.
 *******************************************************************************/
 int32_t ad7746_calibrate(struct ad7746_dev *dev, enum ad7746_md md)
 {
     struct ad7746_config *c = &dev->setup.config;
     int32_t ret, timeout = 10;
     uint8_t reg;
 
     // Validate calibration mode
     if (md != AD7746_MODE_OFFSET_CALIB &&
         md != AD7746_MODE_GAIN_CALIB)
         return -EINVAL; // Return error if mode is invalid
 
     // Set the calibration mode
     c->md = md;
     ret = ad7746_set_config(dev, *c);
     if (ret < 0)
         return ret; // Return error if configuration fails
 
     // Wait for calibration to complete
     do {
         // Wait for a short time to avoid excessive I2C reads
         no_os_mdelay(20);
 
         // Read the configuration register to check calibration status
         ret = ad7746_reg_read(dev, AD7746_REG_CFG, &reg, 1);
         if (ret < 0)
             return ret; // Return error if read fails
 
         // Update the mode from the register
         c->md = reg & AD7746_CONF_MD_MSK;
     } while ((c->md != AD7746_MODE_IDLE) && timeout--);
 
     // Return timeout error if calibration does not complete
     if (!timeout)
         ret = -ETIMEDOUT;
 
     return ret;
 }