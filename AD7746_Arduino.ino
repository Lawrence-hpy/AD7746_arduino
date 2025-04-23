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


  Serial.println("[INIT] AD7746 init done.");

  // Optional: set CAP DAC A
  ad7746_set_cap_dac_a(adc, true, 0x42);
  Serial.println("[INIT] CAP DAC A set.");
}


void loop() {
  uint32_t capData = 0;
  uint32_t temperature = 0;
  int32_t ret;

  Serial.println("in the loop");

  // code for debug
  Serial.println("Register dump:");

  for (uint8_t addr = 0x00; addr <= 0x0F; addr++) {
    uint8_t val = 0;
    ad7746_reg_read(adc, addr, &val, 1);
    Serial.print("Reg 0x");
    Serial.print(addr, HEX);
    Serial.print(": 0x");
    Serial.println(val, HEX);
  }

// debug end

  ret = ad7746_get_cap_data(adc, &capData);
  if (ret != 0) {
    Serial.print("Error reading capacitance: ");
    Serial.println(ret);
  } else {
    float cap_pf = ((int32_t)(capData & 0xFFFFFF) - 0x800000) * 8.192f / 16777216.0f;
    Serial.print("Capacitance (pF): ");
    Serial.println(cap_pf, 6);
  }

  delay(200);
}
