/*
 * 避线小车固件（Hi3861 侧）
 *
 * 功能：小车在场地上自动直行，遇到黑色胶带边界自动转向避开，不会压线。
 *
 * 硬件接线（与 2.0_TCRT_Timer / 14.0_Bluetooth_control 例程一致）：
 *   左侧循迹传感器 TCRT5000 DO -> GPIO13   输出低电平 = 检测到黑色
 *   右侧循迹传感器 TCRT5000 DO -> GPIO14   输出低电平 = 检测到黑色
 *   Hi3861 UART2(GPIO11-TXD, GPIO12-RXD) -> STM32 电机控制板, 115200 8N1
 *
 * 发往 STM32 的电机协议帧（与原车固件一致，STM32 侧 PID 代码无需改动）：
 *   0xFC | 左轮方向 | 左轮速度 | 右轮方向 | 右轮速度 | 0xFD
 *   方向：0=正转 1=反转；速度：0~150
 *
 * 避线决策：
 *   左右都是白 -> 直行
 *   左黑右白   -> 右转避开
 *   左白右黑   -> 左转避开
 *   左右都黑   -> 沿上一次的转向方向继续转（过弯角不卡死）
 */

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_uart.h"

#define TCRT_LEFT  WIFI_IOT_IO_NAME_GPIO_13   /* 左循迹传感器 */
#define TCRT_RIGHT WIFI_IOT_IO_NAME_GPIO_14   /* 右循迹传感器 */

#define FORWARD_SPEED 100   /* 直行速度 */
#define TURN_SPEED    150   /* 转向时快轮速度 */
#define TURN_SLOW     50    /* 转向时慢轮速度（反转） */

static uint8_t uart_sendbuf[20];

/*-----------------电机控制（与 14.0_Bluetooth_control 完全一致）-----------------*/
static void stm32_motor_control(int motorA, int motorB)
{
    uint8_t a_dir = 0;
    uint8_t b_dir = 0;

    if (motorA < 0) {
        a_dir = 1;
        motorA = -motorA;
    }
    if (motorB < 0) {
        b_dir = 1;
        motorB = -motorB;
    }
    if (motorA > 150) {
        motorA = 150;
    }
    if (motorB > 150) {
        motorB = 150;
    }

    uart_sendbuf[0] = 0xFC;    /* 帧头 */
    uart_sendbuf[1] = a_dir;   /* 左轮方向 0正转 1反转 */
    uart_sendbuf[2] = (uint8_t)motorA; /* 左轮速度 */
    uart_sendbuf[3] = b_dir;   /* 右轮方向 0正转 1反转 */
    uart_sendbuf[4] = (uint8_t)motorB; /* 右轮速度 */
    uart_sendbuf[5] = 0xFD;    /* 帧尾 */

    UartWrite(WIFI_IOT_UART_IDX_2, uart_sendbuf, 6);
}

static void car_forward(void)
{
    stm32_motor_control(FORWARD_SPEED, FORWARD_SPEED);
}

static void car_left(void)
{
    /* 左轮反转、右轮正转 -> 原地左转 */
    stm32_motor_control(-TURN_SLOW, TURN_SPEED);
}

static void car_right(void)
{
    /* 左轮正转、右轮反转 -> 原地右转 */
    stm32_motor_control(TURN_SPEED, -TURN_SLOW);
}

static void car_stop(void)
{
    stm32_motor_control(0, 0);
}

/*-----------------传感器读取-----------------*/
/* 返回 1 表示检测到黑色（低电平），返回 0 表示白色（高电平） */
static int read_black(WifiIotIoName pin)
{
    WifiIotGpioValue value = WIFI_IOT_GPIO_VALUE1;

    GpioGetInputVal(pin, &value);
    return (value == WIFI_IOT_GPIO_VALUE0);
}

/*-----------------避线主任务-----------------*/
typedef enum {
    ACTION_FORWARD = 0,
    ACTION_RIGHT,
    ACTION_LEFT,
    ACTION_CORNER,
} CarAction;

static const char *g_action_name[] = {
    "forward", "turn right", "turn left", "corner turn"
};

static void AvoidLineTask(void)
{
    int last_turn_right = 1;  /* 双黑时沿上次转向继续转；首次默认右转 */
    CarAction action = ACTION_FORWARD;
    int last_printed = -1;    /* 只在状态变化时打印，避免刷屏 */

    /* 上电先停车 3 秒，方便把小车放到场地中央 */
    car_stop();
    printf("Avoid-line car will start in 3 seconds...\r\n");
    osDelay(300);

    while (1) {
        int left_black  = read_black(TCRT_LEFT);
        int right_black = read_black(TCRT_RIGHT);

        if (!left_black && !right_black) {
            action = ACTION_FORWARD;          /* 两侧都是白：直行 */
        } else if (left_black && !right_black) {
            action = ACTION_RIGHT;            /* 左侧遇黑线：右转避开 */
            last_turn_right = 1;
        } else if (!left_black && right_black) {
            action = ACTION_LEFT;             /* 右侧遇黑线：左转避开 */
            last_turn_right = 0;
        } else {
            action = ACTION_CORNER;           /* 双黑（弯角）：沿上次方向继续转 */
        }

        switch (action) {
            case ACTION_FORWARD:
                car_forward();
                break;
            case ACTION_RIGHT:
                car_right();
                break;
            case ACTION_LEFT:
                car_left();
                break;
            case ACTION_CORNER:
                if (last_turn_right) {
                    car_right();
                } else {
                    car_left();
                }
                break;
            default:
                break;
        }

        if ((int)action != last_printed) {
            printf("sensor L=%d R=%d -> %s\r\n", left_black, right_black,
                   g_action_name[action]);
            last_printed = (int)action;
        }

        osDelay(2);   /* 每 20ms 决策一次 */
    }
}

/*-----------------初始化入口-----------------*/
static void AvoidLineEntry(void)
{
    uint32_t ret;

    /* 循迹传感器：GPIO13/14 配置为普通 GPIO 输入 */
    GpioInit();
    IoSetFunc(TCRT_LEFT,  WIFI_IOT_IO_FUNC_GPIO_13_GPIO);
    IoSetFunc(TCRT_RIGHT, WIFI_IOT_IO_FUNC_GPIO_14_GPIO);
    GpioSetDir(TCRT_LEFT,  WIFI_IOT_GPIO_DIR_IN);
    GpioSetDir(TCRT_RIGHT, WIFI_IOT_GPIO_DIR_IN);

    /* UART2：与 STM32 电机控制板通信 */
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_UART2_TXD);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_UART2_RXD);

    WifiIotUartAttribute uart_attr2 = {
        .baudRate = 115200,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    ret = UartInit(WIFI_IOT_UART_IDX_2, &uart_attr2, NULL);
    if (ret != WIFI_IOT_SUCCESS) {
        printf("Failed to init uart2! Err code = %d\n", ret);
        return;
    }
    printf("Avoid-line car init OK!\r\n");

    /* 创建避线任务 */
    osThreadAttr_t attr;
    attr.name = "AvoidLineTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 4;
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)AvoidLineTask, NULL, &attr) == NULL) {
        printf("Failed to create AvoidLineTask!\n");
    }
}

APP_FEATURE_INIT(AvoidLineEntry);
