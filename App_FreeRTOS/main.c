#include "stm32f10x.h"  /* STM32 寄存器和标准外设库声明 */
#include "FreeRTOS.h"   /* FreeRTOS 内核类型和配置 */
#include "task.h"       /* 任务管理和调度器接口 */
#include "keynum.h"     /* 红外遥控器键码定义，当前尚未使用 */
#include "ir.h"         /* 红外时间数据队列初始化接口 */

/* 错误发生后可在调试器中查看这些变量。
 * volatile 使编译器保留对变量的实际访问，方便调试观察。
 */
volatile unsigned long g_rtosError;       /* 错误码，未显式初始化的全局变量初值为 0 */
volatile unsigned long g_rtosAssertLine;  /* 触发断言的源代码行号 */
const char * volatile g_rtosAssertFile;   /* 触发断言的文件名指针 */

/* 统一错误处理：屏蔽内核可管理的中断，记录错误码并停留等待调试。
 * Cortex-M3 端口通过 BASEPRI 屏蔽中断，并非关闭所有中断。
 */
static void StopOnError(unsigned long error)
{
    taskDISABLE_INTERRUPTS();
    g_rtosError = error;
    for (;;) {}  /* 停留在此处，不再继续执行后续代码 */
}

/* FreeRTOS 动态内存分配失败钩子，由配置项启用。 */
void vApplicationMallocFailedHook(void)
{
    StopOnError(1);  /* 错误码 1：内核堆内存分配失败 */
}

/* FreeRTOS 检测到任务栈溢出时调用，参数为任务句柄和任务名称。 */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;  /* 当前不使用该参数，显式转换以避免未使用警告 */
    (void)name;
    StopOnError(2);  /* 错误码 2：检测到任务栈溢出 */
}

/* configASSERT 条件不成立时调用，记录断言位置。 */
void vApplicationAssert(const char *file, unsigned long line)
{
    g_rtosAssertFile = file;  /* 保存源文件名 */
    g_rtosAssertLine = line;  /* 保存源代码行号 */
    StopOnError(3);  /* 错误码 3：断言失败 */
}

/* 程序入口：当前未创建业务任务，调度器启动时会创建空闲任务。
 * 尚未包含蜂鸣器控制、红外接收 GPIO 初始化或红外解码逻辑。
 */
int main(void)
{
    SystemCoreClockUpdate();  /* 根据时钟寄存器更新 SystemCoreClock，不是重新配置时钟 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  /* 4 位抢占优先级，无子优先级位 */
    IR_QueueInit();  /* 先创建红外时间数据队列，暂不启用红外接收中断 */
    vTaskStartScheduler();  /* 启动 FreeRTOS 调度器，正常运行时不返回 */
    StopOnError(5);  /* 错误码 5：调度器启动调用返回 */
    return 0;  /* StopOnError 内部为死循环，正常不会执行到这里 */
}
