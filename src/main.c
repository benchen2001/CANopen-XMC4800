/**
 * @file main.c
 * @brief XMC4800 CANopen 主程式
 * @author MCU專業軟體工程師
 * @date 2025-08-21
 * 
 * 本檔案包含 XMC4800 CANopen 節點的主要初始化和執行邏輯
 * 基於 CANopenNode 2.0 函式庫實現完整的 CANopen 協定功能
 */

#include <xmc_gpio.h>
#include <xmc_can.h>
#include <xmc_uart.h>
#include <xmc_scu.h>

/* CANopen includes */
#include "301/CO_driver.h"
#include "301/CO_OD.h"
#include "301/CO_SDO.h"
#include "301/CO_Emergency.h"
#include "301/CO_NMT_Heartbeat.h"
#include "301/CO_SYNC.h"
#include "301/CO_PDO.h"

/* Application includes */
#include "can_driver/can_xmc4800.h"
#include "application/app_main.h"

/* 系統配置 */
#define SYSTEM_CLOCK_FREQ       120000000UL  // 120 MHz
#define CAN_BITRATE            500000UL      // 500 kbps
#define CANOPEN_NODE_ID        0x7F          // 預設節點 ID
#define HEARTBEAT_TIME         1000          // 1 秒心跳

/* LED 狀態指示 */
#define LED_GREEN_PORT         XMC_GPIO_PORT1
#define LED_GREEN_PIN          1
#define LED_RED_PORT           XMC_GPIO_PORT1  
#define LED_RED_PIN            0

/* UART 除錯配置 */
#define DEBUG_UART_PORT        XMC_GPIO_PORT1
#define DEBUG_UART_TX_PIN      5
#define DEBUG_UART_RX_PIN      4

/* 全域變數 */
static CO_t *CO = NULL;
static uint32_t timer1msPrevious = 0;
static uint16_t timer1msToggle = 0;
static bool_t firstRun = true;

/* 函數宣告 */
static void system_init(void);
static void gpio_init(void);
static void uart_debug_init(void);
static void can_hardware_init(void);
static CO_ReturnError_t canopen_init(void);
static void main_loop_1ms(void);
static void main_loop_process(void);
static void nmt_state_changed(CO_NMT_internalState_t state);
static void debug_printf(const char* format, ...);

/**
 * @brief 主程式進入點
 * @return 永不返回
 */
int main(void)
{
    CO_ReturnError_t err;
    
    /* 系統初始化 */
    system_init();
    gpio_init();
    uart_debug_init();
    
    debug_printf("XMC4800 CANopen Node Starting...\r\n");
    debug_printf("System Clock: %lu Hz\r\n", SYSTEM_CLOCK_FREQ);
    debug_printf("CAN Bitrate: %lu bps\r\n", CAN_BITRATE);
    debug_printf("Node ID: 0x%02X\r\n", CANOPEN_NODE_ID);
    
    /* CAN 硬體初始化 */
    can_hardware_init();
    debug_printf("CAN Hardware Initialized\r\n");
    
    /* CANopen 堆疊初始化 */
    err = canopen_init();
    if (err != CO_ERROR_NO) {
        debug_printf("CANopen Init Error: %d\r\n", err);
        /* LED 紅燈指示錯誤 */
        XMC_GPIO_SetOutputLevel(LED_RED_PORT, LED_RED_PIN, XMC_GPIO_OUTPUT_LEVEL_HIGH);
        while(1) {
            /* 錯誤狀態，停止執行 */
        }
    }
    
    debug_printf("CANopen Stack Initialized Successfully\r\n");
    
    /* 設定 NMT 狀態變更回調 */
    CO_NMT_initCallbackChanged(CO->NMT, nmt_state_changed);
    
    /* 進入 Operational 狀態 */
    CO_NMT_sendCommand(CO->NMT, CO_NMT_ENTER_OPERATIONAL, CANOPEN_NODE_ID);
    
    /* LED 綠燈指示正常運行 */
    XMC_GPIO_SetOutputLevel(LED_GREEN_PORT, LED_GREEN_PIN, XMC_GPIO_OUTPUT_LEVEL_HIGH);
    
    debug_printf("Entering Main Loop...\r\n");
    
    /* 主迴圈 */
    while(1) {
        main_loop_process();
    }
    
    /* 永不到達 */
    return 0;
}

/**
 * @brief 系統時鐘和基礎硬體初始化
 */
static void system_init(void)
{
    /* 初始化系統時鐘到 120MHz */
    if (XMC_SCU_CLOCK_Init(&clock_config) != XMC_SCU_CLOCK_STATUS_OK) {
        /* 時鐘初始化失敗 */
        while(1);
    }
    
    /* 初始化系統計時器 */
    SysTick_Config(SYSTEM_CLOCK_FREQ / 1000);  // 1ms 中斷
    
    /* 啟用全域中斷 */
    __enable_irq();
}

/**
 * @brief GPIO 初始化
 */
static void gpio_init(void)
{
    /* LED 腳位配置 */
    XMC_GPIO_CONFIG_t led_config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level = XMC_GPIO_OUTPUT_LEVEL_LOW
    };
    
    XMC_GPIO_Init(LED_GREEN_PORT, LED_GREEN_PIN, &led_config);
    XMC_GPIO_Init(LED_RED_PORT, LED_RED_PIN, &led_config);
}

/**
 * @brief UART 除錯介面初始化
 */
static void uart_debug_init(void)
{
    /* UART 配置 */
    XMC_UART_CH_CONFIG_t uart_config = {
        .baudrate = 115200,
        .data_bits = 8,
        .frame_length = 8,
        .stop_bits = 1,
        .oversampling = 16,
        .parity_mode = XMC_USIC_CH_PARITY_MODE_NONE
    };
    
    /* 初始化 UART */
    XMC_UART_CH_Init(XMC_UART0_CH0, &uart_config);
    
    /* 配置 UART 腳位 */
    XMC_GPIO_CONFIG_t uart_tx_config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2
    };
    XMC_GPIO_CONFIG_t uart_rx_config = {
        .mode = XMC_GPIO_MODE_INPUT_TRISTATE
    };
    
    XMC_GPIO_Init(DEBUG_UART_PORT, DEBUG_UART_TX_PIN, &uart_tx_config);
    XMC_GPIO_Init(DEBUG_UART_PORT, DEBUG_UART_RX_PIN, &uart_rx_config);
    
    /* 設定 UART 輸入 */
    XMC_UART_CH_SetInputSource(XMC_UART0_CH0, XMC_UART_CH_INPUT_RXD, USIC0_C0_DX0_P1_4);
    
    /* 啟動 UART */
    XMC_UART_CH_Start(XMC_UART0_CH0);
}

/**
 * @brief CAN 硬體初始化
 */
static void can_hardware_init(void)
{
    /* CAN 節點配置 */
    XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t can_bit_time;
    
    /* 計算 CAN 位元時間 */
    if (XMC_CAN_NODE_NominalBitTimeConfigure(XMC_CAN_NODE_0, 
                                           SYSTEM_CLOCK_FREQ, 
                                           CAN_BITRATE, 
                                           &can_bit_time) != XMC_CAN_STATUS_SUCCESS) {
        debug_printf("CAN Bit Time Calculation Failed\r\n");
        while(1);
    }
    
    /* 初始化 CAN 模組 */
    XMC_CAN_Init(CAN, XMC_CAN_CANCLKSRC_FPERI, SYSTEM_CLOCK_FREQ);
    
    /* 初始化 CAN 節點 */
    XMC_CAN_NODE_Init(XMC_CAN_NODE_0);
    
    /* 配置 CAN 腳位 */
    XMC_GPIO_CONFIG_t can_tx_config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9
    };
    XMC_GPIO_CONFIG_t can_rx_config = {
        .mode = XMC_GPIO_MODE_INPUT_TRISTATE
    };
    
    XMC_GPIO_Init(XMC_GPIO_PORT1, 13, &can_tx_config);  // CAN_TXD
    XMC_GPIO_Init(XMC_GPIO_PORT1, 12, &can_rx_config);  // CAN_RXD
    
    /* 設定 CAN 輸入 */
    XMC_CAN_NODE_SetReceiveInput(XMC_CAN_NODE_0, XMC_CAN_NODE_RECEIVE_INPUT_RXDCAN);
    
    /* 設定位元時間 */
    XMC_CAN_NODE_SetBitTime(XMC_CAN_NODE_0, &can_bit_time);
    
    /* 啟用 CAN 節點 */
    XMC_CAN_NODE_ResetInitBit(XMC_CAN_NODE_0);
}

/**
 * @brief CANopen 堆疊初始化
 * @return CO_ReturnError_t 初始化結果
 */
static CO_ReturnError_t canopen_init(void)
{
    CO_ReturnError_t err;
    
    /* 分配 CANopen 物件記憶體 */
    CO = CO_new(NULL, NULL);
    if (CO == NULL) {
        debug_printf("CO_new() failed\r\n");
        return CO_ERROR_OUT_OF_MEMORY;
    }
    
    /* CANopen 堆疊初始化 */
    err = CO_CANinit(CO,                    /* CANopen object */
                     (void*)XMC_CAN_NODE_0, /* CAN module address */
                     CAN_BITRATE);          /* bit rate */
    
    if (err != CO_ERROR_NO) {
        debug_printf("CO_CANinit() failed: %d\r\n", err);
        return err;
    }
    
    /* 初始化 CANopen 物件 */
    err = CO_CANopenInit(CO,                /* CANopen object */
                         NULL,              /* alternate NMT */
                         NULL,              /* alternate em */
                         OD,                /* Object dictionary */
                         OD_STATUS_BITS,    /* Optional OD_statusBits */
                         NMT_CONTROL,       /* CO_NMT_control_t */
                         FIRST_HB_TIME,     /* firstHBTime_ms */
                         SDO_SRV_TIMEOUT_TIME, /* SDOserverTimeoutTime_ms */
                         SDO_CLI_TIMEOUT_TIME, /* SDOclientTimeoutTime_ms */
                         SDO_CLI_BLOCK,     /* SDOclientBlockTransfer */
                         CANOPEN_NODE_ID);   /* CANopen Node ID */
    
    if (err != CO_ERROR_NO) {
        debug_printf("CO_CANopenInit() failed: %d\r\n", err);
        return err;
    }
    
    /* 配置 CANopen 參數 */
    *CO->NMT->emPr->errorRegister = 0;
    *CO->NMT->emPr->producerHeartbeatTime = HEARTBEAT_TIME;
    
    return CO_ERROR_NO;
}

/**
 * @brief 1ms 主迴圈處理
 */
static void main_loop_1ms(void)
{
    if (CO != NULL) {
        /* CANopen 同步處理 */
        CO_process_SYNC(CO, 1000, NULL);
        
        /* CANopen RPDO 處理 */
        CO_process_RPDO(CO, 1000, NULL);
        
        /* CANopen TPDO 處理 */
        CO_process_TPDO(CO, 1000, NULL);
        
        /* 應用程式特定處理 */
        timer1msToggle++;
        if (timer1msToggle >= 500) {  // 500ms 週期
            timer1msToggle = 0;
            /* LED 閃爍指示系統運行 */
            XMC_GPIO_ToggleOutput(LED_GREEN_PORT, LED_GREEN_PIN);
        }
    }
}

/**
 * @brief 主迴圈處理
 */
static void main_loop_process(void)
{
    uint32_t timer1msNow = CO_timer1ms;
    uint32_t timer1msDiff = timer1msNow - timer1msPrevious;
    timer1msPrevious = timer1msNow;
    
    /* 1ms 處理 */
    if (timer1msDiff > 0) {
        main_loop_1ms();
    }
    
    if (CO != NULL) {
        /* CANopen 背景處理 */
        CO_process(CO, false, timer1msDiff, NULL);
        
        /* 檢查 CANopen 錯誤 */
        if (CO->NMT->internalState == CO_NMT_INITIALIZING) {
            if (firstRun) {
                firstRun = false;
                debug_printf("CANopen Node Initialized\r\n");
            }
        }
    }
}

/**
 * @brief NMT 狀態變更回調函數
 * @param state 新的 NMT 狀態
 */
static void nmt_state_changed(CO_NMT_internalState_t state)
{
    const char* state_names[] = {
        "INITIALIZING",
        "PRE_OPERATIONAL", 
        "OPERATIONAL",
        "STOPPED"
    };
    
    if (state < sizeof(state_names)/sizeof(state_names[0])) {
        debug_printf("NMT State Changed: %s\r\n", state_names[state]);
    }
    
    /* 根據狀態控制 LED */
    switch (state) {
        case CO_NMT_OPERATIONAL:
            XMC_GPIO_SetOutputLevel(LED_GREEN_PORT, LED_GREEN_PIN, XMC_GPIO_OUTPUT_LEVEL_HIGH);
            XMC_GPIO_SetOutputLevel(LED_RED_PORT, LED_RED_PIN, XMC_GPIO_OUTPUT_LEVEL_LOW);
            break;
        case CO_NMT_PRE_OPERATIONAL:
            /* 綠燈閃爍 */
            break;
        case CO_NMT_STOPPED:
            XMC_GPIO_SetOutputLevel(LED_GREEN_PORT, LED_GREEN_PIN, XMC_GPIO_OUTPUT_LEVEL_LOW);
            XMC_GPIO_SetOutputLevel(LED_RED_PORT, LED_RED_PIN, XMC_GPIO_OUTPUT_LEVEL_HIGH);
            break;
        default:
            break;
    }
}

/**
 * @brief 系統定時器中斷處理
 */
void SysTick_Handler(void)
{
    CO_timer1ms++;
}

/**
 * @brief 除錯訊息輸出 (簡化版實現)
 * @param format 格式字串
 * @param ... 可變參數
 */
static void debug_printf(const char* format, ...)
{
    /* 簡化實現：只輸出格式字串 */
    const char* ptr = format;
    while (*ptr) {
        while (!XMC_UART_CH_GetTransmitBufferStatus(XMC_UART0_CH0));
        XMC_UART_CH_Transmit(XMC_UART0_CH0, *ptr++);
    }
}