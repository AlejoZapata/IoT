#include "ds1307.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO 9
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define DS1307_ADDR 0x68

#define TAG "DS1307"

// Funciones internas
static uint8_t dec_to_bcd(uint8_t val) {
  return ((val / 10) << 4) | (val % 10);
}

static uint8_t bcd_to_dec(uint8_t val) {
  return ((val >> 4) * 10) + (val & 0x0F);
}

// I2C
void ds1307_init(void) {
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };

  i2c_param_config(I2C_MASTER_NUM, &conf);
  i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// SET TIME
void ds1307_set_time(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day,
                     uint8_t date, uint8_t month, uint8_t year) {
  uint8_t data[8];
  data[0] = 0x00; // registro inicial
  data[1] = dec_to_bcd(sec);
  data[2] = dec_to_bcd(min);
  data[3] = dec_to_bcd(hour);
  data[4] = dec_to_bcd(day);
  data[5] = dec_to_bcd(date);
  data[6] = dec_to_bcd(month);
  data[7] = dec_to_bcd(year);
  i2c_master_write_to_device(I2C_MASTER_NUM, DS1307_ADDR, data, 8,
                             1000 / portTICK_PERIOD_MS);
}

// GET TIME
void ds1307_get_time(ds1307_time_t *time) {
  uint8_t reg = 0x00;
  uint8_t data[7];
  i2c_master_write_read_device(I2C_MASTER_NUM, DS1307_ADDR, &reg, 1, data, 7,
                               1000 / portTICK_PERIOD_MS);
  time->sec = bcd_to_dec(data[0] & 0x7F);
  time->min = bcd_to_dec(data[1]);
  time->hour = bcd_to_dec(data[2]);
  time->day = bcd_to_dec(data[3]);
  time->date = bcd_to_dec(data[4]);
  time->month = bcd_to_dec(data[5]);
  time->year = bcd_to_dec(data[6]);
}