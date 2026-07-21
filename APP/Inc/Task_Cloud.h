#ifndef __TASK_CLOUD_H__
#define __TASK_CLOUD_H__

typedef enum{
    TASK_CLOUD_IDLE = 0,
    TASK_CLOUD_RUNNING,
    TASK_CLOUD_LOST,
}Task_Cloud_State_e;

void Task_Cloud_Init(void);
void Task_Cloud_Pith_Init(void);
void Task_Cloud_Toggle(void);
void Task_Cloud_Loop(void);
void Task_Cloud_Exit(void);
void Task_Cloud_Tick(void);

#endif
