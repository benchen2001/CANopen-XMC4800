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

/* Forward declarations */
static void Debug_Printf(const char* format, ...);       /* 正常版本 */
static void Debug_Printf_ISR(const char* format, ...);  /* 中斷安全版本 */
void Debug_ProcessISRBuffer(void);                       /* ISR 緩衝區處理函數 */
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
                /* 使用 XMC 直接 API 檢查 TX 狀態 (DAVE 未提供狀態檢查 API) */
                uint32_t status = XMC_CAN_MO_GetStatus(tx_lmo->mo_ptr);
                if (status & XMC_CAN_MO_STATUS_TX_PENDING) {
                    /* 🎯 按照 STM32 模式：不在中斷中手動清除硬體狀態，讓硬體自動管理 */
                    g_interrupt_tx_count++;
                    CO_CANinterrupt_Tx(g_CANmodule, lmo_idx);
                    event_handled = true;
                }
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

    Debug_Printf("✅ CANopen 軟體層初始化完成\r\n");
    Debug_Printf("🚨 注意：所有硬體已由 DAVE_Init() 配置\r\n");

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
    
    if (tx_lmo != NULL && tx_lmo->mo_ptr != NULL) {
        /* **🔧 完全使用 DAVE API 進行發送** */
        
        /* **✅ 優先使用 DAVE API 更新 ID** */
        CAN_NODE_MO_UpdateID(tx_lmo, buffer->ident & 0x7FF);
        
        /* **⚠️ DLC 設定：DAVE 未提供 API，必須直接設定** */
        tx_lmo->mo_ptr->can_data_length = buffer->DLC;
        
        /* **✅ 優先使用 DAVE API 更新數據** */
        CAN_NODE_STATUS_t update_status = CAN_NODE_MO_UpdateData(tx_lmo, buffer->data);
        
        if (update_status == CAN_NODE_STATUS_SUCCESS) {
            /* **✅ 優先使用 DAVE API 發送** */
            CAN_NODE_STATUS_t tx_status = CAN_NODE_MO_Transmit(tx_lmo);
            
            if (tx_status == CAN_NODE_STATUS_SUCCESS) {
                buffer->bufferFull = false;
                CANmodule->CANtxCount++;
                Debug_Printf("TX OK: ID=0x%03X via DAVE LMO_%02d (MO%d)\r\n", 
                            buffer->ident, (tx_lmo->number - 32) + 1, tx_lmo->number);
            } else {
                buffer->bufferFull = true;
                Debug_Printf("TX ERROR: ID=0x%03X, DAVE status=%d\r\n", buffer->ident, tx_status);
                err = CO_ERROR_TX_OVERFLOW;
            }
        } else {
            buffer->bufferFull = true;
            Debug_Printf("UPDATE ERROR: ID=0x%03X, DAVE update status=%d\r\n", buffer->ident, update_status);
            err = CO_ERROR_TX_OVERFLOW;
        }
    } else {
        Debug_Printf("ERROR: 無法獲取 ID=0x%03X 的 LMO\r\n", buffer->ident);
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
}

/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
    /* 使用 CAN_NODE - 正常模式啟動 */
    if (CANmodule != NULL) {
        CANmodule->CANnormal = true;
        Debug_Printf("=== CAN_NODE 模式激活 ===\r\n");
        
        /* **✅ 使用配置管理函數獲取 RX LMO，避免硬編碼索引** */
        const CAN_NODE_LMO_t *rx_lmo = canopen_get_lmo_for_rx();
        if (rx_lmo != NULL) {
            Debug_Printf("RX LMO 接收事件: %s\r\n", rx_lmo->rx_event_enable ? "已啟用" : "未啟用");
        } else {
            Debug_Printf("❌ 無法獲取 RX LMO 配置\r\n");
        }
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
/* Debug 輸出函式 - 正常版本 (非中斷使用) */
static void Debug_Printf(const char* format, ...)
{
    static char debug_buffer[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(debug_buffer, sizeof(debug_buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        UART_STATUS_t uart_status = UART_Transmit(&UART_0, (uint8_t*)debug_buffer, len);
        if (uart_status == UART_STATUS_SUCCESS) {
            /* 🎯 關鍵修正：不在中斷中等待 UART 完成 */
            /* while (UART_0.runtime->tx_busy == true) { } */
            
            /* 只在非中斷環境中等待完成 */
            uint32_t timeout = 0;
            while (UART_0.runtime->tx_busy == true && timeout < 10000) {
                timeout++;
                /* 簡單的超時保護 */
            }
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
    if (config.baudrate != 250000) {
        Debug_Printf("⚠️ DAVE UI 波特率 %lu bps 與 CANopen 建議 250kbps 不同\r\n", config.baudrate);
    }
    
    Debug_Printf("✅ DAVE UI 配置驗證通過\r\n");
    return true;
}
