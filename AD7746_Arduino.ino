extern "C" {
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_util.h"
  #include "no_os_alloc.h"
  #include "iio_ad7746.h"
}

#include <Wire.h>

// Maximum number of AD7746 devices
#define MAX_DEVICES 4

// Array to store device instances
ad7746_dev* devices[MAX_DEVICES];
int num_devices = 0;

// Define a struct to hold offset settings for CIN1 and CIN2
struct CapOffsetSettings {
  float cin1_offset_pF;  // Use -1.0 for auto-calibration
  float cin2_offset_pF;  // Use -1.0 for auto-calibration
};

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
  // no_os_i2c_init_param i2c_init = {
  //   .device_id = 0,
  //   .slave_address = address,
  //   .platform_ops = NULL
  // };

  // ad7746_setup setup = {
  //   .cap = cap_settings,
  //   .vt = vt_settings,
  //   .config = config_settings,
  //   .exc = {}
  // };

  // ad7746_init_param init_param = {
  //   .i2c_init = i2c_init,
  //   .id = ID_AD7746,
  //   .setup = setup
  // };

  no_os_i2c_init_param i2c_init = {0, address, NULL};
  
  ad7746_setup setup = {
    cap_settings,
    vt_settings,
    config_settings,
    {}
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

  // ===== Auto-calibrate or apply user offsets =====
  // struct iio_ch_info cin1_channel = {.ch_num = 0, .differential = false};
  // struct iio_ch_info cin2_channel = {.ch_num = 1, .differential = false};

  // // Auto-calibrate CIN1 if user offset is -1.0
  // if (offset_settings.cin1_offset_pF < 0) {
  //   float current_pF = read_current_capacitance(dev, cin1_channel);
  //   if (!isnan(current_pF)) {
  //     offset_settings.cin1_offset_pF = current_pF;
  //   } else {
  //     Serial.print("Auto-calibration failed for CIN1 (0x");
  //     Serial.print(address, HEX);
  //     Serial.println(")");
  //     return false;
  //   }
  // }

  // // Auto-calibrate CIN2 if user offset is -1.0
  // if (offset_settings.cin2_offset_pF < 0) {
  //   float current_pF = read_current_capacitance(dev, cin2_channel);
  //   if (!isnan(current_pF)) {
  //     offset_settings.cin2_offset_pF = current_pF;
  //   } else {
  //     Serial.print("Auto-calibration failed for CIN2 (0x");
  //     Serial.print(address, HEX);
  //     Serial.println(")");
  //     return false;
  //   }
  // }

  // Apply offsets
  const long SCALE_FACTOR = 2048000L;
  long offset_rawcode_ch1 = offset_settings.cin1_offset_pF/SCALE_FACTOR;
  long offset_rawcode_ch2 = offset_settings.cin2_offset_pF/SCALE_FACTOR;

  ad7746_set_cap_offset(dev,offset_rawcode_ch1);

  // auto apply_offset = [&](float pF, struct iio_ch_info channel) {
  //   if (pF < 0) return; // Skip invalid values
  //   char buf[16];
  //   long raw_offset = round(pF * SCALE_FACTOR);
  //   int len = snprintf(buf, sizeof(buf), "%ld", raw_offset);
  //   ad7746_iio_write_offset(dev, buf, len, &channel, 0);
  // };

  // apply_offset(offset_settings.cin1_offset_pF, cin1_channel);
  // apply_offset(offset_settings.cin2_offset_pF, cin2_channel);

  
  Serial.print("AD7746 at 0x");
  Serial.print(address, HEX);
  Serial.println(" initialized.");
  num_devices++;
  return true;
}

void setup() {
  Serial.begin(115200); // set baud rate to 115200, required by CN0552
  delay(1000);
  Serial.println("AD7746 Initialization...");
 
  // 250329: scanning connected devices using I2C,  
  Serial.println("Start scanning connected devices using I2C...");
  uint8_t addresses[127]; // create an array to store device address
  byte scan_error, arduino_i2c_address; // init parameters for scanning device
  int device_count = 0;
  
  for (uint8_t arduino_i2c_address = 1; arduino_i2c_address < 127; arduino_i2c_address++)
  {
    Wire.beginTransmission(arduino_i2c_address); // try to communicate with current address see if there's a device connected
    scan_error = Wire.endTransmission(); //checks for acknowledgment. If no error (error == 0), the address is printed.
    
    if (scan_error == 0) { // if there is a device
      addresses[device_count] = arduino_i2c_address; // store the found address to array
      Serial.print("Device found at 0x");
      Serial.println(arduino_i2c_address);
      device_count++;
    }
  }
  // change the format of address
  char formatted_addresses[128][5]; // Stores addresses as "0xXX" strings
  int formatted_device_count = 0;
  formatted_device_count = device_count; 
  for (int i = 0; i < device_count; i++) {
    snprintf(formatted_addresses[i], 5, "0x%02X", addresses[i]);
  }

  // Print formatted addresses
  Serial.println("\nFormatted addresses stored:");
  for (int i = 0; i < formatted_device_count; i++) {
    Serial.println(formatted_addresses[i]);
  }



  if (device_count == 0) {
    Serial.println("No devices found.");
  }

  Serial.println("Scan complete. The number of device is:");
  Serial.println(device_count);
  
  // initialize device settings

  // uint8_t addresses[] = {0x48, 0x49, 0x4A, 0x4B};

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

  for (int i = 0; i < sizeof(addresses)/sizeof(addresses[0]); i++) {
    if (initialize_ad7746(addresses[i], device_settings[i].cap, 
                          device_settings[i].vt, device_settings[i].config,
                          device_settings[i].offset)) {
      if (num_devices >= MAX_DEVICES) break;
    }
  }

  if (num_devices == 0) {
    Serial.println("No AD7746 devices found!");
    while(1);
  }

  Serial.println("AD7746 Initialization complete.");
}
void loop() {
  for (int i = 0; i < num_devices; i++) {
    uint32_t cap_raw_cin1 = 0;
    uint32_t cap_raw_cin2 = 0;
    int32_t result_cin1 = ad7746_get_cap_data(devices[i], &cap_raw_cin1);
    int32_t result_cin2 = ad7746_get_cap_data(devices[i], &cap_raw_cin2);

    if (result_cin1 == 0 && result_cin2 == 0) {
      // Raw value is 24-bit signed (2's complement)
      int32_t signed_cap_cin1 = (cap_raw_cin1 & 0xFFFFFF);
      if (signed_cap_cin1 & 0x800000) {
        signed_cap_cin1 |= 0xFF000000; // Sign extend to 32 bits
      }

      int32_t signed_cap_cin2 = (cap_raw_cin2 & 0xFFFFFF);
      if (signed_cap_cin2 & 0x800000) {
        signed_cap_cin2 |= 0xFF000000; // Sign extend to 32 bits
      }

      // Convert to farads: Full scale = 8.192pF => LSB = 8.192 / 2^24 pF
      float capacitance_cin1 = signed_cap_cin1 * (8.192e-12f / 16777216.0f);
      float capacitance_cin2 = signed_cap_cin2 * (8.192e-12f / 16777216.0f);

      // Get current time (in milliseconds)
      unsigned long millis_now = millis();
      unsigned long secs = millis_now / 1000;
      unsigned long msecs = millis_now % 1000;

      Serial.print("[Device ");
      Serial.print(i);
      Serial.print(" | Time: ");
      Serial.print(secs);
      Serial.print(".");
      Serial.print(msecs);
      Serial.print(" s] Capacitance (CIN1) = ");
      Serial.print(capacitance_cin1, 15); // Print with high precision
      Serial.print(" F, Capacitance (CIN2) = ");
      Serial.print(capacitance_cin2, 15); // Print with high precision
      Serial.println(" F");
    } else {
      Serial.print("Failed to read capacitance from device ");
      Serial.println(i);
    }
  }

  delay(50); // Read every 100 ms
}