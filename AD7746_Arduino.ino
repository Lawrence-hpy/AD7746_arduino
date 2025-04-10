extern "C"{
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_alloc.h"
  #include "iio_ad7746.h"
}

#include <Wire.h>
// extern "C" {
//   #include "platform_support/i2c_platform.h" // Replace with your platform-specific I2C implementation
//   #include "platform_support/uart_platform.h"
// }

ad7746_dev *adc;
uint32_t capData = 0;
uint32_t temperature = 0;
int32_t ret;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("[INIT] Starting AD7746 setup...");

  // I2C setup
  no_os_i2c_init_param i2c_init;
  i2c_init.max_speed_hz = 100000;
  i2c_init.slave_address = AD7746_ADDRESS;
  i2c_init.platform_ops = NULL;
  i2c_init.extra = NULL;

  // AD7746 setup
  ad7746_init_param init_param;
  init_param.i2c_init = i2c_init;
  init_param.id = ID_AD7746;

  init_param.setup.cap.capen = true;
  init_param.setup.cap.cin2 = false;
  init_param.setup.cap.capdiff = false;
  init_param.setup.cap.capchop = true;

  init_param.setup.exc.clkctrl = false;
  init_param.setup.exc.excon = true;
  init_param.setup.exc.excb = AD7746_EXC_PIN_DISABLED;
  init_param.setup.exc.exca = AD7746_EXC_PIN_NORMAL;
  init_param.setup.exc.exclvl = AD7746_EXCLVL_1_DIV_8;

  init_param.setup.config.vtf = 0;
  init_param.setup.config.capf = 0;
  init_param.setup.config.md = AD7746_MODE_CONT;

  ret = ad7746_init(&adc, &init_param);
  if (ret != 0) {
    Serial.print("AD7746 init failed: ");
    Serial.println(ret);
    while (1);
  }

  uint8_t cap_setup = 0x81;  // CAPEN = 1, use CIN1
  uint8_t exc_setup = 0x8A;  // EXCA enabled, EXCLVL = VDD (10)

  ad7746_reg_write(adc, AD7746_REG_CAP_SETUP, &cap_setup, 1);
  ad7746_reg_write(adc, AD7746_REG_EXC_SETUP, &exc_setup, 1);

  Serial.println("[INIT] AD7746 init done.");

  // Optional: set CAP DAC A
  ad7746_set_cap_dac_a(adc, true, 0x60);
  Serial.println("[INIT] CAP DAC A set.");

  // ✅ Force CONFIG register to Continuous mode and 91Hz filter
  Wire.beginTransmission(0x48);
  Wire.write(0x0A); // CONFIG register address
  Wire.write(0x00); // md=00, capf=000, vtf=0
  uint8_t status = Wire.endTransmission(); // ✅ THIS MUST BE INCLUDED

  if (status == 0) {
    Serial.println("[INIT] CONFIG set: MD=CONT, CAPF=0 (91Hz)");
  } else {
    Serial.print("[ERROR] CONFIG write failed. I2C error code: ");
    Serial.println(status);
    while (1);
  }

  delay(100);
}


  
  // CapDAC Code (dec)	CapDAC Code (hex)	cap_dac value (with enable)	Offset Capacitance (pF)
  // 0	0x00	0x80	0.00
  // 8	0x08	0x88	1.32
  // 16	0x10	0x90	2.65
  // 24	0x18	0x98	3.97
  // 32	0x20	0xA0	5.29
  // 40	0x28	0xA8	6.61
  // 48	0x30	0xB0	7.94
  // 56	0x38	0xB8	9.26
  // 64	0x40	0xC0	10.58
  // 72	0x48	0xC8	11.91
  // 80	0x50	0xD0	13.23
  // 88	0x58	0xD8	14.55
  // 96	0x60	0xE0	15.87
  // 103	0x67	0xE7	17.03 (example for 17pF)
  // 112	0x70	0xF0	18.51
  // 120	0x78	0xF8	19.83
  // 127	0x7F	0xFF	21.00 (max)



  void loop() {

    // // Force continuous conversion mode + 91 Hz every loop
    // Wire.beginTransmission(0x48);
    // Wire.write(0x0A); // CONFIG register
    // Wire.write(0x00); // vtf=0, capf=000, md=00
    // Wire.endTransmission();

    // 1. Start timestamp before everything
    unsigned long micros_now = micros();
    float time_sec = micros_now / 1e6;
    Serial.print("T_before:");
    Serial.println(time_sec, 6);
  
    // === Inlined get_cap_data() logic ===
  
    uint8_t status;
    uint8_t cap_buf[3];
    uint32_t capData;
  
    // --- Start RDYCAP polling timer ---
    unsigned long t_poll_start = micros();
    uint8_t poll_attempts = 0;
    const uint8_t max_attempts = 300;

    do {
        poll_attempts++;
        if (poll_attempts >= max_attempts) {
            Serial.println("[ERROR] RDYCAP polling timed out!");
            break;
        }

        Wire.beginTransmission(0x48);
        Wire.write(0x00); // STATUS register
        uint8_t err = Wire.endTransmission(false);

        if (err != 0) {
            Serial.print("[WARN] I2C error in polling: ");
            Serial.println(err);
            delay(1);
            continue;
        }

        Wire.requestFrom(0x48, 1);
        if (Wire.available()) {
            status = Wire.read();
        } else {
            Serial.println("[WARN] No data from STATUS reg");
            delay(1);
            continue;
        }

        delayMicroseconds(300);
    } while (status & 0x01);

    if (status & 0x01) {
      Serial.println("[WARN] RDYCAP bit still high after timeout");
    } else {
      Serial.println("[INFO] RDYCAP cleared successfully");
    }
    
  
    // --- End RDYCAP polling timer ---
    unsigned long t_poll_end = micros();
    Serial.print("RDYCAP polling time: ");
    Serial.print(t_poll_end - t_poll_start);
    Serial.println(" us");
  
    // 3. Read 3 bytes from CAP DATA registers (0x01..0x03)
    Wire.beginTransmission(0x48);
    Wire.write(0x01);                     // CAP_DATA_HIGH
    Wire.endTransmission(false);         // Repeated start
  
    Wire.requestFrom(0x48, 3);
    cap_buf[0] = Wire.read();
    cap_buf[1] = Wire.read();
    cap_buf[2] = Wire.read();
  
    // 4. Combine into 24-bit value
    capData = ((uint32_t)cap_buf[0] << 16) |
              ((uint32_t)cap_buf[1] << 8) |
              cap_buf[2];
  
    // === End of inlined logic ===
  
    // 5. Timestamp after all logic
    micros_now = micros();
    time_sec = micros_now / 1e6;
    Serial.print("T_after:");
    Serial.println(time_sec, 6);

    Serial.print("Raw CAP data: 0x");
    Serial.println(capData, HEX);

  
    // 6. Convert to capacitance and print
    float cap_pf = ((int32_t)(capData & 0xFFFFFF) - 0x800000) * 8.192f / 16777216.0f;
    Serial.print("Capacitance (pF):");
    Serial.println(cap_pf, 6);
  
    // Optional delay to prevent flooding serial monitor
    // delay(100);

    uint8_t config_reg;
    Wire.beginTransmission(0x48);
    Wire.write(0x0A); // CONFIG register address
    Wire.endTransmission(false);
    Wire.requestFrom(0x48, 1);
    config_reg = Wire.read();

    Serial.print("CONFIG reg: 0x");
    Serial.println(config_reg, HEX);

    // Extract CAPF bits (bits 5:3)
    uint8_t capf_index = (config_reg >> 3) & 0x07;
    const uint8_t cap_filter_rate_table[][2] = {
      {91, 12}, {84, 13}, {50, 21}, {26, 39},
      {16, 63}, {13, 78}, {11, 93}, {9, 111}
    };

    Serial.print("CAPF Index: ");
    Serial.println(capf_index);
    Serial.print("→ Actual Sampling Rate: ");
    Serial.print(cap_filter_rate_table[capf_index][0]);
    Serial.println(" Hz");

  }
  