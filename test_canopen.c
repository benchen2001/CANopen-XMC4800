/**
 * @file test_canopen.c
 * @brief 測試 CANopenNode 整合的程式
 * @author MCU專業軟體工程師
 * @date 2025-08-21
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* 首先引用我們的平台特定配置 */
#include "canopen/CO_driver_target.h"

/* 然後引用 CANopenNode 標頭檔 */
#include "CANopenNode/301/CO_driver.h"
#include "CANopenNode/301/CO_config.h"

/* 引用我們的 XMC4800 CAN 驅動 */
#include "canopen/can_xmc4800.h"

/***********************************************************************************************************************
 * 全域變數
 **********************************************************************************************************************/

/* CANopen 模組 */
static CO_CANmodule_t CO_CANmodule[1];

/* CANopen 接收和傳送緩衝區 */
static CO_CANrx_t CO_CANmodule_rxArray0[CO_CAN_RX_BUFFER_SIZE];
static CO_CANtx_t CO_CANmodule_txArray0[CO_CAN_TX_BUFFER_SIZE];

/***********************************************************************************************************************
 * 測試主程式
 **********************************************************************************************************************/
int main(void)
{
    printf("=== XMC4800 CANopenNode 整合測試 ===\n");
    printf("編譯時間: %s %s\n", __DATE__, __TIME__);
    
    /* 顯示 CANopenNode 配置 */
    printf("CANopenNode 配置:\n");
    printf("  CO_CONFIG_NMT: 0x%04X\n", CO_CONFIG_NMT);
    printf("  CO_CONFIG_EM: 0x%04X\n", CO_CONFIG_EM);
    printf("  CO_CONFIG_PDO: 0x%04X\n", CO_CONFIG_PDO);
    
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
    
    /* 顯示記憶體配置 */
    printf("記憶體配置:\n");
    printf("  CO_CANmodule_t: %zu bytes\n", sizeof(CO_CANmodule_t));
    printf("  RX 緩衝區: %zu bytes\n", sizeof(CO_CANmodule_rxArray0));
    printf("  TX 緩衝區: %zu bytes\n", sizeof(CO_CANmodule_txArray0));
    printf("  總記憶體使用: %zu bytes\n", 
           sizeof(CO_CANmodule) + sizeof(CO_CANmodule_rxArray0) + sizeof(CO_CANmodule_txArray0));
    
    printf("CANopenNode 整合測試通過！\n");
    
    return 0;
}