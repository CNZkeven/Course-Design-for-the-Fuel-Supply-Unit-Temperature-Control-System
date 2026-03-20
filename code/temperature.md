# 温度检测与 ADC 转换逻辑说明

当前温度检测模块已经做过一次清理。现在温度输入固定使用 `PA6 / ADC_Channel_6`，未被主程序使用的旧宏、旧接口以及自动选通道逻辑都已删除。当前 `HARDWARE/ADC` 中保留的温度采样相关接口只有：

- `Adc_Init()`
- `Get_Adc()`
- `Get_Adc_Average()`

## 1. 当前工程实际使用的温度检测端口

当前工程主循环里实际使用的是 `ADC1` 的外部通道 `ADC_Channel_6`，默认对应的引脚是 `PA6`。

### 端口与通道对应关系

| 项目 | 当前配置 |
| --- | --- |
| ADC 外设 | `ADC1` |
| GPIO 端口 | `GPIOA` |
| 引脚 | `PA6` |
| ADC 通道 | `ADC_Channel_6` |
| 输入模式 | 模拟输入 `GPIO_Mode_AN` |
| 上下拉 | `GPIO_PuPd_UP` |
| 分辨率 | 12 位 |
| 采样时间 | `ADC_SampleTime_480Cycles` |
| 触发方式 | 软件触发 |

这些配置来自 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L20) 到 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L56)。

其中最关键的端口初始化是：

- 使能 `GPIOA` 时钟：[adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L26)
- 使能 `ADC1` 时钟：[adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L27)
- 将 `PA6` 配置为模拟输入：[adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L30)
- 默认规则通道配置为 `ADC_Channel_6`：[adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L54)

## 2. 当前工程实际运行的温度检测逻辑

当前工程真正用于温度检测的链路在 [main.c](/home/kevenchow/桌面/KS/code/USER/main.c#L328) 到 [main.c](/home/kevenchow/桌面/KS/code/USER/main.c#L351)。

实际流程如下：

1. 上电后调用 `Adc_Init()` 初始化 ADC 和 `PA6` 模拟输入。
2. 温度采样通道固定为 `adc_channel = ADC_Channel_6`。
3. 在主循环中调用 `Get_Adc_Average(adc_channel, 20)` 做 20 次采样平均。
4. 平均后的 ADC 数值传入 `adc2temp()`，换算成温度。
5. 换算后的温度保存到全局变量 `temp`，后续用于显示、超温保护和 PID 控制。

对应代码：

- 默认温度采样通道定义：[main.c](/home/kevenchow/桌面/KS/code/USER/main.c#L75)
- 主循环取样：[main.c](/home/kevenchow/桌面/KS/code/USER/main.c#L350)

## 3. ADC 采样逻辑

### 3.1 单次采样

`Get_Adc(u8 ch)` 会把指定通道重新配置到 `ADC1` 的规则组第 1 个转换序列，然后启动一次软件转换，等待转换完成，最后返回 12 位 ADC 结果。

对应代码在 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L62) 到 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L72)。

逻辑可以概括为：

```c
ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles);
ADC_SoftwareStartConv(ADC1);
while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
return ADC_GetConversionValue(ADC1);
```

### 3.2 多次平均

`Get_Adc_Average(u8 ch, u8 times)` 会对同一个通道连续采样 `times` 次，并在每次采样之间延时 `5ms`，最后取平均值。

对应代码在 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L77) 到 [adc.c](/home/kevenchow/桌面/KS/code/HARDWARE/ADC/adc.c#L87)。

当前主循环调用的是：

```c
adcx = Get_Adc_Average(adc_channel, 20);
```

也就是说：

- 每轮温度更新取 20 次样本
- 每次样本之间延时 5ms
- 仅 ADC 平均这一段就约 100ms

## 4. ADC 值到温度值的转换逻辑

当前工程使用的是 `adc2temp()` 这条换算链路，而不是 STM32 内部温度传感器的标准公式。

对应代码现在位于 [temp_conversion.c](/home/kevenchow/桌面/KS/code/USER/temp_conversion.c#L1)。

实现如下：

```c
#define ADC_FULL_SCALE_CODE       4095.0f
#define ADC_ZERO_CODE             0.0f
#define TEMP_FULL_SCALE_C         150.0f

if (code > ADC_FULL_SCALE_CODE) {
    code = ADC_FULL_SCALE_CODE;
}

if (code <= ADC_ZERO_CODE) {
    return 0.0f;
}

return ((code - ADC_ZERO_CODE) * TEMP_FULL_SCALE_C) /
       (ADC_FULL_SCALE_CODE - ADC_ZERO_CODE);
```

### 4.1 当前采用的标定假设

- 外部温度输入等效为 `0~5V` 模拟量
- 显示温度量程定义为 `0~150℃`
- 外部 `0V` 在 ADC 侧对应 `0`
- 外部 `+5V` 在 ADC 侧对应 `4095`
- 温度换算使用完整 `0~4095` 线性量程
- 因此外部输入 `+5V` 时，屏幕应显示 `150℃`

### 4.2 ADC 值转温度

程序采用的是按“扣除零点偏置后的有效量程”做线性映射：

```text
temperature = max(0, adc_value) * 150 / 4095
```

如果传入值低于零点偏置，则按 `0℃` 处理；如果超过 ADC 满量程，则按 `4095` 钳位：

```text
temperature = max(0, min(adc_value, 4095)) * 150 / 4095
```

### 4.3 示例

如果 ADC 原始值为 `0`：

```text
temperature = 0 * 150 / 4095 = 0℃
```

如果 ADC 原始值为 `4095`：

```text
temperature = 4095 * 150 / 4095 = 150℃
```

## 5. 固定通道策略

当前工程不再进行启动扫描，也不再自动选择 ADC 通道。

程序直接固定使用：

```c
u8 adc_channel = ADC_Channel_6;
```

这样做的原因是：

- 硬件上温度采样端口已经固定为 `PA6`
- `Adc_Init()` 中也只明确初始化了 `PA6` 的模拟输入
- 自动扫描 `CH0 ~ CH15` 对当前硬件方案没有必要
- 删除自动选通道逻辑后，温度采样链路更明确，也更容易维护

## 6. 控制输出链路：PID 控制量到 DAC 码值

当前工程已经把控制输出映射独立成 `control_to_dac_code()`，不再使用“温度转 ADC”的公式去驱动 DAC。

```c
uint16_t control_to_dac_code(float control_output, float output_limit);
```

对应代码位于 [control_output.c](/home/kevenchow/桌面/KS/code/USER/control_output.c)。

当前主循环中的控制输出流程是：

- PID 先根据设定温度和当前温度计算控制量 `control_output`
- 负控制量在输出侧按 `0` 处理，因为当前加热执行机构只需要单极控制
- `0` 控制量对应 DAC 零点码值 `2048`
- 正控制量按 `[0, output_limit]` 线性映射到 DAC 码值 `[2048, 4095]`
- DAC 码值 `0~2048` 统一视为 `0V`，`2048~4095` 线性映射为 `0~5V`

调用位置在 [main.c](/home/kevenchow/桌面/KS/code/USER/main.c) 的 PID 控制分支中。

这样做可以保证软件显示和控制量都与实际硬件输出级一致，避免继续按理想 `0~3.3V` 或旧的 `0.4~3.9V` 标定去估算控制电压。

## 7. 本次清理掉的无用代码

本次已从温度检测模块中删除以下无用或不再需要的内容：

- `ADC_CH5` 宏
- `Get_Temprate()` 的头文件声明
- `Get_Temprate()` 的实现
- `Adc_Init()` 中预先配置 `ADC_Channel_16` 的冗余代码
- `ADC_SCAN_DEBUG`
- `ADC_SCAN_ROUNDS`
- `ADC_AUTO_CHANNEL`
- `ADC_MIN_VALID`
- `adc_scan_log()`
- `adc_detect_channel()`

删除原因是：

- 主程序没有调用 `Get_Temprate()`
- 当前温度检测实际走的是 `Get_Adc_Average() + adc2temp()`
- `ADC_CH5` 在工程中没有被引用
- `Adc_Init()` 初始化后，真正采样时都会在 `Get_Adc()` 中重新配置通道，因此预先把规则通道切到 `ADC_Channel_16` 没有实际作用
- 温度输入硬件端口已经固定为 `PA6 / ADC_Channel_6`
- 启动扫描和自动选通道对当前方案没有实际意义

清理后的实际温度检测主链路仍然是：

```c
adcx = Get_Adc_Average(adc_channel, 20);
temp = adc2temp(adcx);
```

## 8. 可以直接写进报告的简述

本工程使用 STM32F407 的 `ADC1` 对外部模拟温度信号进行采样，实际采样端口固定为 `PA6`，对应 `ADC_Channel_6`。程序先对 ADC 通道进行初始化，将 `PA6` 配置为模拟输入，ADC 工作在 12 位、软件触发、单次转换模式。主循环中对 `ADC_Channel_6` 进行 20 次采样平均，得到原始 ADC 值后，再按“外部 `0V` 对应 ADC `0`、外部 `+5V` 对应 ADC `4095`”的标定关系换算为 `0~150℃` 温度值，因此接地时屏幕显示 `0℃`，外部输入 `+5V` 时屏幕显示 `150℃`。当前程序已去除启动扫描和自动选通道逻辑，因此温度检测链路固定且明确。
