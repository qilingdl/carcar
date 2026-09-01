#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "wifiiot_uart.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "wifiiot_pwm.h"
#include "hi_pwm.h"
#include "hi_uart.h"
#include "wifiiot_gpio_ex.h"

static void thread1(void);
static void thread2(void);
static void thread3(void);

uint8_t uart_sendbuf[20];
osMutexId_t mutex_id;

//彩灯函数声明，彩灯代码在STM32
void colorful_led_test(void);

/***通信协议***/
void stm32_motor_control(int motorA, int motorB)
{
    uint8_t A_dir = 0;
    uint8_t B_dir = 0;

    if (motorA<0)
    {
        A_dir=1;
        motorA = -motorA;
    }
    else{
        A_dir=0;
    }
    if (motorB<0)
    {
        B_dir=1;
        motorB = -motorB;
    }
    else{
        B_dir=0;
    }

    if (motorA > 150)
    {
        motorA = 150;
    }
    if (motorB > 150)
    {
        motorB = 150;
    }

    uart_sendbuf[0] = 0xFC;
    uart_sendbuf[1] = A_dir;
    uart_sendbuf[2] = motorA;
    uart_sendbuf[3] = B_dir;
    uart_sendbuf[4] = motorB;
    uart_sendbuf[5] = 0xFD;

    UartWrite(WIFI_IOT_UART_IDX_2, (unsigned char *)uart_sendbuf, 6);
}

void car_backward(void)
{
    stm32_motor_control(-100, -100);
}

void car_forward(void)
{
    stm32_motor_control(100, 100);
}

void car_left(void)
{
    stm32_motor_control(50, 150);
}

void car_right(void)
{
    stm32_motor_control(150, 50);
}

void car_stop(void)
{
    stm32_motor_control(0, 0);
}

/*****任务一 电机前进*****/
static void thread1(void)
{
    while (1)
    {
        osMutexAcquire(mutex_id, osWaitForever);
        car_forward();
        usleep(1000000);
        osMutexRelease(mutex_id);
    }
}

/*****任务二 电机左转*****/
static void thread2(void)
{
    usleep(1000000);
    while (1)
    {
        osMutexAcquire(mutex_id, osWaitForever);
        car_left();
        usleep(1000000);
        osMutexRelease(mutex_id);
    }
}

/*****任务三【彩灯线程，不占用互斥锁】******/
static void thread3(void)
{
    while(1)
    {
        colorful_led_test();
        usleep(1000000);
    }
}

/*****任务创建*****/
static void correspondence(void)
{
    GpioInit();
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_UART2_RXD);

    WifiIotUartAttribute uart_attr2 = {
        .baudRate = 115200,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    UartInit(WIFI_IOT_UART_IDX_2, &uart_attr2, NULL);

    osThreadAttr_t attr;
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 4;

    //先创建锁！！！！
    mutex_id = osMutexNew(NULL);
    if (mutex_id == NULL)
    {
        printf("Failed to create Mutex!\n");
    }

    // 创建任务1
    attr.name = "thread1";
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)thread1, NULL, &attr) == NULL)
    {
        printf("Failed to create thread1!\n");
    }

    // 创建任务2
    attr.name = "thread2";
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)thread2, NULL, &attr) == NULL)
    {
        printf("Failed to create thread2!\n");
    }

    //创建彩灯任务 thread3
    attr.name = "thread3";
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)thread3, NULL, &attr) == NULL)
    {
        printf("Failed to create thread3!\n");
    }
}

APP_FEATURE_INIT(correspondence);