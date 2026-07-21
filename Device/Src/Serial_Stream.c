/**
  ******************************************************************************
  * @file    Serial_Stream.c
  * @brief   串口流数据管理模块（基于空闲中断 + FIFO）
  * @note    使用方法示例：
  * 
  * @code
  * // 1. 定义串口、FIFO和串口流对象及相关缓冲区
  * Serial_t Serial_Blue;                          // 串口对象
  * FIFO_t FIFO_Blue;                              // FIFO对象
  * Serial_Stream_t Serial_Stream_Blue;            // 串口流对象
  * uint8_t Blue_FIFO_Buff[128];                   // FIFO缓冲区
  * uint8_t Blue_RxTemp_Buff[64];                  // 接收临时缓冲区
  * uint8_t Blue_User_Buff[64];                    // 用户数据缓冲区
  * 
  * // 2. 配置串口流参数
  * Serial_Stream_Confg_t Serial_Stream_Blue_Confg = {
  *   .Serial = &Serial_Blue,                      // 绑定串口对象
  *   .FIFO = &FIFO_Blue,                          // 绑定FIFO对象
  *   .FIFO_Buff = Blue_FIFO_Buff,                 // FIFO缓冲区指针
  *   .Rx_Temp_Buff = Blue_RxTemp_Buff,            // 接收临时缓冲区指针
  *   .Serial_Num = Serial_2,                      // 串口号（如Serial_1/Serial_2/Serial_3）
  *   .FIFO_Size = sizeof(Blue_FIFO_Buff),         // FIFO缓冲区大小
  *   .Rx_Temp_Buff_Size = sizeof(Blue_RxTemp_Buff), // 接收临时缓冲区大小
  * };
  * 
  * // 3. 初始化串口流（在main函数的初始化阶段调用）
  * Serial_Stream_Init(&Serial_Stream_Blue, &Serial_Stream_Blue_Confg);
  * 
  * // 4. 在主循环中读取数据
  * while(1) {
  *   // 方式一：读取单个字节
  *   uint8_t Byte;
  *   if(Serial_Stream_ReadByte(&Serial_Stream_Blue, &Byte)) {
  *     // 处理读取到的字节数据
  *   }
  *   
  *   // 方式二：读取多个字节（推荐）
  *   if(Serial_Stream_ReadArray(&Serial_Stream_Blue, Blue_User_Buff, sizeof(Blue_User_Buff))) {
  *     // 处理读取到的数组数据，Blue_User_Buff中即为接收到的数据
  *   }
  * }
  * @endcode
  * 
  * @note  注意事项：
  *        1. 该模块基于串口空闲中断实现不定长数据接收
  *        2. 数据通过FIFO队列缓存，避免数据丢失
  *        3. Serial_Stream_Flag标志位表示有新数据到达，读取后自动清除
  *        4. 确保在调用Serial_Stream_Init前已配置好相关缓冲区
  ******************************************************************************
  */

#include "Serial_Stream.h"
#include <stdint.h>

/**
  * @brief  串口接收回调函数（空闲中断触发）
  * @param  Serial: 串口对象指针
  * @param  Size: 本次接收到的数据字节数
  * @retval 无
  * @note   当串口空闲中断触发时调用此函数，表示一帧数据接收完成。
  *         该函数会将接收到的数据存入FIFO，设置数据就绪标志，
  *         并重新启动串口接收以便接收下一帧数据。
  */
static void Serial_Stream_RxCallback(Serial_t *Serial, uint16_t Size){
    // 从串口对象的User_Data中获取Serial_Stream实例
    Serial_Stream_t *Serial_Stream = (Serial_Stream_t *)Serial->User_Data;
    
    // 将接收到的临时缓冲区数据写入FIFO队列
    FIFO_EnterArray(Serial_Stream->FIFO, Serial_Stream->Rx_Temp_Buff, Size);
    
    // 设置数据接收完成标志位，通知主程序有新数据可读
    Serial_Stream->Serial_Stream_Flag = 1;
    
    // 重新启动串口空闲中断接收模式，准备接收下一帧数据
    Serial_ReceiveToIdle_IT(Serial_Stream->Serial);
}

/**
  * @brief  串口流初始化函数
  * @param  Serial_Stream: 串口流对象指针
  * @param  Serial_Stream_Confg: 串口流配置结构体指针
  * @retval 无
  * @note   该函数完成以下工作：
  *         1. 将配置参数赋值给串口流对象
  *         2. 初始化FIFO队列和串口外设
  *         3. 绑定串口流对象到串口User_Data，供回调函数使用
  *         4. 设置接收缓冲区和回调函数
  *         5. 启动串口空闲中断接收模式
  */
void Serial_Stream_Init(Serial_Stream_t *Serial_Stream, Serial_Stream_Confg_t *Serial_Stream_Confg){
    // 参数有效性检查
    if(!Serial_Stream || !Serial_Stream_Confg) return;
    
    // 将配置结构体中的参数赋值给串口流对象
    Serial_Stream->Serial = Serial_Stream_Confg->Serial;
    Serial_Stream->FIFO = Serial_Stream_Confg->FIFO;
    Serial_Stream->Rx_Temp_Buff = Serial_Stream_Confg->Rx_Temp_Buff;
    Serial_Stream->Rx_Size = Serial_Stream_Confg->Rx_Temp_Buff_Size;
    Serial_Stream->Serial_Num = Serial_Stream_Confg->Serial_Num;

    // 初始化FIFO队列和串口外设
    FIFO_Init(Serial_Stream->FIFO, Serial_Stream_Confg->FIFO_Buff, Serial_Stream_Confg->FIFO_Size);
    Serial_Init(Serial_Stream->Serial, Serial_Stream->Serial_Num);

    // 将串口流对象指针绑定到串口User_Data，供回调函数获取上下文
    Serial_Stream->Serial->User_Data = (void *)Serial_Stream;
    
    // 设置串口接收缓冲区和接收回调函数
    Serial_SetRxBuffer(Serial_Stream->Serial, Serial_Stream->Rx_Temp_Buff, Serial_Stream->Rx_Size);
    Serial_SetRxCallback(Serial_Stream->Serial, Serial_Stream_RxCallback);
    
    // 启动串口空闲中断接收模式，开始接收数据
    Serial_ReceiveToIdle_IT(Serial_Stream->Serial);
}

/**
  * @brief  从串口流中读取一个字节数据
  * @param  Serial_Stream: 串口流对象指针
  * @param  Byte: 用于存储读取数据的指针
  * @retval 1: 读取成功, 0: 读取失败（无新数据或参数无效）
  * @note   该函数会检查数据就绪标志，读取成功后自动清除标志位
  */
uint8_t Serial_Stream_ReadByte(Serial_Stream_t *Serial_Stream, uint8_t *Byte){
    // 参数有效性检查
    if(!Serial_Stream || !Byte) return 0;
    
    // 检查是否有新数据到达
    if(!Serial_Stream->Serial_Stream_Flag) return 0;
    
    // 清除数据就绪标志
    Serial_Stream->Serial_Stream_Flag = 0;
    
    // 从FIFO队列中读取一个字节
    return FIFO_Exit(Serial_Stream->FIFO, Byte);
}

/**
  * @brief  从串口流中读取多个字节数据
  * @param  Serial_Stream: 串口流对象指针
  * @param  Array: 用于存储读取数据的数组指针
  * @param  Size: 期望读取的数据字节数
  * @retval 实际读取的数据字节数, 0: 读取失败（无新数据或参数无效）
  * @note   该函数会检查数据就绪标志，读取成功后自动清除标志位，
  *         实际读取的字节数可能小于请求的Size（当FIFO中数据不足时）
  */
uint16_t Serial_Stream_ReadArray(Serial_Stream_t *Serial_Stream, uint8_t *Array, uint16_t Size){
    // 参数有效性检查
    if(!Serial_Stream || !Array || Size == 0) return 0;
    
    // 检查是否有新数据到达
    if(!Serial_Stream->Serial_Stream_Flag) return 0;
    
    // 清除数据就绪标志
    Serial_Stream->Serial_Stream_Flag = 0;
    
    // 从FIFO队列中读取多个字节到数组
    return FIFO_ExitArray(Serial_Stream->FIFO, Array, Size);
}