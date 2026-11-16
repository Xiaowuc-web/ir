#include "ir.h"

QueueHandle_t g_irQueue = NULL;

/**
 * @brief  创建用于传递红外时间数据的 FreeRTOS 队列。
 * @param  无。
 * @return 无；创建成功后，g_irQueue 保存有效的队列句柄。
 *
 * 功能说明：
 *   1. 仅在 g_irQueue 为空时创建队列，避免顺序重复调用时重复分配。
 *   2. 队列最多保存 64 个 uint16_t 元素，每次入队会复制一个元素。
 *   3. uint16_t 元素拟用于保存边沿时间数据，单位需与后续接收逻辑统一。
 *
 * 调用时机：
 *   在 main 中启动调度器之前调用，且必须先于任何队列收发操作。
 *
 * 注意事项：
 *   使用 FreeRTOS 堆动态分配内存，实际占用还包括队列管理结构。
 *   当前配置下分配失败会进入内存失败钩子；断言也用于检查句柄。
 *   不能在中断中调用，也不保证多个任务并发初始化时的安全性。
 *   本函数不初始化 TIM2、PB10 或 EXTI，也不创建解码任务。
 */
void IR_QueueInit(void)
{
    if (g_irQueue == NULL)
    {
        g_irQueue = xQueueCreate(64, sizeof(uint16_t));
        configASSERT(g_irQueue != NULL);
    }
}

/**
 * @brief  将 TIM2 配置为自由运行的红外测时计数器，并启动计数。
 * @param  无。
 * @return 无。
 *
 * 功能说明：
 *   1. 开启 TIM2 时钟，根据 APB1 分频关系计算定时器输入时钟。
 *   2. 设置预分频，目标计数频率为 1 MHz，即每次计数约 1 微秒。
 *   3. 设置向上计数，计数范围为 0～65535，清零后启动 TIM2。
 *
 * 调用时机：
 *   系统时钟配置完成后、启用红外边沿中断之前调用。
 *   本函数为文件内部函数，由 IR_TimerInit 调用。
 *
 * 注意事项：
 *   输入时钟不低于 1 MHz、可被 1 MHz 整除且预分频值在有效范围内时，
 *   才能得到准确的 1 微秒计数间隔。
 *   在该计数频率下，每 65.536 ms 回绕一次；后续计算间隔时需处理回绕，
 *   单靠两个 16 位读数不能区分跨越多个回绕周期的长间隔。
 *   本函数会重配并清零 TIM2，不能与其他独立使用 TIM2 的功能直接共用。
 *   本函数不配置定时器中断，也不读取或保存红外边沿时间。
 */
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

/**
 * @brief  配置 PB10 红外接收输入，并启用 EXTI10 下降沿中断。
 * @param  无。
 * @return 无。
 *
 * 功能说明：
 *   1. 开启 GPIOB、AFIO 时钟，将原理图 M8 接口的 PB10 配置为上拉输入。
 *   2. 将 PB10 映射到 EXTI10，配置下降沿中断并清除已有挂起标志。
 *   3. 启用 EXTI15_10 共享中断通道，抢占优先级为 6，子优先级为 0。
 *
 * 调用时机：
 *   中断优先级分组、时间数据队列、测时计数器及中断处理函数准备完成后调用。
 *   本函数为文件内部函数，由 IR_Task 在调度器启动后调用。
 *
 * 注意事项：
 *   必须先实现 EXTI15_10_IRQHandler，处理 EXTI10 并清除对应挂起标志。
 *   EXTI10～15 共用中断入口；若使用其他线路，需要协调其处理逻辑。
 *   配合当前 PriorityGroup_4 和 FreeRTOS 优先级阈值 5，优先级 6 的中断
 *   可以使用允许在中断中调用的 FromISR 接口，不能调用普通阻塞接口。
 *   本函数只配置并启用接收中断，不测量脉宽、不发送队列，也不解码键码。
 */
static void IR_EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
        ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_EXTILineConfig(
        GPIO_PortSourceGPIOB,
        GPIO_PinSource10);

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

/**
 * @brief  初始化并启动红外测时定时器 TIM2。
 * @param  无。
 * @return 无。
 * @note   在创建红外任务前调用，不启用 PB10 接收中断。
 *         TIM2 目标计数周期为 1 微秒，每 65.536 ms 回绕一次。
 */
void IR_TimerInit(void)
{
    IR_TIM2_Init();
}

/**
 * @brief  启用红外接收中断，接收时间戳并计算相邻边沿间隔。
 * @param  argument：任务参数，当前未使用。
 * @return 不返回，任务持续等待队列数据。
 * @note   必须先初始化队列和 TIM2，由调度器启动本任务后再启用 EXTI。
 *         保留 interal 变量名，单位为微秒；首次计算以 pre 的初值 0 为基准，
 *         不能将首次计算结果当作有效的相邻边沿间隔。
 *         当前仅计算间隔，不解码键码；长间隔回绕及丢数据恢复尚未实现。
 */
void IR_Task(void *argument)
{
    uint16_t now, interal;
    uint16_t pre = 0;

    (void)argument;
    IR_EXTI_Init();

    for (;;)
    {
        if (xQueueReceive(g_irQueue, &now, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        interal = (uint16_t)(now - pre);
        pre = now;
        (void)interal;
    }
}