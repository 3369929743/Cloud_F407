#ifndef __TIMER_H__
#define __TIMER_H__

#include "main.h"

#if !defined(HAL_TIM_MODULE_ENABLED)
    typedef void TIM_HandleTypeDef;
#endif

#define TIMER_LIST \
    X(1, TIM1)     \
    X(2, TIM2)     \
    X(3, TIM3)     \
    X(4, TIM4)     \
    X(5, TIM5)     \
    X(6, TIM6)     \
    X(7, TIM7)     \
    X(8, TIM8)     \
    X(9, TIM9)     \
    X(10, TIM10)     \
    X(11, TIM11)     \
    X(12, TIM12)     \
    X(13, TIM13)     \
    X(14, TIM14)     \

typedef enum{
    #define X(Index, Instance) Timer_##Index = Index,
    TIMER_LIST
    #undef X
} Timer_Num_e;

typedef struct Timer_Struct{
    TIM_HandleTypeDef *htim;

    void (*PeriodElapsedCallback)(struct Timer_Struct *timer);
} Timer_t;

uint8_t Timer_Init(Timer_t* timer, Timer_Num_e TIM_Num);
void Timer_Start_IT(Timer_t* timer, void(*Callback)(Timer_t *timer));

#endif