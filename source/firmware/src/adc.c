
/*
 * Open-BLDC - Open BrushLess DC Motor Controller
 * Copyright (C) 2009 by Piotr Esden-Tempski <piotr@esden.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32/rcc.h>
#include <stm32/misc.h>
#include <stm32/adc.h>
#include <stm32/gpio.h>
#include <stm32/tim.h>

#include "config.h"

#include "adc.h"

#include "led.h"
#include "pwm.h"
#include "comm_tim.h"

volatile uint8_t adc_rising = ADC_RISING;
volatile uint16_t adc_delay_count = 0;
volatile uint16_t adc_level = 10;
volatile int adc_comm = 0;
uint16_t adc_count = 0;
uint16_t adc_filtered = 0;

void adc_init(void){
    NVIC_InitTypeDef nvic;
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    /* enable ADC1 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /* Configure and enable ADC interrupt */
    nvic.NVIC_IRQChannel = ADC1_2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    /* GPIOA: ADC Channel 0, 1, 2 as analog input
     * Ch 0 -> BEMF/I_Sense of PHASE A
     * Ch 1 -> BEMF/I_Sense of PHASE B
     * Ch 2 -> BEMF/I_Sense of PHASE C
     */
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    adc_comm = 0;
    adc_filtered = 0;

    /* Configure ADC1 */
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 0;
    ADC_Init(ADC1, &adc);

    ADC_InjectedSequencerLengthConfig(ADC1, 1);

    ADC_InjectedChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_28Cycles5);

    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);

    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);

    /* Enable ADC1 JEOC interrupt */
    ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);

    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* Enable ADC1 reset calibaration register */
    ADC_ResetCalibration(ADC1);

    /* Check the end of ADC1 reset calibration */
    while(ADC_GetResetCalibrationStatus(ADC1));

    /* Start ADC1 calibaration */
    ADC_StartCalibration(ADC1);

    /* Check the end of ADC1 calibration */
    while(ADC_GetCalibrationStatus(ADC1));

    /* Enable ADC1 External Trigger */
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    //ADC_ExternalTrigConvCmd(ADC1, DISABLE);

}

void adc_set(uint8_t channel, uint8_t rising){

    adc_rising = rising;

    ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
    pwm_trig_led=0;

    ADC_InjectedChannelConfig(ADC1, channel, 1, ADC_SampleTime_28Cycles5);

    adc_delay_count = 0;
    adc_count = 0;
    adc_filtered = 0;

    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
    pwm_trig_led=1;
}

void adc1_2_irq_handler(void){
    uint16_t new_value;

    ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);

    new_value = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);

    if(adc_delay_count == 0)
        adc_filtered = new_value;
    else
        adc_filtered = ((adc_filtered * 3) + new_value) >> 2;

    if(adc_delay_count > 5){
        if(adc_rising){
            if(adc_filtered > adc_level + 100){
                if(adc_count < 3){
                    adc_count++;
                }else{
                    LED_ORANGE_TOGGLE();
                    ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
                    pwm_trig_led=0;
                    if(adc_comm) comm_tim_set_next_comm();
                }
            }
        }else{
            if(adc_filtered < adc_level){
                if(adc_count < 3){
                    adc_count++;
                }else{
                    LED_ORANGE_TOGGLE();
                    ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
                    pwm_trig_led=0;
                    if(adc_comm) comm_tim_set_next_comm();
                }
            }
        }
    }else{
        adc_delay_count++;
    }

}
