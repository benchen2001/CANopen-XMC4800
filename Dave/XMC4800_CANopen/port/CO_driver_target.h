/**
 * Target specific definitions for XMC4800 CANopen implementation
 *
 * @file CO_driver_target.h  
 * @author XMC4800 CANopen Team
 * @copyright 2025
 */

#ifndef CO_DRIVER_TARGET_H
#define CO_DRIVER_TARGET_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic definitions */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* XMC4800 DAVE includes */
#include "DAVE.h"

/* **🎯PortNote: XMC4800 特定的類型定義 - 必須在 CANopen 標頭檔之前定義** */

/* **🎯PortNote: 資料儲存物件結構 - 與 CANopenNode 標準一致** */
/**
 * Data storage object for one entry.
 * Must be defined in the CO_driver_target.h file.
 */
typedef struct {
    void* addr;                 /**< Address of data to store, always required. */
    size_t len;                 /**< Length of data to store, always required. */
    uint8_t subIndexOD;         /**< Sub index in OD objects 1010 and 1011, from 2 to 127. */
    uint8_t attr;               /**< Attribute from CO_storage_attributes_t, always required. */
    void* storageModule;        /**< Pointer to storage module, target system specific usage. */
    uint16_t crc;               /**< CRC checksum of the data stored in eeprom, set on store. */
    size_t eepromAddrSignature; /**< Address of entry signature inside eeprom, set by init. */
    size_t eepromAddr;          /**< Address of data inside eeprom, set by init. */
    size_t offset;              /**< Offset of next byte being updated by automatic storage. */
    void* additionalParameters; /**< Additional target specific parameters, optional. */
    /* Additional variables (target specific) */
    void* addrNV;               /**< XMC4800 特定：非揮發性記憶體位址 */
} CO_storage_entry_t;
/* 這些配置必須在包含 CANopen 標頭檔之前定義 */
#ifndef CO_CONFIG_LSS_SLAVE
#define CO_CONFIG_LSS_SLAVE                         0x01
#endif
#define CO_CONFIG_LSS       (CO_CONFIG_LSS_SLAVE)    /* 啟用 LSS Slave 功能 */
#define CO_CONFIG_LEDS      0                        /* 禁用 LED 功能 (XMC4800 自訂實現) */

/* Critical sections - 支援兩種調用方式 */
#define CO_LOCK_CAN_SEND(...)               do { __disable_irq(); } while (0)
#define CO_UNLOCK_CAN_SEND(...)             do { __enable_irq(); } while (0)

#define CO_LOCK_EMCY(...)                   do { __disable_irq(); } while (0)
#define CO_UNLOCK_EMCY(...)                 do { __enable_irq(); } while (0)

#define CO_LOCK_OD(CAN_MODULE)              do { (CAN_MODULE)->primask_od = __get_PRIMASK(); __disable_irq(); } while (0)
#define CO_UNLOCK_OD(CAN_MODULE)            __set_PRIMASK((CAN_MODULE)->primask_od)

/* Data types */
typedef bool                    bool_t;
typedef float                   float32_t;
typedef double                  float64_t;
typedef char                    char_t;
typedef unsigned char           oChar_t;
typedef unsigned char           domain_t;

/* Forward declarations */
typedef struct CO_CANrx_t CO_CANrx_t;
typedef struct CO_CANtx_t CO_CANtx_t;

/* CAN module object - 符合 CANopenNode v2.0 標準 */
typedef struct CO_CANmodule_t {
    void                    *CANptr;            /* From CO_CANmodule_init() */
    CO_CANrx_t              *rxArray;           /* From CO_CANmodule_init() */
    uint16_t                rxSize;             /* From CO_CANmodule_init() */
    CO_CANtx_t              *txArray;           /* From CO_CANmodule_init() */
    uint16_t                txSize;             /* From CO_CANmodule_init() */
    uint16_t                CANerrorStatus;     /* CAN error status bitfield */
    volatile bool_t         CANnormal;         /* CAN module is in normal mode */
    volatile bool_t         useCANrxFilters;   /* Hardware filters usage flag */
    volatile bool_t         bufferInhibitFlag; /* Synchronous PDO message flag */
    volatile bool_t         firstCANtxMessage; /* First transmitted message flag */
    volatile uint16_t       CANtxCount;        /* Number of messages waiting */
    uint32_t                errOld;            /* Previous state of CAN errors */
    
    /* Critical section primask storage - 必要的關鍵段保護 */
    uint32_t                primask_send;
    uint32_t                primask_emcy;
    uint32_t                primask_od;
} CO_CANmodule_t;

/* Receive message buffer */
struct CO_CANrx_t {
    uint32_t ident;             /* Standard CAN identifier (11-bit) */
    uint32_t mask;              /* Acceptance mask */
    void *object;               /* From CO_CANrxBufferInit() */
    void (*CANrx_callback)(void *object, void *message);  /* From CO_CANrxBufferInit() */
    
    /* XMC4800 specific - CAN_NODE 版本 */
    void *dave_lmo;             /* CAN_NODE LMO 配置指針 */
    uint8_t lmo_index;          /* LMO index */
};

/* Transmit message buffer */
struct CO_CANtx_t {
    uint32_t ident;             /* CAN identifier */
    uint8_t DLC;                /* Data Length Code */
    uint8_t data[8];            /* 8 data bytes */
    volatile bool_t bufferFull; /* Buffer full flag */
    volatile bool_t syncFlag;   /* Synchronous flag */
    
    /* XMC4800 specific - CAN_NODE 版本 */
    void *dave_lmo;             /* CAN_NODE LMO 配置指針 */
    uint8_t lmo_index;          /* LMO index */
};

/* CAN message object */
typedef struct {
    uint32_t ident;             /* CAN identifier */
    uint8_t DLC;                /* Data Length Code */
    uint8_t data[8];            /* 8 data bytes */
} CO_CANrxMsg_t;

/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg)      ((uint16_t)(((CO_CANrxMsg_t *)(msg))->ident))
#define CO_CANrxMsg_readDLC(msg)        ((uint8_t)(((CO_CANrxMsg_t *)(msg))->DLC))
#define CO_CANrxMsg_readData(msg)       ((uint8_t *)(((CO_CANrxMsg_t *)(msg))->data))

/* Timer definitions - 移除衝突的外部聲明 */
#define CO_timer1ms                     CO_timer1ms

/* Flag macros for message flags */
#define CO_FLAG_READ(rxNew) ((rxNew) != 0)
#define CO_FLAG_SET(rxNew) do { (rxNew) = 1; } while (0)
#define CO_FLAG_CLEAR(rxNew) do { (rxNew) = 0; } while (0)

/* Endianness and swap macros */
#ifdef CO_LITTLE_ENDIAN
    #define CO_SWAP_16(x) x
    #define CO_SWAP_32(x) x
#else
    #define CO_SWAP_16(x) ((((uint16_t)(x) & 0xFF00U) >> 8) | (((uint16_t)(x) & 0x00FFU) << 8))
    #define CO_SWAP_32(x) ((((uint32_t)(x) & 0xFF000000UL) >> 24) | \
                           (((uint32_t)(x) & 0x00FF0000UL) >> 8)  | \
                           (((uint32_t)(x) & 0x0000FF00UL) << 8)  | \
                           (((uint32_t)(x) & 0x000000FFUL) << 24))
#endif

/* Define endianness for XMC4800 (Cortex-M4 is little endian) */
#define CO_LITTLE_ENDIAN

/* XMC4800 specific functions */
void CO_CANsetConfigurationMode(void *CANptr);
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule);
void CO_CANinterrupt(CO_CANmodule_t *CANmodule);

/* CAN bit rates */
typedef enum {
    CO_CAN_BITRATE_1000  = 1000,    /* 1000 kbps */
    CO_CAN_BITRATE_800   = 800,     /* 800 kbps */
    CO_CAN_BITRATE_500   = 500,     /* 500 kbps */
    CO_CAN_BITRATE_250   = 250,     /* 250 kbps */
    CO_CAN_BITRATE_125   = 125,     /* 125 kbps */
    CO_CAN_BITRATE_50    = 50,      /* 50 kbps */
    CO_CAN_BITRATE_20    = 20,      /* 20 kbps */
    CO_CAN_BITRATE_10    = 10       /* 10 kbps */
} CO_CANbitRate_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_TARGET_H */