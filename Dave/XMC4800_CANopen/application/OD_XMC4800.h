/**
 * Application-specific CANopen Object Dictionary for XMC4800
 * Based on CANopenNode example and extended for monitor functionality
 *
 * @file OD_XMC4800.h
 * @author XMC4800 CANopen Team
 * @copyright 2025
 */

#ifndef OD_XMC4800_H
#define OD_XMC4800_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard includes */
#include <stdint.h>
#include <stdbool.h>

/* CANopenNode includes */
#include "../CANopenNode/301/CO_ODinterface.h"

/* Device profile and identification */
#define OD_DEVICE_TYPE                  0x00000000UL  /* Generic device */
#define OD_VENDOR_ID                    0x00000001UL  /* Custom vendor ID */
#define OD_PRODUCT_CODE                 0x48000001UL  /* XMC4800 CANopen Device */
#define OD_REVISION_NUMBER              0x00010001UL  /* Version 1.1 */
#define OD_SERIAL_NUMBER                0x12345678UL  /* Serial number */

/* Object dictionary size definitions */
#define OD_CNT_NMT                      1
#define OD_CNT_SDO_SRV                  1
#define OD_CNT_EM                       1
#define OD_CNT_SYNC                     1
#define OD_CNT_RPDO                     4
#define OD_CNT_TPDO                     4
#define OD_CNT_HB_CONS                  0
#define OD_CNT_ARR_1003                 8    /* Pre-defined error field */
#define OD_CNT_ARR_1005                 0    /* COB-ID SYNC message */
#define OD_CNT_ARR_1006                 0    /* Communication cycle period */
#define OD_CNT_ARR_1007                 0    /* Synchronous window length */

/* Application-specific object counts */
#define OD_CNT_LEDS                     1    /* LED control */
#define OD_CNT_BUTTONS                  0    /* Button inputs */

/* Communication profile area (0x1000-0x1FFF) */
typedef struct {
    uint8_t  numberOfEntries;
    uint32_t vendorID;
    uint32_t productCode;
    uint32_t revisionNumber;
    uint32_t serialNumber;
} OD_identityObject_t;

/* Manufacturer specific profile area (0x2000-0x5FFF) */
typedef struct {
    uint8_t  numberOfEntries;
    uint32_t monitorMessageCount;       /* Total monitored messages */
    uint32_t monitorErrorCount;         /* Error count */
    uint16_t canBusLoad;                /* CAN bus load percentage */
    uint8_t  operatingMode;             /* 0=Device, 1=Monitor, 2=Hybrid */
    uint8_t  reserved;
} OD_monitorStatus_t;

/* Device profile area (0x6000-0x9FFF) */
typedef struct {
    uint8_t  numberOfEntries;
    uint8_t  digitalInputs;             /* Digital input status */
    uint8_t  digitalOutputs;            /* Digital output control */
    uint16_t analogInputs[4];           /* Analog input values */
    uint16_t analogOutputs[4];          /* Analog output values */
} OD_deviceIO_t;

/* PDO mapping structures */
typedef struct {
    uint32_t mappedObject1;             /* 0x20000120 - Monitor message count */
    uint32_t mappedObject2;             /* 0x60000108 - Digital outputs */
    uint32_t mappedObject3;             /* 0x60000208 - Digital inputs */
    uint32_t mappedObject4;             /* 0x00000000 - Unused */
} OD_TPDOMapping_t;

typedef struct {
    uint32_t mappedObject1;             /* 0x60000108 - Digital outputs */
    uint32_t mappedObject2;             /* 0x60000310 - Analog outputs */
    uint32_t mappedObject3;             /* 0x00000000 - Unused */
    uint32_t mappedObject4;             /* 0x00000000 - Unused */
} OD_RPDOMapping_t;

/* Global object dictionary variables */
extern OD_identityObject_t OD_identity;
extern OD_monitorStatus_t OD_monitorStatus;
extern OD_deviceIO_t OD_deviceIO;

/* Object dictionary entry structures - 使用正確的 CANopenNode v2.0 類型 */
extern const OD_entry_t OD_1000_deviceType;
extern const OD_entry_t OD_1001_errorRegister;
extern const OD_entry_t OD_1017_producerHeartbeatTime;
extern const OD_entry_t OD_1018_identity;

/* Manufacturer specific objects */
extern const OD_entry_t OD_2000_monitorStatus;

/* Device profile objects */  
extern const OD_entry_t OD_6000_digitalInputs;
extern const OD_entry_t OD_6200_digitalOutputs;

/* PDO configuration objects */
extern const OD_entry_t OD_1400_RPDO1_parameter;
extern const OD_entry_t OD_1600_RPDO1_mapping;
extern const OD_entry_t OD_1800_TPDO1_parameter;
extern const OD_entry_t OD_1A00_TPDO1_mapping;

/* Object dictionary table */
extern const OD_entry_t OD[];
extern const uint16_t OD_CNT;

/* Function prototypes */
CO_SDO_abortCode_t OD_writeOriginalData(OD_stream_t *stream,
                                       const void *buf,
                                       size_t count,
                                       size_t *countWritten);

CO_SDO_abortCode_t OD_readOriginalData(OD_stream_t *stream,
                                      void *buf,
                                      size_t count,
                                      size_t *countRead);

/* Application-specific callback functions */
void OD_monitorStatus_update(void);
void OD_deviceIO_update(void);

/* LED control macros */
#define OD_LED_GREEN_ON()       /* Implement LED control */
#define OD_LED_GREEN_OFF()      
#define OD_LED_RED_ON()         
#define OD_LED_RED_OFF()        
#define OD_LED_GREEN_TOGGLE()   
#define OD_LED_RED_TOGGLE()     

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OD_XMC4800_H */