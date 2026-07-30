#ifndef __TASK_BALL_CONTRAL_H__
#define __TASK_BALL_CONTRAL_H__

#include <stdint.h>

typedef enum{
    TASK_BALL_CONTRAL_IDLE = 0,
    TASK_BALL_CONTRAL_RUNNING,
    TASK_BALL_CONTRAL_LOST,
}Task_Ball_Contral_State_e;

typedef enum{
    TASK_BALL_SEQUENCE_OFF = 0,
    TASK_BALL_SEQUENCE_CENTER,
    TASK_BALL_SEQUENCE_POSITIVE_5CM,
    TASK_BALL_SEQUENCE_NEGATIVE_5CM,
    TASK_BALL_SEQUENCE_DONE,
} Task_Ball_Sequence_State_e;

void Task_Ball_Contral_Init(void);
void Task_Ball_Contral_Toggle(void);
void Task_Ball_Contral_Loop(void);
void Task_Ball_Contral_Tick(void);
void Task_Ball_Contral_Goto(float TargetCm);
uint8_t Task_Ball_Contral_Is_Arrived(void);
uint8_t Task_Ball_Contral_Set_Zero(void);
void Task_Ball_Contral_Start_Sequence(void);
Task_Ball_Sequence_State_e Task_Ball_Contral_Get_Sequence_State(void);
void Task_Ball_Contral_Pop_Init(void);
void Task_Ball_Contral_Pop_Ready(void);
void Task_Ball_Contral_Pop_Restore(void);

#endif
