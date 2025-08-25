/**
 * @file test_compile.c
 * @brief 測試編譯用的極簡程式
 * @author MCU專業軟體工程師
 * @date 2025-08-21
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* 基本 CANopen 類型定義 (不依賴外部檔案) */
typedef enum {
    CO_ERROR_NO = 0,
    CO_ERROR_ILLEGAL_ARGUMENT = 1,
    CO_ERROR_OUT_OF_MEMORY = 2,
    CO_ERROR_PARAMETERS = 12
} CO_ReturnError_t;

typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
} CO_CANrxMsg_t;

typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
    volatile bool bufferFull;
    volatile bool syncFlag;
} CO_CANtx_t;

typedef struct {
    uint32_t ident;
    uint32_t mask;
    void *object;
    void (*CANrx_callback)(void *object, CO_CANrxMsg_t *message);
} CO_CANrx_t;

typedef struct {
    void *CANptr;
    CO_CANrx_t *rxArray;
    uint16_t rxSize;
    CO_CANtx_t *txArray;
    uint16_t txSize;
    uint16_t CANerrorStatus;
    volatile bool CANnormal;
    volatile bool useCANrxFilters;
    volatile bool bufferInhibitFlag;
    volatile bool firstCANtxMessage;
    volatile uint16_t CANtxCount;
    uint32_t errOld;
} CO_CANmodule_t;

/***********************************************************************************************************************
 * 常數定義
 **********************************************************************************************************************/
#define CO_CAN_RX_BUFFER_SIZE   16
#define CO_CAN_TX_BUFFER_SIZE   16

/***********************************************************************************************************************
 * 全域變數
 **********************************************************************************************************************/
static CO_CANmodule_t CO_CANmodule[1];
static CO_CANrx_t CO_CANmodule_rxArray0[CO_CAN_RX_BUFFER_SIZE];
static CO_CANtx_t CO_CANmodule_txArray0[CO_CAN_TX_BUFFER_SIZE];

/***********************************************************************************************************************
 * 簡化的 CANopen 函數實作
 **********************************************************************************************************************/
CO_ReturnError_t CO_CANmodule_init(
    CO_CANmodule_t *CANmodule,
    void *CANptr,
    CO_CANrx_t *rxArray,
    uint16_t rxSize,
    CO_CANtx_t *txArray,
    uint16_t txSize,
    uint16_t CANbitRate)
{
    if (CANmodule == NULL || rxArray == NULL || txArray == NULL) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* 初始化 CANmodule 結構 */
    CANmodule->CANptr = CANptr;
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal = false;
    CANmodule->useCANrxFilters = false;
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0;
    CANmodule->errOld = 0;
    
    printf("CAN 模組初始化完成 - 位元速率: %d kbps\n", CANbitRate);
    
    return CO_ERROR_NO;
}

void CO_CANinterrupt(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
        /* 簡化的中斷處理 */
        printf("CAN 中斷處理\n");
    }
}

/***********************************************************************************************************************
 * 測試主程式
 **********************************************************************************************************************/
int main(void)
{
    printf("=== XMC4800 CANopen 編譯測試 ===\n");
    printf("編譯時間: %s %s\n", __DATE__, __TIME__);
    
    /* 測試 CANopen 結構體初始化 */
    CO_ReturnError_t err = CO_CANmodule_init(
        &CO_CANmodule[0],
        NULL,  /* CAN 硬體指標 */
        CO_CANmodule_rxArray0,
        CO_CAN_RX_BUFFER_SIZE,
        CO_CANmodule_txArray0,
        CO_CAN_TX_BUFFER_SIZE,
        500  /* 500 kbps */
    );
    
    if (err != CO_ERROR_NO) {
        printf("CAN 模組初始化失敗: %d\n", err);
        return -1;
    }
    
    printf("CAN 模組初始化成功\n");
    
    /* 測試中斷處理 */
    CO_CANinterrupt(&CO_CANmodule[0]);
    
    printf("編譯測試通過！\n");
    printf("結構體大小:\n");
    printf("  CO_CANmodule_t: %zu bytes\n", sizeof(CO_CANmodule_t));
    printf("  CO_CANrx_t: %zu bytes\n", sizeof(CO_CANrx_t));
    printf("  CO_CANtx_t: %zu bytes\n", sizeof(CO_CANtx_t));
    printf("  CO_CANrxMsg_t: %zu bytes\n", sizeof(CO_CANrxMsg_t));
    
    return 0;
}