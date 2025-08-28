/*
 * main.c
 *
 *  Created on: 2022 May 23 16:28:06
 *  Author: benchen
 */

#include "DAVE.h"                   // DAVE 所有硬體抽象層 (CAN_NODE, DIGITAL_IO, UART, TIMER)
#include "CANopenNode/CANopen.h"     // CANopenNode 主頭檔 (正確路徑)
#include "application/OD.h"          // 物件字典定義
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* Default values */
#define NMT_CONTROL             (CO_NMT_STARTUP_TO_OPERATIONAL | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION)
#define FIRST_HB_TIME           500
#define SDO_SRV_TIMEOUT_TIME    1000
#define SDO_CLI_TIMEOUT_TIME    500
#define SDO_CLI_BLOCK           false
#define OD_STATUS_BITS          NULL
#define TMR_TASK_INTERVAL       (1000)    /* 1ms 任務間隔 */

/* Global objects */
static CO_t                 *CO = NULL;
/* 全域變數定義 */
extern volatile uint32_t    CO_timer1ms;      /* 1ms 計時器變數 (定義在 CO_driver_XMC4800.c) */
extern void Debug_Printf_Raw(const char* format, ...);

/* DAVE UI 專業中斷處理函數前向宣告 */
void TimerHandler(void);
void CAN_Handler(void);

/* 🎯 XMC4800 CANopenNode 架構 - 完全使用 DAVE CAN_NODE APP 配置 */
typedef struct {
    uint8_t     desiredNodeID;      /* 期望的節點 ID */
    uint8_t     activeNodeID;       /* 實際分配的節點 ID */
    uint16_t    baudrate;           /* CAN 波特率 (kbps) - DAVE APP UI 已設定 */
    
    /* **🔧 XMC4800 DAVE APP 硬體抽象** */
    void        (*HWInitFunction)(); /* 硬體初始化函數 (DAVE_Init 已完成) */
    
    /* **🔧 DAVE CAN_NODE APP 抽象 - 完全依賴 DAVE UI 設定** */ 
    void*       CANHandle;          /* DAVE CAN_NODE_0 控制代碼 */
    
    /* **🔧 DAVE DIGITAL_IO APP LED 控制** */
    uint8_t     outStatusLEDGreen;  /* 綠色 LED 狀態 (透過 DAVE DIGITAL_IO) */
    uint8_t     outStatusLEDRed;    /* 紅色 LED 狀態 (透過 DAVE DIGITAL_IO) */
    
    CO_t*       canOpenStack;       /* CANopen 堆疊指標 */
    bool        initialized;        /* 初始化完成標誌 */
} CANopenNodeXMC4800;

/* XMC4800 CANopen 節點實例 - DAVE APP 設定優先 */
static CANopenNodeXMC4800 canopenNodeXMC4800 = {
    .desiredNodeID = 10,
    .activeNodeID = 10,
    .baudrate = 500,            /* 注意：實際波特率由 DAVE CAN_NODE APP UI 設定，此值僅供CANopenNode參考 */
    .HWInitFunction = NULL,     /* XMC4800 使用 DAVE_Init() 完成所有硬體設定 */
    .CANHandle = NULL,          /* 將設為 &CAN_NODE_0 (DAVE APP 物件) */
    .outStatusLEDGreen = 0,
    .outStatusLEDRed = 0,
    .canOpenStack = NULL,
    .initialized = false
};

/* Application variables */
static CO_NMT_reset_cmd_t   reset = CO_RESET_NOT;
static uint8_t              pendingNodeId = 10;
static uint16_t             pendingBitRate = 500;  /* 與 DAVE UI CAN_NODE APP 設定保持一致 */

/* Test CAN message variables */
static uint32_t             test_msg_counter = 0;
static uint32_t             watchdog_counter = 0;
static uint32_t             error_recovery_count = 0;
static uint32_t             errCnt_CO_init = 0;
static char                 debug_buffer[256];

/* CANopen 專業功能變數 */
static uint32_t             sdo_transfer_count = 0;
static uint32_t             pdo_tx_count = 0;
static uint32_t             pdo_rx_count = 0;
static uint32_t             emergency_count = 0;
static uint32_t             heartbeat_count = 0;
static bool                 canopen_ready = false;


static void CO_errExit(char* msg);
static void app_updateLEDs(void);
static void send_test_can_message(void);

/* 🎯 專業 CANopenNode 函數 - 參考 CanOpenSTM32 模式 */
static int canopen_app_init(CANopenNodeXMC4800* canopenXMC4800);
static int canopen_app_resetCommunication(void);

/* **📋 Debug_Printf 函數聲明 - 使用 CO_driver_XMC4800.c 中的實現** */
extern void Debug_Printf_Raw(const char* format, ...);     /* 原始輸出函數 */
extern void Debug_ProcessISRBuffer(void);                  /* ISR 緩衝區處理 */

/* **🎯 簡單的宏定義，將 Debug_Printf 重定向到外部實現** */
#define Debug_Printf Debug_Printf_Raw
static void canopen_app_process(void);

/* ISR 除錯緩衝區處理函數 (在 CO_driver_XMC4800.c 中實現) */
extern void Debug_ProcessISRBuffer(void);

/* 中斷計數器 - 從 CO_driver_XMC4800.c 導入 */
extern volatile uint32_t g_interrupt_rx_count;
extern volatile uint32_t g_interrupt_tx_count; 
extern volatile uint32_t g_interrupt_total_count;

/* CANopen 專業功能函數 */
static void process_canopen_communication(void);
static void report_canopen_statistics(void);
static void handle_nmt_state_change(void);
static void process_pdo_communication(void);
static void monitor_error_register(void);
static void check_canopen_rx_messages(void);

/**
 * @brief main() - Application entry point
 *
 * <b>Details of function</b><br>
 * This routine is the application entry point. It is invoked by the device startup code. It is responsible for
 * invoking the APP initialization dispatcher routine - DAVE_Init() and hosting the place-holder for user application
 * code.
 */
int main(void)
{
    DAVE_STATUS_t status;

    /* 簡單延遲，讓系統穩定 */
    for(volatile int i = 0; i < 1000000; i++);
    
    /* Initialization of DAVE APPs */
    status = DAVE_Init();
    
    if (status != DAVE_STATUS_SUCCESS)
    {
        /* Placeholder for error handler code. The while loop below can be replaced with an user error handler. */
        Debug_Printf("DAVE APPs initialization failed\r\n");
        
        while(1U)
        {
            /* Error: DAVE initialization failed */
        }
    }

    /* DAVE initialization successful - now initialize CANopen */
    Debug_Printf("=== DAVE_Init() successful ===\r\n");
    
    /* 簡化除錯輸出，避免複雜字符 */
    Debug_Printf("XMC4800 CANopen Starting...\r\n");
    Debug_Printf("Hardware: DAVE API\r\n");
    Debug_Printf("Protocol: CANopenNode v2.0\r\n");

    /* 🚀 等待 OpenEDSEditor 重新產生 CANopenNode v4.0 相容檔案 */
    Debug_Printf("EDS file confirmed:\r\n");
    Debug_Printf("   - Product: CANopen-IO\r\n");
    Debug_Printf("   - PDO: 4 RPDO + 4 TPDO\r\n");
    Debug_Printf("   - Baudrate: 10k-1000k bps\r\n");
    
    Debug_Printf("Starting CANopen init...\r\n");
    
    // ✅ 專業 CANopen 初始化 (OD.h 已手動修正完成)
    Debug_Printf("Calling canopen_app_init...\r\n");
    int init_result = canopen_app_init(&canopenNodeXMC4800);
    Debug_Printf("canopen_app_init returned: %d\r\n", init_result);
    
    if (init_result != 0) {
        Debug_Printf("ERROR: CANopen init failed: %d\r\n", init_result);
        while(1) {
            /* 停留在錯誤狀態 */
        }
    }

    Debug_Printf("CANopen init SUCCESS!\r\n");
    
    /* **🎯 CANopen 主循環 - 標準 CanOpenSTM32 架構** */

    /* Main infinite loop */
    while(1U)
    {
        /* 1. CANopen 核心處理 (定時器處理由 TimerHandler() 中斷管理) */
        canopen_app_process();
        
        /* 2. LED 狀態更新 (反映 CANopen NMT 狀態) */
        if (canopenNodeXMC4800.outStatusLEDGreen) {
            DIGITAL_IO_SetOutputHigh(&LED1);
        } else {
            DIGITAL_IO_SetOutputLow(&LED1);
        }
        
        /* 3. 處理除錯緩衝區 */
        Debug_ProcessISRBuffer();
    }
}

/**
 * LED 狀態更新
 */
static void app_updateLEDs(void)
{
    if (CO != NULL && CO->NMT != NULL) {
        switch (CO->NMT->operatingState) {
            case CO_NMT_INITIALIZING:
                DIGITAL_IO_SetOutputLow(&LED1);
                break;
            case CO_NMT_PRE_OPERATIONAL:
                /* 閃爍 LED */
                DIGITAL_IO_ToggleOutput(&LED1);
                break;
            case CO_NMT_OPERATIONAL:
                DIGITAL_IO_SetOutputHigh(&LED1);
                break;
            case CO_NMT_STOPPED:
                DIGITAL_IO_SetOutputLow(&LED1);
                break;
            case CO_NMT_UNKNOWN:
            default:
                DIGITAL_IO_SetOutputLow(&LED1);
                break;
        }
    }
}

/**
 * 專業級 CANopen 功能測試 - 簡化版本
 */
static void send_test_can_message(void)
{
    static uint32_t test_cycle = 0;
    
    test_cycle++;
    
    /* 安全檢查 */
    if (CO == NULL || CO->NMT == NULL) {
        return;
    }
    
    /* 檢查節點狀態 - 允許在 PRE_OPERATIONAL 和 OPERATIONAL 狀態下測試 */
    if (CO->NMT->operatingState != CO_NMT_OPERATIONAL && 
        CO->NMT->operatingState != CO_NMT_PRE_OPERATIONAL) {
        if ((test_cycle % 5) == 0) {  // 減少輸出頻率
            Debug_Printf("[TEST] Node state %d, skipping test #%lu\r\n", 
                        CO->NMT->operatingState, test_cycle);
        }
        return;
    }
    
    if ((test_cycle % 5) == 0) {  // 減少測試輸出頻率
        Debug_Printf("[TEST] CANopen API Test #%lu (Node ID=%d, State=%d)\r\n", 
                    test_cycle, pendingNodeId, CO->NMT->operatingState);
    }
    
    /* 輪流測試不同功能，避免同時發送多個訊息 */
    uint32_t test_type = (test_cycle - 1) % 5;  /* 改為 5 種測試 */
    
    switch (test_type) {
        case 0: /* 測試 Emergency */
            if (CO->em != NULL) {
                CO_errorReport(CO->em, CO_EM_GENERIC_ERROR, CO_EMC_GENERIC, test_cycle);
                emergency_count++;
                Debug_Printf("  -> Emergency sent (count: %lu)\r\n", emergency_count);
                
                /* 立即重置錯誤，避免持續的錯誤狀態 */
                CO_errorReset(CO->em, CO_EM_GENERIC_ERROR, 0);
            }
            break;
            
        case 1: /* 測試 TPDO */
            if (CO->TPDO != NULL && CO->TPDO[0].CANtxBuff != NULL) {
                CO_TPDOsendRequest(&CO->TPDO[0]);
                pdo_tx_count++;
                Debug_Printf("  -> TPDO1 requested (count: %lu)\r\n", pdo_tx_count);
            }
            break;
            
        case 2: /* 測試 Heartbeat */
            if (CO->NMT != NULL && CO->NMT->HB_TXbuff != NULL) {
                CO->NMT->HB_TXbuff->data[0] = CO->NMT->operatingState;
                CO_CANsend(CO->CANmodule, CO->NMT->HB_TXbuff);
                heartbeat_count++;
                Debug_Printf("  -> Heartbeat sent (count: %lu)\r\n", heartbeat_count);
            }
            break;
            
        case 3: /* 測試 NMT 狀態切換 */
            if (CO->NMT != NULL && CO->NMT->operatingState == CO_NMT_PRE_OPERATIONAL) {
                /* 只在 PRE_OPERATIONAL 時嘗試切換到 OPERATIONAL */
                Debug_Printf("  -> Requesting NMT state change to OPERATIONAL\r\n");
                CO->NMT->operatingState = CO_NMT_OPERATIONAL;
            }
            break;
    }
}

/**
 * 錯誤輸出並退出
 */
static void CO_errExit(char* msg)
{
    Debug_Printf("%s\r\n", msg);
    DIGITAL_IO_SetOutputLow(&LED1);
    for(;;) {
        for(uint32_t j = 0; j < 0xFFFF; j++);
        DIGITAL_IO_ToggleOutput(&LED1);
    }
}

/**
 * 專業級 CANopen 通訊處理 - 簡化版本
 */
static void process_canopen_communication(void)
{
    if (CO == NULL) return;
    
    /* **移除錯誤的手動中斷調用 - 讓硬體中斷自然處理** */
    
    /* CANopen 核心處理 - 按照標準 CANopen 規範順序 */
    
    /* 1. 處理 SYNC 對象 */
    bool syncWas = CO_process_SYNC(CO, TMR_TASK_INTERVAL, NULL);
    
    /* 2. 處理接收 PDO (RPDO) */
    CO_process_RPDO(CO, syncWas, TMR_TASK_INTERVAL, NULL);
    
    /* 3. 處理傳送 PDO (TPDO) */
    CO_process_TPDO(CO, syncWas, TMR_TASK_INTERVAL, NULL);
    
    /* 4. 處理 Heartbeat 和 Node Guarding - 簡化 */
    if (syncWas) {
        pdo_tx_count++;
        pdo_rx_count++;
    }
}

/**
 * 詳細分析錯誤暫存器內容
 */
static void analyze_error_register(void)
{
    if (CO == NULL || CO->em == NULL) return;
    
    uint8_t error_reg = *(CO->em->errorRegister);
    
    if (error_reg != 0) {
        Debug_Printf("錯誤暫存器分析 (0x%02X):\r\n", error_reg);
        
        if (error_reg & CO_ERR_REG_GENERIC_ERR) {
            Debug_Printf("  - 0x01: 通用錯誤\r\n");
        }
        if (error_reg & CO_ERR_REG_CURRENT) {
            Debug_Printf("  - 0x02: 電流錯誤\r\n");
        }
        if (error_reg & CO_ERR_REG_VOLTAGE) {
            Debug_Printf("  - 0x04: 電壓錯誤\r\n");
        }
        if (error_reg & CO_ERR_REG_TEMPERATURE) {
            Debug_Printf("  - 0x08: 溫度錯誤\r\n");
        }
        if (error_reg & CO_ERR_REG_COMMUNICATION) {
            Debug_Printf("  - 0x10: 通訊錯誤\r\n");
        }
        if (error_reg & CO_ERR_REG_DEV_PROFILE) {
            Debug_Printf("  - 0x20: 設備配置錯誤\r\n");
        }
        if (error_reg & 0x40) {
            Debug_Printf("  - 0x40: 保留錯誤\r\n");
        }
        if (error_reg & 0x80) {
            Debug_Printf("  - 0x80: 製造商特定錯誤\r\n");
        }
    }
}

/**
 * 監控錯誤暫存器
 */
static void monitor_error_register(void)
{
    if (CO == NULL || CO->em == NULL) return;
    
    static uint8_t last_error_register = 0;
    uint8_t current_error_register = *(CO->em->errorRegister);
    
    /* 檢查錯誤暫存器是否有變化 */
    if (current_error_register != last_error_register) {
        Debug_Printf("CANopen Error Register changed: 0x%02X -> 0x%02X\r\n", 
                    last_error_register, current_error_register);
        
        /* 調用詳細分析 */
        analyze_error_register();
        
        last_error_register = current_error_register;
    }
    
    /* 如果錯誤暫存器持續有錯誤，定期報告 */
    static uint32_t error_report_counter = 0;
    if (current_error_register != 0 && (error_report_counter % 30) == 0) {
        Debug_Printf("持續錯誤：0x%02X\r\n", current_error_register);
        analyze_error_register();
    }
    error_report_counter++;
}

/**
 * 處理 NMT 狀態變化
 */
static void handle_nmt_state_change(void)
{
    if (CO == NULL || CO->NMT == NULL) return;
    
    static uint8_t last_nmt_state = 0xFF;
    uint8_t current_nmt_state = CO->NMT->operatingState;
    
    /* 檢查 NMT 狀態是否有變化 */
    if (current_nmt_state != last_nmt_state) {
        Debug_Printf("=== NMT State Change ===\r\n");
        Debug_Printf("NMT State: %d -> %d\r\n", last_nmt_state, current_nmt_state);
        
        switch (current_nmt_state) {
            case CO_NMT_INITIALIZING:
                Debug_Printf("State: INITIALIZING\r\n");
                canopen_ready = false;
                break;
            case CO_NMT_PRE_OPERATIONAL:
                Debug_Printf("State: PRE_OPERATIONAL\r\n");
                canopen_ready = true;  /* SDO 可用 */
                break;
            case CO_NMT_OPERATIONAL:
                Debug_Printf("State: OPERATIONAL\r\n");
                canopen_ready = true;  /* PDO + SDO 都可用 */
                break;
            case CO_NMT_STOPPED:
                Debug_Printf("State: STOPPED\r\n");
                canopen_ready = false;
                break;
            default:
                Debug_Printf("State: UNKNOWN (%d)\r\n", current_nmt_state);
                canopen_ready = false;
                break;
        }
        
        last_nmt_state = current_nmt_state;
        Debug_Printf("===================\r\n");
    }
}

/**
 * 處理 PDO 通訊 - 簡化版本
 */
static void process_pdo_communication(void)
{
    if (CO == NULL) return;
    
    /* 簡化 PDO 監控 - 避免過度複雜的處理 */
    static uint32_t pdo_cycle_count = 0;
    pdo_cycle_count++;
    
    /* 只在特定週期進行 PDO 狀態檢查 */
    if ((pdo_cycle_count % 10000) == 0) {
        /* 檢查 TPDO 是否需要更新 */
        if (CO->TPDO != NULL) {
            /* 簡單的 TPDO 狀態檢查 */
            pdo_tx_count++;
        }
        
        /* 檢查 RPDO 接收狀態 */
        if (CO->RPDO != NULL) {
            /* 簡單的 RPDO 狀態檢查 */
            pdo_rx_count++;
        }
    }
}

/**
 * 簡化的 CANopen 統計報告 - 減少輸出頻率
 */
static void report_canopen_statistics(void)
{
    Debug_Printf("\r\n╔═══════════════════════════════════╗\r\n");
    Debug_Printf("║        CANopen 狀態報告          ║\r\n");
    Debug_Printf("╚═══════════════════════════════════╝\r\n");
    
    if (CO != NULL && CO->NMT != NULL) {
        Debug_Printf("節點: ID=%d, 位元率=%d kbps\r\n", pendingNodeId, pendingBitRate);
        
        const char* state_name;
        switch (CO->NMT->operatingState) {
            case CO_NMT_OPERATIONAL:     state_name = "OPERATIONAL"; break;
            case CO_NMT_PRE_OPERATIONAL: state_name = "PRE_OPERATIONAL"; break;
            case CO_NMT_INITIALIZING:    state_name = "INITIALIZING"; break;
            case CO_NMT_STOPPED:         state_name = "STOPPED"; break;
            default:                     state_name = "UNKNOWN"; break;
        }
        Debug_Printf("狀態: %s\r\n", state_name);
    }
    
    Debug_Printf("\r\nAPI 測試計數:\r\n");
    Debug_Printf("Emergency: %lu, TPDO: %lu, Heartbeat: %lu\r\n", 
                emergency_count, pdo_tx_count, heartbeat_count);
    
    /* 🎯 新增：中斷統計資訊 */
    Debug_Printf("\r\n中斷統計:\r\n");
    Debug_Printf("總中斷: %lu, RX中斷: %lu, TX中斷: %lu\r\n",
                g_interrupt_total_count, g_interrupt_rx_count, g_interrupt_tx_count);
    
    if (CO != NULL && CO->em != NULL) {
        uint8_t err_reg = *(CO->em->errorRegister);
        Debug_Printf("錯誤暫存器: 0x%02X %s\r\n", err_reg, (err_reg == 0) ? "(正常)" : "(有錯誤)");
    }
    
    Debug_Printf("系統運行: %lu ms\r\n", CO_timer1ms);
    Debug_Printf("═══════════════════════════════════\r\n\r\n");
}

/**
 * 檢查 CANopen RX 訊息 - 簡化版本
 */
static void check_canopen_rx_messages(void)
{
    static uint32_t last_rx_check = 0;
    static uint32_t rx_message_count = 0;
    
    if (CO == NULL || CO->CANmodule == NULL) return;
    
    /* 簡化的 RX 監控 - 不直接存取內部標誌 */
    
    /* 監控 SDO 傳輸活動 */
    if (CO->SDOserver != NULL) {
        /* 簡單的 SDO 活動檢查 */
        static uint32_t last_sdo_count = 0;
        if (sdo_transfer_count > last_sdo_count) {
            rx_message_count++;
            Debug_Printf("[RX] SDO activity detected (total: %lu)\r\n", rx_message_count);
            last_sdo_count = sdo_transfer_count;
        }
    }
    
    /* 定期報告 RX 狀態 */
    if ((CO_timer1ms - last_rx_check) > 15000) {  /* 每 15 秒 */
        if (rx_message_count > 0) {
            Debug_Printf("[RX] Status: Total received activity: %lu\r\n", rx_message_count);
        } else {
            Debug_Printf("[RX] Status: Monitoring active, no messages received\r\n");
        }
        last_rx_check = CO_timer1ms;
    }
}

/******************************************************************************/
/*      🎯 專業 CANopenNode 函數實現 - 參考 CanOpenSTM32 架構              */
/******************************************************************************/

/**
 * @brief CANopen 應用程式初始化 - 參考 CanOpenSTM32 的 canopen_app_init
 * @param canopenXMC4800 XMC4800 CANopen 節點結構指標
 * @return 0=成功, 其他=錯誤代碼
 */
static int canopen_app_init(CANopenNodeXMC4800* canopenXMC4800)
{
    CO_ReturnError_t err;
    uint32_t errInfo = 0;
    
    Debug_Printf("CANopen Professional Init START\r\n");
    Debug_Printf("Architecture: XMC4800 + DAVE API\r\n");
    Debug_Printf("Reference: CanOpenSTM32 + CANopenNode v4.0\r\n");
    
    /* 分配 CANopen 物件記憶體 - 使用標準單一 OD 模式 */
    /* XMC4800 使用標準模式，配置從 OD.h 自動取得 */
    Debug_Printf("Step 1: Calling CO_new...\r\n");
    uint32_t heapMemoryUsed;
    CO = CO_new(NULL, &heapMemoryUsed);
    if (CO == NULL) {
        Debug_Printf("ERROR: CO_new failed - memory allocation failed\r\n");
        return 1;
    } else {
        Debug_Printf("SUCCESS: Allocated %u bytes for CANopen objects\r\n", heapMemoryUsed);
    }
    
    /* 更新節點結構 */
    canopenXMC4800->canOpenStack = CO;
    canopenXMC4800->activeNodeID = canopenXMC4800->desiredNodeID;
    
    Debug_Printf("SUCCESS: CO_new completed, Node ID: %d\r\n", canopenXMC4800->activeNodeID);

    /* **🎯 關鍵：先執行 CO_CANinit() 來設置 TX 緩衝區** */
    /* 注意：CAN 硬體設定完全由 DAVE CAN_NODE APP UI 管理 */
    /* - 波特率：透過 DAVE CAN_NODE APP UI 設定 */
    /* - 接收郵箱：透過 DAVE CAN_NODE APP UI 設定 */
    /* - 傳送郵箱：透過 DAVE CAN_NODE APP UI 設定 */
    /* - GPIO 腳位：透過 DAVE CAN_NODE APP UI 設定 */
    Debug_Printf("Step 2: CO_CANinit - Initializing CAN module (DAVE CAN_NODE APP)\r\n");
    err = CO_CANinit(CO, (void*)&CAN_NODE_0, canopenXMC4800->baudrate);
    if (err != CO_ERROR_NO) {
        Debug_Printf("ERROR: CO_CANinit failed: %d\r\n", err);
        return 2;
    }
    Debug_Printf("SUCCESS: CO_CANinit completed (DAVE CAN_NODE), txSize=%d\r\n", CO->CANmodule->txSize);

    /* **🎯 重新啟用 LSS 初始化 - 不是 stuff error 的原因** */
    Debug_Printf("Step 3: LSS initialization - Following CanOpenSTM32 standard\r\n");
    CO_LSS_address_t lssAddress = {.identity = {
        .vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID,
        .productCode = OD_PERSIST_COMM.x1018_identity.productCode, 
        .revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber,
        .serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber}};
        
    err = CO_LSSinit(CO, &lssAddress, &canopenXMC4800->desiredNodeID, &canopenXMC4800->baudrate);
    if (err != CO_ERROR_NO) {
        Debug_Printf("WARNING: LSS slave initialization failed: %d (continuing without LSS)\r\n", err);
        /* 注意：LSS 失敗不是致命錯誤，可以繼續 */
    } else {
        Debug_Printf("SUCCESS: LSS initialization completed\r\n");
    }

    /* **🎯 更新節點結構 - 參考 CanOpenSTM32 模式** */
    canopenXMC4800->activeNodeID = canopenXMC4800->desiredNodeID;
    canopenXMC4800->CANHandle = (void*)&CAN_NODE_0;  /* 設定 CAN 控制代碼 */
    
    Debug_Printf("SUCCESS: CO_CANinit completed, Node ID: %d\r\n", canopenXMC4800->activeNodeID);
    Debug_Printf("=== DAVE CAN_NODE APP 硬體狀態檢查 ===\r\n");
    Debug_Printf("CAN 波特率: %d kbps (DAVE APP UI 設定)\r\n", canopenXMC4800->baudrate);
    Debug_Printf("CANmodule 狀態: %s\r\n", (CO->CANmodule->CANnormal) ? "正常" : "配置模式");
    Debug_Printf("TX 緩衝區數量: %d (DAVE 硬體)\r\n", CO->CANmodule->txSize);
    Debug_Printf("RX 緩衝區數量: %d (DAVE 硬體)\r\n", CO->CANmodule->rxSize);
    Debug_Printf("節點 ID: %d\r\n", canopenXMC4800->activeNodeID);
    Debug_Printf("CAN_NODE_0: DAVE APP 管理\r\n");
    Debug_Printf("========================================\r\n");

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    /* 儲存體初始化 - 參考 STM32 模式 */
    CO_storage_t storage;
    CO_storageBlank_init(&storage, CO_CONFIG_STORAGE, NULL, 0, NULL, 0);
    
    err = CO_storageInit(&storage, CO, OD, NULL, 0, NULL, 0);
    if (err != CO_ERROR_NO) {
        Debug_Printf("⚠️  Storage init warning: %d\r\n", err);
    }
#endif

    /* LSS 初始化 - 暫時跳過，專注於核心功能 */
    Debug_Printf("⚠️  LSS init skipped - focusing on core CANopen functionality\r\n");

    /* CANopen 主要初始化 - 修正為 CANopenNode v4.0 正確 API */
    err = CO_CANopenInit(CO,                        /* CANopen 物件 */
                         NULL,                      /* alternate NMT */
                         NULL,                      /* alternate em */
                         OD,                        /* Object Dictionary - 重要！不能是 NULL */
                         OD_STATUS_BITS,            /* 選用 OD_statusBits */
                         NMT_CONTROL,               /* NMT 控制位元組 */
                         FIRST_HB_TIME,             /* 首次心跳時間 */
                         SDO_SRV_TIMEOUT_TIME,      /* SDO 伺服器逾時 */
                         SDO_CLI_TIMEOUT_TIME,      /* SDO 客戶端逾時 */
                         SDO_CLI_BLOCK,             /* SDO 區塊傳輸 */
                         canopenXMC4800->activeNodeID,
                         &errInfo);
                         
    if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        if (err == CO_ERROR_OD_PARAMETERS) {
            Debug_Printf("❌ Object Dictionary error: 0x%lX\r\n", errInfo);
        } else {
            Debug_Printf("❌ CANopen init failed: %d\r\n", err);
        }
        return 3;
    }

    /* PDO 初始化 - 使用標準模式 */
    err = CO_CANopenInitPDO(CO, CO->em, OD, canopenXMC4800->activeNodeID, &errInfo);
    if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        if (err == CO_ERROR_OD_PARAMETERS) {
            Debug_Printf("❌ PDO Object Dictionary error: 0x%lX\r\n", errInfo);
        } else {
            Debug_Printf("❌ PDO init failed: %d\r\n", err);
        }
        return 4;
    }

    /* 🎯 移除 XMC4800 SYSTIMER 管理 - 改用 DAVE UI TimerHandler() */
    /* DAVE UI TimerHandler() 已經處理 CANopen 1ms 定時功能 */
    Debug_Printf("✅ Using DAVE UI TimerHandler() for CANopen timing\r\n");

    /* CANopen 回調配置 */
    if (!CO->nodeIdUnconfigured) {
        Debug_Printf("✅ Node ID configured: %d\r\n", canopenXMC4800->activeNodeID);
    } else {
        Debug_Printf("⚠️  Node ID unconfigured - LSS mode\r\n");
    }

    /* 🎯 啟動 CAN 正常模式 - 使用 DAVE API */
    CO_CANsetNormalMode(CO->CANmodule);
    
    /* 標記初始化完成 */
    canopenXMC4800->initialized = true;
    canopen_ready = true;
    
    Debug_Printf("🎯 CANopen Professional READY - DAVE + CANopenNode\r\n");
    Debug_Printf("   波特率: %d kbps, 節點: %d\r\n", 
                canopenXMC4800->baudrate, canopenXMC4800->activeNodeID);
    
    return 0;
}

/**
 * @brief CANopen 通訊重置 - 參考 CanOpenSTM32 的 canopen_app_resetCommunication
 */
static int canopen_app_resetCommunication(void)
{
    Debug_Printf("🔄 CANopen Communication Reset\r\n");
    
    /* DAVE UI TimerHandler() 會自動繼續運行，無需額外管理 */
    
    /* 設置配置模式 */
    CO_CANsetConfigurationMode((void*)&canopenNodeXMC4800);
    
    /* 刪除 CANopen 物件 */
    CO_delete(CO);
    CO = NULL;
    
    /* 重新初始化 */
    return canopen_app_init(&canopenNodeXMC4800);
}

/**
 * @brief CANopen 主循環處理 - 參考 CanOpenSTM32 的 canopen_app_process
 * 此函數應在 main() 的 while(1) 中定期調用
 */
static void canopen_app_process(void)
{
    static uint32_t time_old = 0;
    static uint32_t time_current = 0;
    
    /* 獲取時間差異 */
    time_current = CO_timer1ms;
    
    if ((time_current - time_old) >= 10) {  /* **🎯 修正：至少間隔 10ms，避免過度頻繁處理** */
        /* CANopen 主處理 - 參考 STM32 模式 */
        CO_NMT_reset_cmd_t reset_status;
        uint32_t timeDifference_us = (time_current - time_old) * 1000;
        time_old = time_current;
        
        reset_status = CO_process(CO, false, timeDifference_us, NULL);
        
        /* **🎯 減少額外的 NMT 處理，避免重複發送** */
        /* CO_process 已經包含 NMT 處理，不需要額外調用 CO_NMT_process */
        
        /* 更新 LED 狀態 - XMC4800 簡化版本 (CANopenNode v2.0 相容) */
        /* **注意：CANopenNode v2.0 可能沒有 CO->LEDs，使用簡化方案** */
        if (CO != NULL && CO->NMT != NULL) {
            /* 根據 NMT 狀態設定 LED */
            switch (CO->NMT->operatingState) {
                case CO_NMT_OPERATIONAL:
                    canopenNodeXMC4800.outStatusLEDGreen = 1;
                    canopenNodeXMC4800.outStatusLEDRed = 0;
                    break;
                case CO_NMT_PRE_OPERATIONAL:
                    canopenNodeXMC4800.outStatusLEDGreen = 0;  /* 閃爍邏輯將在主循環處理 */
                    canopenNodeXMC4800.outStatusLEDRed = 0;
                    break;
                case CO_NMT_STOPPED:
                    canopenNodeXMC4800.outStatusLEDGreen = 0;
                    canopenNodeXMC4800.outStatusLEDRed = 1;
                    break;
                default:
                    canopenNodeXMC4800.outStatusLEDGreen = 0;
                    canopenNodeXMC4800.outStatusLEDRed = 1;
                    break;
            }
        }
        
        /* **🎯 XMC4800 LED 控制 - 對應 STM32 的 HAL_GPIO_WritePin** */
        if (canopenNodeXMC4800.outStatusLEDGreen) {
            DIGITAL_IO_SetOutputHigh(&LED1);  /* 綠色 LED 開啟 */
        } else {
            DIGITAL_IO_SetOutputLow(&LED1);   /* 綠色 LED 關閉 */
        }
        
        /* 處理重置命令 - 完全參考 CanOpenSTM32 模式 */
        if (reset_status == CO_RESET_COMM) {
            Debug_Printf("🔄 CANopen Communication Reset requested\r\n");
            
            /* **🎯 完整重置流程 - 參考 CanOpenSTM32** */
            /* 1. 計時器由 DAVE UI TimerHandler() 自動管理，無需手動停止 */
            
            /* 2. 設置 CAN 配置模式 */
            CO_CANsetConfigurationMode((void*)&canopenNodeXMC4800);
            
            /* 3. 刪除 CANopen 物件 */
            CO_delete(CO);
            CO = NULL;
            
            /* 4. 重新初始化 */
            Debug_Printf("Reinitializing CANopen after communication reset...\r\n");
            canopen_app_init(&canopenNodeXMC4800);
            
        } else if (reset_status == CO_RESET_APP) {
            Debug_Printf("🔄 CANopen Application Reset requested\r\n");
            /* XMC4800 系統重置 - 對應 STM32 的 HAL_NVIC_SystemReset() */
            NVIC_SystemReset();
        }
    }
}

/******************************************************************************/
/*      🎯 DAVE UI 專業中斷處理系統 - 統一計時管理                          */
/******************************************************************************/

/* canopen_app_interrupt 函數已移除 - 功能整合到 TimerHandler() 和 canopen_timer_process() 中 */

/**
 * @brief DAVE UI Timer 中斷處理函數 - 專業中斷管理
 * 
 * 🎯 DAVE UI Integration: TimerHandler -> IRQ_Hdlr_57
 * ⏱️ 功能: CANopen 1ms 定時處理
 */
void TimerHandler(void)
{
    /* 調用 CANopen Timer 處理函數 */
    canopen_timer_process(CO);
    
    /* 清除 DAVE Timer 事件 */
    TIMER_ClearEvent(&TIMER_0);
}

/**
 * @brief DAVE UI CAN 中斷處理函數 - 專業中斷管理
 * 
 * 🎯 DAVE UI Integration: CAN_Handler -> IRQ_Hdlr_77  
 * 📡 功能: CAN 收發中斷處理
 */
void CAN_Handler(void)
{
    /* 調用 CANopen CAN 中斷處理函數 */
    canopen_can_interrupt_process();
}

/* test_dave_api_basic() 函數及其測試程式碼已移除 - 不再需要基本 DAVE API 測試 */
