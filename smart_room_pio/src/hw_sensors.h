#pragma once
#include "sensor_manager.h"
/* Khởi tạo BH1750 (I2C), cửa, nút, SHT20 (RS485) và tạo SensorTask (poll mỗi 10 ms). */
void sensor_hw_start(sensor_sample_cb_t on_sample, sensor_button_cb_t on_button);
