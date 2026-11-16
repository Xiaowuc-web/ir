#ifndef IR_H
#define IR_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t g_irQueue;

/* 启动调度器前创建红外时间数据队列，不启用接收中断。 */
void IR_QueueInit(void);

#endif
