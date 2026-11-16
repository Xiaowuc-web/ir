/**
 * @file   ir.h
 * @brief  红外接收模块的公共接口。
 *
 * 硬件连接：M8 红外接收模块，信号引脚为 PB10。
 * 当前仅提供时间数据队列初始化接口，尚未提供完整的红外解码接口。
 * IR_TIM2_Init 和 IR_EXTI_Init 是 ir.c 内部的 static 函数，不在此声明。
 */
#ifndef IR_H
#define IR_H

#include <stdint.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  红外时间数据队列句柄，实体在 ir.c 中定义。
 * @note   初始值为 NULL，必须先调用 IR_QueueInit 再进行队列收发。
 *         每个元素的类型为 uint16_t，收发缓冲区应使用对应类型。
 *         任务中使用普通队列接口，中断中只能使用相应的 FromISR 接口。
 *         时间单位由后续测时逻辑约定；此队列不是解码后的按键队列。
 *         外部代码不应自行覆盖该句柄或删除队列，以免其他使用者访问失效对象。
 */
extern QueueHandle_t g_irQueue;

/**
 * @brief  创建用于传递红外时间数据的 FreeRTOS 队列。
 * @param  无。
 * @return 无；初始化成功后，g_irQueue 保存有效的队列句柄。
 *
 * 功能说明：
 *   队列容量为 64 个 uint16_t 元素，每个元素按值复制。
 *   仅在句柄为 NULL 时创建，顺序重复调用不会重复分配内存。
 *
 * 调用时机：
 *   在 main 中启动调度器之前调用，并先于所有队列收发操作。
 *   本函数不能在中断中调用，也不能由多个任务同时调用进行初始化。
 *
 * 失败处理：
 *   队列使用 FreeRTOS 堆动态分配，除数据区外还需占用队列管理内存。
 *   当前配置下，分配失败会进入内存失败钩子；configASSERT 也会检查句柄。
 *   本函数不通过返回值报告错误，不应将其当作可返回错误码的接口使用。
 *
 * 注意事项：
 *   本函数不会初始化或启动 TIM2、PB10、EXTI，也不会创建接收或解码任务。
 *   仅调用本函数，尚不能接收遥控器信号或获取按键键码。
 */
void IR_QueueInit(void);
void IR_TimerInit(void);
#ifdef __cplusplus
}
#endif

#endif /* IR_H */
