extern "C" {
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_util.h"
  #include "no_os_alloc.h"
  #include "iio_ad7746.h"
}

#include <Wire.h>

// Status register bits (AD7746_REG_STATUS = 0x0B)
#define AD7746_STATUS_CAP_READY  (1 << 0)  // Bit 0: Capacitance data ready
#define AD7746_STATUS_VT_READY   (1 << 1)  // Bit 1: Voltage/Temp data ready
#define AD7746_REG_STATUS 0x0B


// Maximum number of AD7746 devices
#define MAX_DEVICES 4

// Array to store device instances
ad7746_dev* devices[MAX_DEVICES];
int num_devices = 0;

// /////////////////////////////////////////////////
uint8_t addresses[] = {0x48}; // create an array to store device address

// Define a struct to hold offset settings for CIN1 and CIN2
struct CapOffsetSettings {
  float cin1_offset_pF;  // Use -1.0 for auto-calibration
  float cin2_offset_pF;  // Use -1.0 for auto-calibration
};

/***************************************************************************//**
 * @brief Read AD7746 status register
 * 
 * @param dev - Device descriptor pointer
 * @param[out] status - Pointer to store status value
 * @return int32_t - Return code (0 for success, negative for errors)
 *******************************************************************************/
int32_t ad7746_read_status(struct ad7746_dev *dev, uint8_t *status) {
  const uint8_t status_reg = AD7746_REG_STATUS;  // 0x0B
  int32_t ret;
  uint8_t reg_data;

  ret = ad7746_reg_read(dev, status_reg, &reg_data, 1);
  if(ret < 0) {
      return ret;
  }
  
  *status = reg_data;
  return 0;
}

// /***************************************************************************//**
//  * @brief Read current capacitance value for a channel and return in pF.
//  * 
//  * @param dev - AD7746 device descriptor.
//  * @param channel - Channel info (CIN1/CIN2).
//  * @return float - Current capacitance in pF (or NAN on error).
//  *******************************************************************************/
// float read_current_capacitance(struct ad7746_dev *dev, struct iio_ch_info channel) {
//   uint32_t raw_cap;
//   int32_t ret;

//   // Configure channel
//   ret = ad7746_set_cap(dev, dev->setup.cap); // Ensure channel is active
//   if (ret < 0) return NAN;

//   // Wait for conversion (adjust delay based on filter rate)
//   no_os_mdelay(50);

//   // Read raw capacitance
//   ret = ad7746_get_cap_data(dev, &raw_cap);
//   if (ret != 0) return NAN;

//   // Convert raw value to pF
//   int32_t signed_raw = (raw_cap & 0xFFFFFF);
//   if (signed_raw & 0x800000) {
//     signed_raw |= 0xFF000000; // Sign-extend 24-bit to 32-bit
//   }
//   return signed_raw * (8.192e-12f / 16777216.0f); // 8.192pF / 2^24
// }

/***************************************************************************//**
 * @brief Initialize a single AD7746 with optional auto-calibration.
 *******************************************************************************/
bool initialize_ad7746(uint8_t address, ad7746_cap cap_settings, ad7746_vt vt_settings, 
                       ad7746_config config_settings, CapOffsetSettings offset_settings) {

  no_os_i2c_init_param i2c_init = {0, address, NULL};
  
  ad7746_setup setup = {
    cap_settings,
    vt_settings,
    {}, // ad7746_exc (empty initialization)
    config_settings,
  };

  ad7746_init_param init_param = {i2c_init, ID_AD7746, setup};

  if (ad7746_init(&devices[num_devices],  &init_param) != 0) {
    Serial.print("AD7746 init failed for address 0x");
    Serial.println(address, HEX);
    return false;
  }

  struct ad7746_dev *dev = devices[num_devices];

  // Apply settings
  ad7746_set_cap(dev, cap_settings);
  ad7746_set_config(dev, config_settings);

  // Apply offsets
  const long SCALE_FACTOR = 2048000L;
  long offset_rawcode_ch1 = offset_settings.cin1_offset_pF/SCALE_FACTOR;
  long offset_rawcode_ch2 = offset_settings.cin2_offset_pF/SCALE_FACTOR;

  ad7746_set_cap_offset(dev,offset_rawcode_ch1);
  
  Serial.print("AD7746 at 0x");
  Serial.print(address, HEX);
  Serial.println(" initialized.");
  num_devices++;
  return true;
}

void setup() {
  Wire.setClock(100000);  // Add before Wire.begin()
  Serial.begin(115200); // set baud rate to 115200, required by CN0552
  delay(500);
  Serial.println("AD7746 Initialization...\n");
  Serial.println("Ongoing\n");

  Wire.begin();
  Serial.println("Scanning I2C bus...");
  for(uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if(error == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
 
  // //////////////////////////////////////////////////////////////////////////////////////////////
  // 250401: commented the scan address code, working only with on device with address 0x48 //////
  // //////////////////////////////////////////////////////////////////////////////////////////////

  // // 250329: scanning connected devices using I2C,  
  // Serial.println("Start scanning connected devices using I2C...");
  // uint8_t addresses[127]; // create an array to store device address
  // byte scan_error, arduino_i2c_address; // init parameters for scanning device
  // int device_count = 0;
  
  // for (uint8_t arduino_i2c_address = 1; arduino_i2c_address < 127; arduino_i2c_address++)
  // {
  //   Wire.beginTransmission(arduino_i2c_address); // try to communicate with current address see if there's a device connected
  //   scan_error = Wire.endTransmission(); //checks for acknowledgment. If no error (error == 0), the address is printed.
    
  //   if (scan_error == 0) { // if there is a device
  //     addresses[device_count] = arduino_i2c_address; // store the found address to array
  //     Serial.print("Device found at 0x");
  //     Serial.println(arduino_i2c_address);
  //     device_count++;
  //   }
  // }
  // // change the format of address
  // char formatted_addresses[128][5]; // Stores addresses as "0xXX" strings
  // int formatted_device_count = 0;
  // formatted_device_count = device_count; 
  // for (int i = 0; i < device_count; i++) {
  //   snprintf(formatted_addresses[i], 5, "0x%02X", addresses[i]);
  // }

  // // Print formatted addresses
  // Serial.println("\nFormatted addresses stored:");
  // for (int i = 0; i < formatted_device_count; i++) {
  //   Serial.println(formatted_addresses[i]);
  // }

  // if (device_count == 0) {
  //   Serial.println("No devices found.");
  // }

  // Serial.println("Scan complete. The number of device is:");
  // Serial.println(device_count);
  
  // initialize device settings
  struct DeviceSettings { // the struct that stores setting for all device
    ad7746_cap cap;
    ad7746_vt vt;
    ad7746_config config;
    CapOffsetSettings offset;
  };

  // Example: Use auto-calibration for CIN1 (-1.0), user-specified 3.5pF for CIN2
  DeviceSettings device_settings[MAX_DEVICES] = {
    { // Device 1
      {true, // Enable capacitance measurement
        false, // false = CIN1, true = CIN2
        false, // false = single-ended, true = differential
        false}, // No chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // VT (Voltage/Temp) settings (Disable for pure capacitance usage)
      // BELOW: Filter and mode settings (Controls the sampling rate and measurement mode)
      {0,  // Not used (since VT is disabled)
        2, // Filter index 2 = 50Hz sample rate
        AD7746_MODE_CONT}, // Continuous measurement mode
      {4, 4} // Auto-calibrate CIN1, user specifies 3.5pF for CIN2
    },
    { // Device 2
      {true, // Enable capacitance measurement
        false, // false = CIN1, true = CIN2
        false, // false = single-ended, true = differential
        false}, // No chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // VT (Voltage/Temp) settings (Disable for pure capacitance usage)
      // BELOW: Filter and mode settings (Controls the sampling rate and measurement mode)
      {0,  // Not used (since VT is disabled)
        2, // Filter index 2 = 50Hz sample rate
        AD7746_MODE_CONT}, // Continuous measurement mode
      {4, 4} // Auto-calibrate CIN1, user specifies 3.5pF for CIN2
    },
    { // Device 3
      {true, // Enable capacitance measurement
        false, // false = CIN1, true = CIN2
        false, // false = single-ended, true = differential
        false}, // No chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // VT (Voltage/Temp) settings (Disable for pure capacitance usage)
      // BELOW: Filter and mode settings (Controls the sampling rate and measurement mode)
      {0,  // Not used (since VT is disabled)
        2, // Filter index 2 = 50Hz sample rate
        AD7746_MODE_CONT}, // Continuous measurement mode
      {4, 4} // Auto-calibrate CIN1, user specifies 3.5pF for CIN2
    },
    { // Device 4
      {true, // Enable capacitance measurement
        false, // false = CIN1, true = CIN2
        false, // false = single-ended, true = differential
        false}, // No chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // VT (Voltage/Temp) settings (Disable for pure capacitance usage)
      // BELOW: Filter and mode settings (Controls the sampling rate and measurement mode)
      {0,  // Not used (since VT is disabled)
        2, // Filter index 2 = 50Hz sample rate
        AD7746_MODE_CONT}, // Continuous measurement mode
      {4, 4} // Auto-calibrate CIN1, user specifies 3.5pF for CIN2
    }
    // ... similar for other devices
  };

  // for (int i = 0; i < sizeof(addresses)/sizeof(addresses[0]); i++) { // go over all device according to the size of found address
  //   if (initialize_ad7746(addresses[i], device_settings[i].cap, 
  //                         device_settings[i].vt, device_settings[i].config,
  //                         device_settings[i].offset)) {
  //     if (num_devices >= MAX_DEVICES) break;
  //   }
  // }


  // //////////////////////////////////////////////////////////////////////////////////////////////
  // init only default address -x48, should be commented if the address scaning code is uncommented
  // //////////////////////////////////////////////////////////////////////////////////////////////
  num_devices = 1; 
  uint8_t addresses[] = {0x48}; // create an array to store device address
  initialize_ad7746(addresses[0], device_settings[0].cap, 
                            device_settings[0].vt, device_settings[0].config,
                            device_settings[0].offset);

  if (num_devices == 0) {
    Serial.println("No AD7746 devices found!");
    while(1);
  }

  Serial.println("AD7746 Initialization complete.");
}


void loop() {
  uint32_t cap_raw_cin1 = ad7746_get_cap_data(devices[0], &cap_raw_cin1);
  Serial.print('Readout is:');
  Serial.println(cap_raw_cin1);
  delay(500);
  // for (int i = 0; i < num_devices; i++) {
  //   uint32_t cap_raw_cin1 = 0;
  //   uint32_t cap_raw_cin2 = 0;
  //   int32_t ret;

  //   // ===== Read CIN1 =====
  //   // Configure for CIN1
  //   ad7746_cap cin1_config = devices[i]->setup.cap;
  //   cin1_config.cin2 = false;
  //   ad7746_set_cap(devices[i], cin1_config);

  //   // Get CIN1 data
  //   ret = ad7746_get_cap_data(devices[i], &cap_raw_cin1);
    
  //   // ===== Read CIN2 =====
  //   if(ret == 0) {
  //     // Configure for CIN2
  //     ad7746_cap cin2_config = devices[i]->setup.cap;
  //     cin2_config.cin2 = true;
  //     ad7746_set_cap(devices[i], cin2_config);

  //     // Get CIN2 data
  //     ret = ad7746_get_cap_data(devices[i], &cap_raw_cin2);
  //   }

  //   // ===== Process Results =====
  //   if(ret == 0) {
  //     // Convert to signed values
  //     int32_t signed_cin1 = (cap_raw_cin1 & 0x800000) ? 
  //                          (cap_raw_cin1 | 0xFF000000) : cap_raw_cin1;
  //     int32_t signed_cin2 = (cap_raw_cin2 & 0x800000) ? 
  //                          (cap_raw_cin2 | 0xFF000000) : cap_raw_cin2;

  //     // Convert to picofarads (8.192pF full scale)
  //     float cin1_pF = signed_cin1 * (8.192f / 16777216.0f);
  //     float cin2_pF = signed_cin2 * (8.192f / 16777216.0f);

  //     Serial.print("CIN1: ");
  //     Serial.print(cin1_pF, 4);
  //     Serial.print(" pF, CIN2: ");
  //     Serial.print(cin2_pF, 4);
  //     Serial.println(" pF");
  //   } else {
  //     Serial.print("Error reading data: ");
  //     Serial.println(ret);
  //   }
    
  //   delay(100);  // Adjust based on conversion rate
  // }
}