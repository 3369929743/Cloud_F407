#ifndef __TASK_BALL_CONTRAL_H__
#define __TASK_BALL_CONTRAL_H__

typedef enum{
    TASK_BALL_CONTRAL_IDLE = 0,
    TASK_BALL_CONTRAL_RUNNING,
    TASK_BALL_CONTRAL_LOST,
}Task_Ball_Contral_State_e;

void Task_Ball_Contral_Init(void);
void Task_Ball_Contral_Toggle(void);
void Task_Ball_Contral_Loop(void);
void Task_Ball_Contral_Tick(void);
void Task_Ball_Contral_Pop_Init(void);
void Task_Ball_Contral_Pop_Ready(void);
void Task_Ball_Contral_Pop_Restore(void);

#endif