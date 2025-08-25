// clang-format off
/*******************************************************************************
    CANopen Object Dictionary definition for CANopenNode v1 to v2

    This file was automatically generated with
    libedssharp Object Dictionary Editor v0.8-125-g85dfd8e

    https://github.com/CANopenNode/CANopenNode
    https://github.com/robincornelius/libedssharp

    DON'T EDIT THIS FILE MANUALLY !!!!
*******************************************************************************/
// For CANopenNode V2 users, C macro `CO_VERSION_MAJOR=2` has to be added to project options
#ifndef CO_VERSION_MAJOR
 #include "CO_driver.h"
 #include "OD.h"
 #include "CO_SDO.h"
#elif CO_VERSION_MAJOR < 4
 #include "301/CO_driver.h"
 #include "OD.h"
 #include "301/CO_SDOserver.h"
#else
 #error This Object dictionary is not compatible with CANopenNode v4.0 and up!
#endif

/*******************************************************************************
   DEFINITION AND INITIALIZATION OF OBJECT DICTIONARY VARIABLES
*******************************************************************************/


/***** Definition for RAM variables *******************************************/
struct sCO_OD_RAM CO_OD_RAM = {
           CO_OD_FIRST_LAST_WORD,

/*1001*/ 0x0L,
/*1003*/ {0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1010*/ {0x0001L, 0x0001L, 0x0001L, 0x0001L},
/*1011*/ {0x0001L, 0x0001L, 0x0001L, 0x0001L},
/*1200*/ {{0x2L, 0x0600L, 0x0580L}},

           CO_OD_FIRST_LAST_WORD,
};


/***** Definition for PERSIST_COMM variables *******************************************/
struct sCO_OD_PERSIST_COMM CO_OD_PERSIST_COMM = {
           CO_OD_FIRST_LAST_WORD,

/*1000*/ 0x0000L,
/*1005*/ 0x0080L,
/*1006*/ 0x0000L,
/*1007*/ 0x0000L,
/*1012*/ 0x0100L,
/*1014*/ 0x0080L,
/*1015*/ 0x00,
/*1016*/ {0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1017*/ 0x00,
/*1018*/ {0x4L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1019*/ 0x0L,
/*1280*/ {{0x3L, 0x80000000L, 0x80000000L, 0x1L}},
/*1400*/ {{0x5L, 0x80000200L, 0xFEL, 0x00},
/*1401*/ {0x5L, 0x80000300L, 0xFEL, 0x00},
/*1402*/ {0x5L, 0x80000400L, 0xFEL, 0x00},
/*1403*/ {0x5L, 0x80000500L, 0xFEL, 0x00}},
/*1600*/ {{0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1601*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1602*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1603*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L}},
/*1800*/ {{0x6L, 0xC0000180L, 0xFEL, 0x00, 0x0L, 0x00, 0x0L},
/*1801*/ {0x6L, 0xC0000280L, 0xFEL, 0x00, 0x0L, 0x00, 0x0L},
/*1802*/ {0x6L, 0xC0000380L, 0xFEL, 0x00, 0x0L, 0x00, 0x0L},
/*1803*/ {0x6L, 0xC0000480L, 0xFEL, 0x00, 0x0L, 0x00, 0x0L}},
/*1A00*/ {{0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1A01*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1A02*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L},
/*1A03*/ {0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L}},

           CO_OD_FIRST_LAST_WORD,
};


/***** Definition for ROM variables *******************************************/
struct sCO_OD_ROM CO_OD_ROM = {
           CO_OD_FIRST_LAST_WORD,


           CO_OD_FIRST_LAST_WORD,
};


/***** Definition for EEPROM variables *******************************************/
struct sCO_OD_EEPROM CO_OD_EEPROM = {
           CO_OD_FIRST_LAST_WORD,


           CO_OD_FIRST_LAST_WORD,
};




/*******************************************************************************
   STRUCTURES FOR RECORD TYPE OBJECTS
*******************************************************************************/


/*0x1018*/ const CO_OD_entryRecord_t OD_record1018[5] = {
           {(void*)&CO_OD_PERSIST_COMM.identity.highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.identity.vendorID, 0x87, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.identity.productCode, 0x87, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.identity.revisionNumber, 0x87, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.identity.serialNumber, 0x87, 0x4 },
};

/*0x1200*/ const CO_OD_entryRecord_t OD_record1200[3] = {
           {(void*)&CO_OD_RAM.SDOServerParameter[0].highestSubIndexSupported, 0x06, 0x1 },
           {(void*)&CO_OD_RAM.SDOServerParameter[0].COB_IDClientToServer, 0xB6, 0x4 },
           {(void*)&CO_OD_RAM.SDOServerParameter[0].COB_IDServerToClient, 0xB6, 0x4 },
};

/*0x1280*/ const CO_OD_entryRecord_t OD_record1280[4] = {
           {(void*)&CO_OD_PERSIST_COMM.SDOClientParameter[0].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.SDOClientParameter[0].COB_IDClientToServer, 0xBF, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.SDOClientParameter[0].COB_IDServerToClient, 0xBF, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.SDOClientParameter[0].nodeIDOfTheSDOServer, 0x0F, 0x1 },
};

/*0x1400*/ const CO_OD_entryRecord_t OD_record1400[4] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[0].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[0].COB_IDUsedByRPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[0].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[0].eventTimer, 0x8F, 0x2 },
};

/*0x1401*/ const CO_OD_entryRecord_t OD_record1401[4] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[1].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[1].COB_IDUsedByRPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[1].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[1].eventTimer, 0x8F, 0x2 },
};

/*0x1402*/ const CO_OD_entryRecord_t OD_record1402[4] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[2].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[2].COB_IDUsedByRPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[2].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[2].eventTimer, 0x8F, 0x2 },
};

/*0x1403*/ const CO_OD_entryRecord_t OD_record1403[4] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[3].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[3].COB_IDUsedByRPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[3].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOCommunicationParameter[3].eventTimer, 0x8F, 0x2 },
};

/*0x1600*/ const CO_OD_entryRecord_t OD_record1600[9] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[0].applicationObject8, 0x8F, 0x4 },
};

/*0x1601*/ const CO_OD_entryRecord_t OD_record1601[9] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[1].applicationObject8, 0x8F, 0x4 },
};

/*0x1602*/ const CO_OD_entryRecord_t OD_record1602[9] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[2].applicationObject8, 0x8F, 0x4 },
};

/*0x1603*/ const CO_OD_entryRecord_t OD_record1603[9] = {
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.RPDOMappingParameter[3].applicationObject8, 0x8F, 0x4 },
};

/*0x1800*/ const CO_OD_entryRecord_t OD_record1800[7] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].COB_IDUsedByTPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].inhibitTime, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].compatibilityEntry, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].eventTimer, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[0].SYNCStartValue, 0x0F, 0x1 },
};

/*0x1801*/ const CO_OD_entryRecord_t OD_record1801[7] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].COB_IDUsedByTPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].inhibitTime, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].compatibilityEntry, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].eventTimer, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[1].SYNCStartValue, 0x0F, 0x1 },
};

/*0x1802*/ const CO_OD_entryRecord_t OD_record1802[7] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].COB_IDUsedByTPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].inhibitTime, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].compatibilityEntry, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].eventTimer, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[2].SYNCStartValue, 0x0F, 0x1 },
};

/*0x1803*/ const CO_OD_entryRecord_t OD_record1803[7] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].highestSubIndexSupported, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].COB_IDUsedByTPDO, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].transmissionType, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].inhibitTime, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].compatibilityEntry, 0x07, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].eventTimer, 0x8F, 0x2 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOCommunicationParameter[3].SYNCStartValue, 0x0F, 0x1 },
};

/*0x1A00*/ const CO_OD_entryRecord_t OD_record1A00[9] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[0].applicationObject8, 0x8F, 0x4 },
};

/*0x1A01*/ const CO_OD_entryRecord_t OD_record1A01[9] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[1].applicationObject8, 0x8F, 0x4 },
};

/*0x1A02*/ const CO_OD_entryRecord_t OD_record1A02[9] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[2].applicationObject8, 0x8F, 0x4 },
};

/*0x1A03*/ const CO_OD_entryRecord_t OD_record1A03[9] = {
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].numberOfMappedApplicationObjectsInPDO, 0x0F, 0x1 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject1, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject2, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject3, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject4, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject5, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject6, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject7, 0x8F, 0x4 },
           {(void*)&CO_OD_PERSIST_COMM.TPDOMappingParameter[3].applicationObject8, 0x8F, 0x4 },
};

/*******************************************************************************
   OBJECT DICTIONARY
*******************************************************************************/
const CO_OD_entry_t CO_OD[CO_OD_NoOfElements] = {
{0x1000, 0x00, 0x87,  4, (void*)&CO_OD_PERSIST_COMM.deviceType},
{0x1001, 0x00, 0x36,  1, (void*)&CO_OD_RAM.errorRegister},
{0x1003, 0x10, 0x8A,  4, (void*)&CO_OD_RAM.preDefinedErrorField[0]},
{0x1005, 0x00, 0x8F,  4, (void*)&CO_OD_PERSIST_COMM.COB_ID_SYNCMessage},
{0x1006, 0x00, 0x8F,  4, (void*)&CO_OD_PERSIST_COMM.communicationCyclePeriod},
{0x1007, 0x00, 0x8F,  4, (void*)&CO_OD_PERSIST_COMM.synchronousWindowLength},
{0x1010, 0x04, 0x82,  4, (void*)&CO_OD_RAM.storeParameters[0]},
{0x1011, 0x04, 0x82,  4, (void*)&CO_OD_RAM.restoreDefaultParameters[0]},
{0x1012, 0x00, 0x8F,  4, (void*)&CO_OD_PERSIST_COMM.COB_IDTimeStampObject},
{0x1014, 0x00, 0x8F,  4, (void*)&CO_OD_PERSIST_COMM.COB_ID_EMCY},
{0x1015, 0x00, 0x8F,  2, (void*)&CO_OD_PERSIST_COMM.inhibitTimeEMCY},
{0x1016, 0x08, 0x83,  4, (void*)&CO_OD_PERSIST_COMM.consumerHeartbeatTime[0]},
{0x1017, 0x00, 0x8F,  2, (void*)&CO_OD_PERSIST_COMM.producerHeartbeatTime},
{0x1018, 0x04, 0x00,  0, (void*)&OD_record1018},
{0x1019, 0x00, 0x0F,  1, (void*)&CO_OD_PERSIST_COMM.synchronousCounterOverflowValue},
{0x1200, 0x02, 0x00,  0, (void*)&OD_record1200},
{0x1280, 0x03, 0x00,  0, (void*)&OD_record1280},
{0x1400, 0x03, 0x00,  0, (void*)&OD_record1400},
{0x1401, 0x03, 0x00,  0, (void*)&OD_record1401},
{0x1402, 0x03, 0x00,  0, (void*)&OD_record1402},
{0x1403, 0x03, 0x00,  0, (void*)&OD_record1403},
{0x1600, 0x08, 0x00,  0, (void*)&OD_record1600},
{0x1601, 0x08, 0x00,  0, (void*)&OD_record1601},
{0x1602, 0x08, 0x00,  0, (void*)&OD_record1602},
{0x1603, 0x08, 0x00,  0, (void*)&OD_record1603},
{0x1800, 0x06, 0x00,  0, (void*)&OD_record1800},
{0x1801, 0x06, 0x00,  0, (void*)&OD_record1801},
{0x1802, 0x06, 0x00,  0, (void*)&OD_record1802},
{0x1803, 0x06, 0x00,  0, (void*)&OD_record1803},
{0x1A00, 0x08, 0x00,  0, (void*)&OD_record1A00},
{0x1A01, 0x08, 0x00,  0, (void*)&OD_record1A01},
{0x1A02, 0x08, 0x00,  0, (void*)&OD_record1A02},
{0x1A03, 0x08, 0x00,  0, (void*)&OD_record1A03},
};
// clang-format on
