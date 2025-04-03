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
  init_param.setup.config.capf = 0;
  init_param.setup.config.md = AD7746_MODE_CONT;

  int32_t ret = ad7746_init(&adc, &init_param);
  if (ret != 0) {
    Serial.print("AD7746 init failed: ");
    Serial.println(ret);
    while (1);
  }

  uint8_t cap_setup = 0x81;  // CAPEN = 1, use CIN1
  uint8_t exc_setup = 0x8A;  // EXCA enabled, EXCLVL = VDD (10)
  uint8_t config    = 0x01;  // Continuous conversion
  uint8_t cap_dac   = 0x00;  // No offset (you can change this)

  ad7746_reg_write(adc, AD7746_REG_CAP_SETUP, &cap_setup, 1);
  ad7746_reg_write(adc, AD7746_REG_EXC_SETUP, &exc_setup, 1);
  // ad7746_reg_write(adc, AD7746_REG_CONFIGURATION, &config, 1);
  // ad7746_reg_write(adc, AD7746_REG_CAP_DAC_A, &cap_dac, 1);  

  // --- CAP DAC A ---
  ad7746_set_cap_dac_a(adc, true, 0x42); 
  Serial.println("[INIT] CAP DAC A set");
  // ////////////// CAPDAC lookup table //////////////////////////
  //   DAC Code (Hex)	Decimal	Cap Shift (pF)
  // 0x00	0	0.000 pF
  // 0x10	16	2.646 pF
  // 0x1E	30	4.961 pF
  // 0x20	32	5.291 pF
  // 0x2A	42	6.944 pF
  // 0x2F	47	7.771 pF
  // 0x35	53	8.765 pF
  // 0x3F	63	10.417 pF
  // 0x45	69	11.412 pF
  // 0x4B	75	12.407 pF
  // 0x50	80	13.228 pF
  // 0x55	85	14.055 pF
  // 0x5A	90	14.882 pF
  // 0x60	96	15.874 pF
  // 0x66	102	16.867 pF
  // 0x6C	108	17.860 pF
  // 0x72	114	18.854 pF
  // 0x78	120	19.847 pF
  // 0x7F	127	21.000 pF
  // ///////////////////////////////////////////////////////////////////////////////////////

  // Target offset in pF
  float desired_offset_pf = 0.0f;

  // Convert to 24-bit signed integer
  int32_t offset_24bit = (int32_t)(desired_offset_pf / 0.000000488f);  // 4pF / 488aF

  // Sanity clamp: make sure it’s in ±2^23 range
  if (offset_24bit > 0x7FFFFF) offset_24bit = 0x7FFFFF;
  if (offset_24bit < -0x800000) offset_24bit = -0x800000;

  // Extract top 16 bits (signed!)
  int16_t offset_16bit = (int16_t)(offset_24bit >> 8);

  // Write
  ret = ad7746_set_cap_offset(adc, offset_16bit);

  // Debug print
  if (ret < 0) {
    Serial.print("[ERROR] Failed to set CAP offset: ");
    Serial.println(ret);
  } else {
    float actual_offset_pf = (offset_16bit * 256.0f) * 0.000000488f;
    Serial.print("[INIT] CAP offset set to ");
    Serial.print(actual_offset_pf, 6);
    Serial.print(" pF (reg = 0x");
    Serial.print((uint16_t)offset_16bit, HEX);
    Serial.println(")");
  }


  Serial.println("[INIT] AD7746 init done.");
}


void loop() {
  uint32_t capData = 0;
  uint32_t temperature = 0;
  int32_t ret;

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

  ret = ad7746_get_cap_data(adc, &capData);
  if (ret != 0) {
    Serial.print("Error reading capacitance: ");
    Serial.println(ret);
  } else {
    float cap_pf = ((int32_t)(capData & 0xFFFFFF) - 0x800000) * 8.192f / 16777216.0f;
    Serial.print("Capacitance (pF): ");
    Serial.println(cap_pf, 6);
  }

  Serial.print("Raw CAP code: 0x");
  Serial.println(capData & 0xFFFFFF, HEX);


  // debug begin --- Read back CAP offset from hardware ---
  uint8_t offset_buf[2] = {0};
  ad7746_reg_read(adc, AD7746_REG_CAP_OFFH, offset_buf, 2);

  int16_t read_offset_code = ((int16_t)offset_buf[0] << 8) | offset_buf[1];
  float read_offset_pf = ((int32_t)read_offset_code << 8) * 0.000000488f;

  Serial.print("Offset reg: 0x");
  Serial.print(read_offset_code, HEX);
  Serial.print(" → ");
  Serial.print(read_offset_pf, 6);
  Serial.println(" pF");
  // debug end


  delay(20000);
}
