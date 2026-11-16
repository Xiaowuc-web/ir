#include "ir.h"

QueueHandle_t g_irQueue = NULL;

/* 创建红外时间数据队列，在启动调度器前调用，不启用接收中断。 */
void IR_QueueInit(void)
{
    if (g_irQueue == NULL)
    {
        g_irQueue = xQueueCreate(64, sizeof(uint16_t));
        configASSERT(g_irQueue != NULL);
    }
}


/* 初始化 TIM2，以 1 微秒为单位计时，用于测量红外边沿间隔。 */
static void IR_TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    RCC_ClocksTypeDef clocks;
    uint32_t timerClock;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    RCC_GetClocksFreq(&clocks);

    timerClock = clocks.PCLK1_Frequency;

    if (clocks.PCLK1_Frequency != clocks.HCLK_Frequency)
    {
        timerClock *= 2U;
    }

    TIM_TimeBaseStructInit(&TIM_InitStructure);

    TIM_InitStructure.TIM_Prescaler =
        timerClock / 1000000U - 1U;

    TIM_InitStructure.TIM_Period = 65535;

    TIM_InitStructure.TIM_CounterMode =
        TIM_CounterMode_Up;

    TIM_InitStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    TIM_SetCounter(TIM2, 0);

    TIM_Cmd(TIM2, ENABLE);
}

/* 初始化 PB10 红外输入及 EXTI10 下降沿中断，处理函数需另行实现。 */
static void IR_EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
        ENABLE
    );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_EXTILineConfig(
        GPIO_PortSourceGPIOB,
        GPIO_PinSource10
    );

    EXTI_InitStructure.EXTI_Line = EXTI_Line10;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    EXTI_ClearITPendingBit(EXTI_Line10);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
