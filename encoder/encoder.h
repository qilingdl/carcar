#ifndef __ENCODER_H
#define __ENCODER_H

#include "sys.h"

//编码器计数器自动重装值(16位定时器)
#define ENCODER_TIM_PERIOD	65535

void Encoder_Init_TIM2(void);
void Encoder_Init_TIM3(void);
int Read_Encoder(u8 TIMX);

#endif
