#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

/* Inspect these values in the debugger if an error hook is reached. */
volatile unsigned long g_rtosError;
volatile unsigned long g_rtosAssertLine;
const char * volatile g_rtosAssertFile;

static void StopOnError(unsigned long error)
{
    taskDISABLE_INTERRUPTS();
    g_rtosError = error;
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    StopOnError(1);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    StopOnError(2);
}

void vApplicationAssert(const char *file, unsigned long line)
{
    g_rtosAssertFile = file;
    g_rtosAssertLine = line;
    StopOnError(3);
}

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    vTaskStartScheduler();
    StopOnError(5);
    return 0;
}
