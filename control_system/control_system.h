#ifndef __CONTROL_SYSTEM_H
#define __CONTROL_SYSTEM_H
#include "sys.h"

extern int L_coder,R_coder;
extern int Motor_A,Motor_B;
extern int OverflowTime;
extern volatile uint32_t millis;
extern volatile uint32_t seconds;

int Incremental_PI_A(int Encoders_A, int Target_A);
int Incremental_PI_B(int Encoders_B, int Target_B);
int Rs_To_CPR(float rads);
void System_Control(void);

#endif
