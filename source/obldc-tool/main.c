/*
 * Open-BLDC Tool - Open BrushLess DC Motor Controller PC Tool
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

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>

#include "tools.h"
#include "serial.h"

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

in_data_t in_data;
in_data_t *in_datap = &in_data;
unsigned char dat2[1];
char msg[25];
int rot_cnt = 0;
char rot[2] = "/";
int16_t adc_mean;
int16_t adc_dev;
int16_t adc_old;

static WINDOW *mainwnd;
static WINDOW *screen;

void quit(){
    endwin();

    s_close();

    exit(1);
}

void inc_rot(){
    switch(rot_cnt){
    case 0:
        rot[0] = '-';
        break;
    case 1:
        rot[0] = '\\';
        break;
    case 2:
        rot[0] = '|';
        break;
    case 3:
        rot[0] = '/';
        break;
    }
    rot_cnt++;
    if(rot_cnt == 4) rot_cnt = 0;
}

static void update_display(void) {
    int i, j;
    static unsigned int a=0, b=0, c=0;

    inc_rot();
    wclear(screen);
    box(screen, ACS_VLINE, ACS_HLINE);
    curs_set(0);
    mvwprintw(screen,1,2, "Comm:");
    mvwprintw(screen,1,8, msg);
    mvwprintw(screen,1,24, rot);
    mvwprintw(screen,3,2,"PWM Period: %u", in_data.pwm_period);
    mvwprintw(screen,4,2,"PWM Duty: %u", in_data.pwm_duty);
    mvwprintw(screen,5,2,"Comm Force: %u", in_data.comm_force_time);
    mvwprintw(screen,6,2,"ADC Mean: %u", adc_mean);
    mvwprintw(screen,7,2,"ADC Dev: %u", adc_dev);
    #if 0
    #endif
    i=0;
    //while((! (in_data.adc[i] & 0xF000)) && i < 32) i++;
    //for(j=0; j<32; j++){
    //    if(in_data.adc[(i+j)%32] & 0xF000)
    //        mvwprintw(screen,8+j,2,"ADC[%02u]*: %u", (i+j)%32, in_data.adc[(i+j)^32]&0xFFF);
    //    else
    //        mvwprintw(screen,8+j,2,"ADC[%02u] : %u", (i+j)%32, in_data.adc[(i+j)%32]);
    //}
    mvwprintw(screen,8,2,"ADC[0] : %u", in_data.adc[0]);
    mvwprintw(screen,9,2,"ADC[1] : %u", in_data.adc[1]);
    mvwprintw(screen,10,2,"ADC[2] : %u", in_data.adc[2]);
    mvwprintw(screen,11,2,"ADC[3] : %u", in_data.adc[3]);
    wrefresh(screen);
    refresh();
}

int main(int argc, char **argv){
    S_STATUS ret;
    unsigned char *rx_buf;
    s_size_t count;
    unsigned char dat1[1];
    int getch_ret;
    int i;

    signal(SIGINT, quit);

    mainwnd = initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();
    noecho();
    nodelay(mainwnd, TRUE);
    refresh();
    wrefresh(mainwnd);
    screen = newwin(9+4, 27, 1, 1);
    box(screen, ACS_VLINE, ACS_HLINE);

    ret = s_open("A6004kZx", 38400);

    if(ret != S_OK){
        printf("error\n");
        s_close();
        return 0;
    }

    dat1[0]=(unsigned char)'g';

    ret = s_write(dat1, 1);

    if(ret != 1){
        printf("write error\n");
        s_close();
        return 0;
    }

    while(1){
        rx_buf = (unsigned char *)malloc(sizeof(in_data)+1);
        memset((unsigned char *)in_datap, 0, sizeof(in_data));
        count = s_read(rx_buf, sizeof(in_data)+1);

        if(count == 0){
            snprintf(msg, 25, "timeout");
            update_display();

            free(rx_buf);

            ret = s_write(dat1, 1);

            if(ret != 1){
                printf("write error\n");
                s_close();
                return 0;
            }

            continue;
        }

        for(i=0; i<7; i++){
            if(rx_buf[i] == 'o') break;
        }

        if(i==7){
            snprintf(msg, 25, "startbyte not");
            update_display();

            free(rx_buf);
            continue;
        }

        if(i != 0){
            memmove(rx_buf, rx_buf+i, sizeof(in_data)+1-i);

            count = s_read(rx_buf+sizeof(in_data)+1-i, i);

            if(count == 0){
                snprintf(msg, 25, "timeout");
                update_display();

                free(rx_buf);
                continue;
            }
        }

        for(i=0; i<sizeof(in_data); i++)
            ((unsigned char *)in_datap)[i] = rx_buf[i+1];

        //hexdump(rx_buf, 7);

        adc_mean = 0;
        adc_old = in_data.adc[0];
        adc_dev = 0;
        for(i=0; i<32; i++){
            adc_mean+=in_data.adc[i]/32;
            if(adc_dev < abs(((int16_t)in_data.adc[i]) - adc_old))
                adc_dev = abs(((int16_t)in_data.adc[i]) - adc_old);
        }

        snprintf(msg, 25, "OK");
        update_display();

        free(rx_buf);

        getch_ret = getch();

        if((getch_ret != ERR) && (getch_ret != dat1[0])){
            if(getch_ret == 'p'){
                while(getch() != 'p');
            }else{
                dat2[0]=getch_ret;
                s_write(dat2, 1);

                if(ret != 1){
                    printf("write error\n");
                    s_close();
                    return 0;
                }
            }
        }

        s_write(dat1, 1);

        if(ret != 1){
            printf("write error\n");
            s_close();
            return 0;
        }

    }
}
