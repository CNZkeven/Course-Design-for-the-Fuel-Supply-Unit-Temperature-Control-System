#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include "dac.h"
#include "control_output.h"
#include "temp_conversion.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/*
 * 温度控制系统主程序
 *
 * 本文件负责整个课设下位机的主控流程：
 * 1. 初始化 ADC、DAC、LCD、USART 等外设
 * 2. 周期性采集温度并执行 PID 控制
 * 3. 在 LCD 上显示过程变量和控制参数
 * 4. 通过串口向上位机上报运行数据
 * 5. 解析上位机下发的温度/PID/启停等命令
 */

/* 系统节拍与分辨率定义 */
#define SAMPLE_TIME       0.1f    // 主循环目标节拍约为 100ms
#define ADC_RESOLUTION    4096    // STM32 ADC 为 12 位，满量程 0~4095
#define DAC_RESOLUTION    4096    // STM32 DAC 为 12 位，满量程 0~4095

/* LCD 页面布局与配色定义 */
#define UI_BG_COLOR         WHITE
#define UI_HEADER_COLOR     DARKBLUE
#define UI_PANEL_COLOR      WHITE
#define UI_BORDER_COLOR     GRAYBLUE
#define UI_TITLE_COLOR      DARKBLUE
#define UI_TEXT_COLOR       BLACK
#define UI_SUBTEXT_COLOR    GRAYBLUE
#define UI_READY_COLOR      GRAYBLUE
#define UI_RUN_COLOR        GREEN
#define UI_ALARM_COLOR      BRRED

#define UI_STATUS_X1        0
#define UI_STATUS_Y1        0
#define UI_STATUS_X2        239
#define UI_STATUS_Y2        39

#define UI_MAIN_X1          10
#define UI_MAIN_Y1          50
#define UI_MAIN_X2          229
#define UI_MAIN_Y2          145

#define UI_SIGNAL_X1        10
#define UI_SIGNAL_Y1        158
#define UI_SIGNAL_X2        114
#define UI_SIGNAL_Y2        307

#define UI_PID_X1           125
#define UI_PID_Y1           158
#define UI_PID_X2           229
#define UI_PID_Y2           307

#define UI_PANEL_HEAD_H     28

/*
 * 串口协议命令字
 *
 * 上位机使用简单的文本命令控制下位机，所有命令以 CRLF 结尾。
 * 例如：TEMP:60.00\r\n、START\r\n
 */
#define CMD_SET_TEMP    "TEMP:"    // 设置目标温度
#define CMD_SET_P       "P:"       // 设置比例系数
#define CMD_SET_I       "I:"       // 设置积分系数
#define CMD_SET_D       "D:"       // 设置微分系数
#define CMD_START       "START"    // 启动加热
#define CMD_STOP        "STOP"     // 停止加热
#define CMD_SET_LIMIT   "LIMIT:"   // 设置超温阈值
#define CMD_GET_PID     "GET"      // 回显当前参数

/* 全局运行参数 */
float dt = SAMPLE_TIME;
float set_temp = 60.0f;    // 当前目标温度，默认 60℃
float dac_out;             // 本轮准备输出到 DAC 的目标码值
u16 adcx;                  // ADC 原始采样值
float temp;                // 当前温度值
u8 run_enabled = 0;        // 运行使能，1 表示允许控温输出
u8 alarm = 0;              // 报警标志，1 表示超温报警
float over_limit = 75.0f;  // 超温阈值
u16 last_dac = 2048;       // 最近一次实际写入的 DAC 码值
float last_voltage = 0.0f; // 最近一次换算得到的控制电压
u8 adc_channel = ADC_Channel_6; // 温度采样固定使用 PA6 / ADC_Channel_6

/* PID 参数与限幅 */
typedef struct {
    float P, I, D, limit;
} PID;

/*
 * 误差记录结构体
 *
 * Current_Error 在本工程中作为积分累加量使用；
 * Last_Error 用于计算微分项；
 * Previous_Error 预留给增量式 PID 或后续扩展分析。
 */
typedef struct {
    float Current_Error;
    float Last_Error;
    float Previous_Error;
} Error;

PID pid1;
Error error1 = {0, 0, 0};

/* 函数声明 */
void dac_output(float dacval);
void parse_command(char *cmd);
void display_values(void);
void usart_process(void);
float pid_realize(Error *error, PID *pid, float current, float target);
static void display_init_screen(void);
static void display_draw_status(void);
static void lcd_draw_text(u16 x, u16 y, u8 size, u16 fg, u16 bg, const char *text);
static void lcd_draw_textf(u16 x, u16 y, u8 size, u16 fg, u16 bg, const char *fmt, ...);
static void lcd_draw_box(u16 x1, u16 y1, u16 x2, u16 y2, const char *title);
static void lcd_draw_line_color(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);

/*
 * LCD 字符串绘制封装
 *
 * 底层 LCD 驱动通过全局 POINT_COLOR / BACK_COLOR 控制颜色。
 * 这里先保存旧颜色，再切换到目标颜色绘制，最后恢复现场，
 * 避免影响其他显示模块。
 */
static void lcd_draw_text(u16 x, u16 y, u8 size, u16 fg, u16 bg, const char *text) {
    u16 old_point = POINT_COLOR;
    u16 old_back = BACK_COLOR;

    POINT_COLOR = fg;
    BACK_COLOR = bg;
    LCD_ShowString(x, y, lcddev.width - x, size, size, (u8 *)text);

    POINT_COLOR = old_point;
    BACK_COLOR = old_back;
}

/* 带格式化功能的 LCD 文本输出，便于显示温度、PID 等数值。 */
static void lcd_draw_textf(u16 x, u16 y, u8 size, u16 fg, u16 bg, const char *fmt, ...) {
    char buffer[32];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    lcd_draw_text(x, y, size, fg, bg, buffer);
}

/* 在指定颜色下绘制线条，并在绘制后恢复原画笔颜色。 */
static void lcd_draw_line_color(u16 x1, u16 y1, u16 x2, u16 y2, u16 color) {
    u16 old_point = POINT_COLOR;

    POINT_COLOR = color;
    LCD_DrawLine(x1, y1, x2, y2);
    POINT_COLOR = old_point;
}

/* 绘制统一风格的卡片边框和标题，构建 LCD 界面布局。 */
static void lcd_draw_box(u16 x1, u16 y1, u16 x2, u16 y2, const char *title) {
    u16 old_point = POINT_COLOR;

    LCD_Fill(x1, y1, x2, y2, UI_PANEL_COLOR);
    POINT_COLOR = UI_BORDER_COLOR;
    LCD_DrawRectangle(x1, y1, x2, y2);
    lcd_draw_line_color(x1 + 1, y1 + UI_PANEL_HEAD_H, x2 - 1, y1 + UI_PANEL_HEAD_H, UI_BORDER_COLOR);
    lcd_draw_text(x1 + 10, y1 + 7, 16, UI_TITLE_COLOR, UI_PANEL_COLOR, title);

    POINT_COLOR = old_point;
}

/*
 * 初始化 LCD 总体界面
 *
 * 这里绘制的是静态背景和标签文本，只需上电时执行一次；
 * 温度、PID、DAC 等变化值由 display_values() 动态刷新。
 */
static void display_init_screen(void) {
    LCD_Clear(UI_BG_COLOR);
    LCD_Fill(UI_STATUS_X1, UI_STATUS_Y1, UI_STATUS_X2, UI_STATUS_Y2, UI_HEADER_COLOR);
    lcd_draw_text(10, 8, 16, WHITE, UI_HEADER_COLOR, "TEMP CTRL");

    lcd_draw_box(UI_MAIN_X1, UI_MAIN_Y1, UI_MAIN_X2, UI_MAIN_Y2, "PROCESS");
    lcd_draw_box(UI_SIGNAL_X1, UI_SIGNAL_Y1, UI_SIGNAL_X2, UI_SIGNAL_Y2, "SIGNAL");
    lcd_draw_box(UI_PID_X1, UI_PID_Y1, UI_PID_X2, UI_PID_Y2, "CONTROL");

    lcd_draw_text(20, 82, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "TEMP");
    lcd_draw_line_color(20, 121, 219, 121, UI_BORDER_COLOR);
    lcd_draw_text(20, 128, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "SET");

    lcd_draw_text(20, 196, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "ADC");
    lcd_draw_text(20, 224, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "DAC");
    lcd_draw_text(20, 252, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "OUT");
    lcd_draw_text(20, 280, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "CH");

    lcd_draw_text(135, 196, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "P");
    lcd_draw_text(135, 224, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "I");
    lcd_draw_text(135, 252, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "D");
    lcd_draw_text(135, 280, 16, UI_SUBTEXT_COLOR, UI_PANEL_COLOR, "LIM");

    display_draw_status();
}

/*
 * 刷新状态栏与关键过程量
 *
 * READY 表示未启动，RUN 表示正在闭环运行，ALARM 表示超温保护触发。
 * 状态栏与主显示区温度采用不同颜色，便于现场答辩时快速识别。
 */
static void display_draw_status(void) {
    const char *status_text = "READY";
    u16 status_bg = UI_READY_COLOR;
    u16 temp_color = UI_TEXT_COLOR;

    if (alarm) {
        status_text = "ALARM";
        status_bg = UI_ALARM_COLOR;
        temp_color = UI_ALARM_COLOR;
    } else if (run_enabled) {
        status_text = "RUN";
        status_bg = UI_RUN_COLOR;
        temp_color = UI_TITLE_COLOR;
    }

    LCD_Fill(145, 6, 228, 30, status_bg);
    lcd_draw_line_color(145, 6, 228, 6, WHITE);
    lcd_draw_line_color(145, 30, 228, 30, WHITE);
    lcd_draw_line_color(145, 6, 145, 30, WHITE);
    lcd_draw_line_color(228, 6, 228, 30, WHITE);

    lcd_draw_textf(154, 12, 16, WHITE, status_bg, "%-5s", status_text);
    lcd_draw_textf(66, 24, 12, WHITE, UI_HEADER_COLOR, "CH%02u", (unsigned)adc_channel);
    lcd_draw_textf(118, 24, 12, WHITE, UI_HEADER_COLOR, "LIM %5.2fC", over_limit);

    lcd_draw_textf(82, 86, 24, temp_color, UI_PANEL_COLOR, "%6.2fC", temp);
    lcd_draw_textf(146, 128, 16, UI_TITLE_COLOR, UI_PANEL_COLOR, "%6.2fC", set_temp);
}

/*
 * PID 计算函数
 *
 * 这里使用的是位置式 PID 写法：
 *   output = P * e + I * integral(e) + D * de/dt
 * 其中：
 * - 比例项决定当前误差放大倍数
 * - 积分项用于消除稳态误差
 * - 微分项用于抑制温度变化过快导致的超调
 *
 * 由于执行机构最终由 DAC 驱动，因此本函数最后对输出做限幅，
 * 防止控制量超过硬件允许范围。
 */
float pid_realize(Error *error, PID *pid, float current, float target) {
    float error_value = target - current;              // e(k) = 目标温度 - 当前温度
    float d_error;
    float output;

    error->Current_Error += error_value * dt;          // 对误差积分，形成积分项
    d_error = (error_value - error->Last_Error) / dt;  // 一阶差分近似微分项

    output = pid->P * error_value +
             pid->I * error->Current_Error +
             pid->D * d_error;

    if (output > pid->limit) {
        output = pid->limit;
    }
    if (output < -pid->limit) {
        output = -pid->limit;
    }

    error->Last_Error = error_value;
    return output;
}

/* 刷新 LCD 上与动态数据相关的区域。 */
void display_values(void) {
    display_draw_status();
    lcd_draw_textf(56, 196, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%4u", adcx);
    lcd_draw_textf(151, 196, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%6.2f", pid1.P);
    lcd_draw_textf(151, 224, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%6.2f", pid1.I);
    lcd_draw_textf(151, 252, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%6.2f", pid1.D);
    lcd_draw_textf(159, 280, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%5.2fC", over_limit);
    lcd_draw_textf(48, 280, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%02u", (unsigned)adc_channel);
}

/*
 * DAC 输出封装
 *
 * 输入参数 dacval 是 0~4095 的目标码值。由于底层 Dac1_Set_Vol() 接口
 * 使用的是毫伏值，因此这里需要先把 DAC 码值换算成 0~3300mV。
 * 写入硬件后，再把真实 DAC 输出和标定后的执行电压显示到 LCD，并缓存
 * 到全局变量供串口上报使用。
 */
void dac_output(float dacval) {
    u16 dac;
    u16 dac_mv;
    float voltage;

    if (dacval < 0) {
        dacval = 0;
    }
    if (dacval > DAC_CODE_MAX) {
        dacval = DAC_CODE_MAX;
    }

    dac = (u16)(dacval + 0.5f);

    /* Dac1_Set_Vol() 接收的是毫伏值，不是原始 DAC 码值。 */
    dac_mv = (u16)((dac * 3300.0f) / DAC_RESOLUTION);
    Dac1_Set_Vol(dac_mv);

    /* 读回当前 DAC 输出寄存器，便于显示和串口上报。 */
    dac = DAC_GetDataOutputValue(DAC_Channel_1);
    last_dac = dac;

    /*
     * 输出级经过外部电路后，实际控制可控硅的电压范围不是理想 0~3.3V，
     * 因此这里使用独立标定函数把 DAC 码值换算为更贴近实测的控制电压。
     */
    voltage = dac_code_to_output_voltage(dac);
    last_voltage = voltage;

    lcd_draw_textf(56, 224, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%4u", dac);
    lcd_draw_textf(56, 252, 16, UI_TEXT_COLOR, UI_PANEL_COLOR, "%6.3fV", voltage);
}

/*
 * 串口命令解析
 *
 * 本系统采用文本协议，优点是：
 * 1. 便于在串口助手中直接调试
 * 2. 便于 Python 上位机构造命令
 * 3. 在课设答辩时容易展示和解释
 */
void parse_command(char *cmd) {
    if (strncmp(cmd, CMD_SET_TEMP, 5) == 0) {
        set_temp = atof(cmd + 5);
        printf("Set temperature to: %.2f\r\n", set_temp);
    }
    else if (strncmp(cmd, CMD_SET_P, 2) == 0) {
        pid1.P = atof(cmd + 2);
        printf("Set P to: %.2f\r\n", pid1.P);
    }
    else if (strncmp(cmd, CMD_SET_I, 2) == 0) {
        pid1.I = atof(cmd + 2);
        printf("Set I to: %.2f\r\n", pid1.I);
    }
    else if (strncmp(cmd, CMD_SET_D, 2) == 0) {
        pid1.D = atof(cmd + 2);
        printf("Set D to: %.2f\r\n", pid1.D);
    }
    else if (strcmp(cmd, CMD_START) == 0) {
        /*
         * 启动前先判断是否已经超温，避免在报警状态下误启动。
         * 这样做可以把安全逻辑前置，便于答辩时说明“先保护再控制”。
         */
        if (temp >= over_limit) {
            alarm = 1;
            run_enabled = 0;
            printf("Over limit, cannot start\r\n");
        } else {
            alarm = 0;
            run_enabled = 1;
            printf("Start heating\r\n");
        }
    }
    else if (strcmp(cmd, CMD_STOP) == 0) {
        run_enabled = 0;
        printf("Stop heating\r\n");
    }
    else if (strncmp(cmd, CMD_SET_LIMIT, 6) == 0) {
        over_limit = atof(cmd + 6);
        printf("Set limit to: %.2f\r\n", over_limit);
    }
    else if (strcmp(cmd, CMD_GET_PID) == 0) {
        /*
         * GET 命令用于让下位机主动回显当前状态，方便上位机或串口助手
         * 在调试开始前快速确认参数是否已生效。
         */
        printf("Current PID values:\r\n");
        printf("Set temperature: %.2f\r\n", set_temp);
        printf("P: %.2f\r\n", pid1.P);
        printf("I: %.2f\r\n", pid1.I);
        printf("D: %.2f\r\n", pid1.D);
        printf("Limit: %.2f\r\n", over_limit);
        printf("Run: %d\r\n", run_enabled);
        printf("Alarm: %d\r\n", alarm);
    }
    else {
        printf("Unknown command\r\n");
    }
}

/*
 * 主循环中的串口命令处理入口
 *
 * USART_RX_STA 的状态位定义见 usart.h：
 * - bit15 = 1：收到完整一帧（以 CRLF 结束）
 * - bit14 = 1：已经收到 '\r'
 * - bit13~0 ：当前已接收字节数
 *
 * 中断只负责收字节和判定一帧是否结束，真正的命令解析放在主循环中完成，
 * 可以避免在中断里做 atof()/strncmp() 这类相对耗时的操作。
 */
void usart_process(void) {
    if (USART_RX_STA & 0x8000) {
        int len = USART_RX_STA & 0x3fff;

        USART_RX_BUF[len] = '\0';
        parse_command((char *)USART_RX_BUF);
        USART_RX_STA = 0;
    }
}

int main(void) {
    /* PID 初始值可通过上位机继续整定。 */
    pid1.P = 50.0f;
    pid1.I = 0.1f;
    pid1.D = 0.0f;
    pid1.limit = 100.0f;

    /* 外设初始化顺序尽量符合“先基础、后功能”的思路，便于排错。 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);
    uart_init(115200);
    LED_Init();
    LCD_Init();
    Adc_Init();
    Dac1_Init();

    display_init_screen();

    while (1) {
        /*
         * 对同一 ADC 通道做 20 次平均采样，减少噪声抖动。
         * 由于本系统不是高速控制对象，使用平均滤波更利于温度显示稳定。
         */
        adcx = Get_Adc_Average(adc_channel, 20);
        temp = adc2temp(adcx);

        /*
         * 超温保护优先级高于控温输出。
         * 一旦温度达到阈值，就立刻置报警并关断输出。
         */
        if (temp >= over_limit) {
            alarm = 1;
            run_enabled = 0;
        }

        display_values();

        if (run_enabled && !alarm) {
            float control_output = pid_realize(&error1, &pid1, temp, set_temp);

            /*
             * PID 输出先保持为工程量，再统一映射到 DAC 码值。
             * 这种分层写法比“直接把温度差换成 DAC 值”更清楚。
             */
            dac_out = (float)control_to_dac_code(control_output, pid1.limit);
        } else {
            dac_out = (float)DAC_ZERO_CODE;
        }

        dac_output(dac_out);

        /*
         * LED0 用作心跳指示，证明主循环正在运行。
         * 延时 100ms 后再上报一帧数据，使上位机曲线节拍与主循环接近一致。
         */
        LED0 = !LED0;
        delay_ms(100);

        /*
         * MCU -> PC 遥测格式：
         * TEMP,SET,P,I,D,DAC,VOLT,RUN,ALARM,LIMIT
         *
         * 采用一行一帧的 CSV 文本，便于上位机直接解析，也方便记录到 CSV 文件。
         */
        printf("%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.3f,%d,%d,%.2f\n",
               temp, set_temp, pid1.P, pid1.I, pid1.D,
               (unsigned int)last_dac, last_voltage,
               run_enabled, alarm, over_limit);

        usart_process();
    }
}
