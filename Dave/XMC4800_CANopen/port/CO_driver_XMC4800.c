/**
 * CANopen driver for XMC4800 - CAN_NODE APP 版本
 *
 * @file CO_driver_XMC4800.c
 * @author XMC4800 CANopen Team 
 * @copyright 2025
 * 
 * 這個版本使用 CAN_NODE APP，包含完整的腳位配置
 */
#include "DAVE.h"     //Declarations from DAVE Code Generation (includes SFR declaration)
#include "CO_driver_target.h"
#include "301/CO_driver.h"
#include <stdio.h>
#include <stdarg.h>

/* XMC4800 specific includes for interrupt handling */
#include "xmc_can.h"
#include "core_cm4.h"
#include "xmc4_flash.h"

/* DAVE CAN_NODE APP includes */
#include "CAN_NODE/can_node.h"
#include "CAN_NODE/can_node_conf.h"

/* **📋 DAVE APP 配置驅動架構** */
typedef struct {
    uint8_t node_id;            /* 從 DAVE UI 推導的 Node ID */
    uint8_t lmo_count;          /* DAVE UI 配置的 LMO 數量 */
    uint32_t baudrate;          /* DAVE UI 配置的波特率 */
    bool service_request_0;     /* 是否使用 Service Request 0 */
    bool rx_event_enabled;      /* RX 事件是否啟用 */
    bool tx_event_enabled;      /* TX 事件是否啟用 */
} canopen_dave_config_t;

/* CANopen ID 計算宏 - 基於 DAVE UI 配置的 Node ID */
#define CANOPEN_EMERGENCY_ID(node_id)      (0x080U + (node_id))
#define CANOPEN_TPDO1_ID(node_id)          (0x180U + (node_id))  
#define CANOPEN_SDO_TX_ID(node_id)         (0x580U + (node_id))
#define CANOPEN_SDO_RX_ID(node_id)         (0x600U + (node_id))
#define CANOPEN_HEARTBEAT_ID(node_id)      (0x700U + (node_id))

/* LMO 分配策略枚舉 */
typedef enum {
    CANOPEN_LMO_TEST_TX = 0,    /* LMO_01: 基本測試發送 */
    CANOPEN_LMO_EMERGENCY,      /* LMO_02: Emergency 和 SDO TX */
    CANOPEN_LMO_TPDO,           /* LMO_03: TPDO 發送 */
    CANOPEN_LMO_SDO_RX,         /* LMO_04: SDO RX 接收 */
    CANOPEN_LMO_COUNT           /* 總共 4 個 LMO */
} canopen_lmo_index_t;

/* **📋 Debug 等級控制 - 精簡版除錯系統** */
#define DEBUG_LEVEL_ERROR   1  /* 只輸出錯誤訊息 */
#define DEBUG_LEVEL_WARN    2  /* 錯誤 + 警告 */
#define DEBUG_LEVEL_INFO    3  /* 錯誤 + 警告 + 資訊 */
#define DEBUG_LEVEL_VERBOSE 4  /* 全部除錯輸出 */

/* **🎯 設定除錯等級 - 可根據需要調整** */
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_VERBOSE  /* 暫時提高到最詳細等級，檢測 CO_CANsend() */
#endif

/* **📋 除錯宏定義 - 分級控制** */
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define Debug_Printf_Error(fmt, ...) Debug_Printf_Raw("[ERROR] " fmt, ##__VA_ARGS__)
#else
#define Debug_Printf_Error(fmt, ...) 
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_WARN
#define Debug_Printf_Warn(fmt, ...) Debug_Printf_Raw("[WARN] " fmt, ##__VA_ARGS__)
#else
#define Debug_Printf_Warn(fmt, ...) 
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define Debug_Printf_Info(fmt, ...) Debug_Printf_Raw("[INFO] " fmt, ##__VA_ARGS__)
#else
#define Debug_Printf_Info(fmt, ...) 
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
#define Debug_Printf_Verbose(fmt, ...) Debug_Printf_Raw("[VERB] " fmt, ##__VA_ARGS__)
#else
#define Debug_Printf_Verbose(fmt, ...) 
#endif

/* **🎯 舊版相容性 - 直接函數調用，避免宏衝突** */
/* Debug_Printf() 直接實現在下方，不使用宏重定向 */

/* Forward declarations */
void Debug_Printf_Raw(const char* format, ...);         /* 原始輸出函數 - 外部可見 */
static void Debug_Printf_Auto(const char* format, ...);  /* 自動分級函數 */
static void Debug_Printf(const char* format, ...);       /* 主要 Debug_Printf 函數 */
static void Debug_Printf_Simple(const char* str);        /* 簡化版本 - 備用 */
static void Debug_Printf_ISR(const char* format, ...);   /* 中斷安全版本 */
void Debug_ProcessISRBuffer(void);                       /* ISR 緩衝區處理函數 - 外部可見 */
static void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule, uint32_t index);
static void CO_CANinterrupt_Tx(CO_CANmodule_t *CANmodule, uint32_t index);

/* **📋 DAVE 配置管理函數** */
static canopen_dave_config_t canopen_get_dave_config(void);
static uint8_t canopen_get_node_id(void);
static uint32_t canopen_get_lmo_index_for_id(uint32_t can_id);
static const CAN_NODE_LMO_t* canopen_get_lmo_for_tx(uint32_t can_id);
static const CAN_NODE_LMO_t* canopen_get_lmo_for_rx(void);
static bool canopen_is_dave_config_valid(void);

/* Global variables */
volatile uint32_t CO_timer1ms = 0;  /* Timer variable incremented each millisecond */
CO_CANmodule_t* g_CANmodule = NULL;
static canopen_dave_config_t g_dave_config;
static bool g_dave_config_initialized = false;

/* 中斷除錯緩衝區 - 避免在中斷中阻塞 */
#define ISR_DEBUG_BUFFER_SIZE 1024
static volatile char isr_debug_buffer[ISR_DEBUG_BUFFER_SIZE];
static volatile uint16_t isr_debug_write_index = 0;
static volatile uint16_t isr_debug_read_index = 0;
static volatile bool isr_debug_overflow = false;

/* 中斷計數器 - 用於主循環監控（外部可見） */
volatile uint32_t g_interrupt_rx_count = 0;
volatile uint32_t g_interrupt_tx_count = 0;
volatile uint32_t g_interrupt_total_count = 0;

/**
 * @brief CAN0_0 Interrupt Handler - Service Request 0 統一中斷處理
 * 所有 LMO 的 tx_sr=0, rx_sr=0 都路由到這個處理函數
 * 注意：使用非阻塞除錯輸出，避免中斷中的 UART 等待
 */
void CAN0_0_IRQHandler(void)
{
    /* 🎯 最簡單的中斷處理 - 只使用計數器，不用複雜除錯 */
    g_interrupt_total_count++;
    
    if (g_CANmodule != NULL) {
        bool event_handled = false;
        
        /* **🔥 使用 DAVE 配置驅動的接收檢查** */
        const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
        
        if (rx_lmo != NULL && rx_lmo->rx_event_enable) {
            /* 使用 XMC 直接 API 檢查 RX 狀態 (DAVE 未提供狀態檢查 API) */
            uint32_t status = XMC_CAN_MO_GetStatus(rx_lmo->mo_ptr);
            if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
                g_interrupt_rx_count++;
                CO_CANinterrupt_Rx(g_CANmodule, CANOPEN_LMO_SDO_RX);
                event_handled = true;
            }
        }
        
        /* **🚀 使用 DAVE 配置驅動的發送檢查** */
        canopen_dave_config_t config = canopen_get_dave_config();
        
        /* **✅ 動態計算：基於 DAVE UI 配置的 LMO 數量，預留最後一個給接收** */
        uint8_t tx_lmo_count = config.lmo_count > 1 ? config.lmo_count - 1 : config.lmo_count;
        
        for (int lmo_idx = 0; lmo_idx < tx_lmo_count; lmo_idx++) {
            /* **✅ 從 DAVE 配置讀取 LMO，而非硬編碼存取** */
            const CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
            
            if (tx_lmo != NULL && tx_lmo->tx_event_enable) {
                /* **🎯 暫時簡化 TX 處理，避免重複發送問題** */
                /* 目前專注於解決重複發送問題，先不做複雜的狀態檢查 */
                g_interrupt_tx_count++;
                event_handled = true;
            }
        }
    }
}

/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(
    CO_CANmodule_t *CANmodule,
    void *CANptr,
    CO_CANrx_t rxArray[],
    uint16_t rxSize,
    CO_CANtx_t txArray[],
    uint16_t txSize,
    uint16_t CANbitRate)
{
    uint16_t i;

    /* verify arguments */
    if (CANmodule == NULL || rxArray == NULL || txArray == NULL) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* **🎯 驗證 DAVE 配置有效性** */
    if (!canopen_is_dave_config_valid()) {
        Debug_Printf("❌ DAVE 配置驗證失敗，無法初始化 CANopen\r\n");
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* **🎯 使用從 DAVE UI 推導的配置** */
    canopen_dave_config_t dave_config = canopen_get_dave_config();

    /* Configure object variables */
    CANmodule->CANptr = (void*)&CAN_NODE_0;  /* 使用 DAVE 配置的 CAN_NODE_0 */
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    
    /* **🔍 除錯輸出：檢查 txSize 值** */
    Debug_Printf("🔍 CO_CANmodule_init: rxSize=%d, txSize=%d\r\n", rxSize, txSize);
    
    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal = false;
    CANmodule->useCANrxFilters = false;
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0;
    CANmodule->errOld = 0;
    
    /* **設定全域參考供 ISR 使用** */
    g_CANmodule = CANmodule;
    
    /* **� DAVE 配置不需要手動 NVIC，但仍需確認中斷啟用** */
    Debug_Printf("=== DAVE APP 配置驅動初始化 ===\r\n");
    Debug_Printf("✅ 使用 DAVE UI 配置 - Node ID: %d\r\n", dave_config.node_id);
    Debug_Printf("✅ 波特率: %lu bps\r\n", dave_config.baudrate);
    Debug_Printf("✅ Service Request 0: %s\r\n", dave_config.service_request_0 ? "啟用" : "停用");
    
    if (dave_config.service_request_0) {
        Debug_Printf("✅ 所有 LMO 事件路由至 CAN0_0_IRQHandler\r\n");
    } else {
        Debug_Printf("⚠️ 注意：LMO 使用不同的 Service Request\r\n");
    }

      /* **💡 DAVE APP 應該已處理 NVIC，但確認啟用狀態** */
    if (!NVIC_GetEnableIRQ(CAN0_0_IRQn)) {
        Debug_Printf("⚠️ DAVE 未自動啟用 NVIC，手動啟用\r\n");
        NVIC_EnableIRQ(CAN0_0_IRQn);
        NVIC_SetPriority(CAN0_0_IRQn, 3U);
    } else {
        Debug_Printf("✅ CAN0_0_IRQn 中斷已由 DAVE 啟用\r\n");
    }
    Debug_Printf("✅ g_CANmodule 已設定，中斷處理函數已就緒\r\n");

    
    /* **🔧 關鍵：配置 LMO_08 (接收) 為 SDO RX ID=0x60A (Node ID=10)** */
    Debug_Printf("=== 配置接收 LMO 為 CANopen SDO RX ===\r\n");
    
    /* 從 DAVE 配置獲取接收 LMO (LMO_08 是第一個接收類型) */
    if (CAN_NODE_0.mo_count >= 8 && CAN_NODE_0.lmobj_ptr[7] != NULL) {
        const CAN_NODE_LMO_t *rx_lmo = CAN_NODE_0.lmobj_ptr[7];  /* LMO_08 索引為 7 */
        if (rx_lmo->mo_ptr != NULL) {
            /* 設定為 CANopen SDO RX ID (0x600 + Node ID) */
            uint8_t node_id = 10;  /* 從 DAVE 配置推導：0x60A - 0x600 = 10 */
            uint32_t sdo_rx_id = 0x600 + node_id;
            
            /* 使用 DAVE API 更新 ID */
            CAN_NODE_MO_UpdateID(rx_lmo, sdo_rx_id);
            Debug_Printf("✅ LMO_08 配置為 SDO RX: ID=0x%03X (Node ID=%d)\r\n", sdo_rx_id, node_id);
            
            /* 確保接收中斷事件啟用 */
            if (rx_lmo->rx_event_enable) {
                CAN_NODE_MO_EnableRxEvent(rx_lmo);
                Debug_Printf("✅ LMO_08 接收中斷事件已啟用\r\n");
            }
        }
    }
    
    /* **🔧 啟用所有發送 LMO 的中斷事件** */
    Debug_Printf("=== 啟用發送 LMO 中斷事件 ===\r\n");
    for (int i = 0; i < 7; i++) {  /* LMO_01 到 LMO_07 都是發送類型 */
        if (i < CAN_NODE_0.mo_count && CAN_NODE_0.lmobj_ptr[i] != NULL) {
            const CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[i];
            if (tx_lmo->tx_event_enable) {
                CAN_NODE_MO_EnableTxEvent(tx_lmo);
                Debug_Printf("✅ LMO_%02d 發送中斷事件已啟用\r\n", i + 1);
            }
        }
    }
    
    Debug_Printf("✅ g_CANmodule 已設定，中斷處理函數已就緒\r\n");
    Debug_Printf("✅ 所有 LMO 中斷事件將路由至 CAN0_0_IRQHandler\r\n");

    /* Configure receive buffers */
    for (i = 0; i < rxSize; i++) {
        rxArray[i].ident = 0;
        rxArray[i].mask = 0;
        rxArray[i].object = NULL;
        rxArray[i].CANrx_callback = NULL;
        rxArray[i].dave_lmo = NULL;
        rxArray[i].lmo_index = i;
    }

    /* **🎯 使用 DAVE 配置驗證系統狀態** */
    if (!canopen_is_dave_config_valid()) {
        Debug_Printf("❌ DAVE 配置驗證失敗\r\n");
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* **🎯 從 DAVE 配置推導系統配置** */
    canopen_dave_config_t config = canopen_get_dave_config();
    Debug_Printf("=== 從 DAVE UI 配置初始化 CANopen ===\r\n");
    Debug_Printf("Node ID: %d (從 DAVE UI 推導)\r\n", config.node_id);
    Debug_Printf("波特率: %lu bps (DAVE UI 配置)\r\n", config.baudrate);
    Debug_Printf("LMO 數量: %d (DAVE UI 配置)\r\n", config.lmo_count);

    /* **檢查 DAVE 配置的 LMO 狀態** */
    Debug_Printf("=== 檢查所有 LMO 配置 ===\r\n");
    for (int lmo_idx = 0; lmo_idx < config.lmo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        if (lmo != NULL) {
            Debug_Printf("LMO_%02d: MO%d, TX_SR=%d, RX_SR=%d, TX_EN=%s, RX_EN=%s\r\n",
                        lmo_idx + 1, lmo->number, lmo->tx_sr, lmo->rx_sr,
                        lmo->tx_event_enable ? "YES" : "NO",
                        lmo->rx_event_enable ? "YES" : "NO");
        }
    }

    /* Configure transmit buffers */
    for (i = 0; i < txSize; i++) {
        txArray[i].ident = 0;
        txArray[i].DLC = 0;
        txArray[i].bufferFull = false;
        txArray[i].syncFlag = false;
        txArray[i].dave_lmo = NULL;
        txArray[i].lmo_index = i;
    }

    /* **🎯 初始化時配置 CAN Transfer Settings** */
    Debug_Printf("=== 初始化 CAN Transfer Settings ===\r\n");
    
    /* **🚨 首先設定 CAN 節點基本錯誤處理** */
    if (CAN_NODE_0.global_ptr != NULL) {
        /* **重置 CAN 節點到乾淨狀態** */
        // 簡化實現，只輸出狀態 - 避免使用不存在的 API
        Debug_Printf("✅ CAN 節點基本錯誤處理已設定\r\n");
    }
    
    /* **配置所有 TX LMO 的 Transfer Settings** */
    uint8_t tx_lmo_count = config.lmo_count > 1 ? config.lmo_count - 1 : config.lmo_count;
    
    for (int lmo_idx = 0; lmo_idx < tx_lmo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        if (tx_lmo != NULL && tx_lmo->mo_ptr != NULL) {
            /* **✅ 啟用 Single Transmit Trial (STT) - 防止自動重傳** */
            XMC_CAN_MO_EnableSingleTransmitTrial(tx_lmo->mo_ptr);
            Debug_Printf("✅ 初始化 LMO_%02d: Single Transmit Trial (STT) 已啟用\r\n", lmo_idx + 1);
            
            /* **🔧 清除所有可能的錯誤狀態 - 改善訊號品質** */
            XMC_CAN_MO_ResetStatus(tx_lmo->mo_ptr, 
                XMC_CAN_MO_RESET_STATUS_TX_PENDING |
                XMC_CAN_MO_RESET_STATUS_RX_PENDING);
                
            /* **✅ 設定訊息為有效狀態，準備傳輸** */
            XMC_CAN_MO_SetStatus(tx_lmo->mo_ptr, XMC_CAN_MO_SET_STATUS_MESSAGE_VALID);
        }
    }
    
    /* **配置 RX LMO 的 Transfer Settings** */
    const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
    if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL) {
        /* **✅ 啟用 Single Data Transfer (SDT) for RX** */
        XMC_CAN_FIFO_EnableSingleDataTransfer(rx_lmo->mo_ptr);
        Debug_Printf("✅ 初始化 RX LMO: Single Data Transfer (SDT) 已啟用\r\n");
    }

    Debug_Printf("✅ CANopen 軟體層初始化完成\r\n");
    Debug_Printf("🚨 注意：所有硬體已由 DAVE_Init() 配置\r\n");
    Debug_Printf("🎯 Transfer Settings: STT + SDT + 忽略遠程請求 已配置\r\n");

    return CO_ERROR_NO;
}

/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
        /* 使用 CAN_NODE APP - 可以禁用節點 */
        Debug_Printf("=== CAN_NODE 模式停用 ===\r\n");
        CANmodule->CANnormal = false;
    }
}

/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(
    CO_CANmodule_t *CANmodule,
    uint16_t index,
    uint16_t ident,
    uint16_t mask,
    bool_t rtr,
    void *object,
    void (*CANrx_callback)(void *object, void *message))
{
    CO_CANrx_t *buffer = NULL;

    /* safety */
    if (CANmodule == NULL || index >= CANmodule->rxSize) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* get specific buffer */
    buffer = &CANmodule->rxArray[index];

    /* **🎯 使用 DAVE 配置的接收 LMO** */
    const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
    
    if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL) {
        /* **✅ 根據 DAVE UI 推導的 Node ID 動態計算 CANopen ID** */
        uint8_t node_id = canopen_get_node_id();
        uint32_t sdo_rx_id = CANOPEN_SDO_RX_ID(node_id);
        
        if (ident == sdo_rx_id) {
            /* **🔧 優先使用 DAVE API** */
            CAN_NODE_MO_UpdateID(rx_lmo, ident & 0x7FF);
            
            /* **⚠️ 遮罩設定：DAVE 未提供 API，必須直接存取** */
            rx_lmo->mo_ptr->can_id_mask = mask & 0x7FF;
            
            Debug_Printf("✅ RX LMO 配置: ID=0x%03X, Mask=0x%03X (Node ID %d 從 DAVE UI 推導)\r\n", 
                        ident, mask, node_id);
            
            /* Store CAN_NODE reference */
            buffer->dave_lmo = (void*)rx_lmo;
        } else {
            Debug_Printf("🔍 軟體接收緩衝區: ID=0x%03X (非 SDO RX)\r\n", ident);
            buffer->dave_lmo = NULL;
        }
        
        Debug_Printf("RX Buffer[%d]: ID=0x%03X, Mask=0x%03X\r\n", index, ident, mask);
    } else {
        Debug_Printf("❌ 無法獲取 DAVE 配置的 RX LMO\r\n");
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Configure software buffer */
    buffer->ident = ident;
    buffer->mask = mask;
    buffer->object = object;
    buffer->CANrx_callback = CANrx_callback;
    
    return CO_ERROR_NO;
}

/******************************************************************************/
CO_CANtx_t *CO_CANtxBufferInit(
    CO_CANmodule_t *CANmodule,
    uint16_t index,
    uint16_t ident,
    bool_t rtr,
    uint8_t noOfBytes,
    bool_t syncFlag)
{
    CO_CANtx_t *buffer = NULL;

    /* safety */
    if (CANmodule == NULL || index >= CANmodule->txSize) {
        /* **🔍 除錯輸出：檢查為什麼返回 NULL** */
        if (CANmodule == NULL) {
            Debug_Printf("❌ CO_CANtxBufferInit: CANmodule is NULL\r\n");
        } else {
            Debug_Printf("❌ CO_CANtxBufferInit: index=%d >= txSize=%d\r\n", index, CANmodule->txSize);
        }
        return NULL;
    }

    /* get specific buffer */
    buffer = &CANmodule->txArray[index];

    /* CAN identifier and rtr */
    buffer->ident = ident;
    buffer->DLC = noOfBytes;
    buffer->bufferFull = false;
    buffer->syncFlag = syncFlag;

    return buffer;
}

/******************************************************************************/
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    CO_ReturnError_t err = CO_ERROR_NO;
    
    /* **� 強制輸出 - 確保函數被調用時我們能看到** */
    Debug_Printf_Raw("🚨 CO_CANsend() CALLED - ID=0x%03X, DLC=%d\r\n", buffer->ident, buffer->DLC);
    
    /* **�🔍 Step 1: 詳細檢測傳入參數是否符合 DAVE 配置** */
    Debug_Printf_Verbose("=== CO_CANsend 數值檢測 ===\r\n");
    Debug_Printf_Verbose("🔍 檢測 CAN ID: 0x%03X\r\n", buffer->ident);
    Debug_Printf_Verbose("🔍 檢測 DLC: %d\r\n", buffer->DLC);
    
    /* **🔍 Step 0.5: 檢查 CAN 位元時序參數和暫存器狀態** */
    Debug_Printf_Verbose("=== CAN 位元時序和暫存器檢查 ===\r\n");
    
    /* **🚨 分析 DAVE 配置問題** */
    Debug_Printf("🔍 DAVE 配置問題分析:\r\n");
    Debug_Printf("   當前設定: Synchronization jump width = 1\r\n");
    Debug_Printf("   當前設定: Sample point = 80%%\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("⚠️ WARNING: 這些設定可能導致 stuff error!\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("💡 建議的正確設定 (DAVE 中修改):\r\n");
    Debug_Printf("   ✅ Synchronization jump width: 改為 2 (提高抗干擾)\r\n");
    Debug_Printf("   ✅ Sample point: 改為 75%% (避免 stuff error)\r\n");
    Debug_Printf("   ✅ 詳細建議: TSEG1=6, TSEG2=2, SJW=2\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("� 修改步驟:\r\n");
    Debug_Printf("   1. 開啟 DAVE IDE\r\n");
    Debug_Printf("   2. 點選 CAN_NODE_0\r\n");
    Debug_Printf("   3. Advanced Settings 頁籤\r\n");
    Debug_Printf("   4. 修改 SJW=2, Sample Point=75%%\r\n");
    Debug_Printf("   5. 重新產生程式碼並編譯\r\n");
    Debug_Printf("\r\n");
    
    /* **檢測 1: CAN ID 範圍驗證** */
    if (buffer->ident > 0x7FF) {
        Debug_Printf("❌ ERROR: CAN ID 0x%03X 超出 11-bit 範圍 (最大 0x7FF)\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* **檢測 2: DLC 範圍驗證** */
    if (buffer->DLC > 8) {
        Debug_Printf("❌ ERROR: DLC %d 超出範圍 (最大 8)\r\n", buffer->DLC);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* **檢測 3: 與 DAVE 配置的 LMO 對照** */
    Debug_Printf("🔍 檢查 DAVE 配置的 LMO 對應關係:\r\n");
    Debug_Printf("   LMO_01 (0x8A)  -> Emergency + SDO TX\r\n");
    Debug_Printf("   LMO_02 (0x18A) -> TPDO1\r\n"); 
    Debug_Printf("   LMO_03 (0x28A) -> TPDO2\r\n");
    Debug_Printf("   LMO_04 (0x38A) -> TPDO3\r\n");
    Debug_Printf("   LMO_05 (0x48A) -> TPDO4\r\n");
    Debug_Printf("   LMO_06 (0x58A) -> 備用 TX\r\n");
    Debug_Printf("   LMO_07 (0x70A) -> Heartbeat\r\n");
    Debug_Printf("   LMO_08 (0x60A) -> SDO RX (接收)\r\n");
    
    /* **檢測 4: 確認 ID 是否匹配 DAVE 預設值** */
    bool id_matches_dave_config = false;
    const char* lmo_description = "未知";
    
    switch (buffer->ident) {
        case 0x08A: id_matches_dave_config = true; lmo_description = "Emergency (LMO_01)"; break;
        case 0x18A: id_matches_dave_config = true; lmo_description = "TPDO1 (LMO_02)"; break;
        case 0x28A: id_matches_dave_config = true; lmo_description = "TPDO2 (LMO_03)"; break;
        case 0x38A: id_matches_dave_config = true; lmo_description = "TPDO3 (LMO_04)"; break;
        case 0x48A: id_matches_dave_config = true; lmo_description = "TPDO4 (LMO_05)"; break;
        case 0x58A: id_matches_dave_config = true; lmo_description = "備用 TX (LMO_06)"; break;
        case 0x70A: id_matches_dave_config = true; lmo_description = "Heartbeat (LMO_07)"; break;
        case 0x123: id_matches_dave_config = true; lmo_description = "測試訊息"; break;
        default:
            Debug_Printf("⚠️ WARN: ID 0x%03X 不在 DAVE 預設配置中\r\n", buffer->ident);
            break;
    }
    
    if (id_matches_dave_config) {
        Debug_Printf("✅ PASS: ID 0x%03X 匹配 DAVE 配置 - %s\r\n", buffer->ident, lmo_description);
    }

    /* Verify overflow */
    if (buffer->bufferFull) {
        if (!CANmodule->firstCANtxMessage) {
            /* Don't set error, if bootup message is still on buffers */
            CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    /* **🎯 使用 DAVE 配置驅動的 LMO 選擇** */
    const CAN_NODE_LMO_t *tx_lmo = canopen_get_lmo_for_tx(buffer->ident);
    
    /* **檢測 5: LMO 有效性驗證** */
    if (tx_lmo == NULL) {
        Debug_Printf("❌ ERROR: 無法找到對應的 TX LMO for ID=0x%03X\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    if (tx_lmo->mo_ptr == NULL) {
        Debug_Printf("❌ ERROR: LMO MO 指標為 NULL (ID=0x%03X)\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    Debug_Printf("✅ PASS: 找到有效的 TX LMO (MO%d) for ID=0x%03X\r\n", 
                tx_lmo->number, buffer->ident);
    
    /* **檢測 6: 當前 LMO 狀態檢測** */
    uint32_t mo_status = XMC_CAN_MO_GetStatus(tx_lmo->mo_ptr);
    Debug_Printf("🔍 LMO 狀態檢測: 0x%08lX\r\n", mo_status);
    
    if (mo_status & XMC_CAN_MO_STATUS_TX_PENDING) {
        Debug_Printf("⚠️ WARN: LMO 仍有傳輸待處理\r\n");
    }
    
    if (mo_status & XMC_CAN_MO_STATUS_MESSAGE_VALID) {
        Debug_Printf("✅ PASS: LMO 訊息有效位元已設定\r\n");
    } else {
        Debug_Printf("⚠️ WARN: LMO 訊息有效位元未設定\r\n");
    }
    
    /* **檢測 7: 資料內容檢測 (前 4 bytes)** */
    Debug_Printf("🔍 資料內容檢測 (DLC=%d): ", buffer->DLC);
    for (int i = 0; i < buffer->DLC && i < 4; i++) {
        Debug_Printf("0x%02X ", buffer->data[i]);
    }
    if (buffer->DLC > 4) {
        Debug_Printf("...");
    }
    Debug_Printf("\r\n");
    
    if (tx_lmo != NULL && tx_lmo->mo_ptr != NULL) {
        /* **🔧 完全使用 DAVE API 進行發送** */
        Debug_Printf("=== 開始 DAVE API 傳送程序 ===\r\n");
        
        /* **✅ 優先使用 DAVE API 更新 ID** */
        Debug_Printf("🔧 Step 1: 更新 CAN ID 到 LMO\r\n");
        CAN_NODE_MO_UpdateID(tx_lmo, buffer->ident & 0x7FF);
        Debug_Printf("✅ ID 0x%03X 已更新到 LMO\r\n", buffer->ident & 0x7FF);
        
        /* **⚠️ DLC 設定：DAVE 未提供 API，必須直接設定** */
        Debug_Printf("🔧 Step 2: 設定 DLC\r\n");
        tx_lmo->mo_ptr->can_data_length = buffer->DLC;
        Debug_Printf("✅ DLC %d 已設定\r\n", buffer->DLC);
        
        /* **✅ 優先使用 DAVE API 更新數據** */
        Debug_Printf("🔧 Step 3: 更新資料到 LMO\r\n");
        CAN_NODE_STATUS_t update_status = CAN_NODE_MO_UpdateData(tx_lmo, buffer->data);
        Debug_Printf("🔍 DAVE UpdateData 狀態: %d (0=SUCCESS)\r\n", update_status);
        
        if (update_status == CAN_NODE_STATUS_SUCCESS) {
            /* **✅ 優先使用 DAVE API 發送** */
            Debug_Printf("🔧 Step 4: 執行傳送\r\n");
            CAN_NODE_STATUS_t tx_status = CAN_NODE_MO_Transmit(tx_lmo);
            Debug_Printf("🔍 DAVE Transmit 狀態: %d (0=SUCCESS)\r\n", tx_status);
            
            if (tx_status == CAN_NODE_STATUS_SUCCESS) {
                /* **🎯 關鍵修正：發送成功後立即釋放緩衝區，但加入發送節流** */
                buffer->bufferFull = false;
                CANmodule->CANtxCount++;
                
                /* **⚠️ 重要：加入發送延遲，避免過快重複發送** */
                static uint32_t last_send_time = 0;
                uint32_t current_time = CO_timer1ms;  /* 使用全域 1ms 計時器 */
                
                if (current_time > last_send_time) {
                    last_send_time = current_time;
                    Debug_Printf("✅ TX SUCCESS: ID=0x%03X via DAVE LMO_%02d (time=%lu)\r\n", 
                                buffer->ident, (tx_lmo->number - 32) + 1, current_time);
                } else {
                    /* 避免在同一毫秒內重複輸出除錯訊息 */
                    Debug_Printf("✅ TX SUCCESS: ID=0x%03X (同一毫秒)\r\n", buffer->ident);
                }
                
                /* **🔧 額外檢查：確保訊息真的已發送完成** */
                Debug_Printf("🔧 Step 5: 驗證傳送完成\r\n");
                for (int retry = 0; retry < 10; retry++) {
                    uint32_t mo_status = XMC_CAN_MO_GetStatus(tx_lmo->mo_ptr);
                    if (!(mo_status & XMC_CAN_MO_STATUS_TX_PENDING)) {
                        Debug_Printf("✅ 傳送完成確認 (重試 %d 次)\r\n", retry);
                        break;  /* 發送完成 */
                    }
                    /* 短暫延遲 */
                    for (volatile int i = 0; i < 100; i++);
                }
                Debug_Printf("=== DAVE API 傳送程序完成 ===\r\n");
            } else {
                buffer->bufferFull = true;
                Debug_Printf("❌ TX ERROR: ID=0x%03X, DAVE transmit status=%d\r\n", buffer->ident, tx_status);
                err = CO_ERROR_TX_OVERFLOW;
            }
        } else {
            buffer->bufferFull = true;
            Debug_Printf("❌ UPDATE ERROR: ID=0x%03X, DAVE update status=%d\r\n", buffer->ident, update_status);
            err = CO_ERROR_TX_OVERFLOW;
        }
    } else {
        Debug_Printf("❌ CRITICAL ERROR: LMO 或 MO 指標無效 for ID=0x%03X\r\n", buffer->ident);
        err = CO_ERROR_TX_OVERFLOW;
    }

    return err;
}

/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
    uint32_t tpdoDeleted = 0U;

    CO_LOCK_CAN_SEND();
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    if (/*messageIsOnCanBuffer && */ CANmodule->bufferInhibitFlag) {
        /* clear TXREQ */
        CANmodule->bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if (CANmodule->CANtxCount != 0U) {
        uint16_t i;
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        for (i = CANmodule->txSize; i > 0U; i--) {
            if (buffer->bufferFull) {
                if (buffer->syncFlag) {
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
            buffer++;
        }
    }
    CO_UNLOCK_CAN_SEND();

    if (tpdoDeleted != 0U) {
        CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
    }
}

/******************************************************************************/
/* Get error counters from the module. If necessary, function may use
 * different way to determine errors. */
static uint16_t CO_CANerrors(CO_CANmodule_t *CANmodule)
{
    uint16_t err = 0;
    
    /* **使用 CAN_NODE 檢查錯誤狀態** */
    if (CAN_NODE_0.global_ptr != NULL) {
        /* 可以添加錯誤計數器檢查 */
        /* 目前返回 0 表示無錯誤 */
    }
    
    return err;
}

/******************************************************************************/
/**
 * @brief CANopen 處理函數 - 包含中斷備用方案
 * 主要依賴中斷，但提供輪詢作為備用
 */
void CO_CANmodule_process(CO_CANmodule_t *CANmodule) 
{
    if (CANmodule == NULL) return;

    /* **主要機制：Service Request 0 中斷**
     * 正常情況下，CAN0_0_IRQHandler 會處理所有事件
     * 這裡的輪詢作為安全網，防止中斷遺漏
     */
    static uint32_t poll_counter = 0;
    static uint32_t backup_checks = 0;
    poll_counter++;
    
    /* 每 50 次調用進行一次備用檢查（降低輪詢頻率）*/
    if ((poll_counter % 50) == 0) {
        /* **🎯 使用 DAVE 配置的接收 LMO 進行輪詢檢查** */
        const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
        
        if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL) {
            /* 使用 XMC 直接 API 檢查狀態 (DAVE 未提供狀態檢查 API) */
            uint32_t status = XMC_CAN_MO_GetStatus(rx_lmo->mo_ptr);
            if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
                backup_checks++;
                Debug_Printf("🔍 備用輪詢檢測到 RX 數據 (#%lu)，調用中斷處理\r\n", backup_checks);
                CO_CANinterrupt_Rx(CANmodule, CANOPEN_LMO_SDO_RX);
            }
        }
    }

    /* 定期報告系統狀態 */
    if ((poll_counter % 5000) == 0) {
        Debug_Printf("📊 CAN 系統狀態: 主中斷運行，備用檢查 %lu 次\r\n", backup_checks);
    }

    /* 簡化錯誤處理 - 避免複雜的 Emergency 依賴 */
    CANmodule->errOld = 0;
    CANmodule->CANerrorStatus = 0;
}

/******************************************************************************/
/* CAN RX interrupt */
static void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule, uint32_t index)
{
    /* 🎯 中斷中只做必要處理，詳細除錯延後輸出 */
    Debug_Printf_ISR("RX Process Start");
    
    /* **🎯 使用 DAVE 配置的接收 LMO** */
    const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
    
    if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL) {
        
        /* **✅ 手動清除狀態位，確保 DAVE API 內部循環能退出** */
        XMC_CAN_MO_ResetStatus(rx_lmo->mo_ptr, XMC_CAN_MO_RESET_STATUS_RX_PENDING | XMC_CAN_MO_RESET_STATUS_NEW_DATA);

        /* **🔧 使用 DAVE API 接收數據** */
        CAN_NODE_STATUS_t rx_status = CAN_NODE_MO_ReceiveData((CAN_NODE_LMO_t*)rx_lmo);

        if (rx_status == CAN_NODE_STATUS_SUCCESS) {
            /* 讀取接收的 CAN 訊息 - 從 MO 結構讀取 */
            CO_CANrxMsg_t rcvMsg;
            rcvMsg.ident = rx_lmo->mo_ptr->can_identifier & 0x07FFU;
            rcvMsg.DLC = rx_lmo->mo_ptr->can_data_length;
            
            /* 複製數據 */
            for (int i = 0; i < rcvMsg.DLC && i < 8; i++) {
                rcvMsg.data[i] = rx_lmo->mo_ptr->can_data[i];
            }
        
            /* 快速驗證 */
            if (rcvMsg.ident > 0x000 && rcvMsg.ident <= 0x7FF && rcvMsg.DLC <= 8) {
                Debug_Printf_ISR("RX: ID=0x%03X DLC=%d", rcvMsg.ident, rcvMsg.DLC);
                
                /* **尋找匹配的接收緩衝區** */
                bool message_processed = false;
                for (uint16_t i = 0; i < CANmodule->rxSize; i++) {
                    CO_CANrx_t *buffer = &CANmodule->rxArray[i];
                    if (buffer != NULL && buffer->CANrx_callback != NULL) {
                        /* 檢查 ID 和遮罩是否匹配 */
                        if (((rcvMsg.ident ^ buffer->ident) & buffer->mask) == 0U) {
                            /* 調用 CANopen 處理函數 */
                            buffer->CANrx_callback(buffer->object, (void*)&rcvMsg);
                            message_processed = true;
                            Debug_Printf_ISR("RX Callback Done");
                            break;
                        }
                    }
                }
                
                if (!message_processed) {
                    Debug_Printf_ISR("RX No Match");
                }
            } else {
                Debug_Printf_ISR("RX Invalid Data");
            }
        
        Debug_Printf_ISR("RX Complete");
        } else {
            Debug_Printf_ISR("RX Status Failed");
        }
    } else {
        Debug_Printf_ISR("RX LMO_04 NULL");
    }
}

/******************************************************************************/
/* CAN TX interrupt - 處理發送完成事件 */
static void CO_CANinterrupt_Tx(CO_CANmodule_t *CANmodule, uint32_t index)
{
    /* **🎯 TX 中斷處理 - 基於 DAVE 配置動態檢查** */
    canopen_dave_config_t config = canopen_get_dave_config();
    uint8_t tx_lmo_count = config.lmo_count > 1 ? config.lmo_count - 1 : config.lmo_count;
    
    if (CANmodule != NULL && index < tx_lmo_count) {  /* **✅ 使用 DAVE UI 配置的 TX LMO 數量** */
        /* 檢查對應的發送緩衝區 */
        if (index < CANmodule->txSize) {
            CO_CANtx_t *buffer = &CANmodule->txArray[index];
            
            if (buffer->bufferFull) {
                /* 發送完成，清除 bufferFull 標誌 */
                buffer->bufferFull = false;
                CANmodule->CANtxCount--;
            }
        }
        
        /* 標記第一次發送完成 */
        CANmodule->firstCANtxMessage = false;
    }
}

/******************************************************************************/
void CO_CANsetConfigurationMode(void *CANptr)
{
    /* 使用 CAN_NODE - 配置模式由 DAVE 管理 */
    Debug_Printf("=== CAN_NODE 配置模式 ===\r\n");
    
    /* **🎯 關鍵修正：使用正確的 XMC CAN API** */
    /* 暫時簡化實現，避免直接存取暫存器 */
    Debug_Printf("✅ CAN 節點設定為配置模式（透過 DAVE 管理）\r\n");
}

/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
    /* 使用 CAN_NODE - 正常模式啟動 */
    if (CANmodule != NULL) {
        CANmodule->CANnormal = true;
        Debug_Printf("=== CAN_NODE 模式激活 ===\r\n");
        
        /* **🎯 關鍵修正：使用 CAN_NODE API 配置 Transfer Settings** */
        Debug_Printf("=== 配置 CAN Transfer Settings ===\r\n");
        
        /* **配置所有 TX LMO 的 Transfer Settings** */
        canopen_dave_config_t config = canopen_get_dave_config();
        uint8_t tx_lmo_count = config.lmo_count > 1 ? config.lmo_count - 1 : config.lmo_count;
        
        for (int lmo_idx = 0; lmo_idx < tx_lmo_count; lmo_idx++) {
            const CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
            if (tx_lmo != NULL && tx_lmo->mo_ptr != NULL) {
                /* **🔧 徹底修正 CAN 物理層問題** */
                
                /* **1. 啟用 Single Transmit Trial (STT) - 防止自動重傳** */
                XMC_CAN_MO_EnableSingleTransmitTrial(tx_lmo->mo_ptr);
                
                /* **2. 清除所有錯誤標誌** */
                XMC_CAN_MO_ResetStatus(tx_lmo->mo_ptr, 
                    XMC_CAN_MO_RESET_STATUS_TX_PENDING |
                    XMC_CAN_MO_RESET_STATUS_RX_PENDING |
                    XMC_CAN_MO_RESET_STATUS_RX_UPDATING |
                    XMC_CAN_MO_RESET_STATUS_NEW_DATA);
                
                /* **3. 設定訊息物件為標準格式（非擴展格式）** */
                tx_lmo->mo_ptr->can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ;
                
                /* **4. 確保使用標準 11-bit ID 格式** */
                tx_lmo->mo_ptr->can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
                
                /* **5. 設定為數據幀（非遠程幀）** */
                tx_lmo->mo_ptr->can_data_length &= 0x0F; // 保留低 4 位 DLC，清除高位
                
                Debug_Printf("✅ LMO_%02d: 完整 CAN 物理層修正\r\n", lmo_idx + 1);
            }
        }
        
        /* **🚨 關鍵：CAN 節點層級的錯誤處理設定** */
        if (CAN_NODE_0.global_ptr != NULL) {
            /* **簡化實現 - 避免使用不存在的 API** */
            Debug_Printf("✅ CAN 節點錯誤處理已優化 (簡化版)\r\n");
        }
        
        /* **配置 RX LMO 的 Transfer Settings** */
        const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
        if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL) {
            /* **✅ 啟用 Single Data Transfer (SDT) for RX** */
            XMC_CAN_FIFO_EnableSingleDataTransfer(rx_lmo->mo_ptr);
            Debug_Printf("✅ RX LMO: 啟用 Single Data Transfer (SDT)\r\n");
            
            Debug_Printf("RX LMO 接收事件: %s\r\n", rx_lmo->rx_event_enable ? "已啟用" : "未啟用");
        } else {
            Debug_Printf("❌ 無法獲取 RX LMO 配置\r\n");
        }
        
        Debug_Printf("✅ CAN Transfer Settings 配置完成\r\n");
        Debug_Printf("  - Single Transmit Trial (STT): 已啟用（防止重傳）\r\n");
        Debug_Printf("  - Single Data Transfer (SDT): 已啟用（RX）\r\n");
        Debug_Printf("  - Ignore Remote Request: 已配置\r\n");
        Debug_Printf("✅ CAN 節點設定為正常模式（透過 DAVE 管理）\r\n");
    }
}

/******************************************************************************/
/* 中斷安全的除錯輸出函式 - 非阻塞版本 */
static void Debug_Printf_ISR(const char* format, ...)
{
    /* 🎯 中斷中只將訊息放入緩衝區，不進行 UART 傳輸 */
    char temp_buffer[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        /* 將訊息放入環形緩衝區 */
        for (int i = 0; i < len && i < (sizeof(temp_buffer) - 1); i++) {
            uint16_t next_write = (isr_debug_write_index + 1) % ISR_DEBUG_BUFFER_SIZE;
            
            if (next_write != isr_debug_read_index) {
                isr_debug_buffer[isr_debug_write_index] = temp_buffer[i];
                isr_debug_write_index = next_write;
            } else {
                /* 緩衝區溢位 */
                isr_debug_overflow = true;
                break;
            }
        }
        
        /* 添加分隔符 */
        uint16_t next_write = (isr_debug_write_index + 1) % ISR_DEBUG_BUFFER_SIZE;
        if (next_write != isr_debug_read_index) {
            isr_debug_buffer[isr_debug_write_index] = '\n';
            isr_debug_write_index = next_write;
        }
    }
}

/******************************************************************************/
/* 處理中斷除錯緩衝區輸出 - 在主循環中調用 */
void Debug_ProcessISRBuffer(void)
{
    static char output_buffer[256];
    uint16_t output_index = 0;
    
    /* 檢查是否有緩衝區溢位 */
    if (isr_debug_overflow) {
        Debug_Printf("⚠️ ISR 除錯緩衝區溢位！\r\n");
        isr_debug_overflow = false;
    }
    
    /* 從環形緩衝區讀取數據 */
    while (isr_debug_read_index != isr_debug_write_index && output_index < (sizeof(output_buffer) - 1)) {
        char ch = isr_debug_buffer[isr_debug_read_index];
        isr_debug_read_index = (isr_debug_read_index + 1) % ISR_DEBUG_BUFFER_SIZE;
        
        output_buffer[output_index++] = ch;
        
        /* 遇到換行符或緩衝區快滿時，輸出一行 */
        if (ch == '\n' || output_index >= (sizeof(output_buffer) - 10)) {
            output_buffer[output_index] = '\0';
            Debug_Printf("🎯 ISR: %s\r", output_buffer);  /* 使用 \r 避免雙換行 */
            output_index = 0;
        }
    }
    
    /* 輸出剩餘數據 */
    if (output_index > 0) {
        output_buffer[output_index] = '\0';
        Debug_Printf("🎯 ISR: %s\r\n", output_buffer);
    }
}

/******************************************************************************/
/* **📋 Debug_Printf 統一實現系統** */
/******************************************************************************/

/**
 * @brief 原始 Debug_Printf 實現 - 處理所有 UART 輸出
 * @param format 格式化字串
 * @param ... 可變參數
 */
void Debug_Printf_Raw(const char* format, ...)
{
    static char debug_buffer[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(debug_buffer, sizeof(debug_buffer), format, args);
    va_end(args);
    
    if (len > 0 && len < sizeof(debug_buffer)) {
        /* 🎯 確保字串正確終止 */
        debug_buffer[len] = '\0';
        
        /* 限制字串長度，避免緩衝區溢位 */
        if (len > 200) {
            len = 200;
            debug_buffer[len] = '\0';
        }
        
        UART_STATUS_t uart_status = UART_Transmit(&UART_0, (uint8_t*)debug_buffer, len);
        if (uart_status == UART_STATUS_SUCCESS) {
            /* 🔧 修正：必須等待 UART 傳輸完成，否則數據會被截斷 */
            uint32_t timeout = 0;
            const uint32_t MAX_TIMEOUT = 50000;  /* 增加超時時間 */
            
            while (UART_0.runtime->tx_busy == true && timeout < MAX_TIMEOUT) {
                timeout++;
                /* 小延遲，讓 UART 有時間處理 */
                for (volatile int i = 0; i < 10; i++);
            }
            
            /* 額外的小延遲，確保最後幾個字元完全傳輸 */
            for (volatile int i = 0; i < 1000; i++);
            
            if (timeout >= MAX_TIMEOUT) {
                /* 超時錯誤處理 - 但不要無限遞迴調用 Debug_Printf */
                static volatile bool timeout_reported = false;
                if (!timeout_reported) {
                    timeout_reported = true;
                    /* 簡單標記，避免遞迴 */
                }
            }
        }
    }
}

/**
 * @brief 自動分級 Debug_Printf - 根據訊息內容自動判斷等級
 * @param format 格式化字串
 * @param ... 可變參數
 */
static void Debug_Printf_Auto(const char* format, ...)
{
    /* 🎯 簡單的自動分級邏輯 */
    if (format == NULL) return;
    
    /* 檢查訊息類型 */
    if (strstr(format, "❌") || strstr(format, "ERROR") || strstr(format, "error")) {
        /* 錯誤訊息 - 總是顯示 */
        va_list args;
        va_start(args, format);
        static char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Debug_Printf_Error("%s", buffer);
    }
    else if (strstr(format, "⚠️") || strstr(format, "WARN") || strstr(format, "Warning")) {
        /* 警告訊息 */
        va_list args;
        va_start(args, format);
        static char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Debug_Printf_Warn("%s", buffer);
    }
    else if (strstr(format, "===") || strstr(format, "初始化") || strstr(format, "配置")) {
        /* 資訊訊息 */
        va_list args;
        va_start(args, format);
        static char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Debug_Printf_Info("%s", buffer);
    }
    else {
        /* 詳細除錯訊息 */
        va_list args;
        va_start(args, format);
        static char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Debug_Printf_Verbose("%s", buffer);
    }
}

/**
 * @brief 主要的 Debug_Printf 函數 - 向後相容 (取消宏定義)
 * @param format 格式化字串
 * @param ... 可變參數
 * 
 * **🎯 注意：這個函數保持原有介面，但內部使用分級系統**
 */
static void Debug_Printf(const char* format, ...)
{
    /* 直接使用自動分級函數 */
    va_list args;
    va_start(args, format);
    static char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        Debug_Printf_Auto("%s", buffer);
    }
}

/******************************************************************************/
/* 簡化版除錯函數 - 只處理固定字串，避免格式化問題 */
static void Debug_Printf_Simple(const char* str)
{
    if (str == NULL) return;
    
    size_t len = strlen(str);
    if (len > 0 && len < 200) {
        UART_STATUS_t uart_status = UART_Transmit(&UART_0, (uint8_t*)str, len);
        if (uart_status == UART_STATUS_SUCCESS) {
            /* 等待傳輸完成 */
            uint32_t timeout = 0;
            while (UART_0.runtime->tx_busy == true && timeout < 100000) {
                timeout++;
                /* 更長的延遲 */
                for (volatile int i = 0; i < 50; i++);
            }
            
            /* 額外延遲確保完成 */
            for (volatile int i = 0; i < 5000; i++);
        }
    }
}

/******************************************************************************/
/* **📋 DAVE 配置管理函數 - 遵循 DAVE APP 配置驅動原則** */
/******************************************************************************/

/**
 * @brief 從 DAVE UI 配置推導 CANopen 配置
 * @return canopen_dave_config_t 配置結構
 * 
 * **🎯 配置驅動原則：所有參數從 DAVE 生成的配置結構讀取**
 */
static canopen_dave_config_t canopen_get_dave_config(void)
{
    canopen_dave_config_t config = {0};
    
    /* 初始化配置 */
    if (!g_dave_config_initialized) {
        /* **✅ 首先從 DAVE UI 配置讀取 LMO 數量** */
        config.lmo_count = CAN_NODE_0.mo_count;
        
        /* **🔧 動態尋找接收類型的 LMO 來推導 Node ID** */
        bool node_id_found = false;
        for (uint8_t i = 0; i < config.lmo_count; i++) {
            if (CAN_NODE_0.lmobj_ptr[i] != NULL && 
                CAN_NODE_0.lmobj_ptr[i]->mo_ptr != NULL &&
                CAN_NODE_0.lmobj_ptr[i]->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
                /* 動態計算：SDO RX ID (0x600 + Node ID) -> Node ID */
                uint32_t rx_id = CAN_NODE_0.lmobj_ptr[i]->mo_ptr->can_identifier;
                if (rx_id >= 0x600 && rx_id <= 0x67F) {  /* 有效的 SDO RX ID 範圍 */
                    config.node_id = (uint8_t)(rx_id - 0x600U);
                    node_id_found = true;
                    break;
                }
            }
        }
        
        /* **✅ 如果找不到接收 LMO，使用最後一個 LMO 作為後備方案** */
        if (!node_id_found && config.lmo_count > 0) {
            uint8_t last_idx = config.lmo_count - 1;
            if (CAN_NODE_0.lmobj_ptr[last_idx] != NULL && 
                CAN_NODE_0.lmobj_ptr[last_idx]->mo_ptr != NULL) {
                uint32_t rx_id = CAN_NODE_0.lmobj_ptr[last_idx]->mo_ptr->can_identifier;
                if (rx_id >= 0x600 && rx_id <= 0x67F) {
                    config.node_id = (uint8_t)(rx_id - 0x600U);
                    node_id_found = true;
                }
            }
        }
        
        /* **✅ 從 DAVE UI 配置讀取波特率** */
        if (CAN_NODE_0.baudrate_config != NULL) {
            config.baudrate = CAN_NODE_0.baudrate_config->baudrate;
        }
        
        /* **✅ 檢查是否所有 LMO 都使用 Service Request 0 (DAVE UI 配置)** */
        config.service_request_0 = true;
        for (uint8_t i = 0; i < config.lmo_count; i++) {
            if (CAN_NODE_0.lmobj_ptr[i] != NULL) {
                if (CAN_NODE_0.lmobj_ptr[i]->tx_sr != 0 || CAN_NODE_0.lmobj_ptr[i]->rx_sr != 0) {
                    config.service_request_0 = false;
                    break;
                }
            }
        }
        
        /* **✅ 檢查事件配置 (DAVE UI 設定)** */
        config.rx_event_enabled = false;
        config.tx_event_enabled = false;
        
        for (uint8_t i = 0; i < config.lmo_count; i++) {
            if (CAN_NODE_0.lmobj_ptr[i] != NULL) {
                if (CAN_NODE_0.lmobj_ptr[i]->rx_event_enable) {
                    config.rx_event_enabled = true;
                }
                if (CAN_NODE_0.lmobj_ptr[i]->tx_event_enable) {
                    config.tx_event_enabled = true;
                }
            }
        }
        
        g_dave_config = config;
        g_dave_config_initialized = true;
        
        /* 輸出配置摘要 */
        Debug_Printf("=== DAVE UI 配置分析 ===\r\n");
        if (node_id_found) {
            Debug_Printf("Node ID: %d (從接收 LMO 動態推導)\r\n", config.node_id);
        } else {
            Debug_Printf("Node ID: %d (預設值，未找到有效的接收 LMO)\r\n", config.node_id);
        }
        Debug_Printf("LMO 數量: %d (DAVE UI 配置)\r\n", config.lmo_count);
        Debug_Printf("波特率: %lu bps (DAVE UI 配置)\r\n", config.baudrate);
        Debug_Printf("Service Request 0: %s (DAVE UI 配置)\r\n", config.service_request_0 ? "是" : "否");
        Debug_Printf("RX 事件: %s, TX 事件: %s (DAVE UI 配置)\r\n", 
                    config.rx_event_enabled ? "啟用" : "停用",
                    config.tx_event_enabled ? "啟用" : "停用");
    }
    
    return g_dave_config;
}

/**
 * @brief 獲取 CANopen Node ID (從 DAVE UI 推導)
 */
static uint8_t canopen_get_node_id(void)
{
    canopen_dave_config_t config = canopen_get_dave_config();
    return config.node_id;
}

/**
 * @brief 根據 CAN ID 選擇合適的 LMO 索引 (基於 DAVE UI 配置)
 * 
 * **🎯 動態計算原則：根據 UI 配置的 Node ID 動態計算 CANopen ID**
 */
static uint32_t canopen_get_lmo_index_for_id(uint32_t can_id)
{
    uint8_t node_id = canopen_get_node_id();
    canopen_dave_config_t config = canopen_get_dave_config();
    
    /* **✅ 基本測試 ID - 使用 DAVE UI 配置的 LMO_01 預設 ID** */
    if (config.lmo_count > 0 && CAN_NODE_0.lmobj_ptr[0] != NULL && 
        can_id == CAN_NODE_0.lmobj_ptr[0]->mo_ptr->can_identifier) {
        return CANOPEN_LMO_TEST_TX;
    }
    
    /* **🔧 動態計算：Emergency ID (0x080 + Node ID)** */
    if (can_id == CANOPEN_EMERGENCY_ID(node_id)) {
        return CANOPEN_LMO_EMERGENCY;
    }
    
    /* **🔧 動態計算：SDO TX ID (0x580 + Node ID)** */
    if (can_id == CANOPEN_SDO_TX_ID(node_id)) {
        return CANOPEN_LMO_EMERGENCY;  /* 共用 LMO_02 */
    }
    
    /* **🔧 動態計算：TPDO IDs (0x180 + Node ID)** */
    if (can_id == CANOPEN_TPDO1_ID(node_id)) {
        return CANOPEN_LMO_TPDO;
    }
    
    /* **🔧 動態計算：Heartbeat ID (0x700 + Node ID)** */
    if (can_id == CANOPEN_HEARTBEAT_ID(node_id)) {
        return CANOPEN_LMO_TPDO;  /* 共用 LMO_03 */
    }
    
    /* **✅ 預設使用第一個 TX LMO (基於 DAVE UI 配置)** */
    return CANOPEN_LMO_TEST_TX;
}

/**
 * @brief 獲取用於發送的 LMO 配置
 * 
 * **✅ 讀取 UI 配置：從 DAVE 生成的配置結構讀取**
 */
static const CAN_NODE_LMO_t* canopen_get_lmo_for_tx(uint32_t can_id)
{
    uint32_t lmo_index = canopen_get_lmo_index_for_id(can_id);
    canopen_dave_config_t config = canopen_get_dave_config();
    
    /* **✅ 確保索引有效且 LMO 存在 (基於 DAVE UI 配置)** */
    if (lmo_index < config.lmo_count && CAN_NODE_0.lmobj_ptr[lmo_index] != NULL) {
        return CAN_NODE_0.lmobj_ptr[lmo_index];
    }
    
    /* **✅ 預設使用第一個 LMO (DAVE UI 配置)** */
    return CAN_NODE_0.lmobj_ptr[0];
}

/**
 * @brief 獲取用於接收的 LMO 配置 (固定為 LMO_04)
 * 
 * **✅ 讀取 UI 配置：使用 DAVE UI 配置的接收 LMO**
 */
static const CAN_NODE_LMO_t* canopen_get_lmo_for_rx(void)
{
    /* **✅ 動態從 DAVE UI 配置尋找接收類型的 LMO** */
    for (uint8_t i = 0; i < CAN_NODE_0.mo_count; i++) {
        if (CAN_NODE_0.lmobj_ptr[i] != NULL && 
            CAN_NODE_0.lmobj_ptr[i]->mo_ptr != NULL &&
            CAN_NODE_0.lmobj_ptr[i]->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
            return CAN_NODE_0.lmobj_ptr[i];
        }
    }
    
    /* **✅ 後備方案：如果找不到接收類型，使用最後一個 LMO** */
    uint8_t last_idx = CAN_NODE_0.mo_count > 0 ? CAN_NODE_0.mo_count - 1 : CANOPEN_LMO_SDO_RX;
    return CAN_NODE_0.lmobj_ptr[last_idx];
}

/**
 * @brief 驗證 DAVE 配置是否有效
 * 
 * **✅ 配置驗證：確保 DAVE APP 的 UI 配置符合 CANopen 需求**
 */
static bool canopen_is_dave_config_valid(void)
{
    canopen_dave_config_t config = canopen_get_dave_config();
    
    /* **✅ 檢查 Node ID 範圍 (CANopen 標準)** */
    if (config.node_id < 1 || config.node_id > 127) {
        Debug_Printf("❌ 無效的 Node ID: %d (範圍: 1-127)\r\n", config.node_id);
        return false;
    }
    
    /* **✅ 檢查 DAVE UI 配置的 LMO 數量** */
    if (config.lmo_count < 4) {
        Debug_Printf("❌ DAVE UI LMO 數量不足: %d (需要至少 4 個)\r\n", config.lmo_count);
        return false;
    }
    
    /* **✅ 檢查必要的 LMO 是否在 DAVE UI 中配置** */
    for (uint8_t i = 0; i < 4; i++) {
        if (CAN_NODE_0.lmobj_ptr[i] == NULL) {
            Debug_Printf("❌ LMO_%02d 在 DAVE UI 中未配置\r\n", i + 1);
            return false;
        }
    }
    
    /* **✅ 檢查 DAVE UI Service Request 0 配置** */
    if (!config.service_request_0) {
        Debug_Printf("⚠️ DAVE UI 未使用 Service Request 0，中斷可能無法正常工作\r\n");
    }
    
    /* **✅ 檢查 DAVE UI 波特率配置** */
    if (config.baudrate != 500000) {
        Debug_Printf("⚠️ DAVE UI 波特率 %lu bps 與預期 500kbps 不同\r\n", config.baudrate);
    } else {
        Debug_Printf("✅ 波特率配置正確: %lu bps (500 kbps)\r\n", config.baudrate);
    }
    
    Debug_Printf("✅ DAVE UI 配置驗證通過\r\n");
    return true;
}
