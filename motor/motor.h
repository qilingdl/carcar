#ifndef __MOTOR_H
#define __MOTOR_H

#include "sys.h"

//电机方向引脚定义
#define AIN  PBout(14)
#define BIN  PBout(13)

void Motor_Init(void);
void PWM_Init(u16 arr,u16 psc);
u32 myabs(long int a);
void Set_Pwm(int motol,int moto2);

#endif