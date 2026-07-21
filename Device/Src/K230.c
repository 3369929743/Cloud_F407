#include "K230.h"
#include <stdint.h>
#include "Serial.h"

#define K230_RX_BUFFER_SIZE 12

typedef uint16_t K230_BUFFSIZE_t;

static void K230_Rx_Event(Serial_t *Serial,uint16_t Size);

typedef struct {
    Serial_t *Serial;
    uint8_t BufferA[K230_RX_BUFFER_SIZE];
    uint8_t BufferB[K230_RX_BUFFER_SIZE];
    __IO K230_BUFFSIZE_t Size;
    uint8_t* __IO Processptr;
    uint8_t* __IO Readptr;
    __IO uint8_t BufferFlag;  //数据就绪标志
    int16_t Offset_x;
    int16_t Offset_y;
    int16_t Error_x;
    int16_t Error_y;
} K230_BUFF_t;

static K230_BUFF_t K230_Buffer ;

/**
 * @brief  初始化K230串口接收缓冲区及误差偏移量
 * @param  Serial: 串口对象指针
 * @param  Offset_x: X轴误差偏移量（用于校准补偿）
 * @param  Offset_y: Y轴误差偏移量（用于校准补偿）
 * @note   配置双缓冲区（BufferA和BufferB），初始化处理指针和读取指针，
 *         清零数据就绪标志，并启动串口空闲中断接收
 */
void K230_Init(Serial_t *Serial, int16_t Offset_x, int16_t Offset_y)
{
    // 保存X轴和Y轴的误差偏移量，用于后续数据校准
    K230_Buffer.Offset_x = Offset_x;
    K230_Buffer.Offset_y = Offset_y;
    // 保存串口对象指针
    K230_Buffer.Serial = Serial;
    // 初始化双缓冲区指针：Processptr指向BufferA（用于接收新数据），Readptr指向BufferB（用于读取处理）
    K230_Buffer.Processptr = K230_Buffer.BufferA;
    K230_Buffer.Readptr = K230_Buffer.BufferB;
    // 清零数据就绪标志
    K230_Buffer.BufferFlag = 0;
    // 启动串口空闲中断接收，数据将存入Processptr指向的缓冲区
    Serial_SetRxBuffer(Serial, K230_Buffer.Processptr, K230_RX_BUFFER_SIZE);
    Serial_ReceiveToIdle_IT(Serial);
    Serial->RxCallback = K230_Rx_Event;
}

/**
 * @brief  更新误差数据（解析K230接收到的数据帧）
 * @return uint8_t: 返回1表示误差数据更新成功，返回0表示数据无效或未更新
 * @note   数据帧格式：帧头(0xAA 0x55) + Error_x(2字节小端序) + Error_y(2字节小端序)
 *         解析后会自动加上预设的偏移量(Offset_x和Offset_y)
 */
uint8_t K230_Error_Update(void)
{
    if (K230_Buffer.Size == 6) {
        if(K230_Buffer.Readptr[0] == 0xAA && K230_Buffer.Readptr[1] == 0x55){
            K230_Buffer.Error_x = (int16_t)(K230_Buffer.Readptr[3] << 8 | K230_Buffer.Readptr[2]);
            K230_Buffer.Error_y = (int16_t)(K230_Buffer.Readptr[5] << 8 | K230_Buffer.Readptr[4]);
            K230_Buffer.Error_x += K230_Buffer.Offset_x;
            K230_Buffer.Error_y += K230_Buffer.Offset_y;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief 获取并清除K230数据就绪标志
 * @return uint8_t 返回标志值，1表示有新数据就绪，0表示无新数据
 * @note 该函数采用"读取后清零"机制，每次调用后标志位会被自动清除
 */
uint8_t K230_GetFlag(void)
{
    uint8_t Temp = K230_Buffer.BufferFlag;
    K230_Buffer.BufferFlag = 0;
    return Temp;
}

/**
 * @brief K230串口接收事件处理函数（在中断回调中调用）
 * @param Serial 串口对象指针，用于匹配对应的串口外设
 * @param Size 本次接收到的数据字节数
 * @note 该函数实现双缓冲区切换机制：
 *       1. 验证串口对象是否匹配，不匹配则直接返回
 *       2. 更新接收数据大小，将读取指针指向刚接收完的数据缓冲区
 *       3. 切换处理指针到另一个缓冲区（BufferA <-> BufferB交替）
 *       4. 设置数据就绪标志，通知上层有新数据可处理
 *       5. 重新启动空闲中断接收，继续接收下一帧数据
 */
static void K230_Rx_Event(Serial_t *Serial,uint16_t Size)
{
    if(Serial != K230_Buffer.Serial){
        return;
    }
    K230_Buffer.Size = Size; // 更新接收到的数据大小
    // 处理接收到的数据
    K230_Buffer.Readptr = K230_Buffer.Processptr; // 更新读取指针
    if (K230_Buffer.Processptr == K230_Buffer.BufferA) {
        K230_Buffer.Processptr = K230_Buffer.BufferB;
    }
    else {
        K230_Buffer.Processptr = K230_Buffer.BufferA;
    }
    K230_Buffer.BufferFlag = 1; // 设置数据就绪标志
    Serial_SetRxBuffer(Serial, K230_Buffer.Processptr, K230_RX_BUFFER_SIZE);
    Serial_ReceiveToIdle_IT(Serial); // 启动接收中断
}

/**
 * @brief 获取X轴误差值
 * @return int16_t 返回当前X轴误差值（已包含偏移量校正）
 */
int16_t K230_GetError_x(void)
{
    return K230_Buffer.Error_x;
}
/**
 * @brief 获取Y轴误差值
 * @return int16_t 返回当前Y轴误差值（已包含偏移量校正）
 */
int16_t K230_GetError_y(void)
{
    return K230_Buffer.Error_y;
}