/****************************************************************************
 * boards/xtensa/esp32/common/src/esp32_board_adc.c
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

#include <nuttx/config.h>
#include <nuttx/analog/adc.h>
#include <stdio.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <esp32_adc.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_adc_initialize
 *
 * Description:
 *   Initialize and register the Analog to Digital Convertor (ADC) driver.
 *
 * Input Parameters:
 *   path - The device number, used to build the device path as
 *           /dev/adcN
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_adc_initialize(const char *path)
{
  int ret;

  /* Initialize ADC */

  struct adc_dev_s *dev = esp32_adc_initialize();
  if (dev != NULL)
    {
      /* Try to register the ADC */

      ret = adc_register(path, dev);
      if (ret < 0)
        {
          snerr("ERROR: Error registering DAC\n");
        }
    }
  else
    {
      ret = -ENODEV;
    }

  return ret;
}
