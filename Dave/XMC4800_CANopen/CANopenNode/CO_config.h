/**
 * CANopenNode configuration for XMC4800
 * 
 * @file CO_config.h
 * @author XMC4800 CANopen Team
 * @copyright 2025
 */

#ifndef CO_CONFIG_H
#define CO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic definitions */
#define CO_CONFIG_BASIC 0x01

/**
 * @defgroup CO_STACK_CONFIG Stack configuration
 * @{
 *
 * Configuration macros for CANopenNode stack. These macros are used to 
 * enable/disable specific CANopen features and configure stack behavior.
 */

/* CiA 301 - CANopen application layer and communication profile */
#define CO_CONFIG_NMT                   (CO_CONFIG_NMT_ENABLE)
#define CO_CONFIG_SDO_SRV               (CO_CONFIG_SDO_SRV_ENABLE)
#define CO_CONFIG_SDO_CLI               0
#define CO_CONFIG_PDO                   (CO_CONFIG_PDO_ENABLE | CO_CONFIG_RPDO_ENABLE | CO_CONFIG_TPDO_ENABLE)
#define CO_CONFIG_SYNC                  (CO_CONFIG_SYNC_ENABLE)
#define CO_CONFIG_EMERGENCY             (CO_CONFIG_EMERGENCY_ENABLE)
#define CO_CONFIG_HB_CONS               0
#define CO_CONFIG_TIME                  0

/* CiA 303 - Indicator specification */
#define CO_CONFIG_LEDS                  0

/* CiA 305 - Layer setting services (LSS) */  
#define CO_CONFIG_LSS                   0

/* Storage */
#define CO_CONFIG_STORAGE               0

/* Gateway ASCII */
#define CO_CONFIG_GTW                   0

/* Trace */
#define CO_CONFIG_TRACE                 0

/* XMC4800 specific configurations */
#define CO_CONFIG_CAN_TX_BUFFER_SIZE    16
#define CO_CONFIG_CAN_RX_BUFFER_SIZE    16
#define CO_CONFIG_SDO_BUFFER_SIZE       1000
#define CO_CONFIG_RXFIFO_SIZE          16
#define CO_CONFIG_TXFIFO_SIZE          16

/* Enable/disable flags for stack features */
#define CO_CONFIG_NMT_ENABLE            0x01
#define CO_CONFIG_SDO_SRV_ENABLE        0x01  
#define CO_CONFIG_PDO_ENABLE            0x01
#define CO_CONFIG_RPDO_ENABLE           0x02
#define CO_CONFIG_TPDO_ENABLE           0x04
#define CO_CONFIG_SYNC_ENABLE           0x01
#define CO_CONFIG_EMERGENCY_ENABLE      0x01

/* Platform specific definitions */
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) ((((uint16_t)(x) & 0xff) << 8) | (((uint16_t)(x) & 0xff00) >> 8))
#define CO_SWAP_32(x) ((((uint32_t)(x) & 0xff) << 24) | (((uint32_t)(x) & 0xff00) << 8) | \
                       (((uint32_t)(x) & 0xff0000) >> 8) | (((uint32_t)(x) & 0xff000000) >> 24))

/* XMC4800 Memory configuration */
#define CO_CONFIG_HEAP_SIZE            (16 * 1024)  /* 16KB heap for CANopen objects */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_CONFIG_H */