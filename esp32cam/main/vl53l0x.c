/**
 * @file VL53L0X.h
 * @author Ryotaro Onuki (kerikun11+github@gmail.com)
 * @brief C++ Library for VL53L0X as an ESP-IDF component
 * @date 2018-11-07
 * @copyright Copyright (c) 2018
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"

#include "vl53l0x_api.h"
#include "vl53l0x_def.h"
#include "vl53l0x_platform.h"

#include "esp_log.h"

static const uint8_t VL53L0X_I2C_ADDRESS_DEFAULT = 0x29;
static const char *TAG = "VL53L0X";


static VL53L0X_Error print_pal_error(VL53L0X_Error status,
                                     const char *method) {
  char buf[VL53L0X_MAX_STRING_LENGTH];
  VL53L0X_GetPalErrorString(status, buf);
  ESP_LOGE(TAG, "%s API status: %i : %s\n", method, status, buf);
  return status;
}

static void init_i2c_master(i2c_port_t i2c_port,
                     gpio_num_t pin_sda,
                     gpio_num_t pin_scl) {
  i2c_config_t conf;
  uint32_t freq = 400000;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = pin_sda;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = pin_scl;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = freq;
  ESP_ERROR_CHECK(i2c_param_config(i2c_port, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(i2c_port, conf.mode, 0, 0, 0));
}

static bool vl53l0x_software_reset(VL53L0X_Dev_t* vl53l0x_dev) {
  VL53L0X_Error status = VL53L0X_ResetDevice(vl53l0x_dev);
  if (status != VL53L0X_ERROR_NONE) {
    print_pal_error(status, "VL53L0X_ResetDevice");
    return false;
  }
  return true;
}

static VL53L0X_Error _init_vl53l0x(VL53L0X_Dev_t *pDevice) {
  VL53L0X_Error status;
  uint8_t isApertureSpads;
  uint8_t PhaseCal;
  uint32_t refSpadCount;
  uint8_t VhvSettings;
  // Device Initialization (~40ms)
  status = VL53L0X_DataInit(pDevice);
  if (status != VL53L0X_ERROR_NONE)
    return print_pal_error(status, "VL53L0X_DataInit");
  status = VL53L0X_StaticInit(pDevice);
  if (status != VL53L0X_ERROR_NONE)
    return print_pal_error(status, "VL53L0X_StaticInit");
  // SPADs calibration (~10ms)
  status = VL53L0X_PerformRefSpadManagement(pDevice, &refSpadCount,
                                            &isApertureSpads);
  ESP_LOGI(TAG, "refSpadCount = %d, isApertureSpads = %d\n", refSpadCount,
           isApertureSpads);
  if (status != VL53L0X_ERROR_NONE)
    return print_pal_error(status, "VL53L0X_PerformRefSpadManagement");
  // Temperature calibration (~40ms)
  status = VL53L0X_PerformRefCalibration(pDevice, &VhvSettings, &PhaseCal);
  if (status != VL53L0X_ERROR_NONE)
    return print_pal_error(status, "VL53L0X_PerformRefCalibration");
  // Setup in single ranging mode
  status = VL53L0X_SetDeviceMode(pDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING);
  if (status != VL53L0X_ERROR_NONE)
    return print_pal_error(status, "VL53L0X_SetDeviceMode");
  // end
  return status;
}

static bool vl53l0x_set_time_budget(uint32_t TimingBudgetMicroSeconds) {
  VL53L0X_Error status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(
      &vl53l0x_dev, TimingBudgetMicroSeconds);
  if (status != VL53L0X_ERROR_NONE) {
    print_pal_error(status, "VL53L0X_SetMeasurementTimingBudgetMicroSeconds");
    return false;
  }
  this->TimingBudgetMicroSeconds = TimingBudgetMicroSeconds;
  return true;
}

bool init_vl53l0x(VL53L0X_Dev_t* vl53l0x_dev,
                  i2c_port_t i2c_port,
                  gpio_num_t pin_sda,
                  gpio_num_t pin_scl) {

  init_i2c_master(i2c_port, pin_sda, pin_scl);
  vl53l0x_dev->i2c_port_num = i2c_port;
  vl53l0x_dev->i2c_address = VL53L0X_I2C_ADDRESS_DEFAULT;
  vl53l0x_software_reset(vl53l0x_dev);
  if (_init_vl53l0x(vl53l0x_dev) != VL53L0X_ERROR_NONE)
    return false;
  if (VL53L0X_ERROR_NONE !=
      VL53L0X_SetGpioConfig(vl53l0x_dev, 0,
                            VL53L0X_DEVICEMODE_SINGLE_RANGING,
                            VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
                            VL53L0X_INTERRUPTPOLARITY_LOW))
    return false;
  if (!vl53l0x_set_time_budget(33000))
    return false;
  return true;
}

bool vl53l0x_read(VL53L0X_Dev_t* vl53lox_dev, uint16_t *pRangeMilliMeter) {
  // if (gpio_gpio1 != GPIO_NUM_MAX)
  //   return readSingleWithInterrupt(pRangeMilliMeter);
  VL53L0X_RangingMeasurementData_t MeasurementData;
  VL53L0X_Error status =
      VL53L0X_PerformSingleRangingMeasurement(vl53l0x_dev, &MeasurementData);
  if (status != VL53L0X_ERROR_NONE) {
    print_pal_error(status, "VL53L0X_PerformSingleRangingMeasurement");
    return false;
  }
  *pRangeMilliMeter = MeasurementData.RangeMilliMeter;
  if (MeasurementData.RangeStatus != 0)
    return false;
  return true;
}
