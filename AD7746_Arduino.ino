// Arduino sketch to configure and read capacitance from AD7746 using Analog Devices no-OS driver
// Dependencies: ad7746 driver, no_os_delay, no_os_i2c, no_os_util, no_os_alloc must be available in the Arduino libraries folder

extern "C" {
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_util.h"
  #include "no_os_alloc.h"
}

// Global device instance pointer
ad7746_dev* dev;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("AD7746 Initialization...");

  // --------------------------------------------------------
  // 1. I2C configuration for Arduino Uno
  // (AD7746 uses I2C communication)
  // --------------------------------------------------------
  no_os_i2c_init_param i2c_init;
  i2c_init.device_id = 0;                      // Not used for Arduino
  i2c_init.slave_address = AD7746_ADDRESS;     // Default AD7746 I2C address
  i2c_init.platform_ops = NULL;                // Platform ops (not needed for basic Arduino)

  // --------------------------------------------------------
  // 2. Capacitance measurement channel settings
  // (Choose CIN1 or CIN2, single-ended or differential)
  // --------------------------------------------------------
  ad7746_cap cap_settings;
  cap_settings.capen = true;       // Enable capacitance measurement
  cap_settings.cin2 = false;       // false = CIN1, true = CIN2
  cap_settings.capdiff = false;    // false = single-ended, true = differential
  cap_settings.capchop = false;    // No chopping

  // --------------------------------------------------------
  // 3. VT (Voltage/Temp) settings
  // (Disable for pure capacitance usage)
  // --------------------------------------------------------
  ad7746_vt vt_settings;
  vt_settings.vten = false;
  vt_settings.vtmd = AD7746_VTMD_INT_TEMP;
  vt_settings.extref = false;
  vt_settings.vtshort = false;
  vt_settings.vtchop = false;

  // --------------------------------------------------------
  // 4. Filter and mode settings
  // (Controls the sampling rate and measurement mode)
  // --------------------------------------------------------
  ad7746_config config_settings;
  config_settings.vtf = 0;                 // Not used (since VT is disabled)
  config_settings.capf = 2;                // Filter index 2 = 50Hz sample rate
  config_settings.md = AD7746_MODE_CONT;   // Continuous measurement mode

  // --------------------------------------------------------
  // 5. Combine into full setup struct
  // --------------------------------------------------------
  ad7746_setup setup;
  setup.cap = cap_settings;
  setup.vt = vt_settings;
  setup.config = config_settings;
  memset(&setup.exc, 0, sizeof(setup.exc));  // Clear excitation settings (not used here)

  // --------------------------------------------------------
  // 6. Final AD7746 initialization parameters
  // --------------------------------------------------------
  ad7746_init_param init_param;
  init_param.i2c_init = i2c_init;
  init_param.id = ID_AD7746;
  init_param.setup = setup;

  // --------------------------------------------------------
  // 7. Initialize the AD7746 device
  // --------------------------------------------------------
  if (ad7746_init(&dev, &init_param) != 0) {
    Serial.println("AD7746 init failed!");
    while (1); // Halt if init fails
  }

  // --------------------------------------------------------
  // 8. Apply cap and config settings (safety)
  // --------------------------------------------------------
  ad7746_set_cap(dev, cap_settings);
  ad7746_set_config(dev, config_settings);

  // --------------------------------------------------------
  // 9. Set CAPDAC (offset capacitor) — optional
  // (This shifts the input range to prevent saturation)
  // --------------------------------------------------------
  ad7746_set_cap_dac_a(dev, true, 10);  // Enable DAC A with code 10 (~1.65pF)
  ad7746_set_cap_dac_b(dev, false, 0);  // Disable DAC B

  Serial.println("AD7746 Initialized and Ready.");
}


void loop() {
  uint32_t cap_raw = 0;
  int32_t result = ad7746_get_cap_data(dev, &cap_raw);

  if (result == 0) {
    // Raw value is 24-bit signed (2's complement)
    int32_t signed_cap = (cap_raw & 0xFFFFFF);
    if (signed_cap & 0x800000) {
      signed_cap |= 0xFF000000; // Sign extend to 32 bits
    }

    // Convert to farads: Full scale = 8.192pF => LSB = 8.192 / 2^24 pF
    // Convert to farads directly:
    float capacitance = signed_cap * (8.192e-12f / 16777216.0f);

    // Get current time (in seconds + microseconds)
    unsigned long micros_now = micros();
    unsigned long secs = micros_now / 1000000;
    unsigned long usecs = micros_now % 1000000;

    Serial.print("[Time: ");
    Serial.print(secs);
    Serial.print(".");
    Serial.print(usecs);
    Serial.print(" s] Capacitance = ");
    Serial.print(capacitance, 15); // Print with high precision
    Serial.println(" F");
  } else {
    Serial.println("Failed to read capacitance.");
  }

  delay(100); // Read every 100 ms
}
