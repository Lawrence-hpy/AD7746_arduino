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

ad7746_dev *adc; // Global Device Pointer
// ad7746_iio_dev *iio_dev = NULL; // Global IIO Device Pointer

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("[INIT] Starting AD7746 setup...");

  // I2C setup
  no_os_i2c_init_param i2c_init;
  i2c_init.max_speed_hz = 00000;
  i2c_init.slave_address = AD7746_ADDRESS;
  i2c_init.platform_ops = NULL;
  i2c_init.extra = NULL;

  // AD7746 setup
  ad7746_init_param init_param;
  init_param.i2c_init = i2c_init;
  init_param.id = ID_AD7746;

  init_param.setup.cap.capen = true;
  init_param.setup.cap.cin2 = true;
  init_param.setup.cap.capdiff = false;
  init_param.setup.cap.capchop = true;

  // init_param.setup.vt.vten = true;
  // init_param.setup.vt.vtmd = AD7746_VTMD_INT_TEMP;
  // init_param.setup.vt.extref = false;
  // init_param.setup.vt.vtshort = false;
  // init_param.setup.vt.vtchop = true;

  init_param.setup.exc.clkctrl = true;
  init_param.setup.exc.excon = true;
  init_param.setup.exc.excb = AD7746_EXC_PIN_NORMAL;
  init_param.setup.exc.exca = AD7746_EXC_PIN_NORMAL;
  init_param.setup.exc.exclvl = AD7746_EXCLVL_4_DIV_8;

  init_param.setup.config.vtf = 0;
  init_param.setup.config.capf = 0;
  init_param.setup.config.md = AD7746_MODE_CONT;

  int32_t ret = ad7746_init(&adc, &init_param);
  if (ret != 0) {
    Serial.print("AD7746 init failed: ");
    Serial.println(ret);
    while (1);
  }


  Serial.println("[INIT] AD7746 init done.");

  // // Initialize IIO Device, not sure if it is repeated. 

  // struct ad7746_iio_init_param iio_init_param;
  // iio_init_param.ad7746_initial = &init_param;

  // ret = ad7746_iio_init(&iio_dev, &iio_init_param);
  // if (ret != 0) {
  //     Serial.println("IIO init failed");
  //     while(1);
  // }

  // Optional: set CAP DAC A
  ret = ad7746_set_cap_dac_a(adc, true, 0x42);
  if (ret < 0){
    Serial.println("DACA setting failed");
    while (1);
  }
  Serial.println("[INIT] CAP DAC A set.");

  // Update IIO driver's capdac array to match hardware
  // if (iio_dev) {
  //   iio_dev->capdac[0][0] = AD7746_CAPDAC_DACEN_MSK | (0x42 & AD7746_CAPDAC_DACP_MSK);
  //   iio_dev->capdac_set = 0; // Mark DAC settings as applied for channel 0
  // }

  struct iio_ch_info ch_info;
  ch_info.ch_num = 0;
  ch_info.type = IIO_CAPACITANCE;
  ch_info.differential = 0;
  ch_info.address = 0;


  // ret = ad7746_iio_write_offset(adc, offset_buf, strlen(offset_buf), &ch_info, 0);
  // if (ret >= 0) {
  //   Serial.println("4pF offset set successfully!");
  // } else {
  //   Serial.print("Failed to set offset. Code: ");
  //   Serial.println(ret);
  // }

  const char* offset_str = "8192000"; // 4.0pF → 8192000 (based on scale factor)
  ret = ad7746_iio_write_offset(adc, (char*)offset_str, strlen(offset_str), &ch_info, 0);
  if (ret < 0) {
      Serial.print(F("Failed to set offset. Code: "));
      Serial.println(ret);
  }

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

  // delay(25);
  // Print current time in seconds with 6 decimal places
  unsigned long micros_now = micros();
  float time_sec = micros_now / 1e6;
  Serial.print("TIME:");
  Serial.println(time_sec, 6);  // e.g., TIME:3.152365
}
