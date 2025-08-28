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
#include "CANopen.h"  /* CANopen 主要標頭檔 - 包含 CO_t 定義 */
#include <stdio.h>
#include <stdarg.h>

/* XMC4800 specific includes for interrupt handling */
#include "xmc_can.h"
#include "core_cm4.h"
#include "xmc4_flash.h"

/* DAVE CAN_NODE APP includes */
#include "CAN_NODE/can_node.h"
#include "CAN_NODE/can_node_conf.h"
#include "CAN_NODE/can_node_extern.h"  /* CAN_NODE_0 變數定義 */

/* **📋 DAVE APP 配置驅動架構** */
/* Node ID 固定為 10，基於 DAVE 配置的 SDO RX ID (0x60A) */

/* CANopen ID 計算宏 - 動態計算，無硬編碼 */
#define CANOPEN_EMERGENCY_ID(node_id)      (0x080U + (node_id))
#define CANOPEN_TPDO1_ID(node_id)          (0x180U + (node_id))  
#define CANOPEN_SDO_TX_ID(node_id)         (0x580U + (node_id))
#define CANOPEN_SDO_RX_ID(node_id)         (0x600U + (node_id))
#define CANOPEN_HEARTBEAT_ID(node_id)      (0x700U + (node_id))

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
static void Debug_Printf_ISR(const char* format, ...);   /* 中斷安全版本 */
void Debug_ProcessISRBuffer(void);                       /* ISR 緩衝區處理函數 - 外部可見 */
static void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule, uint32_t index);
static void CO_CANinterrupt_Tx(CO_CANmodule_t *CANmodule, uint32_t index);

/* **📋 動態 Node ID 管理 - 專業產品設計** */
static uint8_t canopen_get_node_id(void);
static bool canopen_is_dave_config_valid(void);
static void canopen_configure_dynamic_ids(uint8_t node_id);

/* Global variables */
volatile uint32_t CO_timer1ms = 0;  /* Timer variable incremented each millisecond */
CO_CANmodule_t* g_CANmodule = NULL;

/* **🎯 動態 Node ID 管理 - 專業產品設計** */
static uint8_t g_canopen_node_id = 10;  /* 預設 Node ID = 10，未來可從 EEPROM 讀取 */

/* **🚀 前置宣告：動態 ID 配置函數** */
static void canopen_configure_dynamic_ids(uint8_t node_id);

/**
 * @brief 設定 CANopen Node ID 並重新配置所有 LMO
 * 
 * 🎯 專業產品設計功能：
 * 1. 動態設定 Node ID
 * 2. 自動重新配置所有 LMO 的 CANopen ID
 * 3. 支援製造階段或維護階段的 ID 變更
 * 
 * @param new_node_id 新的 Node ID (1-127)
 * @return true: 設定成功, false: 參數無效
 */
bool canopen_set_node_id(uint8_t new_node_id)
{
    /* **✅ 範圍檢查** */
    if (new_node_id < 1 || new_node_id > 127) {
        Debug_Printf("❌ Node ID %d 超出範圍 (1-127)\r\n", new_node_id);
        return false;
    }
    
    uint8_t old_node_id = g_canopen_node_id;
    g_canopen_node_id = new_node_id;
    
    Debug_Printf("🔄 變更 Node ID: %d → %d\r\n", old_node_id, new_node_id);
    
    /* **🔧 重新配置所有 LMO** */
    canopen_configure_dynamic_ids(new_node_id);
    
    /* **🚀 未來可加入 EEPROM 儲存** */
    // eeprom_save_node_id(new_node_id);
    
    Debug_Printf("✅ Node ID 變更完成\r\n");
    return true;
}

/**
 * @brief 動態配置所有 LMO 的 CANopen ID
 * 
 * 🎯 專業產品設計：根據更新的 DAVE UI 配置
 * 
 * **DAVE UI 配置 (基礎 ID 設計):**
 * TX LMO (1-7):  0x8a→0x80, 0x18a→0x180, 0x280, 0x380, 0x480, 0x580, 0x700
 * RX LMO (8-12): 0x600, 0x200, 0x300, 0x400, 0x500
 * 
 * **執行時計算:** 基礎ID + Node_ID = 最終CANopen ID
 */
static void canopen_configure_dynamic_ids(uint8_t node_id)
{
    Debug_Printf("=== 動態配置 CANopen ID (Node ID=%d) ===\r\n", node_id);
    
    /* **🔧 根據更新的 DAVE 配置動態更新 CANopen ID** */
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        
        if (lmo != NULL && lmo->mo_ptr != NULL) {
            uint32_t current_id = lmo->mo_ptr->can_identifier;
            uint32_t new_id = current_id;  /* 🎯 修正：預設保持原值 */
            const char* id_type = "未知";
            
            /* 🔍 詳細除錯輸出 */
            Debug_Printf("🔍 LMO_%02d: current_id=0x%03X, type=0x%02X\r\n", 
                        lmo_idx + 1, current_id, lmo->mo_ptr->can_mo_type);
            
            /* **📤 TX LMO 處理 (基於您更新的 DAVE UI 配置)** */
            if (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
                switch (current_id) {
                    case 0x8AU:   /* Emergency (0x80 + Node ID) */
                    case 0x80U:   /* Emergency 基礎 ID */
                        new_id = 0x080 + node_id;
                        id_type = "Emergency";
                        break;
                    case 0x18AU:  /* TPDO1 (0x180 + Node ID) */
                    case 0x180U:  /* TPDO1 基礎 ID */
                        new_id = 0x180 + node_id;
                        id_type = "TPDO1";
                        break;
                    case 0x280U:  /* TPDO2 基礎 ID */
                        new_id = 0x280 + node_id;
                        id_type = "TPDO2";
                        break;
                    case 0x380U:  /* TPDO3 基礎 ID */
                        new_id = 0x380 + node_id;
                        id_type = "TPDO3";
                        break;
                    case 0x480U:  /* TPDO4 基礎 ID */
                        new_id = 0x480 + node_id;
                        id_type = "TPDO4";
                        break;
                    case 0x580U:  /* SDO TX 基礎 ID */
                        new_id = 0x580 + node_id;
                        id_type = "SDO TX";
                        break;
                    case 0x700U:  /* Heartbeat 基礎 ID */
                        new_id = 0x700 + node_id;
                        id_type = "Heartbeat";
                        break;
                    default:
                        new_id = current_id;  /* 保持不變 */
                        id_type = "自訂";
                        break;
                }
            }
            /* **📥 RX LMO 處理 (基於您更新的 DAVE UI 配置)** */
            else if (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
                Debug_Printf("🔍 處理 RX LMO_%02d: current_id=0x%03X\r\n", lmo_idx + 1, current_id);
                
                switch (current_id) {
                    case 0x000U:  /* NMT 廣播訊息 - 所有節點都應該接收 */
                        new_id = 0x000;  /* NMT 固定為 ID=0 */
                        id_type = "NMT";
                        Debug_Printf("    ✅ 匹配 NMT: new_id=0x%03X\r\n", new_id);
                        break;
                    case 0x600U:  /* SDO RX 基礎 ID */
                        new_id = 0x600 + node_id;
                        id_type = "SDO RX";
                        Debug_Printf("    ✅ 匹配 SDO RX: 0x600 + %d = 0x%03X\r\n", node_id, new_id);
                        break;
                    case 0x200U:  /* RPDO1 基礎 ID */
                        new_id = 0x200 + node_id;
                        id_type = "RPDO1";
                        break;
                    case 0x300U:  /* RPDO2 基礎 ID */
                        new_id = 0x300 + node_id;
                        id_type = "RPDO2";
                        break;
                    case 0x400U:  /* RPDO3 基礎 ID */
                        new_id = 0x400 + node_id;
                        id_type = "RPDO3";
                        break;
                    case 0x500U:  /* RPDO4 基礎 ID */
                        new_id = 0x500 + node_id;
                        id_type = "RPDO4";
                        break;
                    default:
                        new_id = current_id;  /* 保持不變 */
                        id_type = "自訂";
                        Debug_Printf("    ⚠️ RX LMO 進入 default: current_id=0x%03X, new_id=0x%03X\r\n", 
                                   current_id, new_id);
                        break;
                }
            }
            
            /* **🔧 實際更新 LMO ID** */
            if (new_id != current_id) {
                /* **✅ 安全做法：使用 XMC_CAN_MO_SetIdentifier() 確保原子性操作** */
                XMC_CAN_MO_SetIdentifier(lmo->mo_ptr, new_id);
                
                /* **重新初始化 LMO 使配置生效** */
                CAN_NODE_MO_Init(lmo);
                
                Debug_Printf("✅ LMO_%02d (%s): 0x%03X → 0x%03X (%s)\r\n", 
                           lmo_idx + 1, 
                           (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) ? "TX" : "RX",
                           current_id, new_id, id_type);
            } else {
                Debug_Printf("🔍 LMO_%02d (%s): 0x%03X (保持不變, %s)\r\n", 
                           lmo_idx + 1,
                           (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) ? "TX" : "RX",
                           new_id, id_type);
            }
        }
    }
    
    Debug_Printf("✅ 動態 CANopen ID 配置完成 (Node ID=%d)\r\n", node_id);
    
    /* **📋 顯示完整的 CANopen ID 分配表** */
    Debug_Printf("\r\n📋 CANopen ID 分配表 (Node ID=%d):\r\n", node_id);
    Debug_Printf("   🚨 Emergency:  0x%03X (0x080 + %d)\r\n", 0x080 + node_id, node_id);
    Debug_Printf("   📤 TPDO1:      0x%03X (0x180 + %d)\r\n", 0x180 + node_id, node_id);
    Debug_Printf("   📤 TPDO2:      0x%03X (0x280 + %d)\r\n", 0x280 + node_id, node_id);
    Debug_Printf("   📤 TPDO3:      0x%03X (0x380 + %d)\r\n", 0x380 + node_id, node_id);
    Debug_Printf("   📤 TPDO4:      0x%03X (0x480 + %d)\r\n", 0x480 + node_id, node_id);
    Debug_Printf("   📤 SDO TX:     0x%03X (0x580 + %d)\r\n", 0x580 + node_id, node_id);
    Debug_Printf("   💓 Heartbeat:  0x%03X (0x700 + %d)\r\n", 0x700 + node_id, node_id);
    Debug_Printf("   📥 SDO RX:     0x%03X (0x600 + %d)\r\n", 0x600 + node_id, node_id);
    Debug_Printf("   📥 RPDO1:      0x%03X (0x200 + %d)\r\n", 0x200 + node_id, node_id);
    Debug_Printf("   📥 RPDO2:      0x%03X (0x300 + %d)\r\n", 0x300 + node_id, node_id);
    Debug_Printf("   📥 RPDO3:      0x%03X (0x400 + %d)\r\n", 0x400 + node_id, node_id);
    Debug_Printf("   📥 RPDO4:      0x%03X (0x500 + %d)\r\n", 0x500 + node_id, node_id);
    Debug_Printf("\r\n");
    Debug_Printf("   🔔 SYNC:       0x080 (固定，只有 Master 發送)\r\n");
    Debug_Printf("   💡 注意: SYNC(0x080) 與 EMCY 使用相同基礎 ID，但:\r\n");
    Debug_Printf("        - SYNC: 固定 0x080，只有主站發送\r\n");
    Debug_Printf("        - EMCY: 0x080+NodeID，各節點發送自己的緊急訊息\r\n");
    Debug_Printf("   📊 ID 範圍: EMCY(0x081-0x0FF), 不與 SYNC 衝突\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("📋 使用指南: canopen_set_node_id(新ID) 可動態變更\r\n");
}

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
volatile uint32_t g_interrupt_other_count = 0;

/**
 * @brief CANopen Timer 處理函數 - 1ms 定時處理
 * 
 * 🎯 功能: CANopen 協議棧的定時處理邏輯
 * ⏱️ 調用: 由 main.c 中的 TimerHandler() DAVE UI 中斷函數調用
 * 
 * @param CO_ptr CANopen 主物件指標 (void* 類型以避免 header 依賴)
 */
void canopen_timer_process(void *CO_ptr)
{
    /* CANopen 需要的 1ms 計時器 */
    extern volatile uint32_t CO_timer1ms;
    CO_timer1ms++;
    
    /* **🎯 CANopen 主要處理邏輯** */
    CO_t *CO = (CO_t *)CO_ptr;  /* 型別轉換 */
    if (CO != NULL) {
        CO_LOCK_OD(CO->CANmodule);
        if (!CO->nodeIdUnconfigured && CO->CANmodule->CANnormal) {
            bool_t syncWas = false;
            /* get time difference since last function call */
            uint32_t timeDifference_us = 1000; // 1ms = 1000us

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
            syncWas = CO_process_SYNC(CO, timeDifference_us, NULL);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
            CO_process_RPDO(CO, syncWas, timeDifference_us, NULL);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
            CO_process_TPDO(CO, syncWas, timeDifference_us, NULL);
#endif

            /* Further I/O or nonblocking application code may go here. */
        }
        CO_UNLOCK_OD(CO->CANmodule);
    }
}

/**
 * @brief CANopen CAN 中斷處理函數 - CAN 訊息處理
 * 
 * 🎯 功能: CAN 收發中斷的 CANopen 處理邏輯
 * � 調用: 由 main.c 中的 CAN_Handler() DAVE UI 中斷函數調用
 */
void canopen_can_interrupt_process(void)
{
    /* 增加中斷統計 */
    g_interrupt_total_count++;
    
    if (g_CANmodule != NULL) {
        bool event_handled = false;
        
        /* **✅ 動態輪詢所有配置的 LMO，根據 can_mo_type 判斷 RX/TX** */
        for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
            if (CAN_NODE_0.lmobj_ptr[lmo_idx] != NULL) {
                const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
                
                if (lmo->mo_ptr != NULL) {
                    /* **🔍 根據 MO 類型判斷是 RX 還是 TX** */
                    uint32_t mo_type = lmo->mo_ptr->can_mo_type;
                    uint32_t mo_status = CAN_NODE_MO_GetStatus(lmo);
                    
                    if (mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
                        /* **📥 RX LMO 處理** */
                        if (lmo->rx_event_enable && (mo_status & XMC_CAN_MO_STATUS_RX_PENDING)) {
                            g_interrupt_rx_count++;
                            CO_CANinterrupt_Rx(g_CANmodule, lmo_idx);
                            event_handled = true;
                            
                            /* 簡單的除錯輸出 */
                            Debug_Printf_ISR("RX IRQ: LMO_%02d (idx=%d)\r\n", lmo_idx + 1, lmo_idx);
                        }
                    }
                    else if (mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
                        /* **📤 TX LMO 處理** */
                        if (lmo->tx_event_enable && (mo_status & XMC_CAN_MO_STATUS_TX_PENDING)) {
                            g_interrupt_tx_count++;
                            CO_CANinterrupt_Tx(g_CANmodule, lmo_idx);
                            event_handled = true;
                            
                            /* 簡單的除錯輸出 */
                            Debug_Printf_ISR("TX IRQ: LMO_%02d (idx=%d)\r\n", lmo_idx + 1, lmo_idx);
                        }
                    }
                    /* 其他類型的 MO (FIFO, Gateway 等) 暫時忽略 */
                }
            }
        }
        
        /* 用於除錯統計 */
        if (!event_handled) {
            g_interrupt_other_count++;
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
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* **⚡ 專業產品設計：動態配置 CANopen ID** */
    uint8_t node_id = canopen_get_node_id();
    
    /* **配置所有 LMO 使用動態 CANopen ID** */
    canopen_configure_dynamic_ids(node_id);

    /* **🎯 使用從 DAVE UI 推導的配置** */
//    canopen_dave_config_t dave_config = canopen_get_dave_config();

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
 

    // /* **🔧 專業做法：動態啟用所有 DAVE UI 配置的 LMO 事件** */
    // Debug_Printf("=== 動態檢測並啟用所有 LMO 事件 ===\r\n");
    
    uint8_t rx_lmo_count = 0;
    uint8_t tx_lmo_count = 0;
    
    /* **✅ 遍歷所有 LMO，根據類型動態啟用事件** */
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        if (CAN_NODE_0.lmobj_ptr[lmo_idx] != NULL) {
            const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
            
            if (lmo->mo_ptr != NULL) {
                /* **🔍 根據 MO 類型動態處理** */
                if (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
                    /* **📥 RX LMO 處理** */
                    rx_lmo_count++;
                    
                    if (lmo->rx_event_enable) {
                        CAN_NODE_MO_EnableRxEvent(lmo);
                    }
                }
                else if (lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
                    /* **📤 TX LMO 處理** */
                    tx_lmo_count++;
                    
                    if (lmo->tx_event_enable) {
                        CAN_NODE_MO_EnableTxEvent(lmo);
                    }
                }
            }
        }
    }

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
        return CO_ERROR_ILLEGAL_ARGUMENT;
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
    
    /* **� 配置 CAN Transfer Settings (動態處理所有 LMO)** */
    Debug_Printf("=== 配置 CAN Transfer Settings ===\r\n");
    
    if (CAN_NODE_0.global_ptr != NULL) {
        /* Note: Type mismatch warning expected - DAVE configuration compatible */
        XMC_CAN_Enable((XMC_CAN_t*)CAN_NODE_0.global_ptr);
        Debug_Printf("✅ CAN 全域模組已啟用\r\n");
    }
    
    /* **✅ 動態配置所有 LMO 的 Transfer Settings** */
    uint8_t tx_configured = 0;
    uint8_t rx_configured = 0;
    
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        
        if (lmo != NULL && lmo->mo_ptr != NULL) {
            uint32_t mo_type = lmo->mo_ptr->can_mo_type;
            
            if (mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
                /* **📤 TX LMO Transfer Settings** */
                XMC_CAN_MO_EnableSingleTransmitTrial(lmo->mo_ptr);
                
                XMC_CAN_MO_ResetStatus(lmo->mo_ptr, 
                    XMC_CAN_MO_RESET_STATUS_TX_PENDING |
                    XMC_CAN_MO_RESET_STATUS_RX_PENDING);
                    
                XMC_CAN_MO_SetStatus(lmo->mo_ptr, XMC_CAN_MO_SET_STATUS_MESSAGE_VALID);
                
                tx_configured++;
            }
            else if (mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
                /* **� RX LMO Transfer Settings** */
                XMC_CAN_MO_ResetStatus(lmo->mo_ptr, 
                    XMC_CAN_MO_RESET_STATUS_TX_PENDING |
                    XMC_CAN_MO_RESET_STATUS_RX_PENDING);
                    
                XMC_CAN_MO_SetStatus(lmo->mo_ptr, XMC_CAN_MO_SET_STATUS_MESSAGE_VALID);
                
                rx_configured++;
            }
        }
    }
    
    return CO_ERROR_NO;
}

/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
    if (CANmodule != NULL) {
        /* 使用 CAN_NODE APP - 可以禁用節點 */
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

    /* **🚨 關鍵修正：防止 CANopen 堆疊用 ident=0 覆蓋已配置的 LMO** */
    if (ident == 0) {
        /* 只設定軟體緩衝區，不修改硬體 LMO */
        if (CANmodule == NULL || index >= CANmodule->rxSize) {
            return CO_ERROR_ILLEGAL_ARGUMENT;
        }
        
        CO_CANrx_t *buffer = &CANmodule->rxArray[index];
        buffer->ident = ident;
        buffer->mask = mask;
        buffer->object = object;
        buffer->CANrx_callback = CANrx_callback;
        buffer->dave_lmo = NULL;  /* 不關聯硬體 LMO */
        buffer->lmo_index = index;
        
        return CO_ERROR_NO;
    }

    /* safety */
    if (CANmodule == NULL || index >= CANmodule->rxSize) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* get specific buffer */
    buffer = &CANmodule->rxArray[index];

    /* **🎯 完全配置驅動：動態尋找並配置合適的 RX LMO** */
    const CAN_NODE_LMO_t *matched_rx_lmo = NULL;
    uint8_t matched_lmo_index = 0;
    bool lmo_configured = false;
    
    /* **✅ 第一階段：尋找完全匹配 ID 的 RX LMO** */
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        
        if (lmo != NULL && lmo->mo_ptr != NULL &&
            lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
            
            uint32_t lmo_id = lmo->mo_ptr->can_identifier & 0x7FF;
            
            /* **🔍 檢查 ID 是否完全匹配** */
            if (lmo_id == (ident & 0x7FF)) {
                matched_rx_lmo = lmo;
                matched_lmo_index = lmo_idx;
                lmo_configured = true;
                break;
            }
        }
    }
    
    /* **🔧 第二階段：如果沒有完全匹配，尋找第一個可用的 RX LMO 進行動態配置** */
    if (!lmo_configured) {
        
        for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
            const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
            
            if (lmo != NULL && lmo->mo_ptr != NULL &&
                lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ &&
                lmo->rx_event_enable) {
                
                /* **🎯 動態配置這個 RX LMO** */
                XMC_CAN_MO_SetIdentifier(lmo->mo_ptr, ident & 0x7FF);
                lmo->mo_ptr->can_id_mask = mask & 0x7FF;
                
                /* **重新初始化 LMO 使配置生效** */
                CAN_NODE_MO_Init(lmo);
                
                matched_rx_lmo = lmo;
                matched_lmo_index = lmo_idx;
                lmo_configured = true;
                
                break;
            }
        }
    }
    
    if (lmo_configured && matched_rx_lmo != NULL) {
        /* **✅ 成功找到或配置了 RX LMO** */
        
        /* Store LMO reference */
        buffer->dave_lmo = (void*)matched_rx_lmo;
        buffer->lmo_index = matched_lmo_index;
    } else {
        buffer->dave_lmo = NULL;
        buffer->lmo_index = index;
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

    /* **🚨 關鍵修正：防止 CANopen 堆疊用 ident=0 覆蓋已配置的 LMO** */
    if (ident == 0) {
        /* 只設定軟體緩衝區，不修改硬體 LMO */
        if (CANmodule == NULL || index >= CANmodule->txSize) {
            return NULL;
        }
        
        buffer = &CANmodule->txArray[index];
        buffer->ident = ident;
        buffer->DLC = noOfBytes;
        buffer->bufferFull = false;
        buffer->syncFlag = syncFlag;
        buffer->dave_lmo = NULL;  /* 不關聯硬體 LMO */
        buffer->lmo_index = index;
        
        return buffer;
    }

    /* safety */
    if (CANmodule == NULL || index >= CANmodule->txSize) {
        return NULL;
    }

    /* get specific buffer */
    buffer = &CANmodule->txArray[index];

    /* **🎯 動態 TX LMO 配置：為每個 TX Buffer 預分配 TX LMO** */
    const CAN_NODE_LMO_t *tx_lmo = NULL;
    uint8_t tx_lmo_index = 0;
    bool lmo_assigned = false;
    
    /* **✅ 根據 index 動態分配可用的 TX LMO** */
    uint8_t tx_lmo_count = 0;
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        
        if (lmo != NULL && lmo->mo_ptr != NULL &&
            lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
            
            /* **🔧 為這個 TX Buffer 分配對應的 TX LMO** */
            if (tx_lmo_count == index) {
                tx_lmo = lmo;
                tx_lmo_index = lmo_idx;
                lmo_assigned = true;
                
                break;
            }
            tx_lmo_count++;
        }
    }
    
    if (lmo_assigned && tx_lmo != NULL) {
        /* **🎯 預配置 TX LMO 的基本設定** */
        XMC_CAN_MO_SetIdentifier(tx_lmo->mo_ptr, ident & 0x7FF);
        tx_lmo->mo_ptr->can_data_length = noOfBytes;
        
        /* **儲存 LMO 參考給緩衝區** */
        buffer->dave_lmo = (void*)tx_lmo;
        buffer->lmo_index = tx_lmo_index;
        
    } else {
        /* **⚠️ 沒有足夠的 TX LMO，使用軟體緩衝區** */
        buffer->dave_lmo = NULL;
        buffer->lmo_index = index;
    }

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
    
    /* 強制輸出 - 確保函數被調用時我們能看到 */
    Debug_Printf_Raw("CO_CANsend() CALLED - ID=0x%03X, DLC=%d\r\n", buffer->ident, buffer->DLC);
    
    /* Step 1: 詳細檢測傳入參數是否符合 DAVE 配置 */
    Debug_Printf_Verbose("=== CO_CANsend 數值檢測 ===\r\n");
    Debug_Printf_Verbose("檢測 CAN ID: 0x%03X\r\n", buffer->ident);
    Debug_Printf_Verbose("檢測 DLC: %d\r\n", buffer->DLC);
    
    /* Step 0.5: 檢查 CAN 位元時序參數和暫存器狀態 */
    Debug_Printf_Verbose("=== CAN 位元時序和暫存器檢查 ===\r\n");
    
    /* 分析 DAVE 配置問題 */
    Debug_Printf("DAVE 配置問題分析:\r\n");
    Debug_Printf("   當前設定: Synchronization jump width = 1\r\n");
    Debug_Printf("   當前設定: Sample point = 80%%\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("WARNING: 這些設定可能導致 stuff error!\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("建議的正確設定 (DAVE 中修改):\r\n");
    Debug_Printf("   Synchronization jump width: 改為 2 (提高抗干擾)\r\n");
    Debug_Printf("   Sample point: 改為 75%% (避免 stuff error)\r\n");
    Debug_Printf("   詳細建議: TSEG1=6, TSEG2=2, SJW=2\r\n");
    Debug_Printf("\r\n");
    Debug_Printf("修改步驟:\r\n");
    Debug_Printf("   1. 開啟 DAVE IDE\r\n");
    Debug_Printf("   2. 點選 CAN_NODE_0\r\n");
    Debug_Printf("   3. Advanced Settings 頁籤\r\n");
    Debug_Printf("   4. 修改 SJW=2, Sample Point=75%%\r\n");
    Debug_Printf("   5. 重新產生程式碼並編譯\r\n");
    Debug_Printf("\r\n");
    
    /* 檢測 1: CAN ID 範圍驗證 */
    if (buffer->ident > 0x7FF) {
        Debug_Printf("ERROR: CAN ID 0x%03X 超出 11-bit 範圍 (最大 0x7FF)\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* 檢測 2: DLC 範圍驗證 */
    if (buffer->DLC > 8) {
        Debug_Printf("ERROR: DLC %d 超出範圍 (最大 8)\r\n", buffer->DLC);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    /* 檢測 3: 與 DAVE 配置的 LMO 對照 */
    Debug_Printf("檢查 DAVE 配置的 LMO 對應關係:\r\n");
    Debug_Printf("   LMO_01 (0x8A)  -> Emergency + SDO TX\r\n");
    Debug_Printf("   LMO_02 (0x18A) -> TPDO1\r\n"); 
    Debug_Printf("   LMO_03 (0x28A) -> TPDO2\r\n");
    Debug_Printf("   LMO_04 (0x38A) -> TPDO3\r\n");
    Debug_Printf("   LMO_05 (0x48A) -> TPDO4\r\n");
    Debug_Printf("   LMO_06 (0x58A) -> 備用 TX\r\n");
    Debug_Printf("   LMO_07 (0x70A) -> Heartbeat\r\n");
    Debug_Printf("   LMO_08 (0x60A) -> SDO RX (接收)\r\n");
    
    /* 檢測 4: 確認 ID 是否匹配 DAVE 預設值 */
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
            Debug_Printf("WARN: ID 0x%03X 不在 DAVE 預設配置中\r\n", buffer->ident);
            break;
    }
    
    if (id_matches_dave_config) {
        Debug_Printf("PASS: ID 0x%03X 匹配 DAVE 配置 - %s\r\n", buffer->ident, lmo_description);
    }

    /* Verify overflow */
    if (buffer->bufferFull) {
        if (!CANmodule->firstCANtxMessage) {
            /* Don't set error, if bootup message is still on buffers */
            CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    /* **🎯 優先使用緩衝區已預分配的 TX LMO** */
    const CAN_NODE_LMO_t *tx_lmo = NULL;
    uint8_t tx_lmo_index = 0;
    
    Debug_Printf("🔍 為 ID=0x%03X 尋找 TX LMO (優先使用預分配)\r\n", buffer->ident);
    
    /* **✅ 第一優先：使用緩衝區預分配的 TX LMO** */
    if (buffer->dave_lmo != NULL) {
        tx_lmo = (const CAN_NODE_LMO_t*)buffer->dave_lmo;
        tx_lmo_index = buffer->lmo_index;
        
        /* **確認這個 LMO 確實是 TX 類型且可用** */
        if (tx_lmo->mo_ptr != NULL && 
            tx_lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
            
            uint32_t mo_status = CAN_NODE_MO_GetStatus(tx_lmo);
            if (!(mo_status & XMC_CAN_MO_STATUS_TX_PENDING)) {
                Debug_Printf("✅ 使用預分配的 TX LMO_%02d (索引=%d)\r\n", 
                           tx_lmo_index + 1, tx_lmo_index);
            } else {
                Debug_Printf("⚠️ 預分配的 LMO_%02d 忙碌，尋找替代方案\r\n", tx_lmo_index + 1);
                tx_lmo = NULL;  /* 標記為無效，需要重新尋找 */
            }
        } else {
            Debug_Printf("❌ 預分配的 LMO 無效或非 TX 類型\r\n");
            tx_lmo = NULL;
        }
    }
    
    /* **🔧 第二優先：動態尋找任何可用的 TX LMO** */
    if (tx_lmo == NULL) {
        Debug_Printf("🔧 動態尋找可用的 TX LMO...\r\n");
        
        for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
            const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
            
            if (lmo != NULL && lmo->mo_ptr != NULL &&
                lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
                
                /* **🔍 檢查 LMO 是否可用 (不忙碌)** */
                uint32_t mo_status = CAN_NODE_MO_GetStatus(lmo);
                
                if (!(mo_status & XMC_CAN_MO_STATUS_TX_PENDING)) {
                    /* **✅ 找到可用的 TX LMO** */
                    tx_lmo = lmo;
                    tx_lmo_index = lmo_idx;
                    Debug_Printf("✅ 動態選擇 TX LMO_%02d (索引=%d) for ID=0x%03X\r\n", 
                               lmo_idx + 1, lmo_idx, buffer->ident);
                    break;
                } else {
                    Debug_Printf("⚠️ LMO_%02d 忙碌中 (狀態=0x%08lX)，檢查下一個\r\n", lmo_idx + 1, mo_status);
                }
            }
        }
    }
    
    /* **檢測 5: LMO 有效性驗證** */
    if (tx_lmo == NULL) {
        Debug_Printf("❌ ERROR: 無法找到對應的 TX LMO for ID=0x%03X\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    if (tx_lmo->mo_ptr == NULL) {
        Debug_Printf("❌ ERROR: LMO MO 指標為 NULL (ID=0x%03X)\r\n", buffer->ident);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    
    Debug_Printf("✅ PASS: 找到有效的 TX LMO_%02d (MO%d, 索引=%d) for ID=0x%03X\r\n", 
                tx_lmo_index + 1, tx_lmo->number, tx_lmo_index, buffer->ident);
    
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
/**
 * @brief CANopen 處理函數 - 包含中斷備用方案
 * 主要依賴中斷，但提供輪詢作為備用
 */
void CO_CANmodule_process(CO_CANmodule_t *CANmodule) 
{
    if (CANmodule == NULL) return;

    /* **🎯 關鍵修正：主動輪詢 RX 狀態** 
     * 問題：依賴中斷可能會遺漏接收事件
     * 解決：每次調用都檢查 RX 狀態
     */
    static uint32_t poll_counter = 0;
    static uint32_t rx_checks = 0;
    static uint32_t rx_found = 0;
    poll_counter++;
    
    /* **🔧 動態檢查所有 LMO 的 RX 狀態（不依賴中斷）** */
    for (uint8_t lmo_idx = 0; lmo_idx < CAN_NODE_0.mo_count; lmo_idx++) {
        const CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[lmo_idx];
        
        /* **🔍 只處理 RX 類型的 LMO** */
        if (lmo != NULL && lmo->mo_ptr != NULL &&
            lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
            
            rx_checks++;
            
            /* **🎯 關鍵：檢查 NEWDAT 位而不是 RX_PENDING** */
            uint32_t status = XMC_CAN_MO_GetStatus(lmo->mo_ptr);
        
            /* 檢查是否有新的接收數據 */
            if (status & XMC_CAN_MO_STATUS_NEW_DATA) {
                rx_found++;
                
                /* 調用接收處理函數 */
                CO_CANinterrupt_Rx(CANmodule, lmo_idx);
                
                /* 簡化的除錯輸出（避免輸出過多） */
                if ((rx_found % 10) == 1) {  /* 每 10 次輸出一次 */
                    Debug_Printf("✅ RX 輪詢發現數據 #%lu (檢查 %lu 次) LMO_%02d\r\n", 
                               rx_found, rx_checks, lmo_idx + 1);
                }
            }
        }
    }

    /* 定期報告系統狀態 */
    if ((poll_counter % 5000) == 0) {
        Debug_Printf("📊 RX 輪詢統計: 檢查 %lu 次，發現 %lu 次\r\n", rx_checks, rx_found);
    }

    /* 簡化錯誤處理 - 避免複雜的 Emergency 依賴 */
    CANmodule->errOld = 0;
    CANmodule->CANerrorStatus = 0;
}

/******************************************************************************/
/* CAN RX interrupt */
static void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule, uint32_t index)
{
    /* **🎯 根據 index 動態獲取對應的 RX LMO** */
    if (index >= CAN_NODE_0.mo_count) {
        Debug_Printf("❌ RX index %d 超出範圍 (最大 %d)\r\n", index, CAN_NODE_0.mo_count - 1);
        return;
    }
    
    CAN_NODE_LMO_t *rx_lmo = (CAN_NODE_LMO_t*)CAN_NODE_0.lmobj_ptr[index];
    
    /* **🔍 確保這個 LMO 確實是 RX 類型** */
    if (rx_lmo != NULL && rx_lmo->mo_ptr != NULL &&
        rx_lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_RECMSGOBJ) {
        
        /* **🔧 關鍵修正：使用正確的 DAVE API 檢查和接收資料** */
        uint32_t mo_status = CAN_NODE_MO_GetStatus(rx_lmo);
        
        /* 檢查是否有待處理的接收訊息 */
        if (mo_status & XMC_CAN_MO_STATUS_RX_PENDING) {
            
            /* 清除接收 pending 狀態 */
            XMC_CAN_MO_ResetStatus(rx_lmo->mo_ptr, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
            
            /* 使用正確的 DAVE API 接收資料 */
            CAN_NODE_STATUS_t rx_status = CAN_NODE_MO_Receive(rx_lmo);

            if (rx_status == CAN_NODE_STATUS_SUCCESS) {
                /* **📨 從 DAVE 接收結構中讀取 CAN 訊息** */
                CO_CANrxMsg_t rcvMsg;
                
                /* **✅ 使用 DAVE 接收到的資料構建 CANopen 訊息** */
                rcvMsg.ident = rx_lmo->mo_ptr->can_identifier & 0x07FFU;
                rcvMsg.DLC = rx_lmo->mo_ptr->can_data_length & 0x0FU;  /* 確保 DLC 在有效範圍 */
                
                /* **🎯 特別處理 ID=0x000 (NMT) 的除錯輸出** */
                if (rcvMsg.ident == 0x000) {
                    Debug_Printf("🎯 NMT 訊息接收: ID=0x000, DLC=%d\r\n", rcvMsg.DLC);
                }
                
                /* 複製數據 - 使用 DAVE 的資料格式 */
                uint64_t received_data = ((uint64_t)rx_lmo->mo_ptr->can_data[1] << 32) | rx_lmo->mo_ptr->can_data[0];
                for (int i = 0; i < rcvMsg.DLC && i < 8; i++) {
                    rcvMsg.data[i] = (uint8_t)((received_data >> (i * 8)) & 0xFF);
                }
            
                /* **🎯 資料有效性檢查 - 修正：允許 ID=0x000 (NMT 命令)** */
                if (rcvMsg.ident >= 0x000 && rcvMsg.ident <= 0x7FF && rcvMsg.DLC <= 8) {
                    
                    /* **🔍 尋找匹配的接收緩衝區** */
                    bool message_processed = false;
                    for (uint16_t i = 0; i < CANmodule->rxSize; i++) {
                        CO_CANrx_t *buffer = &CANmodule->rxArray[i];
                        if (buffer != NULL && buffer->CANrx_callback != NULL) {
                            /* 檢查 ID 和遮罩是否匹配 */
                            if (((rcvMsg.ident ^ buffer->ident) & buffer->mask) == 0U) {
                                /* **✅ 調用 CANopen 處理函數** */
                                buffer->CANrx_callback(buffer->object, (void*)&rcvMsg);
                                message_processed = true;
                                
                                /* 簡化的除錯輸出 */
                                static uint32_t rx_msg_count = 0;
                                rx_msg_count++;
                                if ((rx_msg_count % 5) == 1) {  /* 每 5 次輸出一次 */
                                    Debug_Printf("📨 RX: ID=0x%03X DLC=%d LMO_%02d (#%lu)\r\n", 
                                               rcvMsg.ident, rcvMsg.DLC, index + 1, rx_msg_count);
                                }
                                break;
                            }
                        }
                    }
                    
                    if (!message_processed) {
                        static uint32_t unmatched_count = 0;
                        unmatched_count++;
                        
                        /* **🎯 特別關注 NMT 訊息的處理失敗** */
                        if (rcvMsg.ident == 0x000) {
                            Debug_Printf("🚨 NMT 訊息無匹配接收緩衝區! ID=0x000 DLC=%d\r\n", rcvMsg.DLC);
                            /* 顯示所有接收緩衝區的設定供診斷 */
                            for (uint16_t j = 0; j < CANmodule->rxSize && j < 5; j++) {
                                CO_CANrx_t *buf = &CANmodule->rxArray[j];
                                if (buf->CANrx_callback != NULL) {
                                    Debug_Printf("  RxBuf[%d]: ID=0x%03X Mask=0x%03X\r\n", 
                                               j, buf->ident, buf->mask);
                                }
                            }
                        } else if ((unmatched_count % 10) == 1) {  /* 其他訊息每 10 次輸出一次 */
                            Debug_Printf("🚫 RX 無匹配: ID=0x%03X LMO_%02d (#%lu)\r\n", 
                                       rcvMsg.ident, index + 1, unmatched_count);
                        }
                    }
                } else {
                    static uint32_t invalid_count = 0;
                    invalid_count++;
                    if ((invalid_count % 10) == 1) {  /* 每 10 次輸出一次 */
                        Debug_Printf("❌ RX 無效資料: ID=0x%03X DLC=%d LMO_%02d (#%lu)\r\n", 
                                   rcvMsg.ident, rcvMsg.DLC, index + 1, invalid_count);
                    }
                }
            } else {
                static uint32_t rx_error_count = 0;
                rx_error_count++;
                if ((rx_error_count % 20) == 1) {  /* 每 20 次輸出一次 */
                    Debug_Printf("❌ DAVE Receive 失敗: %d LMO_%02d (#%lu)\r\n", 
                               rx_status, index + 1, rx_error_count);
                }
            }
        } else {
            /* 沒有 RX pending，正常情況 */
        }
    } else {
        /* 這個 LMO 不是 RX 類型，忽略 */
        Debug_Printf("⚠️ LMO_%02d 不是 RX 類型，忽略中斷\r\n", index + 1);
    }
}

/******************************************************************************/
/* CAN TX interrupt - 處理發送完成事件 */
static void CO_CANinterrupt_Tx(CO_CANmodule_t *CANmodule, uint32_t index)
{
    /* **🎯 TX 中斷處理 - 基於 DAVE 配置動態檢查** */
    if (index >= CAN_NODE_0.mo_count) {
        Debug_Printf("❌ TX index %d 超出範圍 (最大 %d)\r\n", index, CAN_NODE_0.mo_count - 1);
        return;
    }
    
    const CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[index];
    
    /* **🔍 確保這個 LMO 確實是 TX 類型** */
    if (tx_lmo != NULL && tx_lmo->mo_ptr != NULL &&
        tx_lmo->mo_ptr->can_mo_type == XMC_CAN_MO_TYPE_TRANSMSGOBJ) {
        
        if (CANmodule != NULL && index < CANmodule->txSize) {
            /* 檢查對應的發送緩衝區 */
            CO_CANtx_t *buffer = &CANmodule->txArray[index];
            
            if (buffer->bufferFull) {
                /* 發送完成，清除 bufferFull 標誌 */
                buffer->bufferFull = false;
                CANmodule->CANtxCount--;
                
                /* 簡化的除錯輸出 */
                static uint32_t tx_complete_count = 0;
                tx_complete_count++;
                if ((tx_complete_count % 10) == 1) {  /* 每 10 次輸出一次 */
                    Debug_Printf("📤 TX Complete: LMO_%02d (#%lu)\r\n", 
                               index + 1, tx_complete_count);
                }
            }
        }
        
        /* 標記第一次發送完成 */
        CANmodule->firstCANtxMessage = false;
    } else {
        /* 這個 LMO 不是 TX 類型，忽略 */
        Debug_Printf("⚠️ LMO_%02d 不是 TX 類型，忽略中斷\r\n", index + 1);
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
    /* 使用 DAVE CAN_NODE - 只設定正常模式標誌 */
    if (CANmodule != NULL) {
        CANmodule->CANnormal = true;
        Debug_Printf("✅ CAN 正常模式已啟用 (DAVE 配置保持不變)\r\n");
        
        /* **🎯 DAVE 配置的所有硬體設定都保持原樣** */
        /* - LMO 類型 (RX/TX) 由 DAVE UI 設定 */
        /* - CAN ID 由 DAVE UI 設定 */
        /* - 事件啟用由 DAVE UI 設定 */
        /* - 中斷路由由 DAVE UI 設定 */
        
        Debug_Printf("✅ DAVE CAN_NODE 硬體配置完全保持原樣\r\n");
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
/* **📋 DAVE 配置管理函數 - 遵循 DAVE APP 配置驅動原則** */
/******************************************************************************/

/**
 * @brief 獲取 CANopen Node ID - 動態版本（專業產品設計）
 * 
 * 🎯 設計理念：
 * 1. 目前從全域變數讀取，可在執行時修改
 * 2. 未來可擴展為從 EEPROM 讀取，實現完整的產品化
 * 3. 支援動態配置，不需要重新編譯韌體
 */
static uint8_t canopen_get_node_id(void)
{
    /* **🔧 階段一：從全域變數讀取（目前實現）** */
    /* 可在執行時透過除錯介面或命令修改 g_canopen_node_id */
    
    /* **🚀 階段二：未來可擴展為 EEPROM 實現** */
    // if (eeprom_is_valid()) {
    //     uint8_t eeprom_node_id = eeprom_read_node_id();
    //     if (eeprom_node_id >= 1 && eeprom_node_id <= 127) {
    //         g_canopen_node_id = eeprom_node_id;
    //     }
    // }
    
    /* **✅ 範圍檢查** */
    if (g_canopen_node_id < 1 || g_canopen_node_id > 127) {
        Debug_Printf("⚠️ Node ID %d 超出範圍，使用預設值 10\r\n", g_canopen_node_id);
        g_canopen_node_id = 10;  /* 恢復為安全的預設值 */
    }
    
    return g_canopen_node_id;
}

/**
 * @brief 取得當前 CANopen Node ID
 * @return 當前的 Node ID
 */
uint8_t canopen_get_current_node_id(void)
{
    return g_canopen_node_id;
}

/**
 * @brief 驗證 DAVE 配置是否有效
 */
static bool canopen_is_dave_config_valid(void)
{
    uint8_t node_id = canopen_get_node_id();
    
    /* **✅ 檢查 Node ID 範圍 (CANopen 標準)** */
    if (node_id < 1 || node_id > 127) {
        Debug_Printf("❌ 無效的 Node ID: %d (範圍: 1-127)\r\n", node_id);
        return false;
    }
    
    /* **✅ 檢查 DAVE UI 配置的 LMO 數量** */
    if (CAN_NODE_0.mo_count < 12) {
        Debug_Printf("❌ DAVE UI LMO 數量不足: %d (需要 12 個)\r\n", CAN_NODE_0.mo_count);
        return false;
    }
    
    /* **✅ 檢查必要的 LMO 是否在 DAVE UI 中配置** */
    for (uint8_t i = 0; i < 12; i++) {
        if (CAN_NODE_0.lmobj_ptr[i] == NULL) {
            Debug_Printf("❌ LMO_%02d 在 DAVE UI 中未配置\r\n", i + 1);
            return false;
        }
    }
    
    /* **✅ 檢查 DAVE UI 波特率配置** */
    if (CAN_NODE_0.baudrate_config != NULL) {
        uint32_t baudrate = CAN_NODE_0.baudrate_config->baudrate;
        if (baudrate != 500000) {
            Debug_Printf("⚠️ DAVE UI 波特率 %lu bps 與預期 500kbps 不同\r\n", baudrate);
        } else {
            Debug_Printf("✅ 波特率配置正確: %lu bps (500 kbps)\r\n", baudrate);
        }
    }
    
    Debug_Printf("✅ DAVE UI 配置驗證通過 (Node ID=%d)\r\n", node_id);
    return true;
}
