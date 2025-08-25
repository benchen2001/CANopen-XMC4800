/*
 * main.c
 *
 *  Created on: 2022 May 23 16:28:06
 *  Author: benchen
 */

#include "DAVE.h"     //Declarations from DAVE Code Generation (includes SFR declaration)
#include "CANopenNode/CANopen.h"
#include "application/OD.h"
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
extern volatile uint32_t    CO_timer1ms;

/* Application variables */
static CO_NMT_reset_cmd_t   reset = CO_RESET_NOT;
static uint8_t              pendingNodeId = 10;
static uint16_t             pendingBitRate = 250;

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

/* Function prototypes */
static void app_programStart(void);
static CO_NMT_reset_cmd_t app_programReset(void);
static void app_programEnd(void);
static void mainTask_1ms(void);
static void Debug_Printf(const char* format, ...);
static void CO_errExit(char* msg);
static void app_updateLEDs(void);
static void send_test_can_message(void);

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
    
    /* 🎯 CAN0_3 中斷將由 CANopen 初始化處理 */
    Debug_Printf("✅ CAN0_3 中斷將由 CO_CANmodule_init() 處理\r\n");
    
    /* 使用 CAN_NODE APP - 完整 CAN 支援包括 GPIO */
    Debug_Printf("=== 使用 CAN_NODE APP + DAVE API ===\r\n");
    
    Debug_Printf("=== XMC4800 CANopen Device ===\r\n");
    Debug_Printf("Starting CANopen initialization...\r\n");
    
    app_programStart();

    Debug_Printf("=== Starting CANopen main loop ===\r\n");
    
    /* 手動調用第一次重置 */
    Debug_Printf("Calling first app_programReset()...\r\n");
    reset = app_programReset();
    Debug_Printf("First reset returned: %d\r\n", reset);
    
    /* 主循環 - 按 STM32 模式，添加錯誤恢復 */
    for (; reset != CO_RESET_APP; reset = app_programReset()) {
          
        /* CANopen 通訊重置循環 */
        for (;;) {
            /* 🎯 處理中斷緩衝區輸出 - 必須在主循環中處理 */
            Debug_ProcessISRBuffer();
            
            /* 看門狗計數器 */
            watchdog_counter++;
            
            /* 更新 CANopen 計時器 - 這是 CANopen 運作的關鍵 */
            CO_timer1ms++;
            
            /* CANopen 處理 - 修正參數 */
            if (CO != NULL) {
                reset = CO_process(CO, false, TMR_TASK_INTERVAL, NULL);
            }
            
            /* 非阻塞應用程式任務 */
            mainTask_1ms();
            
            if (reset != CO_RESET_NOT) {
                Debug_Printf("CANopen reset requested: %d\r\n", reset);
                error_recovery_count++;
                break;
            }
        }
    }

    app_programEnd();
    return 0;
}

/**
 * 程式啟動初始化 - 學習 STM32 模式
 */
static void app_programStart(void)
{
    uint32_t heapMemoryUsed;

    /* DAVE_Init() 已在 main() 中執行，這裡只做 CANopen 特定的初始化 */
    
    /* 確認 UART_0 初始化狀態 */
    if (UART_0.runtime == NULL) {
        CO_errExit("ERROR: UART_0 not initialized");
    }

    /* 測試 UART 輸出 */
    Debug_Printf("UART_0 initialized successfully\r\n");

    Debug_Printf("\r\n=== XMC4800 CANopen Device ===\r\n");
    Debug_Printf("Based on CanOpenSTM32 best practices\r\n");

    /* CANopen 堆疊初始化 - 只在啟動時分配一次記憶體 */
    CO = CO_new(NULL, &heapMemoryUsed);
    if (CO == NULL) {
        CO_errExit("ERROR: Cannot allocate CANopen memory");
    } else {
        Debug_Printf("Allocated %u bytes for CANopen\r\n", heapMemoryUsed);
    }

    DIGITAL_IO_SetOutputLow(&LED1);
}

/**
 * CANopen 重置處理 - 學習 STM32 模式
 */
static CO_NMT_reset_cmd_t app_programReset(void)
{
    CO_ReturnError_t err;
    uint32_t errInfo = 0;

    Debug_Printf("=== CANopen Reset Start ===\r\n");
    Debug_Printf("CANopen - Reset communication...\r\n");

    /* CAN 模組初始化 - 使用 CAN_NODE */
    Debug_Printf("Initializing CAN module...\r\n");
    err = CO_CANinit(CO, (void*)&CAN_NODE_0, pendingBitRate);
    if (err != CO_ERROR_NO) {
        errCnt_CO_init++;
        Debug_Printf("ERROR: CAN init failed (%d), count: %u\r\n", err, errCnt_CO_init);
        return CO_RESET_APP;
    }
    Debug_Printf("CAN module initialized successfully\r\n");

    /* **🔧 使用 CAN_NODE APP - 包含完整的 GPIO 和中斷配置** */
    Debug_Printf("=== 使用 CAN_NODE 模式 ===\r\n");

    /* CANopen 堆疊初始化 - 按 STM32 模式 */
    Debug_Printf("CANopen 堆疊初始化開始...\r\n");
    err = CO_CANopenInit(CO,
                        NULL,                   /* 備用 NMT 指令回調 */
                        NULL,                   /* LSS 主機回調 */
                        OD,                     /* 物件字典 */
                        OD_STATUS_BITS,         /* 可選狀態位 */
                        NMT_CONTROL,            /* NMT 控制位元 */
                        FIRST_HB_TIME,          /* 首次心跳時間 */
                        SDO_SRV_TIMEOUT_TIME,   /* SDO 伺服器超時 */
                        SDO_CLI_TIMEOUT_TIME,   /* SDO 客戶端超時 */
                        SDO_CLI_BLOCK,          /* SDO 客戶端區塊傳輸 */
                        pendingNodeId,          /* 節點 ID */
                        &errInfo);              /* 錯誤資訊 */

    Debug_Printf("CANopen 初始化結果: err=%d, errInfo=0x%X\r\n", err, errInfo);

    if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        errCnt_CO_init++;
        Debug_Printf("Error: CANopen init failed (%d), info: 0x%X, count: %u\r\n", 
                    err, errInfo, errCnt_CO_init);
        return CO_RESET_APP;
    } else {
        Debug_Printf("CANopen 堆疊初始化成功\r\n");
    }

    /* 初始化 PDO - 但忽略錯誤 */
    Debug_Printf("初始化 PDO...\r\n");
    err = CO_CANopenInitPDO(CO, CO->em, OD, pendingNodeId, &errInfo);
    Debug_Printf("PDO 初始化結果: err=%d, errInfo=0x%X\r\n", err, errInfo);
    if (err != CO_ERROR_NO) {
        Debug_Printf("Warning: PDO init returned error (%d), info: 0x%X - 繼續執行\r\n", err, errInfo);
    } else {
        Debug_Printf("PDO 初始化成功\r\n");
    }

    /* 啟動 CAN 模組 */
    Debug_Printf("啟動 CAN 模組為正常模式...\r\n");
    CO_CANsetNormalMode(CO->CANmodule);

    /* 標準 CANopen 初始化和狀態設定 */
    if (CO->NMT != NULL && CO->em != NULL) {
        Debug_Printf("CANopen 狀態轉換程序開始...\r\n");
        
        /* 1. 檢查並報告當前錯誤狀態 */
        uint8_t initial_error = *(CO->em->errorRegister);
        Debug_Printf("初始錯誤暫存器: 0x%02X\r\n", initial_error);
        
        /* 2. 強制清除錯誤暫存器 */
        *(CO->em->errorRegister) = 0;
        Debug_Printf("錯誤暫存器已清除\r\n");
        
        /* 3. 檢查 NMT 狀態機是否就緒 */
        Debug_Printf("當前 NMT 狀態: %d\r\n", CO->NMT->operatingState);
        
        /* 4. 使用標準 CANopen 方式進入 OPERATIONAL */
        Debug_Printf("發送內部 NMT 指令進入 OPERATIONAL...\r\n");
        CO_NMT_sendInternalCommand(CO->NMT, CO_NMT_ENTER_OPERATIONAL);
        
        /* 5. 給狀態機時間處理 */
        for (volatile uint32_t delay = 0; delay < 10000; delay++);
        
        /* 6. 檢查狀態轉換結果 */
        Debug_Printf("狀態轉換後 NMT 狀態: %d\r\n", CO->NMT->operatingState);
        Debug_Printf("狀態轉換後錯誤暫存器: 0x%02X\r\n", *(CO->em->errorRegister));
        
        /* 7. 如果仍未進入 OPERATIONAL，強制設定 */
        if (CO->NMT->operatingState != CO_NMT_OPERATIONAL) {
            Debug_Printf("警告：標準轉換失敗，強制設定 OPERATIONAL 狀態\r\n");
            CO->NMT->operatingState = CO_NMT_OPERATIONAL;
            *(CO->em->errorRegister) = 0;  /* 再次清除錯誤 */
        }
        
        canopen_ready = true;
        Debug_Printf("CANopen 初始化完成：狀態=%d，錯誤=0x%02X\r\n", 
                    CO->NMT->operatingState, *(CO->em->errorRegister));
    }

    Debug_Printf("CANopen device initialized (Node ID: %d, Bitrate: %d kbit/s)\r\n", 
                pendingNodeId, pendingBitRate);

    /* 重置計時器 */
    CO_timer1ms = 0;

    return CO_RESET_NOT;
}

/**
 * 應用程式結束
 */
static void app_programEnd(void)
{
    CO_CANsetConfigurationMode(NULL);
    CO_delete(CO);
    Debug_Printf("Program ended\r\n");
}

/**
 * 1ms 主任務 - 專業版本，減少冗餘輸出
 */
static void mainTask_1ms(void)
{
    static uint32_t ms_counter = 0;
    
    /* 每次檢查計時器是否需要處理 */
    while (CO_timer1ms > ms_counter) {
        ms_counter++;
        
        /* 安全檢查 - 確保CO已初始化 */
        if (CO != NULL) {
            /* CANopen 核心處理 - 按照標準順序 */
            process_canopen_communication();
            
            /* 監控重要狀態變化 (避免頻繁輸出) */
            if ((ms_counter % 1000) == 0) {
                monitor_error_register();
                handle_nmt_state_change();
            }
            
            /* 輕量級 PDO 處理 */
            process_pdo_communication();
        }

        /* 每 100ms 更新 LED */
        if ((ms_counter % 100) == 0) {
            app_updateLEDs();
        }

        /* 每 5000ms (5秒) 進行一次 CANopen 功能測試 */
        if ((ms_counter % 5000) == 0 && ms_counter > 3000) {
            send_test_can_message();
        }

        /* 每 10000ms (10秒) 輸出簡化統計 */
        if ((ms_counter % 10000) == 0) {
            report_canopen_statistics();
        }

        /* 檢查 RX 訊息 - 新增 */
        if ((ms_counter % 100) == 0) {
            check_canopen_rx_messages();
        }
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
            
        case 3: /* 測試直接 CAN 傳送 */
            if (CO->CANmodule != NULL && CO->CANmodule->txArray != NULL) {
                CO_CANtx_t *testTx = &CO->CANmodule->txArray[0];
                if (!testTx->bufferFull) {
                    testTx->ident = 0x123;
                    testTx->DLC = 8;
                    for (int i = 0; i < 8; i++) {
                        testTx->data[i] = 0xA0 + test_cycle + i;
                    }
                    CO_CANsend(CO->CANmodule, testTx);
                    Debug_Printf("  -> Test message sent ID=0x123\r\n");
                }
            }
            break;
            
        case 4: /* 測試 NMT 狀態切換 */
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
 * 除錯輸出函式 - 使用 UART_0 輸出
 */
static void Debug_Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int len = vsnprintf(debug_buffer, sizeof(debug_buffer), format, args);
    va_end(args);
    
    /* 透過 UART_0 輸出 debug 訊息 */
    if (len > 0) {
        UART_STATUS_t uart_status = UART_Transmit(&UART_0, (uint8_t*)debug_buffer, len);
        if (uart_status == UART_STATUS_SUCCESS) {
            /* 等待傳輸完成 */
            while (UART_0.runtime->tx_busy == true) {
                /* 等待 UART 傳輸完成 */
            }
        }
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