extern "C"{
  #include "ad7746.h"
  #include "no_os_delay.h"
  #include "no_os_i2c.h"
  #include "no_os_alloc.h"
  #include "iio_ad7746.h"
}
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

  // init_param.setup.vt.vten = true;
  // init_param.setup.vt.vtmd = AD7746_VTMD_INT_TEMP;
  // init_param.setup.vt.extref = false;
  // init_param.setup.vt.vtshort = false;
  // init_param.setup.vt.vtchop = true;

  init_param.setup.exc.clkctrl = false;
  init_param.setup.exc.excon = true;
  init_param.setup.exc.excb = AD7746_EXC_PIN_DISABLED;
  init_param.setup.exc.exca = AD7746_EXC_PIN_NORMAL;
  init_param.setup.exc.exclvl = AD7746_EXCLVL_1_DIV_8;

  init_param.setup.config.vtf = 0;
  init_param.setup.config.capf = 3;
  init_param.setup.config.md = AD7746_MODE_CONT;

  ret = ad7746_init(&adc, &init_param);
  if (ret != 0) {
    Serial.print("AD7746 init failed: ");
    Serial.println(ret);
    while (1);
  }

  uint8_t cap_setup = 0x81;  // CAPEN = 1, use CIN1
  uint8_t exc_setup = 0x8A;  // EXCA enabled, EXCLVL = VDD (10)
  // uint8_t config    = 0x01;  // Continuous conversion
  // uint8_t cap_dac   = 0x00;  // No offset (you can change this)

  ad7746_reg_write(adc, AD7746_REG_CAP_SETUP, &cap_setup, 1);
  ad7746_reg_write(adc, AD7746_REG_EXC_SETUP, &exc_setup, 1);
  // ad7746_reg_write(adc, AD7746_REG_CONFIGURATION, &config, 1);
  // ad7746_reg_write(adc, AD7746_REG_CAP_DAC_A, &cap_dac, 1);


  Serial.println("[INIT] AD7746 init done.");

  // Optional: set CAP DAC A
  ad7746_set_cap_dac_a(adc, true, 0x28);
  Serial.println("[INIT] CAP DAC A set.");
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
  // unsigned long micros_now = micros();
  // float time_sec = micros_now / 1e6;
  // Serial.print("T1:");
  // Serial.println(time_sec, 6);
  // uint32_t capData = 0;
  // uint32_t temperature = 0;
  // int32_t ret;

  // Serial.println("in the loop");

//   // code for debug
//   Serial.println("Register dump:");

//   for (uint8_t addr = 0x00; addr <= 0x0F; addr++) {
//     uint8_t val = 0;
//     ad7746_reg_read(adc, addr, &val, 1);
//     Serial.print("Reg 0x");
//     Serial.print(addr, HEX);
//     Serial.print(": 0x");
//     Serial.println(val, HEX);
//   }

// // debug end

  // uint8_t reg_val = 0;
  // ret = ad7746_reg_read(adc, AD7746_REG_CAPDACA, &reg_val, 1);
  // if (ret == 0) {
  //   Serial.print("[LOOP] CAP DAC A readback: 0x");
  //   Serial.println(reg_val, HEX);

  //   bool dac_enabled = reg_val & 0x80;
  //   uint8_t dac_code = reg_val & 0x7F;

  //   Serial.print("[LOOP] Enable bit: ");
  //   Serial.println(dac_enabled ? "ON" : "OFF");
  //   Serial.print("[LOOP] DAC code: 0x");
  //   Serial.println(dac_code, HEX);

  //   if (dac_enabled && dac_code == 0x20) {
  //     Serial.println("[LOOP] CAP DAC A set correctly.");
  //   } else {
  //     Serial.println("[LOOP] CAP DAC A setting did NOT persist!");
  //     while(1);
  //   }
  // } else {
  //   Serial.print("[LOOP] Failed to read CAP DAC A, error: ");
  //   Serial.println(ret);
  //   while(1);
  // }

  // // check the sampling rate
  // uint8_t cfg_reg;
  // int32_t ret;

  // // Read the configuration register
  // ret = ad7746_reg_read(adc, AD7746_REG_CFG, &cfg_reg, 1);
  // if (ret == 0) {
  //     // Extract the CAPF bits (bits 3 to 5)
  //     uint8_t capf_index = (cfg_reg >> 3) & 0x07;

  //     // Optional: print out the actual frequency
  //     const uint8_t cap_filter_rate_table[][2] = {
  //         {91, 12}, {84, 13}, {50, 21}, {26, 39},
  //         {16, 63}, {13, 78}, {11, 93}, {9, 111}
  //     };

  //     Serial.print(F("Current capf index: "));
  //     Serial.println(capf_index);
  //     Serial.print(F("→ Sampling rate: "));
  //     Serial.print(cap_filter_rate_table[capf_index][0]);
  //     Serial.println(F(" Hz"));
  // } else {
  //     Serial.println(F("Failed to read config register"));
  // }

  unsigned long micros_now = micros();
  float time_sec1 = micros_now / 1e6;
  // Serial.print("T_before:");
  // Serial.println(time_sec1, 6);
  
  ret = ad7746_get_cap_data(adc, &capData);

  // Print current time in seconds with 6 decimal places
  micros_now = micros();
  float time_sec2 = micros_now / 1e6;
  Serial.print("delta_T:");
  Serial.println(time_sec2-time_sec1, 6);
  
  if (ret != 0) {
    Serial.print("Error reading capacitance: ");
    Serial.println(ret);
    while (1);
  } else {
    float cap_pf = ((int32_t)(capData & 0xFFFFFF) - 0x800000) * 8.192f / 16777216.0f;
    Serial.print("Capacitance (pF):");
    Serial.println(cap_pf, 6);
  }

  

  // delay();
  

}