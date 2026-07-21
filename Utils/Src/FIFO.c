#include "FIFO.h"
#include <stdint.h>

/**
 * @brief  初始化FIFO环形缓冲区
 * @param  FIFO: FIFO结构体指针
 * @param  Data: 缓冲区数据数组指针
 * @param  Size: 缓冲区大小（必须为2的幂次方，如2、4、8、16、32等）
 * @retval None
 * @note   使用2的幂次方大小可以利用位运算优化取模操作，提高读写效率
 */
void FIFO_Init(FIFO_t *FIFO, uint8_t *Data, uint16_t Size){
    /* 参数检查：指针非空且Size必须为2的幂次方 */
    if(!FIFO || !Data || (Size & ( Size -1)) != 0) return;
    
    FIFO->Data = Data;          /* 绑定数据缓冲区 */
    FIFO->Size = Size;          /* 记录缓冲区大小 */
    FIFO->Mask = Size - 1;      /* 计算掩码，用于位运算替代取模运算 */
    FIFO->Head = 0;             /* 初始化写指针（队尾） */
    FIFO->Tail = 0;             /* 初始化读指针（队头） */
}

/**
 * @brief  向FIFO缓冲区写入一个字节数据
 * @param  FIFO: FIFO结构体指针
 * @param  Data: 要写入的数据
 * @retval None
 * @note   如果缓冲区已满，则丢弃该数据
 */
void FIFO_Enter(FIFO_t *FIFO, uint8_t Data){
    if(!FIFO) return;                                   /* 指针检查 */
    if(FIFO->Head - FIFO->Tail >= FIFO->Size) return;   /* 缓冲区已满，丢弃数据 */
    FIFO->Data[FIFO->Head & FIFO->Mask] = Data;         /* 使用掩码实现环形写入 */
    FIFO->Head++;                                       /* 写指针递增 */
}

/**
 * @brief  清空FIFO缓冲区
 * @param  FIFO: FIFO结构体指针
 * @retval None
 * @note   将读写指针重置为0，缓冲区中的数据不会被清除，但会被视为无效
 */
void FIFO_Clear(FIFO_t *FIFO){
    if(!FIFO) return;       /* 指针检查 */
    FIFO->Head = 0;         /* 重置写指针 */
    FIFO->Tail = 0;         /* 重置读指针 */
}

/**
 * @brief  向FIFO缓冲区批量写入数据
 * @param  FIFO: FIFO结构体指针
 * @param  Data: 要写入的数据数组指针
 * @param  Size: 要写入的数据长度
 * @retval 实际成功写入的数据字节数
 * @note   如果缓冲区空间不足，则写入部分数据后提前退出
 */
uint16_t FIFO_EnterArray(FIFO_t *FIFO, uint8_t *Data, uint16_t Size){
    if(!FIFO || !Data) return 0;              /* 指针检查 */
    uint16_t i = 0;
    for(i = 0; i < Size; i++){
        if(FIFO->Head - FIFO->Tail >= FIFO->Size) break;  /* 缓冲区已满，停止写入 */
        FIFO->Data[FIFO->Head & FIFO->Mask] = Data[i];    /* 环形写入数据 */
        FIFO->Head++;                                     /* 写指针递增 */
    }
    return i;  /* 返回实际写入的字节数 */
}

/**
 * @brief  从FIFO缓冲区读取一个字节数据
 * @param  FIFO: FIFO结构体指针
 * @param  Data: 用于存储读取数据的指针
 * @retval 1: 读取成功, 0: 读取失败（缓冲区为空或参数无效）
 */
uint8_t FIFO_Exit(FIFO_t *FIFO, uint8_t *Data){
    if(!FIFO || !Data || FIFO->Tail == FIFO->Head) return 0;  /* 参数检查或缓冲区为空 */
    *Data = FIFO->Data[FIFO->Tail & FIFO->Mask];              /* 环形读取数据 */
    FIFO->Tail++;                                             /* 读指针递增 */
    return 1;                                                 /* 读取成功 */
}

uint8_t FIFO_ExitArray(FIFO_t *FIFO, uint8_t *Data, uint16_t Size){
    if(!FIFO || !Data || FIFO->Tail == FIFO->Head) return 0;  /* 参数检查或缓冲区为空 */
    uint16_t i = 0;
    for(i = 0; i < Size; i++){
        if(FIFO->Tail == FIFO->Head) break;
        *Data = FIFO->Data[FIFO->Tail & FIFO->Mask];
        FIFO->Tail++;
        Data++;
    }
    return i;                                                 /* 读取成功 */
}

/**
 * @brief  判断FIFO缓冲区是否为空
 * @param  FIFO: FIFO结构体指针
 * @retval 1: 缓冲区为空, 0: 缓冲区不为空或参数无效
 */
uint8_t FIFO_isEmpty(FIFO_t *FIFO){
    if(!FIFO) return 0;                   /* 指针无效，返回非空 */
    if(FIFO->Tail == FIFO->Head) return 1; /* 读写指针相等，表示缓冲区为空 */
    return 0;                             /* 缓冲区不为空 */
}

/**
 * @brief  获取FIFO缓冲区中已存储的数据字节数
 * @param  FIFO: FIFO结构体指针
 * @retval 缓冲区中当前待读取的数据字节数
 */
uint16_t FIFO_Get_Count(FIFO_t *FIFO){
    return FIFO->Head - FIFO->Tail;  /* 写指针减去读指针等于已存储数据量 */
}