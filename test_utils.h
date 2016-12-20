#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#if defined __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <arpa/inet.h>

#include "Serial.h"
#include "SerialLink.h"

#define DEBUG_PRINTF printf

#define MAXDEVNUM 300

// ------------------------------------------------------------------
// Endpoints
// ------------------------------------------------------------------

#define ZB_ENDPOINT_ZHA             1   // Zigbee-HA
#define ZB_ENDPOINT_ONOFF           1   // ON/OFF cluster
#define ZB_ENDPOINT_GROUP           1   // GROUP cluster
#define ZB_ENDPOINT_SCENE           1   // SCENE cluster
#define ZB_ENDPOINT_SIMPLE          1   // SimpleDescriptor cluster
#define ZB_ENDPOINT_LAMP            1   // ON/OFF, Color control
#define ZB_ENDPOINT_TUNNEL          1   // For tunnel mesages
#define ZB_ENDPOINT_ATTR            1   // For attrs

/** Enumerated type of module modes */
typedef enum
{
    E_MODE_COORDINATOR      = 0,        /**< Start module as a coordinator */
    E_MODE_ROUTER           = 1,        /**< Start module as a router */
    E_MODE_HA_COMPATABILITY = 2,        /**< Start module as router in HA compatability mode */
} teModuleMode;

/** Enumerated type of ZigBee address modes */
typedef enum
{
    E_ZB_ADDRESS_MODE_BOUND                 = 0x00,
    E_ZB_ADDRESS_MODE_GROUP                 = 0x01,
    E_ZB_ADDRESS_MODE_SHORT                 = 0x02,
    E_ZB_ADDRESS_MODE_IEEE                  = 0x03,
    E_ZB_ADDRESS_MODE_BROADCAST             = 0x04,
    E_ZB_ADDRESS_MODE_NO_TRANSMIT           = 0x05,
    E_ZB_ADDRESS_MODE_BOUND_NO_ACK          = 0x06,
    E_ZB_ADDRESS_MODE_SHORT_NO_ACK          = 0x07,
    E_ZB_ADDRESS_MODE_IEEE_NO_ACK           = 0x08,
} eZigbee_AddressMode;

typedef enum
{
    /* Zigbee ZCL status codes */
    E_ZCB_OK                            = 0x00,
    E_ZCB_ERROR                         = 0x01,
    
    /* ZCB internal status codes */
    E_ZCB_ERROR_NO_MEM                  = 0x10,
    E_ZCB_COMMS_FAILED                  = 0x11,
    E_ZCB_UNKNOWN_NODE                  = 0x12,
    E_ZCB_UNKNOWN_ENDPOINT              = 0x13,
    E_ZCB_UNKNOWN_CLUSTER               = 0x14,
    E_ZCB_REQUEST_NOT_ACTIONED          = 0x15,
    
    /* Zigbee ZCL status codes */
    E_ZCB_NOT_AUTHORISED                = 0x7E, 
    E_ZCB_RESERVED_FIELD_NZERO          = 0x7F,
    E_ZCB_MALFORMED_COMMAND             = 0x80,
    E_ZCB_UNSUP_CLUSTER_COMMAND         = 0x81,
    E_ZCB_UNSUP_GENERAL_COMMAND         = 0x82,
    E_ZCB_UNSUP_MANUF_CLUSTER_COMMAND   = 0x83,
    E_ZCB_UNSUP_MANUF_GENERAL_COMMAND   = 0x84,
    E_ZCB_INVALID_FIELD                 = 0x85,
    E_ZCB_UNSUP_ATTRIBUTE               = 0x86,
    E_ZCB_INVALID_VALUE                 = 0x87,
    E_ZCB_READ_ONLY                     = 0x88,
    E_ZCB_INSUFFICIENT_SPACE            = 0x89,
    E_ZCB_DUPLICATE_EXISTS              = 0x8A,
    E_ZCB_NOT_FOUND                     = 0x8B,
    E_ZCB_UNREPORTABLE_ATTRIBUTE        = 0x8C,
    E_ZCB_INVALID_DATA_TYPE             = 0x8D,
    E_ZCB_INVALID_SELECTOR              = 0x8E,
    E_ZCB_WRITE_ONLY                    = 0x8F,
    E_ZCB_INCONSISTENT_STARTUP_STATE    = 0x90,
    E_ZCB_DEFINED_OUT_OF_BAND           = 0x91,
    E_ZCB_INCONSISTENT                  = 0x92,
    E_ZCB_ACTION_DENIED                 = 0x93,
    E_ZCB_TIMEOUT                       = 0x94,
    
    E_ZCB_HARDWARE_FAILURE              = 0xC0,
    E_ZCB_SOFTWARE_FAILURE              = 0xC1,
    E_ZCB_CALIBRATION_ERROR             = 0xC2,
} teZcbStatus;

typedef enum
{
  E_ZD_ADDRESS_MODE_BOUND = 0x00,
  E_ZD_ADDRESS_MODE_GROUP = 0x01,
  E_ZD_ADDRESS_MODE_SHORT = 0x02,
  E_ZD_ADDRESS_MODE_IEEE = 0x03,
  E_ZD_ADDRESS_MODE_BROADCAST = 0x04,
  E_ZD_ADDRESS_MODE_NO_TRANSMIT = 0x05,
  E_ZD_ADDRESS_MODE_BOUND_NO_ACK = 0x06,
  E_ZD_ADDRESS_MODE_SHORT_NO_ACK = 0x07,
  E_ZD_ADDRESS_MODE_IEEE_NO_ACK = 0x08,
} teZDAddressMode;

/** Enumerated type of Zigbee Cluster IDs */
typedef enum
{
    E_ZB_CLUSTERID_BASIC                    = 0x0000,
    E_ZB_CLUSTERID_POWER                    = 0x0001,
    E_ZB_CLUSTERID_DEVICE_TEMPERATURE       = 0x0002,
    E_ZB_CLUSTERID_IDENTIFY                 = 0x0003,
    E_ZB_CLUSTERID_GROUPS                   = 0x0004,
    E_ZB_CLUSTERID_SCENES                   = 0x0005,
    E_ZB_CLUSTERID_ONOFF                    = 0x0006,
    E_ZB_CLUSTERID_ONOFF_CONFIGURATION      = 0x0007,
    E_ZB_CLUSTERID_LEVEL_CONTROL            = 0x0008,
    E_ZB_CLUSTERID_ALARMS                   = 0x0009,
    E_ZB_CLUSTERID_TIME                     = 0x000A,
    E_ZB_CLUSTERID_RSSI_LOCATION            = 0x000B,
    E_ZB_CLUSTERID_ANALOG_INPUT_BASIC       = 0x000C,
    E_ZB_CLUSTERID_ANALOG_OUTPUT_BASIC      = 0x000D,
    E_ZB_CLUSTERID_VALUE_BASIC              = 0x000E,
    E_ZB_CLUSTERID_BINARY_INPUT_BASIC       = 0x000F,
    E_ZB_CLUSTERID_BINARY_OUTPUT_BASIC      = 0x0010,
    E_ZB_CLUSTERID_BINARY_VALUE_BASIC       = 0x0011,
    E_ZB_CLUSTERID_MULTISTATE_INPUT_BASIC   = 0x0012,
    E_ZB_CLUSTERID_MULTISTATE_OUTPUT_BASIC  = 0x0013,
    E_ZB_CLUSTERID_MULTISTATE_VALUE_BASIC   = 0x0014,
    E_ZB_CLUSTERID_COMMISSIONING            = 0x0015,
    
    /* HVAC */
    E_ZB_CLUSTERID_THERMOSTAT               = 0x0201,
    
    /* Lighting */
    E_ZB_CLUSTERID_COLOR_CONTROL            = 0x0300,
    E_ZB_CLUSTERID_BALLAST_CONFIGURATION    = 0x0301,
    
    /* Sensing */
    E_ZB_CLUSTERID_MEASUREMENTSENSING_ILLUM = 0x0400,
    E_ZB_CLUSTERID_MEASUREMENTSENSING_TEMP  = 0x0402,
    E_ZB_CLUSTERID_MEASUREMENTSENSING_HUM   = 0x0405,
    E_ZB_CLUSTERID_OCCUPANCYSENSING         = 0x0406,
    
    /* Metering */
    E_ZB_CLUSTERID_SIMPLE_METERING          = 0x0702,
    
    /* Electrical Measurement */
    E_ZB_CLUSTERID_ELECTRICAL_MEASUREMENT   = 0x0B04,
    
    /* ZLL */
    E_ZB_CLUSTERID_ZLL_COMMISIONING         = 0x1000,
} eZigbee_ClusterID;

typedef enum
{
  // Operation was successful.
  SUCCESS=0x00,
  // Operation was not successful.
  FAILURE=0x01,
  // The sender of the command does not have authorization to carry out this command.
  NOT_AUTHORIZED=0x7e,
  // A reserved field/subfield/bit contains a non-zero value.
  RESERVED_FIELD_NOT_ZERO=0x7f,
  // The command appears to contain the wrong fields, as detected either by
  // the presence of one or more invalid field entries or by there being
  // missing fields. Command not carried out. Implementer has discretion as
  // to whether to return this error or INVALID_FIELD.
  MALFORMED_COMMAND=0x80,
  // The specified cluster command is not supported on the device. Command not carried out.
  UNSUP_CLUSTER_COMMAND=0x81,
  // The specified general ZCL command is not supported  on the device.
  UNSUP_GENERAL_COMMAND=0x82,
  // A manufacturer specific unicast, cluster specific command was
  // received with an unknown manufacturercode, or the manufacturer code
  // was recognized but the command is not supported.
  UNSUP_MANUF_CLUSTER_COMMAND=0x83,
  //  A manufacturer specific unicast, ZCL specific command was received
  // with an unknown manufacturer code, or the manufacturer code was
  // recognized but the command is not supported.
  UNSUP_MANUF_GENERAL_COMMAND=0x84,
  // At least one field of the command contains an incorrect value, according to
  // the specification the device isimplemented to.
  INVALID_FIELD=0x85,
  //  Out of range error, or set to a reserved value. Attribute
  // keeps its old value.
  UNSUPPORTED_ATTRIBUTE=0x86,
  //  The specified attribute does not exist on the device.
  INVALID_VALUE=0x87,
  //  Attempt to write a read only attribute.
  //   INSUFFICIENT_SPACE 0x89 An operation (e.g. an attempt to create an entry in a
  //   table) failed due to an insufficient amount of free space
  //   available.
  READ_ONLY=0x88,
  //  An attempt to create an entry in a table failed due to a
  //   duplicate entry already being present in the table.
  //   NOT_FOUND 0x8b The requested information (e.g. table entry) could not be
  //   found.
  DUPLICATE_EXISTS=0x8a,
  //  Periodic reports cannot be issued for this attribute.
  //   INVALID_DATA_TYPE 0x8d The data type given for an attribute is incorrect.
  //   Command not carried out.
  UNREPORTABLE_ATTRIBUTE=0x8c,
  //  The selector for an attribute is incorrect.
  //   WRITE_ONLY 0x8f A request has been made to read an attribute that the
  //   requestor is not authorized to read. No action taken.
  INVALID_SELECTOR=0x8e,
  //  Setting the requested values would put the device in an
  //   inconsistent state on startup. No action taken.
  INCONSISTENT_STARTUP_STATE=0x90,
  //  An attempt has been made to write an attribute that is
  //   present but is defined using an out-of-band method and
  //   not over the air.
  DEFINED_OUT_OF_BAND=0x91,
  //  The supplied values (e.g. contents of table cells) are
  //  inconsistent.
  INCONSISTENT=0x92,
  //  The credentials presented by the device sending the
  //   command are not sufficient to perform this action.
  ACTION_DENIED=0x93,
  //  The exchange was aborted due to excessive response
  //  time.
  TIMEOUT=0x94,
  //  Failed case when a client or a server decides to abort the
  //  upgrade process.
  ABORT=0x95,
  // Invalid OTA upgrade image (ex. failed signature
  // validation or signer information check or CRC check).
  INVALID_IMAGE=0x96,
  //  Server does not have data block available yet.
  WAIT_FOR_DATA=0x97,
  // No OTA upgrade image available for a particular client.
  NO_IMAGE_AVAILABLE=0x98,
  //  The client still requires more OTA upgrade image files
  //   in order to successfully upgrade.
  REQUIRE_MORE_IMAGE=0x99,
  //  An operation was unsuccessful due to a hardware failure.
  HARDWARE_FAILURE=0xc0,
  //  An operation was unsuccessful due to a software failure.
  SOFTWARE_FAILURE=0xc1,
  //  An error occurred during calibration.
CALIBRATION_ERROR=0xc2,
} teZCL_Status;

/* Attribute types (from ZCL spec - Table 2.15) */
typedef enum 
{
    /* Null */
    E_ZCL_NULL            = 0x00,

    /* General Data */
    E_ZCL_GINT8           = 0x08,                // General 8 bit - not specified if signed or not
    E_ZCL_GINT16,
    E_ZCL_GINT24,
    E_ZCL_GINT32,
    E_ZCL_GINT40,
    E_ZCL_GINT48,
    E_ZCL_GINT56,
    E_ZCL_GINT64,

    /* Logical */
    E_ZCL_BOOL            = 0x10,

    /* Bitmap */
    E_ZCL_BMAP8            = 0x18,                // 8 bit bitmap
    E_ZCL_BMAP16,
    E_ZCL_BMAP24,
    E_ZCL_BMAP32,
    E_ZCL_BMAP40,
    E_ZCL_BMAP48,
    E_ZCL_BMAP56,
    E_ZCL_BMAP64,

    /* Unsigned Integer */
    E_ZCL_UINT8           = 0x20,                // Unsigned 8 bit
    E_ZCL_UINT16,
    E_ZCL_UINT24,
    E_ZCL_UINT32,
    E_ZCL_UINT40,
    E_ZCL_UINT48,
    E_ZCL_UINT56,
    E_ZCL_UINT64,

    /* Signed Integer */
    E_ZCL_INT8            = 0x28,                // Signed 8 bit
    E_ZCL_INT16,
    E_ZCL_INT24,
    E_ZCL_INT32,
    E_ZCL_INT40,
    E_ZCL_INT48,
    E_ZCL_INT56,
    E_ZCL_INT64,

    /* Enumeration */
    E_ZCL_ENUM8            = 0x30,                // 8 Bit enumeration
    E_ZCL_ENUM16,

    /* Floating Point */
    E_ZCL_FLOAT_SEMI    = 0x38,                // Semi precision
    E_ZCL_FLOAT_SINGLE,                        // Single precision
    E_ZCL_FLOAT_DOUBLE,                        // Double precision

    /* String */
    E_ZCL_OSTRING        = 0x41,                // Octet string
    E_ZCL_CSTRING,                            // Character string
    E_ZCL_LOSTRING,                            // Long octet string
    E_ZCL_LCSTRING,                            // Long character string

    /* Ordered Sequence */
    E_ZCL_ARRAY          = 0x48,
    E_ZCL_STRUCT         = 0x4c,

    E_ZCL_SET            = 0x50,
    E_ZCL_BAG            = 0x51,

    /* Time */
    E_ZCL_TOD            = 0xe0,                // Time of day
    E_ZCL_DATE,                                // Date
    E_ZCL_UTCT,                                // UTC Time

    /* Identifier */
    E_ZCL_CLUSTER_ID    = 0xe8,                // Cluster ID
    E_ZCL_ATTRIBUTE_ID,                        // Attribute ID
    E_ZCL_BACNET_OID,                        // BACnet OID

    /* Miscellaneous */
    E_ZCL_IEEE_ADDR        = 0xf0,                // 64 Bit IEEE Address
    E_ZCL_KEY_128,                            // 128 Bit security key, currently not supported as it would add to code space in u16ZCL_WriteTypeNBO and add extra 8 bytes to report config record for each reportable attribute

    /* NOTE:
     * 0xfe is a reserved value, however we are using it to denote a message signature.
     * This may have to change some time if ZigBee ever allocate this value to a data type
     */
    E_ZCL_SIGNATURE     = 0xfe,             // ECDSA Signature (42 bytes)

    /* Unknown */
    E_ZCL_UNKNOWN        = 0xff

} teZCL_ZCLAttributeType;

/* On/Off Command - Payload */
typedef enum 
{

    E_CLD_ONOFF_CMD_OFF                      = 0x00,     /* Mandatory */
    E_CLD_ONOFF_CMD_ON,                                  /* Mandatory */
    E_CLD_ONOFF_CMD_TOGGLE,                              /* Mandatory */

    E_CLD_ONOFF_CMD_OFF_EFFECT               = 0x40,
    E_CLD_ONOFF_CMD_ON_RECALL_GLOBAL_SCENE,
    E_CLD_ONOFF_CMD_ON_TIMED_OFF
} teCLD_OnOff_Command;


typedef struct
{
    char *pCmdStr;
    char *pCmdTipStr;
}tsInputCmd;

typedef struct 
{
	uint16_t	u16ShortAddress;
	uint64_t	u64IEEEAddress;
	uint8_t 	u8MacCapability;
} __attribute__((__packed__)) tsDeviceAnnounce;

typedef struct 
{
	uint16_t	u16ShortAddress;
	uint64_t	u64IEEEAddress;
	uint8_t 	u8MacCapability;
	uint8_t     u8DevLqiReqSentFlag; // used as a flag in nwk disc indicating if mgmt_lqi_req has been sent in current discovery
}tsDevInfoRec;


typedef struct
{
  uint8_t		  u8AddressMode;
  uint16_t		  u16ShortAddress;
  uint8_t		  u8SourceEndPointId;
  uint8_t		  u8DestinationEndPointId;
  uint16_t		  u16ClusterId;
  uint8_t		  bDirectionIsServerToClient;
  uint8_t		  bIsManufacturerSpecific;
  uint16_t		  u16ManufacturerCode;
  uint8_t		  u8NumberOfIdentifiers;
  uint16_t		  au16AttributeList[10]; /* 10 is max number hard-coded in ZCB */
}__attribute__((__packed__)) tsZDReadAttrReq;

typedef struct
{
  uint8_t    u8LocalEp;
  uint8_t    u8RemoteEp;
  uint16_t   u16ClustId;
  uint16_t   u16AttrId;
}tsDefaultParaOfCmd;

typedef struct
 {
   uint8_t	 u8SequenceNumber;
   uint16_t  u16ShortAddress;
   uint8_t	 u8SourceEndPointId;
   uint16_t  u16ClusterId;
   uint16_t  u16AttributeId;
   uint8_t	 u8AttributeStatus;
   uint8_t	 u8AttributeDataType;
   uint16_t  u16SizeOfAttributesInBytes;
   uint8_t	 au8AttributeValue[];
 }__attribute__((__packed__)) tsZDReadAttrResp;

typedef struct  
{
    uint8_t 			u8SequenceNo;			/**< Sequence number of outgoing message */
    uint8_t 			u8Endpoint; 			/**< Source endpoint */
    uint16_t			u16ClusterID;			/**< Source cluster ID */
    uint8_t 			u8CommandID;			/**< Source command ID */
    uint8_t 			u8Status;				/**< Command status */
} __attribute__((__packed__)) tsZDDefaultRsp;

typedef struct _tsTxCmdPendingAckRecord
{
  uint32_t                    u32TxStamp;
  uint16_t                    u16DstAddr;
  uint8_t                     u8ZclSeqNum;
  struct _tsTxCmdPendingAckRecord    *next;
}tsTxCmdPendingAckRecord;

typedef struct
{
  uint8_t		  bEnable;
  uint32_t		  u32RfActiveOutDioMask;
  uint32_t		  u32StatusOutDioMask;
  uint32_t		  u32TxConfInDioMask;
  uint8_t         bRegisterCallback;
  uint16_t		  u16PollPeriod;
  uint8_t		  u8TimerId;
}__attribute__((__packed__)) tsIPNConfigReq;

typedef struct 
{
	uint16_t	u16TargetAddress;
	uint8_t 	u8StartIndex;
}__attribute__((__packed__)) tsManagementLQIRequest;

typedef struct 
{
	uint8_t 	u8SequenceNo;
	uint8_t 	u8Status;
	uint8_t 	u8NeighbourTableSize;
	uint8_t 	u8TableEntries;
	uint8_t 	u8StartIndex;
	struct
	{
		uint16_t	u16ShortAddress;
		uint64_t	u64PanID;
		uint64_t	u64IEEEAddress;
		uint8_t 	u8Depth;
		uint8_t 	u8LQI;
		struct
		{
			unsigned	uDeviceType : 2;
			unsigned	uPermitJoining: 2;
			unsigned	uRelationship : 2;
			unsigned	uMacCapability : 2;
		} __attribute__((__packed__)) sBitmap;
	} __attribute__((__packed__)) asNeighbours[2];
}__attribute__((__packed__)) tsManagementLQIResponse;

typedef struct  
{
	uint8_t 	u8SequenceNo;
	uint8_t 	u8Status;
} __attribute__((__packed__)) tsBindUnbindRsp;

typedef struct
{		 
	uint8_t 	u8SequenceNo;
	uint16_t    u16SrcAddr;
	uint8_t     u8SrcEp;
	uint16_t    u16ClusterId;
	uint8_t 	u8Status;
} __attribute__((__packed__)) tsConfigAttrReportRsp;


extern void init_global_vars(void);
extern teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo);
extern int input_cmd_handler(void);
extern void *pvSerialReaderThread(void *p);

#if defined __cplusplus
}
#endif

#endif

