#include "Delay.h"

void Delay_us(uint32_t us)
{
    // 使用 static 变量标记是否已初始化，保证只执行一次
    static uint8_t is_init = 0;

    if (!is_init) {
        // 1. 使能调试跟踪模块 (必须步骤，否则 CYCCNT 不工作)
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        // 2. 清零并启动周期计数器
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        is_init = 1; // 标记为已初始化
    }

    // 计算目标计数值 (防止溢出，使用 64 位中间变量)
    // SystemCoreClock 是 CMSIS 标准变量，代表当前 CPU 主频
    uint32_t ticks = (uint64_t)us * SystemCoreClock / 1000000;

    uint32_t start = DWT->CYCCNT;

    // 等待直到经过的周期数达到目标值
    // 这种写法完美处理了计数器溢出归零的情况
    while ((DWT->CYCCNT - start) < ticks);
}