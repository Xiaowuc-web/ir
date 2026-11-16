# FreeRTOS 蜂鸣器移植说明

## 打开工程

使用 Keil uVision5 打开 Project_FreeRTOS.uvprojx，选择 FreeRTOS 目标，按 F7 编译。
原来的 Project.uvprojx 和 User、System 目录中的裸机代码未修改。

本移植针对现有工程的 STM32F103C8（Cortex-M3，64 KB Flash、20 KB RAM）和 ARM Compiler 5。
不要直接切换到 ARM Compiler 6；本工程使用 RVDS/ARM_CM3 端口。

## 文件与配置

- App_FreeRTOS/main.c：PB12 低电平有效的蜂鸣器任务及错误钩子。
- App_FreeRTOS/FreeRTOSConfig.h：1 kHz 节拍、8 KB heap_4 堆、5 个任务优先级。
- App_FreeRTOS/stm32f10x_it.c：保留异常处理，将 SVC、PendSV、SysTick 交给内核端口。
- FreeRTOS：官方 V10.6.2 内核所需源码、头文件和 MIT 许可证；SOURCE.json 记录来源及 SHA-256。

内核来源：https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/V10.6.2
版本固定用于现有 ARMCC5 工程兼容移植，不表示它是最新版本。

蜂鸣器任务优先级为 2，栈深度为 256 个 StackType_t（Cortex-M3 上为 1024 字节）。
调度器还会创建空闲任务。动态分配失败和栈溢出会进入错误钩子。
任务延时使用 vTaskDelay，节奏为响 100 ms、停 100 ms、响 100 ms、停 700 ms 循环。
延时时间受节拍量化和任务调度影响。

## SysTick 和中断注意事项

FreeRTOS 独占 SysTick；原 System/Delay.c 未加入新工程。
任务中使用 vTaskDelay(pdMS_TO_TICKS(ms))，不要重新加入会修改 SysTick 寄存器的旧 Delay 函数。
微秒延时如有需求应另用硬件定时器或 DWT 实现。
主函数调用 SystemCoreClockUpdate，内核按实际 SystemCoreClock 配置节拍。

NVIC 使用 NVIC_PriorityGroup_4（4 位抢占优先级）。
SVC、PendSV、SysTick 通过宏直接映射到启动文件的同名向量，不能再定义空处理函数。
今后在外设中断中调用 FreeRTOS API 时必须使用 FromISR 版本，抢占优先级数值设置为 5～15，子优先级为 0。
数值 0～4 的中断不可调用 FreeRTOS API；中断中不能使用 vTaskDelay。

## 编译与上板验证

当前已完成工程文件路径检查，以及三个内核异常处理函数的映射与重复定义检查。
命令行启动 Keil 被运行环境拦截，界面工具也返回“Computer Use was not approved to use Keil uVision5”。
因此本次尚未通过真实编译、链接或上板验证；请不要把文件检查当作编译通过。

1. 打开新工程并按 F7；若缺少器件包，使用原工程对应的 STM32F1xx_DFP；确认 ARM Compiler 5 可用。
2. 确认输出为 0 Error，检查链接输出的 Flash/RAM 占用。
3. 成功编译后输出位于 Objects_FreeRTOS/Project_FreeRTOS.hex。
4. 下载到原开发板，确认 PB12 双响节奏与原程序一致；可用示波器检查时序。
5. 调试观察 xTaskGetTickCount() 持续增加，g_rtosError 保持 0；按需要检查任务栈余量。

错误码 g_rtosError：1=堆分配失败，2=任务栈溢出，3=断言失败，4=蜂鸣器任务创建失败，5=调度器启动返回。
断言位置由 g_rtosAssertFile 和 g_rtosAssertLine 记录。错误处理会关闭蜂鸣器并停止运行。
