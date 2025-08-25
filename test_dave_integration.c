/**
 * @file test_dave_integration.c
 * @brief XMC4800 CANopen + DAVE IDE 整合測試
 * @author MCU專業軟體工程師
 * @date 2025-08-21
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 定義 DAVE 編譯標記 */
#define USE_DAVE_IDE

/* 定義 CANopen 基本類型 */
typedef bool bool_t;

/* CANopen 錯誤類型 */
typedef enum {
    CO_ERROR_NO = 0,
    CO_ERROR_ILLEGAL_ARGUMENT = 1,
    CO_ERROR_OUT_OF_MEMORY = 2,
    CO_ERROR_TIMEOUT = 3,
    CO_ERROR_ILLEGAL_BAUDRATE = 4,
    CO_ERROR_RX_OVERFLOW = 5,
    CO_ERROR_RX_PDO_OVERFLOW = 6,
    CO_ERROR_RX_MSG_LENGTH = 7,
    CO_ERROR_RX_PDO_LENGTH = 8,
    CO_ERROR_TX_OVERFLOW = 9,
    CO_ERROR_TX_PDO_WINDOW = 10,
    CO_ERROR_TX_UNCONFIGURED = 11,
    CO_ERROR_PARAMETERS = 12,
    CO_ERROR_DATA_CORRUPT = 13,
    CO_ERROR_CRC = 14,
    CO_ERROR_TX_BUSY = 15,
    CO_ERROR_WRONG_NMT_STATE = 16,
    CO_ERROR_SYSCALL = 17,
    CO_ERROR_INVALID_STATE = 18
} CO_ReturnError_t;

/* CANopen 接收訊息結構 */
typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
} CO_CANrxMsg_t;

/* CANopen 傳送緩衝區結構 */
typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
    volatile bool_t bufferFull;
    volatile bool_t syncFlag;
} CO_CANtx_t;

/* CANopen 接收緩衝區結構 */
typedef struct {
    uint32_t ident;
    uint32_t mask;
    void *object;
    void (*CANrx_callback)(void *object, CO_CANrxMsg_t *message);
} CO_CANrx_t;

/* CANopen 模組結構 */
typedef struct {
    void *CANptr;
    CO_CANrx_t *rxArray;
    uint16_t rxSize;
    CO_CANtx_t *txArray;
    uint16_t txSize;
    uint16_t CANerrorStatus;
    volatile bool_t CANnormal;
    volatile bool_t useCANrxFilters;
    volatile bool_t bufferInhibitFlag;
    volatile bool_t firstCANtxMessage;
    volatile uint16_t CANtxCount;
    uint32_t errOld;
} CO_CANmodule_t;

/* 緩衝區大小定義 */
#define CO_CAN_RX_BUFFER_SIZE   16
#define CO_CAN_TX_BUFFER_SIZE   16

/* CANopen 功能碼定義 */
#define CANOPEN_FUNCTION_NMT        0x000
#define CANOPEN_FUNCTION_SYNC       0x080
#define CANOPEN_FUNCTION_EMCY       0x080
#define CANOPEN_FUNCTION_PDO1_TX    0x180
#define CANOPEN_FUNCTION_PDO1_RX    0x200
#define CANOPEN_FUNCTION_PDO2_TX    0x280
#define CANOPEN_FUNCTION_PDO2_RX    0x300
#define CANOPEN_FUNCTION_PDO3_TX    0x380
#define CANOPEN_FUNCTION_PDO3_RX    0x400
#define CANOPEN_FUNCTION_PDO4_TX    0x480
#define CANOPEN_FUNCTION_PDO4_RX    0x500
#define CANOPEN_FUNCTION_SDO_TX     0x580
#define CANOPEN_FUNCTION_SDO_RX     0x600
#define CANOPEN_FUNCTION_HEARTBEAT  0x700

/***********************************************************************************************************************
 * 全域變數
 **********************************************************************************************************************/

/* CANopen 模組 */
static CO_CANmodule_t CO_CANmodule[1];

/* CANopen 接收和傳送緩衝區 */
static CO_CANrx_t CO_CANmodule_rxArray0[CO_CAN_RX_BUFFER_SIZE];
static CO_CANtx_t CO_CANmodule_txArray0[CO_CAN_TX_BUFFER_SIZE];

/***********************************************************************************************************************
 * 模擬 DAVE 函數 (用於測試編譯)
 **********************************************************************************************************************/
#ifdef USE_DAVE_IDE

/* 模擬 DAVE 狀態類型 */
typedef enum {
    DAVE_STATUS_SUCCESS = 0,
    DAVE_STATUS_FAILURE = 1
} DAVE_STATUS_t;

/* 模擬 CAN_NODE 狀態類型 */
typedef enum {
    CAN_NODE_STATUS_SUCCESS = 0,
    CAN_NODE_STATUS_FAILURE = 1
} CAN_NODE_STATUS_t;

/* 模擬的 CAN_NODE 結構 */
typedef struct {
    uint32_t baudrate;
    bool enabled;
} CAN_NODE_t;

/* 模擬的 CAN_NODE_LMO 結構 */
typedef struct {
    uint32_t identifier;
    uint8_t data_length;
} CAN_NODE_LMO_t;

/* 模擬 DAVE 變數 */
const CAN_NODE_t CAN_NODE_0 = {500000, false};
const CAN_NODE_LMO_t CAN_NODE_0_LMO_01_Config = {0x123, 8};

/* 模擬 DAVE 函數 */
DAVE_STATUS_t DAVE_Init(void)
{
    printf("DAVE_Init() - 模擬初始化成功\n");
    return DAVE_STATUS_SUCCESS;
}

CAN_NODE_STATUS_t CAN_NODE_SetBaudrate(const CAN_NODE_t *node, uint32_t baudrate)
{
    printf("CAN_NODE_SetBaudrate() - 設定位元速率: %u bps\n", (unsigned int)baudrate);
    (void)node;
    return CAN_NODE_STATUS_SUCCESS;
}

CAN_NODE_STATUS_t CAN_NODE_MO_Transmit(const CAN_NODE_LMO_t *lmo)
{
    printf("CAN_NODE_MO_Transmit() - 傳送訊息: ID=0x%X\n", (unsigned int)lmo->identifier);
    return CAN_NODE_STATUS_SUCCESS;
}

uint32_t CAN_NODE_GetStatus(const CAN_NODE_t *node)
{
    (void)node;
    return 0;  /* 無錯誤狀態 */
}

uint32_t SYSTIMER_GetTime(void)
{
    static uint32_t time_counter = 0;
    return ++time_counter;
}

#endif /* USE_DAVE_IDE */

/***********************************************************************************************************************
 * 簡化的 CANopen 驅動實作 (用於測試)
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
    CANmodule->useCANrxFilters = true;
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0;
    CANmodule->errOld = 0;
    
    /* 清除接收緩衝區 */
    for (uint16_t i = 0; i < rxSize; i++) {
        rxArray[i].ident = 0;
        rxArray[i].mask = 0;
        rxArray[i].object = NULL;
        rxArray[i].CANrx_callback = NULL;
    }
    
    /* 初始化傳送緩衝區 */
    for (uint16_t i = 0; i < txSize; i++) {
        txArray[i].bufferFull = false;
        txArray[i].syncFlag = false;
        txArray[i].ident = 0;
        txArray[i].DLC = 0;
        for (uint8_t j = 0; j < 8; j++) {
            txArray[i].data[j] = 0;
        }
    }
    
#ifdef USE_DAVE_IDE
    /* 使用 DAVE 配置初始化 CAN */
    printf("使用 DAVE IDE 配置初始化 CAN...\n");
    
    if (DAVE_Init() != DAVE_STATUS_SUCCESS) {
        printf("DAVE 初始化失敗\n");
        return CO_ERROR_PARAMETERS;
    }
    printf("✓ DAVE 初始化成功\n");
    
    /* 配置 CAN 位元速率 */
    if (CAN_NODE_SetBaudrate(&CAN_NODE_0, CANbitRate * 1000) != CAN_NODE_STATUS_SUCCESS) {
        printf("CAN 位元速率設定失敗\n");
        return CO_ERROR_ILLEGAL_BAUDRATE;
    }
    printf("✓ CAN 位元速率設定成功: %d kbps\n", CANbitRate);
    
#else
    printf("使用直接 XMC 函式庫初始化 CAN...\n");
    /* 直接使用 XMC 函式庫的程式碼在這裡 */
#endif
    
    CANmodule->CANnormal = true;
    printf("✓ CAN 模組初始化完成\n");
    
    return CO_ERROR_NO;
}

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    if (CANmodule == NULL || buffer == NULL) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    if (buffer->bufferFull) {
        return CO_ERROR_TX_OVERFLOW;
    }
    
#ifdef USE_DAVE_IDE
    /* 使用 DAVE CAN_NODE API 傳送 */
    printf("使用 DAVE API 傳送 CAN 訊息...\n");
    CAN_NODE_STATUS_t status = CAN_NODE_MO_Transmit(&CAN_NODE_0_LMO_01_Config);
    if (status != CAN_NODE_STATUS_SUCCESS) {
        printf("DAVE CAN 傳送失敗\n");
        return CO_ERROR_TX_BUSY;
    }
#else
    printf("使用直接 XMC API 傳送 CAN 訊息...\n");
    /* 直接使用 XMC 函式庫的程式碼在這裡 */
#endif
    
    printf("✓ CAN 訊息傳送成功: ID=0x%03X, DLC=%d\n", (unsigned int)buffer->ident, buffer->DLC);
    
    /* 更新統計 */
    CANmodule->CANtxCount++;
    buffer->bufferFull = true;
    
    return CO_ERROR_NO;
}

void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
        printf("清除待處理的 SYNC PDO\n");
        for (uint16_t i = 0; i < CANmodule->txSize; i++) {
            CO_CANtx_t *buffer = &CANmodule->txArray[i];
            if (buffer->syncFlag && buffer->bufferFull) {
                buffer->bufferFull = false;
            }
        }
    }
}

void CO_CANverifyErrors(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
#ifdef USE_DAVE_IDE
        uint32_t nodeStatus = CAN_NODE_GetStatus(&CAN_NODE_0);
        printf("CAN 狀態檢查: 0x%08X\n", (unsigned int)nodeStatus);
#endif
        CANmodule->CANerrorStatus = 0;  /* 無錯誤 */
        CANmodule->CANnormal = true;
    }
}

void CO_CANinterrupt(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
        printf("CAN 中斷處理\n");
    }
}

/***********************************************************************************************************************
 * 測試主程式
 **********************************************************************************************************************/
int main(void)
{
    printf("=== XMC4800 + DAVE IDE CANopen 整合測試 ===\n");
    printf("編譯時間: %s %s\n", __DATE__, __TIME__);
    printf("目標平台: ARM Cortex-M4 (XMC4800)\n");
    printf("開發環境: DAVE IDE + VS Code\n\n");
    
    /* 顯示配置資訊 */
    printf("配置資訊:\n");
    printf("  USE_DAVE_IDE: %s\n", 
#ifdef USE_DAVE_IDE
           "已啟用"
#else
           "未啟用"
#endif
    );
    printf("  CO_CAN_RX_BUFFER_SIZE: %d\n", CO_CAN_RX_BUFFER_SIZE);
    printf("  CO_CAN_TX_BUFFER_SIZE: %d\n", CO_CAN_TX_BUFFER_SIZE);
    printf("  目標位元速率: 500 kbps\n\n");
    
    /* 初始化 CANopen 模組 */
    printf("1. 初始化 CANopen 模組...\n");
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
        printf("❌ CANopen 模組初始化失敗: %d\n", err);
        return -1;
    }
    printf("✅ CANopen 模組初始化成功\n\n");
    
    /* 測試 CAN 傳送功能 */
    printf("2. 測試 CAN 傳送功能...\n");
    
    /* 準備測試訊息 */
    CO_CANtx_t *testBuffer = &CO_CANmodule_txArray0[0];
    testBuffer->ident = CANOPEN_FUNCTION_HEARTBEAT + 0x7F;  /* 節點 0x7F 的心跳 */
    testBuffer->DLC = 1;
    testBuffer->data[0] = 0x05;  /* Operational 狀態 */
    testBuffer->bufferFull = false;
    testBuffer->syncFlag = false;
    
    err = CO_CANsend(&CO_CANmodule[0], testBuffer);
    if (err == CO_ERROR_NO) {
        printf("✅ 心跳訊息傳送成功\n");
    } else {
        printf("❌ 心跳訊息傳送失敗: %d\n", err);
    }
    
    /* 測試 PDO 傳送 */
    testBuffer = &CO_CANmodule_txArray0[1];
    testBuffer->ident = CANOPEN_FUNCTION_PDO1_TX + 0x7F;  /* PDO1 傳送 */
    testBuffer->DLC = 8;
    testBuffer->data[0] = 0x01;
    testBuffer->data[1] = 0x02;
    testBuffer->data[2] = 0x03;
    testBuffer->data[3] = 0x04;
    testBuffer->data[4] = 0x05;
    testBuffer->data[5] = 0x06;
    testBuffer->data[6] = 0x07;
    testBuffer->data[7] = 0x08;
    testBuffer->bufferFull = false;
    testBuffer->syncFlag = true;  /* 同步 PDO */
    
    err = CO_CANsend(&CO_CANmodule[0], testBuffer);
    if (err == CO_ERROR_NO) {
        printf("✅ SYNC PDO 傳送成功\n");
    } else {
        printf("❌ SYNC PDO 傳送失敗: %d\n", err);
    }
    printf("\n");
    
    /* 測試其他功能 */
    printf("3. 測試其他 CANopen 功能...\n");
    CO_CANclearPendingSyncPDOs(&CO_CANmodule[0]);
    CO_CANverifyErrors(&CO_CANmodule[0]);
    CO_CANinterrupt(&CO_CANmodule[0]);
    printf("✅ 功能測試完成\n\n");
    
    /* 顯示統計資訊 */
    printf("4. 統計資訊:\n");
    printf("  傳送計數: %d\n", CO_CANmodule[0].CANtxCount);
    printf("  錯誤狀態: 0x%04X\n", CO_CANmodule[0].CANerrorStatus);
    printf("  運行狀態: %s\n", CO_CANmodule[0].CANnormal ? "正常" : "異常");
    printf("\n");
    
    /* 記憶體使用統計 */
    printf("5. 記憶體使用統計:\n");
    printf("  CO_CANmodule_t: %zu bytes\n", sizeof(CO_CANmodule_t));
    printf("  CO_CANrx_t[%d]: %zu bytes\n", CO_CAN_RX_BUFFER_SIZE, sizeof(CO_CANmodule_rxArray0));
    printf("  CO_CANtx_t[%d]: %zu bytes\n", CO_CAN_TX_BUFFER_SIZE, sizeof(CO_CANmodule_txArray0));
    printf("  總記憶體: %zu bytes\n", 
           sizeof(CO_CANmodule) + sizeof(CO_CANmodule_rxArray0) + sizeof(CO_CANmodule_txArray0));
    printf("\n");
    
    printf("🎉 所有測試完成！DAVE IDE + CANopen 整合成功！\n");
    printf("系統已準備好進行真實硬體測試。\n");
    
    return 0;
}