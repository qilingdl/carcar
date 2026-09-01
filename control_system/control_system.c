#include "stm32f10x.h"
#include "sys.h"
#include "motor.h" 
#include "control_system.h"
#include <string.h>

//从串口文件引入接收标志与解析缓存
extern u8 uart_rec_flag;
extern u8 CAR_buff[4];

u16 Read_Encoder(u8 ch);
void Set_Pwm(int motoA,int motoB);

/*电机 左A 右B*/
int L_coder,R_coder;

int Motor_A,Motor_B;	    //电机PWM变量
int OverflowTime=100;
volatile uint32_t millis = 0;	// 记录毫秒数
volatile uint32_t seconds = 0;	// 记录秒数

//保存串口下发的目标转速
float Target_MotorA = 0.0f;
float Target_MotorB = 0.0f;

/*********************************************************
函数功能:增量式PI控制器
入口参数:编码器测量值,目标速度
返回 值: 电机PWM公式
*********************************************************/
int Incremental_PI_A(int Encoders_A, int Target_A)
{
	float Velocity_KP = 7.0, Velocity_KI = 0.016, Velocity_KD = 0.003;
	static int Pwm_A = 0;
	static int Integral_A = 0;
	static float Error_prev_A = 0;
	float MaxIntegral = 0.0;
	float MinIntegral = 0.0;
	float Error_A = (float)(Target_A - Encoders_A);	// 计算偏差

	Integral_A += Error_A;	// 积分项更新

	//积分限幅
	MaxIntegral = (float)(7199 / Velocity_KI);
	MinIntegral = -(float)(7199 / Velocity_KI);
	if(Integral_A > MaxIntegral) Integral_A = MaxIntegral;
	else if(Integral_A < MinIntegral) Integral_A = MinIntegral;

	Pwm_A += Velocity_KP * Error_A + Velocity_KD * (Error_A - Error_prev_A);

	if (Pwm_A > 7199) Pwm_A = 7199;
	else if (Pwm_A < -7199) Pwm_A = -7199;

	Error_prev_A = Error_A;	// 保存上一次偏差

	return Pwm_A;	// 增量输出
}

int Incremental_PI_B(int Encoders_B, int Target_B)
{
	float Velocity_KP = 7.0, Velocity_KI = 0.016, Velocity_KD = 0.003;
	static int Pwm_B = 0;
	static int Integral_B = 0;
	static float Error_prev_B = 0;
	float MaxIntegral = 0.0;
	float MinIntegral = 0.0;
	float Error_B = (float)(Target_B - Encoders_B);	// 计算偏差

	Integral_B += Error_B;	// 积分项更新

	//积分限幅
	MaxIntegral = (float)(7199 / Velocity_KI);
	MinIntegral = -(float)(7199 / Velocity_KI);
	if(Integral_B > MaxIntegral) Integral_B = MaxIntegral;
	else if(Integral_B < MinIntegral) Integral_B = MinIntegral;

	Pwm_B += Velocity_KP * Error_B + Velocity_KD * (Error_B - Error_prev_B);

	if (Pwm_B > 7199) Pwm_B = 7199;
	else if (Pwm_B < -7199) Pwm_B = -7199;

	Error_prev_B = Error_B;	// 保存上一次偏差

	return Pwm_B;	// 增量输出
}

/*********************************************************
函数功能: 转每秒转脉冲数函数
入口参数: float
返回 值: int
*********************************************************/
int Rs_To_CPR(float rads)
{
	int CRP=0;
	CRP=rads * ((700*4)/(1000/OverflowTime));
	return CRP;
}

/*********************************************************
函数功能: 系统控制函数
入口参数:
返回 值:
*********************************************************/
void System_Control(void)
{
	int TageA=0;
	int TageB=0;

	L_coder=Read_Encoder(2);
	R_coder=Read_Encoder(3);
	printf("left_coder : %d\r\n",L_coder);
	printf("right_coder : %d\r\n",R_coder);

	//接收鸿蒙串口下发指令
	if(uart_rec_flag)
	{
		Target_MotorA=CAR_buff[1]/100.00f;
		Target_MotorB=CAR_buff[3]/100.00f;

		if(CAR_buff[0]==1)
		{
			Target_MotorA=-1*Target_MotorA;
		}
		if(CAR_buff[2]==1)
		{
			Target_MotorB=-1*Target_MotorB;
		}

		uart_rec_flag=0;
		memset(CAR_buff,0,4);
	}

	TageA=Rs_To_CPR(Target_MotorA);
	TageB=Rs_To_CPR(Target_MotorB);

	printf("TageA coder : %d\r\n",TageA);
	printf("TageB coder : %d\r\n",TageB);

	Motor_A=Incremental_PI_A(L_coder,TageA);
	Motor_B=Incremental_PI_B(R_coder,TageB);

	printf("Motor_A pwm : %d\r\n",Motor_A);
	printf("Motor_B pwm : %d\r\n",Motor_B);

	Set_Pwm(Motor_A,Motor_B);
}
