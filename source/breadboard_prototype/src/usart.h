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

#ifndef __USART_H
#define __USART_H

typedef struct {
    uint16_t pwm_period;
    uint16_t pwm_duty;
    uint32_t comm_force_time;
    uint16_t adc[32];
} out_data_t;

typedef struct {
    uint16_t pwm_period;
    uint16_t pwm_duty;
    uint32_t comm_force_time;
    uint16_t adc[32];
} in_data_t;

extern volatile out_data_t out_data;

void usart_rcc_init(void);
void usart_nvic_init(void);
void usart_gpio_init(void);
void usart_init(void);
void usart3_irq_handler(void);

#endif /* __USART_H */
