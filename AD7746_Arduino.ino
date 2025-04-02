extern "C" {
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_util.h"
  #include "no_os_alloc.h"
  #include "iio_ad7746.h"
}

// Define a struct to hold offset settings for CIN1 and CIN2
struct CapOffsetSettings {
  float cin1_offset_pF;  // Use -1.0 for auto-calibration
  float cin2_offset_pF;  // Use -1.0 for auto-calibration
};

extern "C" void debug_log(const char* label, uint8_t val) {
  Serial.print(label);
  Serial.print(" = 0x");
  Serial.println(val, HEX);
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

/***************************************************************************//**
 * @brief Initialize a single AD7746 with optional auto-calibration.
 *******************************************************************************/
bool initialize_ad7746(uint8_t address, ad7746_cap cap_settings, ad7746_vt vt_settings, 
                       ad7746_config config_settings, ad7746_exc exc_settings, CapOffsetSettings offset_settings) {

  no_os_i2c_init_param i2c_init = {0, address, NULL};
  
  ad7746_setup setup = {
    cap_settings,
    vt_settings,
    exc_settings, // ad7746_exc (empty initialization)
    config_settings,
  };

  setup.config.md = AD7746_MODE_CONT;
  


  ad7746_init_param init_param = {i2c_init, ID_AD7746, setup};

  // === Excitation Setup ===
  setup.exc.excon = true;                            // Enable excitation
  setup.exc.clkctrl = false;                         // Use internal clock
  setup.exc.exca = AD7746_EXC_PIN_NORMAL;            // Enable EXCA output
  setup.exc.excb = AD7746_EXC_PIN_DISABLED;          // Disable EXCB
  setup.exc.exclvl = AD7746_EXCLVL_2_DIV_8;          // Set excitation voltage level


  if (ad7746_init(&devices[num_devices],  &init_param) != 0) {
    Serial.print("AD7746 init failed for address 0x");
    Serial.println(address, HEX);
    return false;
  }

  // MANUALLY SET THE I2C ADDRESS
  devices[num_devices]->i2c_dev->slave_address = address;

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

  
  // initialize device settings
  struct DeviceSettings { // the struct that stores setting for all device
    ad7746_cap cap;
    ad7746_vt vt;
    ad7746_config config;
    ad7746_exc exc;  // ADD THIS LIN
    CapOffsetSettings offset;
  };

  // Example: Use auto-calibration for CIN1 (-1.0), user-specified 3.5pF for CIN2
  DeviceSettings device_settings[MAX_DEVICES] = {
    { // Device 1
    // Capacitance settings
    {true, false, false, false},
    // VT settings
    {false, AD7746_VTMD_INT_TEMP, false, false, false},
    // Configuration
    {0, 2, AD7746_MODE_CONT},
    // Excitation settings (ADJUSTED TO MATCH ad7746_exc FIELDS)
    {
      .clkctrl = false,    // Enable internal clock?
      .excon = true,      // Enable excitation output
      .excb = AD7746_EXC_PIN_DISABLED, // EXCB pin disabled
      .exca = AD7746_EXC_PIN_NORMAL,     // Use EXCA pin
      .exclvl = AD7746_EXCLVL_4_DIV_8      // Excitation level (4V)
    },
    // Offsets
    {4, 4}
  },
  { // Device 1
    // Capacitance settings
    {true, false, false, false},
    // VT settings
    {false, AD7746_VTMD_INT_TEMP, false, false, false},
    // Configuration
    {0, 2, AD7746_MODE_CONT},
    // Excitation settings (ADJUSTED TO MATCH ad7746_exc FIELDS)
    {
      .clkctrl = true,    // Enable internal clock?
      .excon = true,      // Enable excitation output
      .excb = AD7746_EXC_PIN_DISABLED, // EXCB pin disabled
      .exca = AD7746_EXC_PIN_NORMAL,        // Use EXCA pin
      .exclvl = AD7746_EXCLVL_4_DIV_8       // Excitation level (4V)
    },
    // Offsets
    {4, 4}
  },
  { // Device 1
    // Capacitance settings
    {true, false, false, false},
    // VT settings
    {false, AD7746_VTMD_INT_TEMP, false, false, false},
    // Configuration
    {0, 2, AD7746_MODE_CONT},
    // Excitation settings (ADJUSTED TO MATCH ad7746_exc FIELDS)
    {
      .clkctrl = true,    // Enable internal clock?
      .excon = true,      // Enable excitation output
      .excb = AD7746_EXC_PIN_DISABLED, // EXCB pin disabled
      .exca = AD7746_EXC_PIN_NORMAL,        // Use EXCA pin
      .exclvl = AD7746_EXCLVL_4_DIV_8       // Excitation level (4V)
    },
    // Offsets
    {4, 4}
  },
  { // Device 1
    // Capacitance settings
    {true, false, false, false},
    // VT settings
    {false, AD7746_VTMD_INT_TEMP, false, false, false},
    // Configuration
    {0, 2, AD7746_MODE_CONT},
    // Excitation settings (ADJUSTED TO MATCH ad7746_exc FIELDS)
    {
      .clkctrl = true,    // Enable internal clock?
      .excon = true,      // Enable excitation output
      .excb = AD7746_EXC_PIN_DISABLED, // EXCB pin disabled
      .exca = AD7746_EXC_PIN_NORMAL,        // Use EXCA pin
      .exclvl = AD7746_EXCLVL_4_DIV_8       // Excitation level (4V)
    },
    // Offsets
    {4, 4}
  }
  };


  // //////////////////////////////////////////////////////////////////////////////////////////////
  // init only default address -x48, should be commented if the address scaning code is uncommented
  // //////////////////////////////////////////////////////////////////////////////////////////////
  num_devices = 0; 
  uint8_t addresses[] = {0x48}; // create an array to store device address
  initialize_ad7746(
    addresses[0], 
    device_settings[0].cap, 
    device_settings[0].vt, 
    device_settings[0].config,
    device_settings[0].exc,  // Pass excitation settings
    device_settings[0].offset
  );

  if (num_devices == 0) {
    Serial.println("No AD7746 devices found!");
    while(1);
  }

  Serial.println("AD7746 Initialization complete.");
}


void loop() {
  if (num_devices == 0 || !devices[0]) { // Check device validity
    Serial.println("No valid devices!");
    return;
  }

  uint32_t cap_raw_cin1 = 0;
  int32_t ret;

  // Debug: Print I2C address from the device descriptor
  Serial.print("Device I2C address: 0x");
  Serial.println(devices[0]->i2c_dev->slave_address, HEX);

  // code to debug, not necessary
  uint8_t status = 0;
  ret = ad7746_read_status(devices[0], &status);
  Serial.print("STATUS before get_cap_data = 0x");
  Serial.println(status, HEX);


  ret = ad7746_get_cap_data(devices[0], &cap_raw_cin1);

  if (ret == 0) {
    Serial.print("Readout is: ");
    Serial.println(cap_raw_cin1);
  } else {
    Serial.print("Error reading data: ");
    Serial.println(ret);
  }
  
  delay(500);
}