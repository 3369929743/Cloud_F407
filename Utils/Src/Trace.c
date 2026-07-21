#include "Trace.h"

#include <stddef.h>

/**
 * @brief  计算绝对值（内联函数）
 * @param  Value: 输入的16位有符号整数
 * @retval 绝对值
 */
static inline int16_t Trace_Abs(int16_t Value)
{
    return (Value < 0) ? (int16_t)-Value : Value;
}

/**
 * @brief  将值限制在指定范围内（内联函数）
 * @param  Value: 待限制的32位有符号整数
 * @param  Limit: 限制值的绝对值上限
 * @retval 限制后的值，范围在[-Limit, Limit]之间
 */
static inline int16_t Trace_Clamp(int32_t Value, int16_t Limit)
{
    if (Value > Limit) {
        return Limit;
    }
    if (Value < -Limit) {
        return (int16_t)-Limit;
    }
    return (int16_t)Value;
}

/**
 * @brief  四舍五入除法（内联函数）
 * @param  Sum: 被除数（16位有符号整数）
 * @param  Count: 除数（8位无符号整数）
 * @retval 四舍五入后的除法结果
 */
static inline int16_t Trace_DivideRounded(int16_t Sum, uint8_t Count)
{
    int16_t Half = (int16_t)(Count / 2U);

    if (Sum < 0) {
        return (int16_t)((Sum - Half) / (int16_t)Count);
    }
    return (int16_t)((Sum + Half) / (int16_t)Count);
}

/**
 * @brief  获取循迹模块的默认配置参数
 * @param  Config: 指向配置结构体的指针，用于存储默认配置
 * @retval 无
 */
void Trace_GetDefaultConfig(Trace_Config_t *Config)
{
    static const int8_t Default_Weight[TRACE_SENSOR_NUM] = {
        -54, -38, -24, -10, 10, 24, 38, 54
    };

    if (Config == NULL) {
        return;
    }

    for (uint8_t i = 0U; i < TRACE_SENSOR_NUM; ++i) {
        Config->Weight[i] = Default_Weight[i];
    }
    Config->Lost_Error = 80;
    Config->Filter_Alpha = 0.75f;
    Config->Lost_Deadband = 3U;
}

/**
 * @brief  重置循迹模块的所有状态变量
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 无
 */
void Trace_Reset(Trace_t *Trace)
{
    if (Trace == NULL) {
        return;
    }

    Trace->Cur_Error = 0;
    Trace->Pre_Error = 0;
    Trace->Raw_Error = 0;
    Trace->Pin_Data = 0U;
    Trace->Active_Count = 0U;
    Trace->Lost_Count = 0U;
    Trace->Status = TRACE_STATUS_LOST_UNKNOWN;
    Trace->Has_Valid_Error = 0U;
}

/**
 * @brief  初始化循迹模块，加载配置参数
 * @param  Trace: 指向循迹模块实例的指针
 * @param  Config: 指向配置结构体的指针，传NULL则使用内置默认配置
 * @retval 无
 */
void Trace_Init(Trace_t *Trace, const Trace_Config_t *Config)
{
    Trace_Config_t Default_Config;
    Trace_GetDefaultConfig(&Default_Config);
    const Trace_Config_t *Source = Config;

    if (Trace == NULL) {
        return;
    }
    if (Source == NULL) {
        Source = &Default_Config;
    }

    for (uint8_t i = 0U; i < TRACE_SENSOR_NUM; ++i) {
        Trace->Config.Weight[i] = Source->Weight[i];
    }
    Trace->Config.Lost_Error = (Source->Lost_Error > 0) ? Source->Lost_Error : Default_Config.Lost_Error;

    float Input_Alpha = Source->Filter_Alpha;
    if(Input_Alpha <= 0){
        Input_Alpha = Default_Config.Filter_Alpha;
    }
    uint16_t Filter_Alpha = (uint16_t)(Input_Alpha * TRACE_FILTER_ALPHA_MAX + 0.5f);

    if(Filter_Alpha > TRACE_FILTER_ALPHA_MAX){
        Filter_Alpha = TRACE_FILTER_ALPHA_MAX;
    }
    else if(Filter_Alpha == 0){
        Filter_Alpha = 1;
    }
    Trace->Config.Filter_Alpha = Filter_Alpha;

    Trace->Config.Lost_Deadband = (Source->Lost_Deadband > 0) ? Source->Lost_Deadband : Default_Config.Lost_Deadband;

    Trace_Reset(Trace);
}

/**
 * @brief  处理丢失黑线情况（内部函数）
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 恢复搜索时的误差值
 */
static int16_t Trace_HandleLostLine(Trace_t *Trace)
{
    int16_t Direction_Error;
    int32_t Recovery_Step;
    int32_t Recovery_Error;

    if (Trace->Lost_Count < UINT16_MAX) {
        ++Trace->Lost_Count;
    }

    Direction_Error = Trace->Cur_Error;
    if ((!Trace->Has_Valid_Error) ||
        (Trace_Abs(Direction_Error) <= (int16_t)Trace->Config.Lost_Deadband)) {
        Trace->Status = TRACE_STATUS_LOST_UNKNOWN;
        Trace->Pre_Error = Trace->Cur_Error;
        Trace->Cur_Error = 0;
        return 0;
    }

    /* 保持最近的趋势，设置最小步长使稳定误差也能继续搜索 */
    Recovery_Step = (int32_t)Trace->Cur_Error - Trace->Pre_Error;
    if (Direction_Error < 0) {
        Recovery_Step = -Recovery_Step;
    }
    if (Recovery_Step < 4) {
        Recovery_Step = 4;
    }
    if (Recovery_Step > Trace->Config.Lost_Error) {
        Recovery_Step = Trace->Config.Lost_Error;
    }
    Recovery_Error = (Direction_Error < 0) ?
                     (int32_t)Direction_Error - Recovery_Step :
                     (int32_t)Direction_Error + Recovery_Step;

    Trace->Status = (Direction_Error < 0) ?
                    TRACE_STATUS_LOST_LEFT : TRACE_STATUS_LOST_RIGHT;
    Trace->Pre_Error = Trace->Cur_Error;
    Trace->Cur_Error = Trace_Clamp(Recovery_Error, Trace->Config.Lost_Error);
    return Trace->Cur_Error;
}

/**
 * @brief  更新循迹传感器数据并计算误差值
 * @param  Trace: 指向循迹模块实例的指针
 * @param  Pin_Data: 传感器引脚数据，每一位代表一个传感器的检测状态
 * @retval 当前计算得到的误差值（经过滤波处理）
 */
int16_t Trace_Update(Trace_t *Trace)
{
    int16_t Weight_Sum = 0;
    int16_t Filtered_Error;

    if (Trace == NULL) {
        return 0;
    }
    uint8_t Temp_Pin_Data = Trace->Get_Pin_Data();
    Trace->Pin_Data = Temp_Pin_Data;
    Trace->Active_Count = 0;
    for (uint8_t i = 0; i < TRACE_SENSOR_NUM; ++i) {
        if ((Temp_Pin_Data & (uint8_t)(1U << i)) != 0U) {
            Weight_Sum = (int16_t)(Weight_Sum + Trace->Config.Weight[i]);
            ++Trace->Active_Count;
        }
    }

    if (Trace->Active_Count == 0U) {
        return Trace_HandleLostLine(Trace);
    }

    Trace->Raw_Error = Trace_DivideRounded(Weight_Sum, Trace->Active_Count);
    Trace->Lost_Count = 0U;
    Trace->Status = (Trace->Active_Count == TRACE_SENSOR_NUM) ?
                    TRACE_STATUS_CROSS_LINE : TRACE_STATUS_TRACKING;

    Trace->Pre_Error = Trace->Cur_Error;
    if (!Trace->Has_Valid_Error) {
        Filtered_Error = Trace->Raw_Error;
        Trace->Has_Valid_Error = 1U;
    } else {
        int32_t Delta = (int32_t)Trace->Raw_Error - Trace->Cur_Error;
        int32_t Rounded = (Delta >= 0) ?
                          (int32_t)(TRACE_FILTER_ALPHA_MAX / 2U) :
                          -(int32_t)(TRACE_FILTER_ALPHA_MAX / 2U);
        Filtered_Error = (int16_t)(Trace->Cur_Error +
                         (Delta * Trace->Config.Filter_Alpha + Rounded) /
                         (int32_t)TRACE_FILTER_ALPHA_MAX);
    }
    Trace->Cur_Error = Trace_Clamp(Filtered_Error, Trace->Config.Lost_Error);
    return Trace->Cur_Error;
}

/**
 * @brief  获取当前滤波后的误差值
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 当前误差值，如果Trace为NULL则返回0
 */
int16_t Trace_GetError(const Trace_t *Trace)
{
    return (Trace == NULL) ? 0 : Trace->Cur_Error;
}

/**
 * @brief  获取当前原始误差值（未经滤波处理）
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 原始误差值，如果Trace为NULL则返回0
 */
int16_t Trace_GetRawError(const Trace_t *Trace)
{
    return (Trace == NULL) ? 0 : Trace->Raw_Error;
}

/**
 * @brief  获取当前循迹状态
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 循迹状态枚举值，如果Trace为NULL则返回TRACE_STATUS_LOST_UNKNOWN
 */
Trace_Status_t Trace_GetStatus(const Trace_t *Trace)
{
    return (Trace == NULL) ? TRACE_STATUS_LOST_UNKNOWN : Trace->Status;
}

/**
 * @brief  获取当前检测到黑线的传感器数量
 * @param  Trace: 指向循迹模块实例的指针
 * @retval 激活的传感器数量，如果Trace为NULL则返回0
 */
uint8_t Trace_GetActiveCount(const Trace_t *Trace)
{
    return (Trace == NULL) ? 0U : Trace->Active_Count;
}

void Trace_SetDataCallback(Trace_t *Trace, uint8_t (*Get_Pin_Data)(void))
{
    if (Trace == NULL) {
        return;
    }
    Trace->Get_Pin_Data = Get_Pin_Data;
}