// clang-format off
/*******************************************************************************
    CANopen Object Dictionary definition for CANopenNode v1 to v2

    This file was automatically generated with
    libedssharp Object Dictionary Editor v0.8-125-g85dfd8e

    https://github.com/CANopenNode/CANopenNode
    https://github.com/robincornelius/libedssharp

    DON'T EDIT THIS FILE MANUALLY !!!!
*******************************************************************************/
#ifndef CO_OD_H_
#define CO_OD_H_

/*******************************************************************************
   CANopen DATA TYPES
*******************************************************************************/
   typedef bool_t       BOOLEAN;
   typedef uint8_t      UNSIGNED8;
   typedef uint16_t     UNSIGNED16;
   typedef uint32_t     UNSIGNED32;
   typedef uint64_t     UNSIGNED64;
   typedef int8_t       INTEGER8;
   typedef int16_t      INTEGER16;
   typedef int32_t      INTEGER32;
   typedef int64_t      INTEGER64;
   typedef float32_t    REAL32;
   typedef float64_t    REAL64;
   typedef char_t       VISIBLE_STRING;
   typedef oChar_t      OCTET_STRING;

   #ifdef DOMAIN
   #undef DOMAIN
   #endif

   typedef domain_t     DOMAIN;


/*******************************************************************************
   FILE INFO:
      FileName:     DS301_profile.eds
      FileVersion:  1
      CreationTime: 1:00PM
      CreationDate: 11-23-2020
      CreatedBy:    
*******************************************************************************/


/*******************************************************************************
   DEVICE INFO:
      VendorName:     
      VendorNumber:   
      ProductName:    New Product
      ProductNumber:  
*******************************************************************************/


/*******************************************************************************
   FEATURES
*******************************************************************************/
  #define CO_NO_SYNC                     1   //Associated objects: 1005-1007
  #define CO_NO_EMERGENCY                1   //Associated objects: 1014, 1015
  #define CO_NO_TIME                     1   //Associated objects: 1012, 1013
  #define CO_NO_SDO_SERVER               1   //Associated objects: 1200-127F
  #define CO_NO_SDO_CLIENT               1   //Associated objects: 1280-12FF
  #define CO_NO_GFC                      0   //Associated objects: 1300
  #define CO_NO_SRDO                     0   //Associated objects: 1301-1341, 1381-13C0
  #define CO_NO_LSS_SERVER               1   //LSS Slave
  #define CO_NO_LSS_CLIENT               0   //LSS Master
  #define CO_NO_RPDO                     4   //Associated objects: 14xx, 16xx
  #define CO_NO_TPDO                     4   //Associated objects: 18xx, 1Axx
  #define CO_NO_NMT_MASTER               0
  #define CO_NO_TRACE                    0


/*******************************************************************************
   OBJECT DICTIONARY
*******************************************************************************/
   #define CO_OD_NoOfElements             33


/*******************************************************************************
   TYPE DEFINITIONS FOR RECORDS
*******************************************************************************/
/*1018      */ typedef struct {
               UNSIGNED8      highestSubIndexSupported;
               UNSIGNED32     vendorID;
               UNSIGNED32     productCode;
               UNSIGNED32     revisionNumber;
               UNSIGNED32     serialNumber;
               }              OD_identity_t;
/*1200      */ typedef struct {
               UNSIGNED8      highestSubIndexSupported;
               UNSIGNED32     COB_IDClientToServer;
               UNSIGNED32     COB_IDServerToClient;
               }              OD_SDOServerParameter_t;
/*1280      */ typedef struct {
               UNSIGNED8      highestSubIndexSupported;
               UNSIGNED32     COB_IDClientToServer;
               UNSIGNED32     COB_IDServerToClient;
               UNSIGNED8      nodeIDOfTheSDOServer;
               }              OD_SDOClientParameter_t;
/*1400      */ typedef struct {
               UNSIGNED8      highestSubIndexSupported;
               UNSIGNED32     COB_IDUsedByRPDO;
               UNSIGNED8      transmissionType;
               UNSIGNED16     eventTimer;
               }              OD_RPDOCommunicationParameter_t;
/*1600      */ typedef struct {
               UNSIGNED8      numberOfMappedApplicationObjectsInPDO;
               UNSIGNED32     applicationObject1;
               UNSIGNED32     applicationObject2;
               UNSIGNED32     applicationObject3;
               UNSIGNED32     applicationObject4;
               UNSIGNED32     applicationObject5;
               UNSIGNED32     applicationObject6;
               UNSIGNED32     applicationObject7;
               UNSIGNED32     applicationObject8;
               }              OD_RPDOMappingParameter_t;
/*1800      */ typedef struct {
               UNSIGNED8      highestSubIndexSupported;
               UNSIGNED32     COB_IDUsedByTPDO;
               UNSIGNED8      transmissionType;
               UNSIGNED16     inhibitTime;
               UNSIGNED8      compatibilityEntry;
               UNSIGNED16     eventTimer;
               UNSIGNED8      SYNCStartValue;
               }              OD_TPDOCommunicationParameter_t;
/*1A00      */ typedef struct {
               UNSIGNED8      numberOfMappedApplicationObjectsInPDO;
               UNSIGNED32     applicationObject1;
               UNSIGNED32     applicationObject2;
               UNSIGNED32     applicationObject3;
               UNSIGNED32     applicationObject4;
               UNSIGNED32     applicationObject5;
               UNSIGNED32     applicationObject6;
               UNSIGNED32     applicationObject7;
               UNSIGNED32     applicationObject8;
               }              OD_TPDOMappingParameter_t;

/*******************************************************************************
   TYPE DEFINITIONS FOR OBJECT DICTIONARY INDEXES

   some of those are redundant with CO_SDO.h CO_ObjDicId_t <Common CiA301 object
   dictionary entries>
*******************************************************************************/
/*1000 */
        #define OD_1000_deviceType                                  0x1000

/*1001 */
        #define OD_1001_errorRegister                               0x1001

/*1003 */
        #define OD_1003_preDefinedErrorField                        0x1003

        #define OD_1003_0_preDefinedErrorField_maxSubIndex          0
        #define OD_1003_1_preDefinedErrorField_standardErrorField   1
        #define OD_1003_2_preDefinedErrorField_standardErrorField   2
        #define OD_1003_3_preDefinedErrorField_standardErrorField   3
        #define OD_1003_4_preDefinedErrorField_standardErrorField   4
        #define OD_1003_5_preDefinedErrorField_standardErrorField   5
        #define OD_1003_6_preDefinedErrorField_standardErrorField   6
        #define OD_1003_7_preDefinedErrorField_standardErrorField   7
        #define OD_1003_8_preDefinedErrorField_standardErrorField   8
        #define OD_1003_9_preDefinedErrorField_standardErrorField   9
        #define OD_1003_10_preDefinedErrorField_standardErrorField  10
        #define OD_1003_11_preDefinedErrorField_standardErrorField  11
        #define OD_1003_12_preDefinedErrorField_standardErrorField  12
        #define OD_1003_13_preDefinedErrorField_standardErrorField  13
        #define OD_1003_14_preDefinedErrorField_standardErrorField  14
        #define OD_1003_15_preDefinedErrorField_standardErrorField  15
        #define OD_1003_16_preDefinedErrorField_standardErrorField  16

/*1005 */
        #define OD_1005_COB_ID_SYNCMessage                          0x1005

/*1006 */
        #define OD_1006_communicationCyclePeriod                    0x1006

/*1007 */
        #define OD_1007_synchronousWindowLength                     0x1007

/*1010 */
        #define OD_1010_storeParameters                             0x1010

        #define OD_1010_0_storeParameters_maxSubIndex               0
        #define OD_1010_1_storeParameters_saveAllParameters         1
        #define OD_1010_2_storeParameters_saveCommunicationParameters 2
        #define OD_1010_3_storeParameters_saveApplicationParameters 3
        #define OD_1010_4_storeParameters_saveManufacturerDefinedParameters 4

/*1011 */
        #define OD_1011_restoreDefaultParameters                    0x1011

        #define OD_1011_0_restoreDefaultParameters_maxSubIndex      0
        #define OD_1011_1_restoreDefaultParameters_restoreAllDefaultParameters 1
        #define OD_1011_2_restoreDefaultParameters_restoreCommunicationDefaultParameters 2
        #define OD_1011_3_restoreDefaultParameters_restoreApplicationDefaultParameters 3
        #define OD_1011_4_restoreDefaultParameters_restoreManufacturerDefinedDefaultParameters 4

/*1012 */
        #define OD_1012_COB_IDTimeStampObject                       0x1012

/*1014 */
        #define OD_1014_COB_ID_EMCY                                 0x1014

/*1015 */
        #define OD_1015_inhibitTimeEMCY                             0x1015

/*1016 */
        #define OD_1016_consumerHeartbeatTime                       0x1016

        #define OD_1016_0_consumerHeartbeatTime_maxSubIndex         0
        #define OD_1016_1_consumerHeartbeatTime_consumerHeartbeatTime 1
        #define OD_1016_2_consumerHeartbeatTime_consumerHeartbeatTime 2
        #define OD_1016_3_consumerHeartbeatTime_consumerHeartbeatTime 3
        #define OD_1016_4_consumerHeartbeatTime_consumerHeartbeatTime 4
        #define OD_1016_5_consumerHeartbeatTime_consumerHeartbeatTime 5
        #define OD_1016_6_consumerHeartbeatTime_consumerHeartbeatTime 6
        #define OD_1016_7_consumerHeartbeatTime_consumerHeartbeatTime 7
        #define OD_1016_8_consumerHeartbeatTime_consumerHeartbeatTime 8

/*1017 */
        #define OD_1017_producerHeartbeatTime                       0x1017

/*1018 */
        #define OD_1018_identity                                    0x1018

        #define OD_1018_0_identity_maxSubIndex                      0
        #define OD_1018_1_identity_vendorID                         1
        #define OD_1018_2_identity_productCode                      2
        #define OD_1018_3_identity_revisionNumber                   3
        #define OD_1018_4_identity_serialNumber                     4

/*1019 */
        #define OD_1019_synchronousCounterOverflowValue             0x1019

/*1200 */
        #define OD_1200_SDOServerParameter                          0x1200

        #define OD_1200_0_SDOServerParameter_maxSubIndex            0
        #define OD_1200_1_SDOServerParameter_COB_IDClientToServer   1
        #define OD_1200_2_SDOServerParameter_COB_IDServerToClient   2

/*1280 */
        #define OD_1280_SDOClientParameter                          0x1280

        #define OD_1280_0_SDOClientParameter_maxSubIndex            0
        #define OD_1280_1_SDOClientParameter_COB_IDClientToServer   1
        #define OD_1280_2_SDOClientParameter_COB_IDServerToClient   2
        #define OD_1280_3_SDOClientParameter_nodeIDOfTheSDOServer   3

/*1400 */
        #define OD_1400_RPDOCommunicationParameter                  0x1400

        #define OD_1400_0_RPDOCommunicationParameter_maxSubIndex    0
        #define OD_1400_1_RPDOCommunicationParameter_COB_IDUsedByRPDO 1
        #define OD_1400_2_RPDOCommunicationParameter_transmissionType 2
        #define OD_1400_5_RPDOCommunicationParameter_eventTimer     5

/*1401 */
        #define OD_1401_RPDOCommunicationParameter                  0x1401

        #define OD_1401_0_RPDOCommunicationParameter_maxSubIndex    0
        #define OD_1401_1_RPDOCommunicationParameter_COB_IDUsedByRPDO 1
        #define OD_1401_2_RPDOCommunicationParameter_transmissionType 2
        #define OD_1401_5_RPDOCommunicationParameter_eventTimer     5

/*1402 */
        #define OD_1402_RPDOCommunicationParameter                  0x1402

        #define OD_1402_0_RPDOCommunicationParameter_maxSubIndex    0
        #define OD_1402_1_RPDOCommunicationParameter_COB_IDUsedByRPDO 1
        #define OD_1402_2_RPDOCommunicationParameter_transmissionType 2
        #define OD_1402_5_RPDOCommunicationParameter_eventTimer     5

/*1403 */
        #define OD_1403_RPDOCommunicationParameter                  0x1403

        #define OD_1403_0_RPDOCommunicationParameter_maxSubIndex    0
        #define OD_1403_1_RPDOCommunicationParameter_COB_IDUsedByRPDO 1
        #define OD_1403_2_RPDOCommunicationParameter_transmissionType 2
        #define OD_1403_5_RPDOCommunicationParameter_eventTimer     5

/*1600 */
        #define OD_1600_RPDOMappingParameter                        0x1600

        #define OD_1600_0_RPDOMappingParameter_maxSubIndex          0
        #define OD_1600_1_RPDOMappingParameter_applicationObject1   1
        #define OD_1600_2_RPDOMappingParameter_applicationObject2   2
        #define OD_1600_3_RPDOMappingParameter_applicationObject3   3
        #define OD_1600_4_RPDOMappingParameter_applicationObject4   4
        #define OD_1600_5_RPDOMappingParameter_applicationObject5   5
        #define OD_1600_6_RPDOMappingParameter_applicationObject6   6
        #define OD_1600_7_RPDOMappingParameter_applicationObject7   7
        #define OD_1600_8_RPDOMappingParameter_applicationObject8   8

/*1601 */
        #define OD_1601_RPDOMappingParameter                        0x1601

        #define OD_1601_0_RPDOMappingParameter_maxSubIndex          0
        #define OD_1601_1_RPDOMappingParameter_applicationObject1   1
        #define OD_1601_2_RPDOMappingParameter_applicationObject2   2
        #define OD_1601_3_RPDOMappingParameter_applicationObject3   3
        #define OD_1601_4_RPDOMappingParameter_applicationObject4   4
        #define OD_1601_5_RPDOMappingParameter_applicationObject5   5
        #define OD_1601_6_RPDOMappingParameter_applicationObject6   6
        #define OD_1601_7_RPDOMappingParameter_applicationObject7   7
        #define OD_1601_8_RPDOMappingParameter_applicationObject8   8

/*1602 */
        #define OD_1602_RPDOMappingParameter                        0x1602

        #define OD_1602_0_RPDOMappingParameter_maxSubIndex          0
        #define OD_1602_1_RPDOMappingParameter_applicationObject1   1
        #define OD_1602_2_RPDOMappingParameter_applicationObject2   2
        #define OD_1602_3_RPDOMappingParameter_applicationObject3   3
        #define OD_1602_4_RPDOMappingParameter_applicationObject4   4
        #define OD_1602_5_RPDOMappingParameter_applicationObject5   5
        #define OD_1602_6_RPDOMappingParameter_applicationObject6   6
        #define OD_1602_7_RPDOMappingParameter_applicationObject7   7
        #define OD_1602_8_RPDOMappingParameter_applicationObject8   8

/*1603 */
        #define OD_1603_RPDOMappingParameter                        0x1603

        #define OD_1603_0_RPDOMappingParameter_maxSubIndex          0
        #define OD_1603_1_RPDOMappingParameter_applicationObject1   1
        #define OD_1603_2_RPDOMappingParameter_applicationObject2   2
        #define OD_1603_3_RPDOMappingParameter_applicationObject3   3
        #define OD_1603_4_RPDOMappingParameter_applicationObject4   4
        #define OD_1603_5_RPDOMappingParameter_applicationObject5   5
        #define OD_1603_6_RPDOMappingParameter_applicationObject6   6
        #define OD_1603_7_RPDOMappingParameter_applicationObject7   7
        #define OD_1603_8_RPDOMappingParameter_applicationObject8   8

/*1800 */
        #define OD_1800_TPDOCommunicationParameter                  0x1800

        #define OD_1800_0_TPDOCommunicationParameter_maxSubIndex    0
        #define OD_1800_1_TPDOCommunicationParameter_COB_IDUsedByTPDO 1
        #define OD_1800_2_TPDOCommunicationParameter_transmissionType 2
        #define OD_1800_3_TPDOCommunicationParameter_inhibitTime    3
        #define OD_1800_4_TPDOCommunicationParameter_compatibilityEntry 4
        #define OD_1800_5_TPDOCommunicationParameter_eventTimer     5
        #define OD_1800_6_TPDOCommunicationParameter_SYNCStartValue 6

/*1801 */
        #define OD_1801_TPDOCommunicationParameter                  0x1801

        #define OD_1801_0_TPDOCommunicationParameter_maxSubIndex    0
        #define OD_1801_1_TPDOCommunicationParameter_COB_IDUsedByTPDO 1
        #define OD_1801_2_TPDOCommunicationParameter_transmissionType 2
        #define OD_1801_3_TPDOCommunicationParameter_inhibitTime    3
        #define OD_1801_4_TPDOCommunicationParameter_compatibilityEntry 4
        #define OD_1801_5_TPDOCommunicationParameter_eventTimer     5
        #define OD_1801_6_TPDOCommunicationParameter_SYNCStartValue 6

/*1802 */
        #define OD_1802_TPDOCommunicationParameter                  0x1802

        #define OD_1802_0_TPDOCommunicationParameter_maxSubIndex    0
        #define OD_1802_1_TPDOCommunicationParameter_COB_IDUsedByTPDO 1
        #define OD_1802_2_TPDOCommunicationParameter_transmissionType 2
        #define OD_1802_3_TPDOCommunicationParameter_inhibitTime    3
        #define OD_1802_4_TPDOCommunicationParameter_compatibilityEntry 4
        #define OD_1802_5_TPDOCommunicationParameter_eventTimer     5
        #define OD_1802_6_TPDOCommunicationParameter_SYNCStartValue 6

/*1803 */
        #define OD_1803_TPDOCommunicationParameter                  0x1803

        #define OD_1803_0_TPDOCommunicationParameter_maxSubIndex    0
        #define OD_1803_1_TPDOCommunicationParameter_COB_IDUsedByTPDO 1
        #define OD_1803_2_TPDOCommunicationParameter_transmissionType 2
        #define OD_1803_3_TPDOCommunicationParameter_inhibitTime    3
        #define OD_1803_4_TPDOCommunicationParameter_compatibilityEntry 4
        #define OD_1803_5_TPDOCommunicationParameter_eventTimer     5
        #define OD_1803_6_TPDOCommunicationParameter_SYNCStartValue 6

/*1A00 */
        #define OD_1A00_TPDOMappingParameter                        0x1A00

        #define OD_1A00_0_TPDOMappingParameter_maxSubIndex          0
        #define OD_1A00_1_TPDOMappingParameter_applicationObject1   1
        #define OD_1A00_2_TPDOMappingParameter_applicationObject2   2
        #define OD_1A00_3_TPDOMappingParameter_applicationObject3   3
        #define OD_1A00_4_TPDOMappingParameter_applicationObject4   4
        #define OD_1A00_5_TPDOMappingParameter_applicationObject5   5
        #define OD_1A00_6_TPDOMappingParameter_applicationObject6   6
        #define OD_1A00_7_TPDOMappingParameter_applicationObject7   7
        #define OD_1A00_8_TPDOMappingParameter_applicationObject8   8

/*1A01 */
        #define OD_1A01_TPDOMappingParameter                        0x1A01

        #define OD_1A01_0_TPDOMappingParameter_maxSubIndex          0
        #define OD_1A01_1_TPDOMappingParameter_applicationObject1   1
        #define OD_1A01_2_TPDOMappingParameter_applicationObject2   2
        #define OD_1A01_3_TPDOMappingParameter_applicationObject3   3
        #define OD_1A01_4_TPDOMappingParameter_applicationObject4   4
        #define OD_1A01_5_TPDOMappingParameter_applicationObject5   5
        #define OD_1A01_6_TPDOMappingParameter_applicationObject6   6
        #define OD_1A01_7_TPDOMappingParameter_applicationObject7   7
        #define OD_1A01_8_TPDOMappingParameter_applicationObject8   8

/*1A02 */
        #define OD_1A02_TPDOMappingParameter                        0x1A02

        #define OD_1A02_0_TPDOMappingParameter_maxSubIndex          0
        #define OD_1A02_1_TPDOMappingParameter_applicationObject1   1
        #define OD_1A02_2_TPDOMappingParameter_applicationObject2   2
        #define OD_1A02_3_TPDOMappingParameter_applicationObject3   3
        #define OD_1A02_4_TPDOMappingParameter_applicationObject4   4
        #define OD_1A02_5_TPDOMappingParameter_applicationObject5   5
        #define OD_1A02_6_TPDOMappingParameter_applicationObject6   6
        #define OD_1A02_7_TPDOMappingParameter_applicationObject7   7
        #define OD_1A02_8_TPDOMappingParameter_applicationObject8   8

/*1A03 */
        #define OD_1A03_TPDOMappingParameter                        0x1A03

        #define OD_1A03_0_TPDOMappingParameter_maxSubIndex          0
        #define OD_1A03_1_TPDOMappingParameter_applicationObject1   1
        #define OD_1A03_2_TPDOMappingParameter_applicationObject2   2
        #define OD_1A03_3_TPDOMappingParameter_applicationObject3   3
        #define OD_1A03_4_TPDOMappingParameter_applicationObject4   4
        #define OD_1A03_5_TPDOMappingParameter_applicationObject5   5
        #define OD_1A03_6_TPDOMappingParameter_applicationObject6   6
        #define OD_1A03_7_TPDOMappingParameter_applicationObject7   7
        #define OD_1A03_8_TPDOMappingParameter_applicationObject8   8

/*******************************************************************************
   STRUCTURES FOR VARIABLES IN DIFFERENT MEMORY LOCATIONS
*******************************************************************************/
#define  CO_OD_FIRST_LAST_WORD     0x55 //Any value from 0x01 to 0xFE. If changed, EEPROM will be reinitialized.

/***** Structure for RAM variables ********************************************/
struct sCO_OD_RAM{
               UNSIGNED32     FirstWord;

/*1001      */ UNSIGNED8      errorRegister;
/*1003      */ UNSIGNED32      preDefinedErrorField[16];
/*1010      */ UNSIGNED32      storeParameters[4];
/*1011      */ UNSIGNED32      restoreDefaultParameters[4];
/*1200      */ OD_SDOServerParameter_t SDOServerParameter[1];

               UNSIGNED32     LastWord;
};

/***** Structure for PERSIST_COMM variables ********************************************/
struct sCO_OD_PERSIST_COMM{
               UNSIGNED32     FirstWord;

/*1000      */ UNSIGNED32     deviceType;
/*1005      */ UNSIGNED32     COB_ID_SYNCMessage;
/*1006      */ UNSIGNED32     communicationCyclePeriod;
/*1007      */ UNSIGNED32     synchronousWindowLength;
/*1012      */ UNSIGNED32     COB_IDTimeStampObject;
/*1014      */ UNSIGNED32     COB_ID_EMCY;
/*1015      */ UNSIGNED16     inhibitTimeEMCY;
/*1016      */ UNSIGNED32      consumerHeartbeatTime[8];
/*1017      */ UNSIGNED16     producerHeartbeatTime;
/*1018      */ OD_identity_t   identity;
/*1019      */ UNSIGNED8      synchronousCounterOverflowValue;
/*1280      */ OD_SDOClientParameter_t SDOClientParameter[1];
/*1400      */ OD_RPDOCommunicationParameter_t RPDOCommunicationParameter[4];
/*1600      */ OD_RPDOMappingParameter_t RPDOMappingParameter[4];
/*1800      */ OD_TPDOCommunicationParameter_t TPDOCommunicationParameter[4];
/*1A00      */ OD_TPDOMappingParameter_t TPDOMappingParameter[4];

               UNSIGNED32     LastWord;
};

/***** Structure for ROM variables ********************************************/
struct sCO_OD_ROM{
               UNSIGNED32     FirstWord;


               UNSIGNED32     LastWord;
};

/***** Structure for EEPROM variables ********************************************/
struct sCO_OD_EEPROM{
               UNSIGNED32     FirstWord;


               UNSIGNED32     LastWord;
};

/***** Declaration of Object Dictionary variables *****************************/
extern struct sCO_OD_RAM CO_OD_RAM;

extern struct sCO_OD_PERSIST_COMM CO_OD_PERSIST_COMM;

extern struct sCO_OD_ROM CO_OD_ROM;

extern struct sCO_OD_EEPROM CO_OD_EEPROM;

/*******************************************************************************
   ALIASES FOR OBJECT DICTIONARY VARIABLES
*******************************************************************************/
/*1000, Data Type: UNSIGNED32 */
        #define OD_deviceType                                       CO_OD_PERSIST_COMM.deviceType

/*1001, Data Type: UNSIGNED8 */
        #define OD_errorRegister                                    CO_OD_RAM.errorRegister

/*1003, Data Type: UNSIGNED32, Array[16] */
        #define OD_preDefinedErrorField                             CO_OD_RAM.preDefinedErrorField
        #define ODL_preDefinedErrorField_arrayLength                16
        #define ODA_preDefinedErrorField_standardErrorField         0

/*1005, Data Type: UNSIGNED32 */
        #define OD_COB_ID_SYNCMessage                               CO_OD_PERSIST_COMM.COB_ID_SYNCMessage

/*1006, Data Type: UNSIGNED32 */
        #define OD_communicationCyclePeriod                         CO_OD_PERSIST_COMM.communicationCyclePeriod

/*1007, Data Type: UNSIGNED32 */
        #define OD_synchronousWindowLength                          CO_OD_PERSIST_COMM.synchronousWindowLength

/*1010, Data Type: UNSIGNED32, Array[4] */
        #define OD_storeParameters                                  CO_OD_RAM.storeParameters
        #define ODL_storeParameters_arrayLength                     4
        #define ODA_storeParameters_saveAllParameters               0
        #define ODA_storeParameters_saveCommunicationParameters     1
        #define ODA_storeParameters_saveApplicationParameters       2
        #define ODA_storeParameters_saveManufacturerDefinedParameters 3

/*1011, Data Type: UNSIGNED32, Array[4] */
        #define OD_restoreDefaultParameters                         CO_OD_RAM.restoreDefaultParameters
        #define ODL_restoreDefaultParameters_arrayLength            4
        #define ODA_restoreDefaultParameters_restoreAllDefaultParameters 0
        #define ODA_restoreDefaultParameters_restoreCommunicationDefaultParameters 1
        #define ODA_restoreDefaultParameters_restoreApplicationDefaultParameters 2
        #define ODA_restoreDefaultParameters_restoreManufacturerDefinedDefaultParameters 3

/*1012, Data Type: UNSIGNED32 */
        #define OD_COB_IDTimeStampObject                            CO_OD_PERSIST_COMM.COB_IDTimeStampObject

/*1014, Data Type: UNSIGNED32 */
        #define OD_COB_ID_EMCY                                      CO_OD_PERSIST_COMM.COB_ID_EMCY

/*1015, Data Type: UNSIGNED16 */
        #define OD_inhibitTimeEMCY                                  CO_OD_PERSIST_COMM.inhibitTimeEMCY

/*1016, Data Type: UNSIGNED32, Array[8] */
        #define OD_consumerHeartbeatTime                            CO_OD_PERSIST_COMM.consumerHeartbeatTime
        #define ODL_consumerHeartbeatTime_arrayLength               8
        #define ODA_consumerHeartbeatTime_consumerHeartbeatTime     0

/*1017, Data Type: UNSIGNED16 */
        #define OD_producerHeartbeatTime                            CO_OD_PERSIST_COMM.producerHeartbeatTime

/*1018, Data Type: identity_t */
        #define OD_identity                                         CO_OD_PERSIST_COMM.identity

/*1019, Data Type: UNSIGNED8 */
        #define OD_synchronousCounterOverflowValue                  CO_OD_PERSIST_COMM.synchronousCounterOverflowValue

/*1200, Data Type: SDOServerParameter_t */
        #define OD_SDOServerParameter                               CO_OD_RAM.SDOServerParameter

/*1280, Data Type: SDOClientParameter_t */
        #define OD_SDOClientParameter                               CO_OD_PERSIST_COMM.SDOClientParameter

/*1400, Data Type: RPDOCommunicationParameter_t */
        #define OD_RPDOCommunicationParameter                       CO_OD_PERSIST_COMM.RPDOCommunicationParameter

/*1600, Data Type: RPDOMappingParameter_t */
        #define OD_RPDOMappingParameter                             CO_OD_PERSIST_COMM.RPDOMappingParameter

/*1800, Data Type: TPDOCommunicationParameter_t */
        #define OD_TPDOCommunicationParameter                       CO_OD_PERSIST_COMM.TPDOCommunicationParameter

/*1A00, Data Type: TPDOMappingParameter_t */
        #define OD_TPDOMappingParameter                             CO_OD_PERSIST_COMM.TPDOMappingParameter

#endif
// clang-format on
