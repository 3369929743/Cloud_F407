/**
 ******************************************************************************
 * @file           : DO_Device.c
 * @brief          : 数字输出设备控制模块，支持常开、常闭、闪烁及闪烁次数控制
 ******************************************************************************
 * @使用方法
 * 
 * 1. 定义数字输出设备对象和引脚写入函数
 *    DO_Device_t MyDevice;
 *    void MyDevice_Write_Pin(uint8_t Level) {
 *        HAL_GPIO_WritePin(GPIOx, GPIO_PIN_x, Level ? GPIO_PIN_SET : GPIO_PIN_RESET);
 *    }
 * 
 * 2. 初始化数字输出设备对象
 *    DO_Device_Init(&MyDevice, DO_LEVEL_HIGH, MyDevice_Write_Pin);
 *    // DO_LEVEL_HIGH表示高电平开启，DO_LEVEL_LOW表示低电平开启
 * 
 * 3. 控制设备状态
 *    DO_Device_ON(&MyDevice);                                // 开启设备常开
 *    DO_Device_OFF(&MyDevice);                               // 关闭设备
 *    DO_Device_BLINK(&MyDevice, 100, 100, 5);               // 闪烁5次，开100ms，关100ms
 *    DO_Device_BLINK(&MyDevice, 100, 100, DO_BLINK_FOREVER);  // 无限循环闪烁
 * 
 * 4. 在定时器中断中周期性调用DO_Device_Tick维持闪烁
 *    void Timer_Callback(Timer_t *Timer) {
 *        DO_Device_Tick(&MyDevice);
 *    }
 * 
 * 完整示例（参考main.c）:
 *    // 定义数字输出设备实例
 *    DO_Device_t User_LED;
 *    DO_Device_t User_Buz;
 *    
 *    // 定义引脚写入函数
 *    void LED_Write_Pin(uint8_t Level) {
 *        HAL_GPIO_WritePin(User_LED_GPIO_Port, User_LED_Pin, Level ? GPIO_PIN_SET : GPIO_PIN_RESET);
 *    }
 *    
 *    void Buz_Write_Pin(uint8_t Level) {
 *        HAL_GPIO_WritePin(GPIO_BUZ_GPIO_Port, GPIO_BUZ_Pin, Level ? GPIO_PIN_SET : GPIO_PIN_RESET);
 *    }
 *    
 *    // 在初始化代码中
 *    DO_Device_Init(&User_LED, DO_LEVEL_HIGH, LED_Write_Pin);
 *    DO_Device_Init(&User_Buz, DO_LEVEL_HIGH, Buz_Write_Pin);
 *    
 *    // 在定时器回调中更新设备状态（维持闪烁）
 *    void Timer_TickCallback(Timer_t *Timer) {
 *        DO_Device_Tick(&User_LED);
 *        DO_Device_Tick(&User_Buz);
 *    }
 *    
 *    // 在事件回调中控制设备
 *    void Key_Global_Callback(Key_t *Key, Key_Event_e Event) {
 *        switch(Event) {
 *            case KEY_EVENT_ONCE_PRESS:
 *                DO_Device_ON(&User_LED);      // 单击开启
 *                DO_Device_ON(&User_Buz);
 *                break;
 *            case KEY_EVENT_DOUBLE_PRESS:
 *                DO_Device_OFF(&User_LED);     // 双击关闭
 *                DO_Device_OFF(&User_Buz);
 *                break;
 *            case KEY_EVENT_LONG_PRESS:
 *                DO_Device_BLINK(&User_LED, 100, 100, 5);  // 长按闪烁5次
 *                DO_Device_BLINK(&User_Buz, 100, 100, 5);
 *                break;
 *            default:
 *                break;
 *        }
 *    }
 * 
 * @note   - DO_Device_Tick()需在定时器中断中周期性调用，默认周期为1ms
 *         - DO_BLINK_FOREVER宏定义表示无限循环闪烁
 *         - 闪烁时间参数单位为毫秒，会自动转换为Tick次数
 *         - DO_Write_Pin函数由用户实现，用于适配不同硬件引脚
 *         - 调用DO_Device_ON或DO_Device_OFF会自动停止闪烁状态
 *         - 该模块适用于LED、蜂鸣器、继电器等数字输出设备
 ******************************************************************************
 */
#include "DO_Device.h"

#include <stddef.h>

#define TICK_TIME 1 // 每次调用的间隔时间（定时器中断周期）

/**
 * @brief  初始化数字输出设备对象，配置默认电平和引脚写入函数
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @param  Init_Level: 设备初始电平状态（DO_LEVEL_HIGH或DO_LEVEL_LOW）
 * @param  DO_Write_Pin: 引脚写入函数指针，用于控制设备引脚电平，函数签名为 void(*)(uint8_t Level)
 * @retval 无
 * @note   该函数会清空设备状态并调用DO_Device_OFF确保设备初始为关闭状态
 */
void DO_Device_Init(DO_Device_t *Device, uint8_t Init_Level, void (*DO_Write_Pin)(uint8_t Level)){
    if(Device == NULL) return;
    Device->Default_Level = Init_Level;
    Device->DO_Write_Pin = DO_Write_Pin;
    Device->is_On = 0;
    Device->is_Blinking = 0;
    Device->Tick_Time = 0;

    DO_Device_OFF(Device);
}

/**
 * @brief  向数字输出设备引脚写入电平状态（内联函数）
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @param  Level: 要写入的逻辑电平（0表示关闭，1表示开启）
 * @retval 无
 * @note   该函数会根据设备的默认电平状态进行取反处理，
 *         确保逻辑电平与物理电平的映射正确（高/低电平触发）
 */
static inline void DO_Device_Write(DO_Device_t *Device, uint8_t Level){
    if(Device == NULL || Device->DO_Write_Pin == NULL) return;
    uint8_t Out_Level = Level ? !Device->Default_Level : Device->Default_Level;
    Device->DO_Write_Pin(Out_Level);
}

/**
 * @brief  开启数字输出设备，关闭闪烁状态并设置为常开
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @retval 无
 * @note   该函数会停止设备闪烁（如果正在闪烁），并将设备设置为常开状态
 */
void DO_Device_ON(DO_Device_t *Device){
    if(Device == NULL) return;
    Device->is_Blinking = 0;
    Device->is_On = 1;
    DO_Device_Write(Device, DO_STATE_ON);
}

/**
 * @brief  关闭数字输出设备，关闭闪烁状态并设置为关闭
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @retval 无
 * @note   该函数会停止设备闪烁（如果正在闪烁），并将设备设置为关闭状态
 */
void DO_Device_OFF(DO_Device_t *Device){
    if(Device == NULL) return;
    Device->is_Blinking = 0;
    Device->is_On = 0; 
    DO_Device_Write(Device, DO_STATE_OFF); 
}

/**
 * @brief  设置数字输出设备闪烁模式（间歇输出）
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @param  On_Time: 设备开启时间（单位：毫秒）
 * @param  Off_Time: 设备关闭时间（单位：毫秒）
 * @param  Count: 闪烁次数，设置为DO_BLINK_FOREVER表示无限循环闪烁
 * @retval 无
 * @note   该函数会启动设备间歇输出模式，需在定时器中断中周期性调用DO_Device_Tick()以维持闪烁
 *         时间参数会自动根据TICK_TIME转换为Tick次数
 */
void DO_Device_BLINK(DO_Device_t *Device, uint16_t On_Time, uint16_t Off_Time, uint16_t Count){
    if(Device == NULL) return;
    if(Count == 0) return;
    Device->On_Time = On_Time / TICK_TIME;
    Device->Off_Time = Off_Time / TICK_TIME;
    Device->Blink_Count = Count;
    Device->is_Blinking = 1;
    Device->is_On = 1;
    Device->Tick_Time = 0;
    DO_Device_Write(Device, DO_STATE_ON);
}

/**
 * @brief  数字输出设备闪烁状态机处理函数（需在定时器中断中周期性调用）
 * @param  Device: 指向数字输出设备对象结构体的指针
 * @retval 无
 * @note   该函数实现设备闪烁状态机，根据当前状态（开启/关闭）进行计时和状态切换：
 *         - 关闭状态（DO_STATE_OFF）：计时达到Off_Time后，减少闪烁次数，若次数为0则停止闪烁，否则切换为开启
 *         - 开启状态（DO_STATE_ON）：计时达到On_Time后，切换为关闭状态
 *         需在定时器中断中周期性调用（默认周期1ms）以维持闪烁效果
 */
void DO_Device_Tick(DO_Device_t *Device){
    if(Device == NULL) return;
    if(!Device->is_Blinking) return;
    switch(Device->is_On){
        case DO_STATE_OFF:
            Device->Tick_Time ++;
            if(Device->Tick_Time >= Device->Off_Time){
                Device->Tick_Time = 0;
                if(Device->Blink_Count != DO_BLINK_FOREVER){
                    Device->Blink_Count --;
                }
                 if(Device->Blink_Count == 0){
                    Device->is_Blinking = 0;
                    Device->is_On = 0;
                }
                else{
                    Device->is_On = 1;
                    DO_Device_Write(Device, DO_STATE_ON);
                }
            }
            break;
        case DO_STATE_ON:
            Device->Tick_Time ++;
            if(Device->Tick_Time >= Device->On_Time){
                Device->Tick_Time = 0;
                Device->is_On = 0;
                DO_Device_Write(Device, DO_STATE_OFF);
            }
            break;
        default:
            break;
    }
}

uint8_t DO_Device_GetState(DO_Device_t *Device){
    if(Device == NULL) return 0;
    return Device->is_On;
}
