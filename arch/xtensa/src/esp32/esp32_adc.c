/****************************************************************************
 * arch/xtensa/src/esp32/esp32_adc.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "xtensa.h"
#include <nuttx/config.h>
#include <nuttx/irq.h>
#include <nuttx/analog/adc.h>
#include <debug.h>
#include "esp32_adc.h"
#include "esp32_rtc_gpio.h"
#include "hardware/esp32_rtc_io.h"
#include "hardware/esp32_dport.h"
#include "hardware/esp32_sens.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum esp32_adc_unit_e {
  ESP32_ADC_UNIT_1 = 0,
  ESP32_ADC_UNIT_2 = 1
};

enum esp32_adc1_channel_e {
  ESP32_ADC1_CHANNEL_0 = 1,
  ESP32_ADC1_CHANNEL_1 = 2,
  ESP32_ADC1_CHANNEL_2 = 3,
  ESP32_ADC1_CHANNEL_3 = 4,
  ESP32_ADC1_CHANNEL_4 = 5,
  ESP32_ADC1_CHANNEL_5 = 6,
  ESP32_ADC1_CHANNEL_6 = 7,
  ESP32_ADC1_CHANNEL_7 = 8
};

enum esp32_adc2_channel_e {
  ESP32_ADC2_CHANNEL_0 = 11,
  ESP32_ADC2_CHANNEL_1 = 12,
  ESP32_ADC2_CHANNEL_2 = 13,
  ESP32_ADC2_CHANNEL_3 = 14,
  ESP32_ADC2_CHANNEL_4 = 15,
  ESP32_ADC2_CHANNEL_5 = 16,
  ESP32_ADC2_CHANNEL_6 = 17,
  ESP32_ADC2_CHANNEL_7 = 18,
  ESP32_ADC2_CHANNEL_8 = 19,
  ESP32_ADC2_CHANNEL_9 = 20
};

enum esp32_adc_atten_e {
  ESP32_ADC_ATTEN_0_Db = 0,
  ESP32_ADC_ATTEN_2_5_DB = 1,
  ESP32_ADC_ATTEN_6_DB = 2,
  ESP32_ADC_ATTEN_11_DB = 3
};

enum esp32_adc_width_e {
  ESP32_ADC_WIDTH_9_BIT = 0,
  ESP32_ADC_WIDTH_10_BIT = 1,
  ESP32_ADC_WIDTH_11_BIT = 2,
  ESP32_ADC_WIDTH_12_BIT = 3
};

const int esp32_adc1_channel_to_gpio[] = {
  ESP32_ADC1_CHANNEL_0,
  ESP32_ADC1_CHANNEL_1,
  ESP32_ADC1_CHANNEL_2,
  ESP32_ADC1_CHANNEL_3,
  ESP32_ADC1_CHANNEL_4,
  ESP32_ADC1_CHANNEL_5,
  ESP32_ADC1_CHANNEL_6,
  ESP32_ADC1_CHANNEL_7
};

const int esp32_adc2_channel_to_gpio[] = {
  ESP32_ADC2_CHANNEL_0,
  ESP32_ADC2_CHANNEL_1,
  ESP32_ADC2_CHANNEL_2,
  ESP32_ADC2_CHANNEL_3,
  ESP32_ADC2_CHANNEL_4,
  ESP32_ADC2_CHANNEL_5,
  ESP32_ADC2_CHANNEL_6,
  ESP32_ADC2_CHANNEL_7,
  ESP32_ADC2_CHANNEL_8,
  ESP32_ADC2_CHANNEL_9
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct adc_ops_s esp32_adc_ops = {
  .ao_bind = NULL,
  .ao_reset = NULL,
  .ao_setup = NULL,
  .ao_shutdown = NULL,
  .ao_rxint = NULL,
  .ao_ioctl = NULL,
};

static struct adc_dev_s g_adc = {
    .ad_ops = &esp32_adc_ops,
    .ad_priv = NULL
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int adc_channel_to_gpio_number(enum esp32_adc_unit_e unit,
                                      uint8_t channel) {
  switch (unit) {
    case ESP32_ADC_UNIT_1:
      return esp32_adc1_channel_to_gpio[channel];
    case ESP32_ADC_UNIT_2:
      return esp32_adc2_channel_to_gpio[channel];
    default:
      return -1;
  }
}

static void adc_set_width(enum esp32_adc_unit_e unit,
                          enum esp32_adc_width_e width) {
  uint32_t clear_bits = width ^ (1 << 0 | 1 << 1);
  uint32_t set_bits = width;

  switch (unit) {
    case ESP32_ADC_UNIT_1:
      modifyreg32(SENS_SAR_START_FORCE_REG,
                  clear_bits << SENS_SAR1_BIT_WIDTH_S,
                  set_bits << SENS_SAR1_BIT_WIDTH_S);
      modifyreg32(SENS_SAR_READ_CTRL_REG,
                  clear_bits << SENS_SAR1_SAMPLE_BIT_S,
                  set_bits << SENS_SAR1_SAMPLE_BIT_S);

      break;
    case ESP32_ADC_UNIT_2:
      modifyreg32(SENS_SAR_START_FORCE_REG,
                  clear_bits << SENS_SAR1_BIT_WIDTH_S,
                  set_bits << SENS_SAR1_BIT_WIDTH_S);
      modifyreg32(SENS_SAR_READ_CTRL2_REG,
                  clear_bits << SENS_SAR2_SAMPLE_BIT_S,
                  set_bits << SENS_SAR2_SAMPLE_BIT_S);
      break;
  }
}

static void adc1_set_atten(enum esp32_adc1_channel_e channel,
                          enum esp32_adc_atten_e atten) {
  uint32_t clear_bits = atten ^ (1 << 0 | 1 << 1);
  uint32_t set_bits = atten;

  modifyreg32(SENS_SAR_ATTEN1_REG,
              clear_bits << (channel - 1) * 2,
              set_bits << (channel - 1) * 2);
}

static void adc2_set_atten(enum esp32_adc2_channel_e channel,
                          enum esp32_adc_atten_e atten) {
  uint32_t clear_bits = atten ^ (1 << 0 | 1 << 1);
  uint32_t set_bits = atten;

  modifyreg32(SENS_SAR_ATTEN2_REG,
              clear_bits << (channel - 11) * 2,
              set_bits << (channel - 11) * 2);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp32_adc_initialize
 *
 * Description:
 *   Initialize the ADC.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Valid adc device structure reference on success; a NULL on failure.
 *
 ****************************************************************************/

struct adc_dev_s *esp32_adc_initialize(void)
{
  return &g_adc;
}
