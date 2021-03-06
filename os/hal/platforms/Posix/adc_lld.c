/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    templates/adc_lld.c
 * @brief   ADC Driver subsystem low level driver source template.
 *
 * @addtogroup ADC
 * @{
 */

#include "ch.h"
#include "hal.h"
#include <math.h> 
#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   ADC1 driver identifier.
 */
#if PLATFORM_ADC_USE_ADC1 || defined(__DOXYGEN__)
ADCDriver ADCD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
//create this thread to emulate if the buffer is circular
static WORKING_AREA(circular,128);
static msg_t buffer_thread(void *arg)
{
    (void)arg;
    while(ADCD1.toConvert)
    {

        int n=0;
 #if CONVERT     
        float exp = powf(2, ADCD1.grpp->bits);
        float adc_val = ADCD1.grpp->volt_range /exp;
        
        uint32_t    volt_input[ADCD1.grpp->buffer_size]; 
        float sample[ADCD1.grpp->buffer_size];
        n = 0;
        for(;n < (int)ADCD1.grpp->buffer_size;n++)
        {
            sample[n] = exp *  ADCD1.samples[n]/ADCD1.grpp->volt_range; 
            volt_input[n] = sample[n]*adc_val;    
            sim_printf(ADC_IO, "sample: %hd\n", sample[n]);
            sim_printf(ADC_IO, "sample: %.2f\n", volt_input[n]);
        }
        ADCD1.adc_val = sample;
        ADCD1.volt_input = volt_input;
#else

        for(;n < (int)ADCD1.grpp->buffer_size; n++)
        {
         //  printf("sample %d: %d \n",n ,ADCD1.samples[n]);
           sim_printf(ADC_IO, "sample: %hd\n", ADCD1.samples[n]);
        } 
#endif 
  chThdSleepMilliseconds(1000);       
    } 
    return -1;
}
/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if PLATFORM_ADC_USE_ADC1
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
#endif /* PLATFORM_ADC_USE_ADC1 */
}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start(ADCDriver *adcp) {

  if (adcp->state == ADC_STOP) {
    /* Enables the peripheral.*/
#if PLATFORM_ADC_USE_ADC1
    if (&ADCD1 == adcp) {

    }
#endif /* PLATFORM_ADC_USE_ADC1 */
  }
  /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop(ADCDriver *adcp) {

  if (adcp->state == ADC_READY) {
    /* Resets the peripheral.*/

    /* Disables the peripheral.*/
#if PLATFORM_ADC_USE_ADC1
    if (&ADCD1 == adcp) {

    }
#endif /* PLATFORM_ADC_USE_ADC1 */
  }
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start_conversion(ADCDriver *adcp) {

  (void)adcp;
  const ADCConversionGroup *grrp = adcp->grpp;
  adcp->toConvert = true;
  if(grrp->circular)
  { //going to be a thread... 
      chThdCreateStatic(circular, sizeof(circular), NORMALPRIO, buffer_thread, NULL);
  }
  else 
  {

        int n=0;
 #if CONVERT
        //prints out the calculated digital outputs if there's no digit outputs within
        //the buffer.      
        float exp = powf(2, grrp->bits);
        float adc_val = grrp->volt_range /exp;
        float sample[adcp->grpp->buffer_size];
        uint32_t    volt_input[adcp->grpp->buffer_size]; 
        n = 0;
        for(;n < (int)grrp->buffer_size;n++)
        {
            sample[n] = exp * adcp->samples[n]/grrp->volt_range; 
            volt_input[n] = sample[n]* adc_val;    
            sim_printf(ADC_IO, "sample: %.2f\n", sample[n]);
            sim_printf(ADC_IO, "sample: %.2f\n", volt_input[n]);
        }
        adcp->adc_val = sample;
        adcp->volt_input = volt_input;
#else

      //prints out the digital outputs
        for(;n < (int)grrp->buffer_size; n++)
        {
            sim_printf(ADC_IO, "sample: %hd\n", adcp->samples[n]);
        } 
#endif        
  } 
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop_conversion(ADCDriver *adcp) {


  (void)adcp;
  adcp->toConvert= false;
}

#endif /* HAL_USE_ADC */

/** @} */
