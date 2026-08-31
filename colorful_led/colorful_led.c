#include "colorful_led.h"

u8 R_ws_data[ws_num];
u8 L_ws_data[ws_num];    //前灯数组，放到全局，所有函数外面！

/**************************************************************************
函数功能：colorful_led接口初始化
入口参数：无
返回  值：无
**************************************************************************/
void colorful_led_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//使能端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14;	//端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//50M
	GPIO_Init(GPIOC, &GPIO_InitStructure);					//根据设定参数初始化GPIO
}

//=========后灯驱动=========
void R_send_0(void)
{
	DIR=1;
	Wait400ns;
	DIR=0;
	Wait850ns;
}

void R_send_1(void)
{
	DIR=1;
	Wait850ns;
	DIR=0;
	Wait400ns;
}

void R_ws2812_reset(void)
{
	DIR=0;
	delay_us(66);
}

void R_ws2812_rgb(u8 R_ws_num,u8 ws_r,u8 ws_g,u8 ws_b)		//将颜色数据发送到数组中
{
	R_ws_data[(R_ws_num-1)*3]=ws_g;
	R_ws_data[(R_ws_num-1)*3+1]=ws_r;
	R_ws_data[(R_ws_num-1)*3+2]=ws_b;
}

void R_ws2812_refresh(u8 ws_count)							//根据数组数据对对应的灯进行点亮
{
	u8 R_ws_ri=0;

	for(;R_ws_ri<ws_count*3;R_ws_ri++)
	{
		if((R_ws_data[R_ws_ri]&0x80)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x40)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x20)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x10)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x08)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x04)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x02)==0) R_send_0(); else R_send_1();
		if((R_ws_data[R_ws_ri]&0x01)==0) R_send_0(); else R_send_1();
	}
	//延时一段时间
	R_ws2812_reset();
}

/******后灯跑马灯效果******/
void R_runingled(void)				//后灯跑马灯
{
	u8 i,j;
	/*流光*/
	for(j=1;j<7;j++)
	{
		for(i=1;i<7;i++)			//把灯的颜色写在每个灯的数组中
		{
			if(i==j)
				R_ws2812_rgb(i, WS_WHITE);
			else
				R_ws2812_rgb(i, WS_DARK);
		}
		R_ws2812_refresh(led_num);	//更新灯颜色
		delay_ms(100);
	}
	/*反流光*/
	for(j=6;j>=1;j--)
	{
		for(i=6;i>=1;i--)			//把灯的颜色写在每个灯的数组中
		{
			if(i==j)
				R_ws2812_rgb(i, WS_WHITE);
			else
				R_ws2812_rgb(i, WS_DARK);
		}
		R_ws2812_refresh(led_num);	//更新灯颜色
		delay_ms(100);
	}
}   //=====这里！！R_runingled 在这里就闭合！！

//=========前灯驱动，全部放在函数外面=========
void L_send_0(void)
{
	DIL=1;
	Wait400ns;
	DIL=0;
	Wait850ns;
}

void L_send_1(void)
{
	DIL=1;
	Wait850ns;
	DIL=0;
	Wait400ns;
}

void L_ws2812_reset(void)
{
	DIL=0;
	delay_us(66);
}

void L_ws2812_rgb(u8 L_ws_num,u8 ws_r,u8 ws_g,u8 ws_b)		//将颜色数据发送到数组中
{
	L_ws_data[(L_ws_num-1)*3]=ws_g;
	L_ws_data[(L_ws_num-1)*3+1]=ws_r;
	L_ws_data[(L_ws_num-1)*3+2]=ws_b;
}

void L_ws2812_refresh(u8 ws_count)							//根据数组数据对对应的灯进行点亮
{
	u8 L_ws_ri=0;

	for(;L_ws_ri<ws_count*3;L_ws_ri++)
	{
		if((L_ws_data[L_ws_ri]&0x80)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x40)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x20)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x10)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x08)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x04)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x02)==0) L_send_0(); else L_send_1();
		if((L_ws_data[L_ws_ri]&0x01)==0) L_send_0(); else L_send_1();
	}
	//延时一段时间
	L_ws2812_reset();
}

/******前灯跑马灯效果******/
void L_runingled(void)				//前灯跑马灯
{
	u8 i,j;
	/*流光*/
	for(j=1;j<7;j++)
	{
		for(i=1;i<7;i++)			//把灯的颜色写在每个灯的数组中
		{
			if(i==j)
				L_ws2812_rgb(i, WS_WHITE);
			else
				L_ws2812_rgb(i, WS_DARK);
		}
		L_ws2812_refresh(led_num);	//更新灯颜色
		delay_ms(100);
	}
	/*反流光*/
	for(j=6;j>=1;j--)
	{
		for(i=6;i>=1;i--)			//把灯的颜色写在每个灯的数组中
		{
			if(i==j)
				L_ws2812_rgb(i, WS_WHITE);
			else
				L_ws2812_rgb(i, WS_DARK);
		}
		L_ws2812_refresh(led_num);	//更新灯颜色
		delay_ms(100);
	}
}
