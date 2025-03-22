extern "C" {
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_util.h"
  #include "no_os_alloc.h"
}

// Maximum number of AD7746 devices
#define MAX_DEVICES 4

// Array to store device instances
ad7746_dev* devices[MAX_DEVICES];
int num_devices = 0;

// Function to initialize a single AD7746 device
bool initialize_ad7746(uint8_t address, ad7746_cap cap_settings, ad7746_vt vt_settings, ad7746_config config_settings) {
  no_os_i2c_init_param i2c_init;
  i2c_init.device_id = 0;                      // Not used for Arduino
  i2c_init.slave_address = address;            // AD7746 I2C address
  i2c_init.platform_ops = NULL;                // Platform ops (not needed for basic Arduino)

  ad7746_setup setup;
  setup.cap = cap_settings;
  setup.vt = vt_settings;
  setup.config = config_settings;
  memset(&setup.exc, 0, sizeof(setup.exc));    // Clear excitation settings (not used here)

  ad7746_init_param init_param;
  init_param.i2c_init = i2c_init;
  init_param.id = ID_AD7746;                   // Use the defined ID
  init_param.setup = setup;

  if (ad7746_init(&devices[num_devices], &init_param) != 0) {
    Serial.print("AD7746 init failed for address 0x");
    Serial.println(address, HEX);
    return false;
  }

  // Apply settings
  ad7746_set_cap(devices[num_devices], cap_settings);
  ad7746_set_config(devices[num_devices], config_settings);

  // Optional: Set CAPDAC (offset capacitor)
  ad7746_set_cap_dac_a(devices[num_devices], true, 10);  // Enable DAC A with code 10 (~1.65pF)
  ad7746_set_cap_dac_b(devices[num_devices], false, 0);  // Disable DAC B

  Serial.print("AD7746 at address 0x");
  Serial.print(address, HEX);
  Serial.println(" initialized.");
  num_devices++;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AD7746 Initialization...");

  // List of possible AD7746 I2C addresses
  uint8_t addresses[] = {0x48, 0x49, 0x4A, 0x4B}; // Adjust based on your hardware

  // Define a struct to hold settings for each device
  struct DeviceSettings {
    ad7746_cap cap;
    ad7746_vt vt;
    ad7746_config config;
  };

  // Define settings for each device
  DeviceSettings device_settings[MAX_DEVICES] = {
    { // Device 0
      {true, false, false, false}, // cap_settings: Enable CIN1, single-ended, no chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // vt_settings: Disable VT
      {0, 2, AD7746_MODE_CONT} // config_settings: Filter index 2, continuous mode
    },
    { // Device 1
      {true, true, false, false}, // cap_settings: Enable CIN2, single-ended, no chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // vt_settings: Disable VT
      {0, 3, AD7746_MODE_CONT} // config_settings: Filter index 3, continuous mode
    },
    { // Device 2
      {true, false, false, true}, // cap_settings: Enable CIN1, single-ended, with chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // vt_settings: Disable VT
      {0, 1, AD7746_MODE_CONT} // config_settings: Filter index 1, continuous mode
    },
    { // Device 3
      {true, true, false, true}, // cap_settings: Enable CIN2, single-ended, with chopping
      {false, AD7746_VTMD_INT_TEMP, false, false, false}, // vt_settings: Disable VT
      {0, 4, AD7746_MODE_CONT} // config_settings: Filter index 4, continuous mode
    }
  };

  // Initialize all connected AD7746 devices with individual settings
  for (int i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
    if (initialize_ad7746(addresses[i], device_settings[i].cap, device_settings[i].vt, device_settings[i].config)) {
      if (num_devices >= MAX_DEVICES) break; // Stop if max devices reached
    }
  }

  if (num_devices == 0) {
    Serial.println("No AD7746 devices found!");
    while (1); // Halt if no devices are found
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

  delay(100); // Read every 100 ms
}