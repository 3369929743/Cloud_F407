#include "Manage.h"
#include "Task_Cloud.h"

typedef struct {
    void (*Init)(void);
    void (*Loop)(void);
    void (*Exit)(void);
} Manage_t;


static Manage_t Manage_List[]={
    // {Task_1_Init(), Task_1_Loop(), Task_1_Exit()}
    {Task_Cloud_Init, Task_Cloud_Loop, Task_Cloud_Exit},
};

typedef struct {
    uint8_t CurMode;
    uint8_t NextMode;
}Mode_t;

static Mode_t Mode = {0, 0};

#define MANAGE_LIST_SIZE (sizeof(Manage_List) / sizeof(Manage_t))

/**
  * @brief  任务管理循环
  * @param  无
  * @retval 无
  * @note   状态机模式: 相同模式执行Loop，模式切换时执行Exit和Init
  */
void Manage_Loop(void)
{
    /* 模式未变: 执行当前任务循环 */
    if(Mode.CurMode == Mode.NextMode)
    {
        Manage_List[Mode.CurMode].Loop();
    }
    /* 模式切换: 退出旧任务，初始化新任务 */
    else
    {
        Manage_List[Mode.CurMode].Exit();
        Manage_List[Mode.NextMode].Init();
        Mode.CurMode = Mode.NextMode;
    }
}