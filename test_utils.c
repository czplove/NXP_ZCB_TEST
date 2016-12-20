#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "test_utils.h"

#define HELP_CMD  "help"
#define RESET_CMD  "reset"
#define ERASEPDM_CMD  "erasepdm"
#define SETDEVTYPE_CMD  "setdevtype"
#define SETCMSK_CMD  "setcmsk"
#define STARTNWK_CMD  "startnwk"
#define PERMITJOIN_CMD  "permitjoin"
#define DISCDEVS  "discdevs"
#define LISTDEVS  "listdevs"
#define READATTR  "readattr"
#define TOGGLE  "toggle"
#define TOGGLETEST "toggletest"
#define TXRXTEST  "txrxtest"
#define BIND  "bind"
#define CONFIGREPORT "configreport"
#define SETTXPLEVEL "settxplevel"
#define SETRXPLEVEL "setrxplevel"

#define GETVERSION_CMD  "getversion"
#define WRITEATTR  "writeattr"
#define QUIT_CMD  "quit"


static tsSL_Msg_Status gTxMsgStatus;
static pthread_mutex_t gSeialMsgSendMutex;
static pthread_mutex_t gTxAckListMutex;
static pthread_mutex_t gDevsListMutex;
static tsInputCmd gaInputCmdList[] =
{
    {HELP_CMD, "Show help info."},
    {RESET_CMD, "Sw reset ZCB."},
    {ERASEPDM_CMD, "Erase PDM of ZCB."},
    {SETDEVTYPE_CMD" <DevType>", "Set device type of ZCB, <DevType> is in 0~2, 0 means coordinator."},
    {SETCMSK_CMD" <ChannelMask>", "Set channel mask for nwk creation, \n\t  <ChannelMask> is in hex fmt 0xXXXX, such as 0xB or 0x800 means channel 11."},
    {STARTNWK_CMD, "Start to create nwk."},
    {PERMITJOIN_CMD" <Duration>", "Permit nwk join, <Duration> is in 0~255."},
    {DISCDEVS, "Discovery all devices in nwk by sending mgmt_lqi_req recursively."},
    {LISTDEVS,  "Display all joined/discovered devices"},
    {READATTR" <DstShortAddr>", "Read attribute from a remote device, in the read attribute command, by\n\t  default the using srcEP and dstEP are both 1, and the attribute to read\n\t  is zcl verion attribute of basic cluster,\n\t  <DstShortAddr> is the destination short addr to send command, in hex fmt 0xXXXX."},
    {TOGGLE" <DstShortAddr>", "Send a toggle command to remote device (the remote device should support OnOff cluster server),\n\t  <DstShortAddr> is the destination short addr to send command, in hex fmt 0xXXXX."},
    {TOGGLETEST" <DstShorAddr> <TxTimes> <TxInterval>", "Continously send toggle command to a remote device, get a count\n\t  of the total number of sent commands and received default response\n\t  to check the success ratio of response receiving,\n\t  <DstShorAddr> is the destination short addr to send command, in hex fmt 0xXXXX,\n\t  <TxTimes> is the number of commands to send, in decimal fmt, such as 100,\n\t  <TxInterval> is the interval(in ms) between two sending commands, in decimal fmt, >100."},
    {TXRXTEST" <DstShorAddr> <TxTimes> <TxInterval>", "Continously send read attribute command to a remote device, get a count\n\t  of the total number of sent commands and received attribute read response\n\t  to check the success ratio of response receiving,\n\t  <DstShorAddr> is the destination short addr to send command, in hex fmt 0xXXXX,\n\t  <TxTimes> is the number of commands to send, in decimal fmt, such as 100,\n\t  <TxInterval> is the interval(in ms) between two sending commands, in decimal fmt, >100."},
    {BIND" <DstIeeeAddr> <CoordIeeeAddr> <ClusterId>", "send a bind request to remote device to let it bind to coordinator for specific cluster, \n\t  by default the using srcEP and dstEP are both 1, \n\t  <DstIeeeAddr> is the ieee addr of remote node, 64 bit in hex fmt, start with 0x, such as 0x00158D0000E06AB5, \n\t  <CoordIeeeAddr> is the ieee addr of coordinator, 64 bit in hex fmt, start with 0x, such as 0x00158D0001153E60, \n\t  <ClusterId> is the cluster id to do binding, 16 bit in hex fmt, start with 0x, such as 0x0006"},
    {CONFIGREPORT" <DstShorAddr> <ClusterId> <AttrId> <AttrType> <MinInterval> <MaxInterval>", "Send config attribute command to a remote device, by default the using srcEP and dstEP are both 1, \n\t  <DstShorAddr> is the destination short addr to send command, in hex fmt 0xXXXX, \n\t  <ClusterId> is the cluster id to be configured, 16 bit in hex fmt, start with 0x, such as 0x0006, \n\t  <AttrId> is the attribute id to be configured, 16 bit in hex fmt, start with 0x, such as 0x0000, \n\t  <AttrType> is data type of the specified attribute id, 8 bit in hex fmt, start with 0x, such as 0x10, \n\t  <MinInterval> is the minimal report interval, uint is second, in decimal fmt, 1~65535, \n\t  <MaxInterval> is the maximal report interval, unit is second, in decimal fmt, 60~65535, should bigger than <MinInterval>, \n\t  eg. to config a OnOff attribute whose short addr is 0xC616 with min interval as 2 and max interval as 60, the cmd is: \n\t\tconfigreport 0xC616 0x6 0x0 0x10 2 60"},
    {SETTXPLEVEL" <PriorityLevel>", "Sets the number of retry attempts until the priority pin is raised high, \n\t  <PriorityLevel> is within range 0~3"},
    {SETRXPLEVEL" <EnableDisable>", "Enable or disable the priority for receiving pkts, 0-disable, 1-enable "},
    {GETVERSION_CMD, "get version of ZCB."},
    {WRITEATTR" <DstShortAddr> <ClusterId> <u8Direction> <AttrId> <AttrType> <AttrDate> ", "Write attribute to a remote device, in the write attribute command, by\n\t  default the using srcEP and dstEP are both 1, and the attribute to write\n\t  is zcl verion attribute of basic cluster,\n\t  <DstShortAddr> is the destination short addr to send command, in hex fmt 0xXXXX."},
    {QUIT_CMD, "Quit the test application."},
};

static tsDevInfoRec gatSavedDevList[MAXDEVNUM];
static tsDefaultParaOfCmd gtDefaultParaOfSendReq;
static tsTxCmdPendingAckRecord *gpTxCmdPendingAckList;
static uint32_t gu32NumOfTxCmd;
static uint32_t gu32NumOfAckedTxCmd;
static uint8_t  gu8CheckDefRespFlag=0;

//-int verbosity = 1;               /** Default log level */
int verbosity = 11; 

extern uint32_t 	gu32RfActiveOutDioMask;
extern uint32_t 	gu32StatusOutDioMask;
extern uint32_t 	gu32TxConfInDioMask;
extern uint16_t 	gu16PollPeriod;

/***************************************************************************/
static void show_help(void)
{
    int i;

    printf("\t*** test cmd usage ***\n");
    for (i = 0; i < (sizeof(gaInputCmdList)/sizeof(tsInputCmd)); i ++)
        printf("\t%s\n\t  %s\n\n", gaInputCmdList[i].pCmdStr, gaInputCmdList[i].pCmdTipStr);

    printf("\n");
}

/*
关于脱离线程的说明：
使用pthread_create()函数创建线程时，函数第二个参数为NULL，则使用线程属性的默认
参数，其中非分离属性需要程序退出之前运行pthread_join把各个线程归并到一起。如果
想让线程向创建它的线程返回数据，就必须这样做。但是如果既不需要第二个线程向主线
程返回信息，也不需要主线程等待它，可以设置分离属性，创建“脱离线程”。
*/
/* used by pvSerialReaderThread to send serial msg by detached thread */
static void CreateDetachedThread( void * (*send_routine), void *arg)	//?创建分离线程
{
    pthread_attr_t attr;
    pthread_t thread;

    pthread_attr_init (&attr);	//-初始化一个线程对象的属性,需要用pthread_attr_destroy函数对其去除初始化。
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);	//-线程分离属性设置
    pthread_create (&thread, &attr, (void *)send_routine, arg);	//-最后一个参数是运行函数的参数
    pthread_attr_destroy (&attr);	//-销毁线程属性结构,它在重新初始化之前不能重新使用
}

static void clear_txmsg_status()
{
    memset(&gTxMsgStatus, 0xFF, sizeof(tsSL_Msg_Status));
}

static void set_txmsg_status(tsSL_Msg_Status *pStatusMsg)
{
    //printf("\n msg=0x%04X, status=%d\n", pStatusMsg->u16MessageType, pStatusMsg->eStatus);
    memcpy(&gTxMsgStatus, pStatusMsg, sizeof(tsSL_Msg_Status));
}

static int check_txmsg_status(uint16_t u16MessageType, uint16_t max_delay)
{
    int i;

    for(i = 0; i < max_delay; i ++)
    {
        IOT_MSLEEP(1);
        //printf("\n\t curr read out msg status is 0x%04X", ntohs(gTxMsgStatus.u16MessageType));
        if(ntohs(gTxMsgStatus.u16MessageType) == u16MessageType)
        {
            if (gTxMsgStatus.eStatus != E_SL_MSG_STATUS_SUCCESS)
            {
                //printf("\n\t*** Msg status for tx msg 0x%04X = %d,  \n", u16MessageType, gTxMsgStatus.eStatus);
                return 1; // msg status received but status is not sucessfully
            }
            else
            {
                //printf("\n OK, Tx msg 0x%04X status = %d, i = %d,  \n", u16MessageType, gTxMsgStatus.eStatus, i);       
                 return 0; // msg status received and status is sucessfully 
            }
        }
    }

    printf("\n\t ***Wait msg status for tx msg 0x%04X expired  within %d ms\n", u16MessageType, max_delay);
    return 2; // means receive status msg expired 
}

static teZcbStatus eSetDeviceType(teModuleMode eModuleMode) {

    struct _SetDeviceTypeMessage {
        uint16_t    u8ModuleMode;
    } __attribute__((__packed__)) sSetDeviceTypeMessage;

    sSetDeviceTypeMessage.u8ModuleMode = eModuleMode;

    if (eSL_SendMessage(E_SL_MSG_SET_DEVICETYPE, sizeof(struct _SetDeviceTypeMessage),
            &sSetDeviceTypeMessage, NULL) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}

static teZcbStatus eSetChannelMask(uint32_t u32ChannelMask) {

    struct _SetChannelMaskMessage {
        uint32_t    u32ChannelMask;
    } __attribute__((__packed__)) sSetChannelMaskMessage;

    sSetChannelMaskMessage.u32ChannelMask  = htonl(u32ChannelMask);

    if (eSL_SendMessage(E_SL_MSG_SET_CHANNELMASK, sizeof(struct _SetChannelMaskMessage),
            &sSetChannelMaskMessage, NULL) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}

static teZcbStatus eSetPermitJoining(uint8_t u8Interval) {

    struct _PermitJoiningMessage {
        uint16_t    u16TargetAddress;
        uint8_t     u8Interval;
        uint8_t     u8TCSignificance;
    } __attribute__((__packed__)) sPermitJoiningMessage;

    sPermitJoiningMessage.u16TargetAddress  = htons(E_ZB_BROADCAST_ADDRESS_ROUTERS);
    sPermitJoiningMessage.u8Interval        = u8Interval;
    sPermitJoiningMessage.u8TCSignificance  = 0;

    if (eSL_SendMessage(E_SL_MSG_PERMIT_JOINING_REQUEST, sizeof(struct _PermitJoiningMessage),
           &sPermitJoiningMessage, NULL) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}

static teZcbStatus eSetTxPriorityLevel(uint8_t u8PriorityLevel) 
{

    struct _SetPriLevelMessage {
        uint8_t    u8PriLevel;
    } __attribute__((__packed__)) sSetPriLevelMessage;

    sSetPriLevelMessage.u8PriLevel = u8PriorityLevel;

    if (eSL_SendMessage(E_SL_MSG_AHI_IPN_SET_TX_PRIORITY_RETRY, sizeof(struct _SetPriLevelMessage),
            &sSetPriLevelMessage, NULL) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}

static teZcbStatus eSetRxPriorityLevel(uint8_t u8PriorityLevel) 
{

    struct _SetPriLevelMessage {
        uint8_t    u8PriLevel;
    } __attribute__((__packed__)) sSetPriLevelMessage;

    sSetPriLevelMessage.u8PriLevel = u8PriorityLevel;

    if (eSL_SendMessage(E_SL_MSG_AHI_IPN_SET_RX_PRIORITY, sizeof(struct _SetPriLevelMessage),
            &sSetPriLevelMessage, NULL) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}

// return 0 if the device is already there, return 1 if the device is newly saved
static int CheckSaveDevice(tsDevInfoRec *psDevInfo)
{
    int i, new_dev_flag;

    pthread_mutex_lock(&gDevsListMutex);
    /* check if the device is already in the saved list */
    new_dev_flag = 1; // 1 means a new device that was not saved at the list yet
    for (i = 0; i < MAXDEVNUM; i ++)
    {
        if (gatSavedDevList[i].u64IEEEAddress == psDevInfo->u64IEEEAddress)
        {
            new_dev_flag = 0;
            // the device is already in list, so update it
            gatSavedDevList[i].u16ShortAddress = psDevInfo->u16ShortAddress;
            gatSavedDevList[i].u8MacCapability = psDevInfo->u8MacCapability;
            break;
        }
    }

    if(new_dev_flag)
    {
        /* find first found blank position and save it */
		for (i = 0; i < MAXDEVNUM; i ++)
        {
            /* fill it into first MAC address match cell or blank cell */
            if (gatSavedDevList[i].u64IEEEAddress == 0)
            {
                // a blank cell is found, sav to it
                gatSavedDevList[i].u64IEEEAddress = psDevInfo->u64IEEEAddress;
                gatSavedDevList[i].u16ShortAddress = psDevInfo->u16ShortAddress;
                gatSavedDevList[i].u8MacCapability = psDevInfo->u8MacCapability;
                gatSavedDevList[i].u8DevLqiReqSentFlag = 0;
                break;
            }
        }
    }

    pthread_mutex_unlock(&gDevsListMutex);
    return new_dev_flag;
}

static void ClearAllDevsMgmtLqiReqSentFlag(void)
{
    int i;

    pthread_mutex_lock(&gDevsListMutex);
    for (i = 0; i < MAXDEVNUM; i ++)
        gatSavedDevList[i].u8DevLqiReqSentFlag = 0;

    pthread_mutex_unlock(&gDevsListMutex);
}

static uint8_t GetDevMgmtLqiReqSentFlag(uint16_t u16DevShortAddr)
{
    int i;
    uint8_t sent_flag = 0;

    for (i = 0; i < MAXDEVNUM; i ++)
    {
        if (gatSavedDevList[i].u16ShortAddress == u16DevShortAddr)
        {
            sent_flag = gatSavedDevList[i].u8DevLqiReqSentFlag;
            break;
        }
    }

    return sent_flag;
}

static void SetDevMgmtLqiReqSentFlag(uint16_t u16DevShortAddr)
{
    int i;

    if (u16DevShortAddr == 0)
        return;

    for (i = 0; i < MAXDEVNUM; i ++)
    {
        if (gatSavedDevList[i].u16ShortAddress == u16DevShortAddr)
            gatSavedDevList[i].u8DevLqiReqSentFlag = 1;
    }
}

static void ZCB_HandleNetworkJoinedFormed(void *pvMessage) {
    struct _tsNetworkJoinedFormedShort {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
        uint16_t    u16Panid;
    } __attribute__((__packed__)) *psMessageShort = (struct _tsNetworkJoinedFormedShort *)pvMessage;

    
    psMessageShort->u16ShortAddress = ntohs(psMessageShort->u16ShortAddress);
    psMessageShort->u64IEEEAddress  = be64toh(psMessageShort->u64IEEEAddress);

    if (psMessageShort->u8Status == 0x01) 
    {
        printf("\n\tNwk formed on Channel %d, panid=0x%04X\n", psMessageShort->u8Channel, psMessageShort->u16Panid);
    }
    else
    {
        printf("\n\t***Nwk formed failed, status=0x%02X\n", psMessageShort->u8Status);
    }
    
}

static void ZCB_HandleDeviceAnnounceMsg(void *pvMessage) 
{
    tsDevInfoRec sDevInfo;
    tsDeviceAnnounce *psMessage = (tsDeviceAnnounce *)pvMessage;
    
    sDevInfo.u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    sDevInfo.u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    sDevInfo.u8MacCapability = psMessage->u8MacCapability;
    
    printf("\n\tDevice Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X\n", 
                sDevInfo.u16ShortAddress,
                (unsigned long long int)sDevInfo.u64IEEEAddress,
                sDevInfo.u8MacCapability
            );
  
    /* save the device  */
    CheckSaveDevice(&sDevInfo);
    return;
}

static void ShowDevicesInNwk(void)
{
    int i, count;

    printf("\tList of  devices:\n");
    count = 0;
    for (i = 0; i < MAXDEVNUM; i ++)
    {
        if(gatSavedDevList[i].u64IEEEAddress != 0)
        {
            char *pDevTypeStr;
            if (gatSavedDevList[i].u16ShortAddress == 0)
                pDevTypeStr = "Coord";
            else
                pDevTypeStr = (gatSavedDevList[i].u8MacCapability & 0x02)? "Router":"Enddevice";
            
            printf("\t#%d, ShortAddress= 0x%04X, MacAddr= 0x%016llX, type= %s\n",
                    ++count,
                    gatSavedDevList[i].u16ShortAddress,
                    (unsigned long long int)gatSavedDevList[i].u64IEEEAddress,
                    pDevTypeStr
                   );
        }
    }

    printf("\tTotal number of devices= %d\n", count);
    
}


static teZcbStatus eZCB_SendBindCommand(uint64_t u64IEEEAddress, uint64_t u64CoordIeeeAddr, uint16_t u16ClusterId )
{

    struct _BindUnbindReq {
        uint64_t u64SrcAddress;
        uint8_t u8SrcEndpoint;
        uint16_t u16ClusterId;
        uint8_t u8DstAddrMode;
        union {
            struct {
                uint16_t u16DstAddress;
            } __attribute__((__packed__)) sShort;
            struct {
                uint64_t u64DstAddress;
                uint8_t u8DstEndPoint;
            } __attribute__((__packed__)) sExtended;
        } __attribute__((__packed__)) uAddressField;
    } __attribute__((__packed__)) sBindUnbindReq;
        
    uint16_t u16Length = sizeof(struct _BindUnbindReq);
    teSL_Status eStatus = E_ZCB_COMMS_FAILED;
    
    memset( (char *)&sBindUnbindReq, 0, u16Length );

    sBindUnbindReq.u64SrcAddress = htobe64( u64IEEEAddress );
    sBindUnbindReq.u8SrcEndpoint = gtDefaultParaOfSendReq.u8RemoteEp;
    sBindUnbindReq.u16ClusterId  = htons( u16ClusterId );
    sBindUnbindReq.u8DstAddrMode = 0x03;
    sBindUnbindReq.uAddressField.sExtended.u64DstAddress = htobe64(u64CoordIeeeAddr);
    sBindUnbindReq.uAddressField.sExtended.u8DstEndPoint = gtDefaultParaOfSendReq.u8LocalEp;
    
    DEBUG_PRINTF("Send Binding request to 0x%016llX\n", (long long unsigned int)u64IEEEAddress);
    
    eStatus = eSL_SendMessage(E_SL_MSG_BIND,
                              u16Length, 
                              &sBindUnbindReq, 
                              NULL);
    if (eStatus != E_SL_OK)
    {
        DEBUG_PRINTF( "\tSending of bind cmd failed (0x%02x)\n", eStatus);
    }

    return eStatus;
}

static teZcbStatus eZCB_SendConfigureReportingCommand(uint16_t u16ShortAddress,
                                                                        uint16_t u16ClusterID,
                                                                        uint16_t u16AttributeID, 
                                                                        teZCL_ZCLAttributeType eType,
                                                                        uint16_t u16MinInterval,
                                                                        uint16_t u16MaxInterval)
{
    struct _AttributeReportingConfigurationRequest {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Direction;
        uint8_t     u8ManuSpecific;
        uint16_t    u16ManuID;
        uint8_t     u8NumOfAttrs;
        
        uint8_t     u8DirectionIsReceived;
        uint8_t     eAttributeDataType;
        uint16_t    u16AttributeEnum;
        uint16_t    u16MinimumReportingInterval;
        uint16_t    u16MaximumReportingInterval;
        uint16_t    u16TimeoutPeriodField;
        uint8_t    uAttributeReportableChange;
    } __attribute__((__packed__)) sAttributeReportingConfigurationRequest;
    
    uint16_t u16Length = sizeof(struct _AttributeReportingConfigurationRequest);
    uint8_t u8SequenceNo;
    teSL_Status eStatus;
    
    sAttributeReportingConfigurationRequest.u8TargetAddressMode   = E_ZD_ADDRESS_MODE_SHORT;
    sAttributeReportingConfigurationRequest.u16TargetAddress      = htons(u16ShortAddress);
    sAttributeReportingConfigurationRequest.u8SourceEndpoint      = gtDefaultParaOfSendReq.u8LocalEp;
    sAttributeReportingConfigurationRequest.u8DestinationEndpoint = gtDefaultParaOfSendReq.u8RemoteEp;    
    sAttributeReportingConfigurationRequest.u16ClusterID             = htons(u16ClusterID);
    sAttributeReportingConfigurationRequest.u8Direction              = 0;
    sAttributeReportingConfigurationRequest.u8ManuSpecific           = 0;
    sAttributeReportingConfigurationRequest.u16ManuID                = 0x0000;    
    sAttributeReportingConfigurationRequest.u8NumOfAttrs             = 1;
    
    sAttributeReportingConfigurationRequest.u8DirectionIsReceived       = 0;
    sAttributeReportingConfigurationRequest.eAttributeDataType          = (uint8_t)eType;
    sAttributeReportingConfigurationRequest.u16AttributeEnum            = htons(u16AttributeID);
    sAttributeReportingConfigurationRequest.u16MinimumReportingInterval = htons(u16MinInterval);
    sAttributeReportingConfigurationRequest.u16MaximumReportingInterval = htons(u16MaxInterval);
    sAttributeReportingConfigurationRequest.u16TimeoutPeriodField       = 0;
    sAttributeReportingConfigurationRequest.uAttributeReportableChange  = 0;

    eStatus = eSL_SendMessage( E_SL_MSG_CONFIG_REPORTING_REQUEST,
                               u16Length, 
                               &sAttributeReportingConfigurationRequest, 
                               &u8SequenceNo);
    if (eStatus != E_SL_OK)
    {
        DEBUG_PRINTF( "\tSending config attr report cmd failed (0x%02x)\n", eStatus);
    }

    return eStatus;
}


static teSL_Status ZDReadAttrReq(
    uint16_t 			u16ShortAddress,
    uint8_t				u8SrcEndpoint,
    uint8_t				u8DestEndpoint,
    eZigbee_ClusterID 	u16ClusterID,
    uint8_t			u8NbAttr,
    uint16_t*			au16AttrList,
    uint8_t*        pu8SequenceNo)
{
  ;
  teSL_Status eStatus;

  tsZDReadAttrReq sReadAttrReq =
  {
      .u8AddressMode               = E_ZD_ADDRESS_MODE_SHORT,
      .u16ShortAddress            = htons(u16ShortAddress),
      .u8SourceEndPointId 		    = u8SrcEndpoint,
      .u8DestinationEndPointId 	  = u8DestEndpoint,
      .u16ClusterId 				      = htons(u16ClusterID),
      .bDirectionIsServerToClient	= 0,
      .bIsManufacturerSpecific	  = 0,
      .u16ManufacturerCode		    = 0,
      .u8NumberOfIdentifiers      = u8NbAttr,
  };

  for (uint16_t i = 0; i < u8NbAttr; i++)
  {
    sReadAttrReq.au16AttributeList[i] = htons(au16AttrList[i]);
  }

  /* when test with read attribute, don't check the default response */
  gu8CheckDefRespFlag = 0;
  
  eStatus = eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST,
      sizeof(tsZDReadAttrReq), &sReadAttrReq, pu8SequenceNo);
  if (eStatus != E_SL_OK)
  {
    printf("\tSend Read Attribute Request to 0x%04x : Fail (0x%x)\n",
        u16ShortAddress, eStatus);
  }

  return eStatus;
}

// ------------------------------------------------------------------
// Write attribute
// ------------------------------------------------------------------

teZcbStatus eZCB_WriteAttributeRequest(uint16_t u16ShortAddress,
                                    uint16_t u16ClusterID,
                                    uint8_t u8Direction, 
                                    uint8_t u8ManufacturerSpecific, 
                                    uint16_t u16ManufacturerID,
                                    uint16_t u16AttributeID, 
                                    teZCL_ZCLAttributeType eType, 
                                    void *pvData)
{
    struct _WriteAttributeRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Direction;
        uint8_t     u8ManufacturerSpecific;
        uint16_t    u16ManufacturerID;
        uint8_t     u8NumAttributes;
        uint16_t    u16AttributeID;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) sWriteAttributeRequest;
    
#if 0
    struct _WriteAttributeResponse
    {
        /**\todo ZCB-Sheffield: handle default response properly */
        uint8_t     au8ZCLHeader[3];
        uint16_t    u16MessageType;
        
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) *psWriteAttributeResponse = NULL;
#endif
    
    struct _DataIndication
    {
        /**\todo ZCB-Sheffield: handle data indication properly */
        uint8_t     u8ZCBStatus;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8SourceAddressMode;
        uint16_t    u16SourceShortAddress; /* OR uint64_t u64IEEEAddress */
        uint8_t     u8DestinationAddressMode;
        uint16_t    u16DestinationShortAddress; /* OR uint64_t u64IEEEAddress */
        
        uint8_t     u8FrameControl;
        uint8_t     u8SequenceNo;
        uint8_t     u8Command;
        uint8_t     u8Status;
        uint16_t    u16AttributeID;
    } __attribute__((__packed__)) *psDataIndication = NULL;

    
    uint16_t u16Length = sizeof(struct _WriteAttributeRequest) - sizeof(sWriteAttributeRequest.uData);
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DEBUG_PRINTF("Send Write Attribute request to 0x%04X\n", u16ShortAddress);
    
    sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    sWriteAttributeRequest.u16TargetAddress      = htons(u16ShortAddress);
    sWriteAttributeRequest.u8SourceEndpoint      = ZB_ENDPOINT_ATTR;
    sWriteAttributeRequest.u8DestinationEndpoint = ZB_ENDPOINT_ATTR;
    
    sWriteAttributeRequest.u16ClusterID             = htons(u16ClusterID);
    sWriteAttributeRequest.u8Direction              = u8Direction;
    sWriteAttributeRequest.u8ManufacturerSpecific   = u8ManufacturerSpecific;
    sWriteAttributeRequest.u16ManufacturerID        = htons(u16ManufacturerID);
    sWriteAttributeRequest.u8NumAttributes          = 1;
    sWriteAttributeRequest.u16AttributeID           = htons(u16AttributeID);
    sWriteAttributeRequest.u8Type                   = (uint8_t)eType;
    
    switch(eType)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(&sWriteAttributeRequest.uData.u8Data, pvData, sizeof(uint8_t));
                        u16Length += sizeof(uint8_t);
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            memcpy(&sWriteAttributeRequest.uData.u16Data, pvData, sizeof(uint16_t));
            sWriteAttributeRequest.uData.u16Data = ntohs(sWriteAttributeRequest.uData.u16Data);
                        u16Length += sizeof(uint16_t);
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            memcpy(&sWriteAttributeRequest.uData.u32Data, pvData, sizeof(uint32_t));
            sWriteAttributeRequest.uData.u32Data = ntohl(sWriteAttributeRequest.uData.u32Data);
                        u16Length += sizeof(uint32_t);
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            memcpy(&sWriteAttributeRequest.uData.u64Data, pvData, sizeof(uint64_t));
            sWriteAttributeRequest.uData.u64Data = be64toh(sWriteAttributeRequest.uData.u64Data);
                        u16Length += sizeof(uint64_t);
            break;
            
        default:
            printf( "Unknown attribute data type (%d)", eType);
            return E_ZCB_ERROR;
    }
    
//    printf( "sWriteAttributeRequest:\n" );
//    dump( (char *)&sWriteAttributeRequest, u16Length );
    
    if (eSL_SendMessage(0x0110 /*E_SL_MSG_WRITE_ATTRIBUTE_REQUEST*/, u16Length, &sWriteAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
// printf( "kok1\n" );
        goto done;
    }
    
    while (1)
    {
// printf( "kok2\n" );
        /* Wait 1 second for the message to arrive */
        /**\todo ZCB-Sheffield: handle data indication here for now - BAD Idea! Implement a general case handler in future! */
        //-先不考虑这种出错情况,后续需要把下面等待函数理解然后移植过来考虑
        //-if (eSL_MessageWait(E_SL_MSG_DATA_INDICATION, 1000, &u16Length, (void**)&psDataIndication) != E_SL_OK)
        //-{
        //-    DEBUG_PRINTF( "No response to write attribute request\n");
        //-    eStatus = E_ZCB_COMMS_FAILED;
        //-    goto done;
        //-}
        IOT_SLEEP(1);
        
        DEBUG_PRINTF( "Got data indication\n");
        
        //-if (u8SequenceNo == psDataIndication->u8SequenceNo)
        //-{
            break;
        //-}
        //-else
        //-{
        //-    printf( "Write Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psDataIndication->u8SequenceNo, u8SequenceNo);
        //-    goto done;
        //-}
    }
    
    DEBUG_PRINTF( "Got write attribute response\n");
    
    //-eStatus = psDataIndication->u8Status;	//-如果没有赋值而进行了这里的操作就会系统崩溃,报Segmentation fault (core dumped)错误

done:
    free(psDataIndication);
// printf( "kok3\n" );
    return eStatus;
}


static teZcbStatus ZDOnOffCmd( uint16_t u16ShortAddress, 
                              uint8_t u8SrcEndpoint,
                              uint8_t u8DestEndpoint,
                              uint8_t u8Cmd,
                              uint8_t* pu8SequenceNo)
{
    teSL_Status         eStatus;

    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Cmd;
    } __attribute__((__packed__)) sOnOffMessage;

    if (u8Cmd > 2) {
        /* Illegal value */
        return E_ZCB_ERROR;
    }

    sOnOffMessage.u8TargetAddressMode   = E_ZD_ADDRESS_MODE_SHORT;
    sOnOffMessage.u16TargetAddress      = htons(u16ShortAddress);
    sOnOffMessage.u8SourceEndpoint      = u8SrcEndpoint;
    sOnOffMessage.u8DestinationEndpoint = u8DestEndpoint;
    sOnOffMessage.u8Cmd = u8Cmd;
    
    eStatus = eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage),
        &sOnOffMessage, pu8SequenceNo);

    if (eStatus != E_SL_OK)
    {
      DEBUG_PRINTF( "Sending Command %d failed (0x%02x)\n",u8Cmd, eStatus);
    }

    return eStatus;
}
static uint32_t u32GetAttributeActualSize(uint32_t u32Type)
{
    uint32_t u32Size = 0;
    switch(u32Type)
    {
    case(E_ZCL_GINT8):
    case(E_ZCL_UINT8):
    case(E_ZCL_INT8):
    case(E_ZCL_ENUM8):
    case(E_ZCL_BMAP8):
    case(E_ZCL_BOOL):
    case(E_ZCL_OSTRING):
    case(E_ZCL_CSTRING):
        u32Size = sizeof(uint8_t);
        break;
    case(E_ZCL_LOSTRING):
    case(E_ZCL_LCSTRING):
    case(E_ZCL_STRUCT):
    case (E_ZCL_INT16):
    case (E_ZCL_UINT16):
    case (E_ZCL_ENUM16):
    case (E_ZCL_CLUSTER_ID):
    case (E_ZCL_ATTRIBUTE_ID):
        u32Size = sizeof(uint16_t);
        break;


    case E_ZCL_UINT24:
    case E_ZCL_UINT32:
    case E_ZCL_TOD:
    case E_ZCL_DATE:
    case E_ZCL_UTCT:
    case E_ZCL_BACNET_OID:
    case E_ZCL_INT24:
        u32Size = sizeof(uint32_t);
        break;

    case E_ZCL_UINT40:
    case E_ZCL_UINT48:
    case E_ZCL_UINT56:
    case E_ZCL_UINT64:
    case E_ZCL_IEEE_ADDR:
        u32Size = sizeof(uint64_t);
        break;
    default:
        u32Size = 0;
        break;
    }
    return u32Size;
}

static void AddTxCmdToPendingAckList(uint16_t u16ShortAddr, uint8_t u8ZclSeqNum)
{
    tsTxCmdPendingAckRecord *ptsTxCmdRec;
    time_t curr_timestamp;

    ptsTxCmdRec = (tsTxCmdPendingAckRecord *)malloc(sizeof(tsTxCmdPendingAckRecord));
    if (!ptsTxCmdRec)
    {
        printf("\n\tAllocate buffer for tx cmd recording failed!");
        return;
    }
    
    time(&curr_timestamp);
    ptsTxCmdRec->u32TxStamp = curr_timestamp;
    ptsTxCmdRec->u16DstAddr = u16ShortAddr;
    ptsTxCmdRec->u8ZclSeqNum = u8ZclSeqNum;
    ptsTxCmdRec->next = NULL;
    
    pthread_mutex_lock(&gTxAckListMutex);
    if(gpTxCmdPendingAckList == NULL)
    {
        gpTxCmdPendingAckList = ptsTxCmdRec;
    }
    else
    {
        ptsTxCmdRec->next = gpTxCmdPendingAckList;
        gpTxCmdPendingAckList = ptsTxCmdRec;
    }

    //printf("\t add tx cmd seq %d\n", u8ZclSeqNum);
    gu32NumOfTxCmd ++;
    pthread_mutex_unlock(&gTxAckListMutex);
}

static void CheckAndAckPendingTxCmd(uint8_t u8ZclSeqNum)
{
    tsTxCmdPendingAckRecord *ptsCurrRec, *ptsPrevRec;

    pthread_mutex_lock(&gTxAckListMutex);
    ptsPrevRec = ptsCurrRec = gpTxCmdPendingAckList;
    while(ptsCurrRec != NULL)
    {
        // search the list and remove found item from it
        //printf("\t checking ShortAddr=0x%04X, ZclSeqNum=%d for seq %d\n", ptsCurrRec->u16DstAddr, ptsCurrRec->u8ZclSeqNum, u8ZclSeqNum);
        if(ptsCurrRec->u8ZclSeqNum == u8ZclSeqNum)
        {
            printf("\tAcknowledged tx pkt, ShortAddr=0x%04X, ZclSeqNum=%d\n\n", ptsCurrRec->u16DstAddr, ptsCurrRec->u8ZclSeqNum);
            if (ptsCurrRec == gpTxCmdPendingAckList) // it is at head
                gpTxCmdPendingAckList = ptsCurrRec->next;
            else
                ptsPrevRec->next = ptsCurrRec->next;

            free(ptsCurrRec);
            gu32NumOfAckedTxCmd ++;
            break;
        }

        ptsPrevRec = ptsCurrRec;
        ptsCurrRec = ptsCurrRec->next;
    }
    pthread_mutex_unlock(&gTxAckListMutex);
    
}

static void DisplayAndFreeTxCmdPendingAckList(void)
{
    tsTxCmdPendingAckRecord *ptsCurrRec, *ptsTempRec;
    int diff = gu32NumOfTxCmd - gu32NumOfAckedTxCmd;

    if(diff)
        printf("\n\t%d Tx cmds of total %d was not ackowledged:\n", diff, gu32NumOfTxCmd);
    else
        printf("\n\tOK, all %d Tx cmds have been ackowledged.\n", gu32NumOfTxCmd);

    pthread_mutex_lock(&gTxAckListMutex);
    ptsCurrRec = gpTxCmdPendingAckList;
    while(ptsCurrRec != NULL)
    {
        printf("\t\tShortAddr=0x%04X, ZclSeqNum=%d\n", ptsCurrRec->u16DstAddr, ptsCurrRec->u8ZclSeqNum);
        ptsTempRec = ptsCurrRec;
        ptsCurrRec = ptsCurrRec->next;
        free(ptsTempRec);
    }
    gpTxCmdPendingAckList = NULL;
    pthread_mutex_unlock(&gTxAckListMutex);
}

/* Note, the input pointer will be freed on func exit */
static void ZCB_HandleReadAttrResp(void *pvMessage)
{
    int i;
    tsZDReadAttrResp *psMessage = (tsZDReadAttrResp *) pvMessage;

	printf("\n\tRead attr rsp from device 0x%04x , zcl_seq=%d, clust=0x%04X, attr=0x%04X, status=0x%02X",				
			ntohs(psMessage->u16ShortAddress),
			psMessage->u8SequenceNumber,
			ntohs(psMessage->u16ClusterId),
			ntohs(psMessage->u16AttributeId),
			psMessage->u8AttributeStatus);

    if (psMessage->u8AttributeStatus == SUCCESS)
    {
	     // only process for the first attr value
	     printf(", val=0x");
	     for (i = 0; i < u32GetAttributeActualSize(psMessage->u8AttributeDataType); i ++)
	         printf("%02X", psMessage->au8AttributeValue[i]);

	     printf("\n");		 
    }

    CheckAndAckPendingTxCmd(psMessage->u8SequenceNumber);
    
    if (pvMessage)
        free(pvMessage);
    
    return;
}


/* display the received attribute report */
static void ZCB_HandleReceivedAttrReport(void *pvMessage)
{
    int i;
    tsZDReadAttrResp *psMessage = (tsZDReadAttrResp *) pvMessage;

	printf("\n\tReceived attr report from device 0x%04x , zcl_seq=%d, clust=0x%04X, attr=0x%04X, status=0x%02X",				
			ntohs(psMessage->u16ShortAddress),
			psMessage->u8SequenceNumber,
			ntohs(psMessage->u16ClusterId),
			ntohs(psMessage->u16AttributeId),
			psMessage->u8AttributeStatus);

    if (psMessage->u8AttributeStatus == SUCCESS)
    {
	     // only process for the first attr value
	     printf(", val=0x");
	     for (i = 0; i < u32GetAttributeActualSize(psMessage->u8AttributeDataType); i ++)
	         printf("%02X", psMessage->au8AttributeValue[i]);

	     printf("\n");		 
    }
        
    return;
}

/* Note, the input pointer will be freed on func exit */
static void ZCB_HandleDefaultResponse(void *pvMessage) 
{
    tsZDDefaultRsp *psMessage = (tsZDDefaultRsp *)pvMessage;
    
    psMessage->u16ClusterID  = ntohs(psMessage->u16ClusterID);

    printf("\tDefault Response : cluster 0x%04X Command 0x%02X status: 0x%02X\n",
            psMessage->u16ClusterID, psMessage->u8CommandID, psMessage->u8Status);

    if (gu8CheckDefRespFlag)
    {   
        CheckAndAckPendingTxCmd(psMessage->u8SequenceNo);
    }

    if (pvMessage)
        free(pvMessage);
        
    return;
}


/* Note, the input pointer will be freed on func exit */
static int eSendMgmtLqiReq(tsManagementLQIRequest *psMgmtLQIReq) 
{
    tsManagementLQIRequest sManagementLQIRequest;
    teSL_Status eStatus;  
    uint8_t startIndex;

	pthread_mutex_lock(&gDevsListMutex);
	if (! GetDevMgmtLqiReqSentFlag(psMgmtLQIReq->u16TargetAddress))
	{
        SetDevMgmtLqiReqSentFlag(psMgmtLQIReq->u16TargetAddress);

        /* loop sending mgmt_lqi_req to requst entries with start index increased 2 per time */
        for (startIndex = 0; startIndex < 26; startIndex += 2)
        {
            printf("\n\tSend mgmt_lqi_req to 0x%04X for entries starting at %d\n", 
                    psMgmtLQIReq->u16TargetAddress, startIndex);

            sManagementLQIRequest.u16TargetAddress = htons(psMgmtLQIReq->u16TargetAddress);
            sManagementLQIRequest.u8StartIndex     = startIndex; //psMgmtLQIReq->u8StartIndex;

            eStatus = eSL_SendMessage(E_SL_MSG_MANAGEMENT_LQI_REQUEST,
                                      sizeof(tsManagementLQIRequest), &sManagementLQIRequest, NULL);
            IOT_MSLEEP(200);
        }
    }
    
    if (psMgmtLQIReq)
        free(psMgmtLQIReq);

	pthread_mutex_unlock(&gDevsListMutex);
    return eStatus;
}

/* Note, the input pointer will be freed on func exit */
static void ZCB_HandleMgmtLqiRsp(tsManagementLQIResponse *psMgmtLQIRsp) 
{
    int i;
    tsDevInfoRec sDevInfo;    
    
    if (psMgmtLQIRsp->u8Status == SUCCESS)
    {
      printf("\tReceived management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
              psMgmtLQIRsp->u8NeighbourTableSize,
              psMgmtLQIRsp->u8TableEntries,
              psMgmtLQIRsp->u8StartIndex);

      for (i = 0; i < psMgmtLQIRsp->u8TableEntries; i++)
      {
        psMgmtLQIRsp->asNeighbours[i].u16ShortAddress    = ntohs(psMgmtLQIRsp->asNeighbours[i].u16ShortAddress);
        psMgmtLQIRsp->asNeighbours[i].u64PanID           = be64toh(psMgmtLQIRsp->asNeighbours[i].u64PanID);
        psMgmtLQIRsp->asNeighbours[i].u64IEEEAddress     = be64toh(psMgmtLQIRsp->asNeighbours[i].u64IEEEAddress);

        if ((psMgmtLQIRsp->asNeighbours[i].u16ShortAddress >= 0xFFFA) ||
            (psMgmtLQIRsp->asNeighbours[i].u64IEEEAddress  == 0))
        {
          /* Illegal short / IEEE address */
          continue;
        }

        DEBUG_PRINTF( "\tEntry %02d: Short Address 0x%04X, EPID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
            psMgmtLQIRsp->asNeighbours[i].u16ShortAddress,
            (unsigned long long int)psMgmtLQIRsp->asNeighbours[i].u64PanID,
            (unsigned long long int)psMgmtLQIRsp->asNeighbours[i].u64IEEEAddress);

        DEBUG_PRINTF( "\t  Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
            psMgmtLQIRsp->asNeighbours[i].sBitmap.uDeviceType,
            psMgmtLQIRsp->asNeighbours[i].sBitmap.uPermitJoining,
            psMgmtLQIRsp->asNeighbours[i].sBitmap.uRelationship,
            psMgmtLQIRsp->asNeighbours[i].sBitmap.uMacCapability);

        DEBUG_PRINTF( "\t  Depth: %d, LQI: %d\n", 
            psMgmtLQIRsp->asNeighbours[i].u8Depth,
            psMgmtLQIRsp->asNeighbours[i].u8LQI);

        /* check and save the device info */
        sDevInfo.u16ShortAddress = psMgmtLQIRsp->asNeighbours[i].u16ShortAddress;
        sDevInfo.u64IEEEAddress = psMgmtLQIRsp->asNeighbours[i].u64IEEEAddress;
        sDevInfo.u8MacCapability = (psMgmtLQIRsp->asNeighbours[i].sBitmap.uDeviceType) << 1;
        CheckSaveDevice(&sDevInfo);

        /* send the mgmt_lqi_req to the node if not sent yet and it is a router device */
        if (  (psMgmtLQIRsp->asNeighbours[i].sBitmap.uDeviceType == 1) &&
              (psMgmtLQIRsp->asNeighbours[i].u16ShortAddress != 0x0000))
        {
            tsManagementLQIRequest *psMgmtLQIReq = malloc(sizeof(tsManagementLQIRequest));		  
            if (psMgmtLQIReq)
            {
                psMgmtLQIReq->u16TargetAddress = sDevInfo.u16ShortAddress;
                psMgmtLQIReq->u8StartIndex = 0;
                CreateDetachedThread((void *)eSendMgmtLqiReq, (void *)psMgmtLQIReq);
            }
		    else
		    {
                printf("\t***Allocate memory for mgmt lqi req failed.");		
            }            
        }
      }      
    } // if (psMgmtLQIRsp->u8Status == CZD_NW_STATUS_SUCCESS)

    if(psMgmtLQIRsp)
        free(psMgmtLQIRsp);

    return;
}


/*********************************** exported functions ******************************************/
void init_global_vars(void)
{
    // init all mutexs
    pthread_mutex_init (&gSeialMsgSendMutex, NULL);	//-以动态方式创建互斥锁的，参数attr指定了新建互斥锁的属性。
    pthread_mutex_init (&gTxAckListMutex, NULL);	//-如果参数attr为空，则使用默认的互斥锁属性，默认属性为快速互斥锁 。
    pthread_mutex_init (&gDevsListMutex, NULL);	//-函数成功执行后，互斥锁被初始化为未锁住态。

    // init the list of joined devices
    memset(gatSavedDevList, 0, sizeof(gatSavedDevList));

    //init the default para of read attribure req
    gtDefaultParaOfSendReq.u8LocalEp = 1;
    gtDefaultParaOfSendReq.u8RemoteEp = 1;
    gtDefaultParaOfSendReq.u16ClustId = E_ZB_CLUSTERID_BASIC;
    gtDefaultParaOfSendReq.u16AttrId = 0; // zcl version atribute of basic cluster

    //list of tx cmd pending ack
    gpTxCmdPendingAckList = NULL;
}

teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo)
{
    teSL_Status eStatus;
    int ret;

    pthread_mutex_lock(&gSeialMsgSendMutex);	//-上锁,如果已锁将阻塞等待
    clear_txmsg_status();	//-清除发送标志位的状态
    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8_t *)pvMessage);
    if (eStatus == E_SL_OK)
    {
        ret = check_txmsg_status(u16Type, 100);	//-检查发送标志位的状态,确保对方已经收到,通过延时等待实现的,另一个线程在接收处理
        if (ret == 0)
        {//-等到了需要的状态,即正确的
            // get the zcl seq num
            if(pu8SequenceNo)
                *pu8SequenceNo = gTxMsgStatus.u8SequenceNo;

            eStatus = E_SL_OK;
        }
        else
        {
            eStatus = E_SL_ERROR;
        }
    }
    else
    {
        printf("\n\t eSL_WriteMessage fail (%d)", eStatus);        
    } 

    pthread_mutex_unlock(&gSeialMsgSendMutex);
    return eStatus;
}

static teSL_Status SendIPNConfigure(tsIPNConfigReq *ptsIPNCfg)
{
  teSL_Status eStatus;
  tsIPNConfigReq sIpnConfigReq =
  {
      .bEnable                = 1,
      .u32RfActiveOutDioMask  = htonl(gu32RfActiveOutDioMask),
      .u32StatusOutDioMask    = htonl(gu32StatusOutDioMask),
      .u32TxConfInDioMask 	  = htonl(gu32TxConfInDioMask),
      .bRegisterCallback 	  = 0,
      .u16PollPeriod	      = htons(gu16PollPeriod),
      .u8TimerId	          = 4,
  };

  // sleep 200ms before issue cmd to make sure ZCB has finished stack initialization
  IOT_MSLEEP(200);

  printf("\tIPN config is: RF_request_pin=0x%X, status_pin=0x%X, grant_pin=0x%X, poll_period=0x%X\n",    
             gu32RfActiveOutDioMask, gu32StatusOutDioMask, gu32TxConfInDioMask, gu16PollPeriod);
  
  eStatus = eSL_SendMessage(E_SL_MSG_AHI_IPN_CONFIGURE,
                            sizeof(tsIPNConfigReq), &sIpnConfigReq, NULL);
  return eStatus;
}

// return 1 if quit command is received, otherwise return 0
int input_cmd_handler(void)
{
    #define MAX_CMD_LEN 16 // max length of cmd string, not including the following para seperated by space
    int i, j, cmd_start_flag, cmd_end_flag, cmd_para_start_index;
    int ret;
    char inputBuf[256];
    char cmd_str[MAX_CMD_LEN];

    // read cmd from standard input
    memset(inputBuf, 0, sizeof(inputBuf));
    memset(cmd_str, 0, sizeof(cmd_str));
    i = 0;
    j = 0;
    cmd_start_flag = 0;
    cmd_end_flag = 0;
    while(1)	//-阻塞等待命令,直到一个命令成功接收后返回
    {
    	//-当程序调用getchar时.程序就等着用户按键.用户输入的字符被存放在键盘缓冲
    	//-区中.直到用户按回车为止（回车字符也放在缓冲区中）.当用户键入回车之后，
    	//-getchar才开始从stdio流中每次读入一个字符.getchar函数的返回值是用户输
    	//-入的字符的ASCII码，如出错返回-1，且将用户输入的字符回显到屏幕.如用户在
    	//-按回车之前输入了不止一个字符，其他字符会保留在键盘缓存区中，等待后续
    	//-getchar调用读取.也就是说，后续的getchar调用不会等待用户按键，而直接读
    	//-取缓冲区中的字符，直到缓冲区中的字符读完为后，才等待用户按键.      
      inputBuf[i] = getchar();	//-从stdio流中读字符，相当于getc(stdin），它从标准输入里读取下一个字符。
	  if(inputBuf[i] != ' ')
          cmd_start_flag = 1;
      else
      {
          if(cmd_start_flag)
          {
              if(!cmd_end_flag)
                  cmd_para_start_index = i + 1;

              cmd_end_flag =1;               
          }
      }

      if((cmd_start_flag) && (! cmd_end_flag) && (j < MAX_CMD_LEN))
      {
          if ((inputBuf[i] != ' ') && (inputBuf[i] != '\n'))
              cmd_str[j++] = inputBuf[i];
      }
      
      if(inputBuf[i] == '\n')
      {
          inputBuf[i] = 0;
          break;
      }
      
      i++;
    }

    // check input command and do accordingly
    //printf("\ncmd=%s, len=%d, para index=%d, \n", cmd_str, strlen(cmd_str), cmd_para_start_index);
    if (strcmp(cmd_str, HELP_CMD) == 0)
    {
        show_help();
    }
    else if (strcmp(cmd_str, RESET_CMD) == 0)
    {
        // send reset cmd to ZCB
        eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL);        
    }
    else if (strcmp(cmd_str, ERASEPDM_CMD) == 0)
    {
        // send erase PDM cmd to ZCB
        eSL_SendMessage(E_SL_MSG_ERASE_PERSISTENT_DATA, 0, NULL, NULL);
    }
    else if (strcmp(cmd_str, SETDEVTYPE_CMD) == 0)
    {
        int devicetype;
        sscanf(inputBuf + cmd_para_start_index, "%d", &devicetype);
        printf("\tdevice type=%d\n", devicetype);
        eSetDeviceType(devicetype);        
    }
    else if (strcmp(cmd_str, SETCMSK_CMD) == 0)
    {
        int cmsk;
        sscanf(inputBuf + cmd_para_start_index, "0x%x", &cmsk);
        printf("\tchannel mask=0x%08X\n", cmsk);        
        eSetChannelMask(cmsk);
    }
    else if (strcmp(cmd_str, STARTNWK_CMD) == 0)
    {
		eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL);
    }
    else if (strcmp(cmd_str, PERMITJOIN_CMD) == 0)
    {
        int duration;
        sscanf(inputBuf + cmd_para_start_index, "%d", &duration);
        printf("\tpermit join duration=%d\n", duration);  
        eSetPermitJoining((uint8_t)duration);
    }
    else if (strcmp(cmd_str, DISCDEVS) == 0)
    {
        ClearAllDevsMgmtLqiReqSentFlag();
        tsManagementLQIRequest *psMgmtLQIReq = malloc(sizeof(tsManagementLQIRequest));        
        if (psMgmtLQIReq)
        {
            psMgmtLQIReq->u16TargetAddress = 0; // firstly send to coordinator
            psMgmtLQIReq->u8StartIndex = 0;       
            eSendMgmtLqiReq((void *)psMgmtLQIReq);
        }
        else
        {
            printf("\t***Allocate memory for mgmt lqi req failed.");
        }
    }
    else if (strcmp(cmd_str, LISTDEVS) == 0)
    {
        ShowDevicesInNwk();
    }
    else if (strcmp(cmd_str, BIND) == 0)
    {
        uint64_t u64IEEEAddress;
        uint64_t u64CoordIeeeAddr; 
        uint32_t u32ClusterId;
        sscanf(inputBuf + cmd_para_start_index, "0x%llx 0x%llx 0x%x", 
               (long long unsigned int *)&u64IEEEAddress, 
               (long long unsigned int *)&u64CoordIeeeAddr, 
               &u32ClusterId);
        printf("\tdstIeee=0x%llX, coordIeee=0x%llX, clusterId=0x%X\n", 
                (long long unsigned int)u64IEEEAddress, 
                (long long unsigned int)u64CoordIeeeAddr, 
                u32ClusterId);
        
        eZCB_SendBindCommand(u64IEEEAddress, u64CoordIeeeAddr, (uint16_t)u32ClusterId);
    }
    else if (strcmp(cmd_str, CONFIGREPORT) == 0)
    {
        int DstShortAddr, ClusterId, AttrId, AttrType, MinInterval, MaxInterval;
        sscanf(inputBuf + cmd_para_start_index, "0x%x 0x%x 0x%x 0x%x %d %d", 
               &DstShortAddr,
               &ClusterId,
               &AttrId,
               &AttrType,
               &MinInterval,
               &MaxInterval);
        printf("\tConfig attr report, addr=0x%04X, clusterId=0x%04X, attrId=0x%04X, attrType=0x%02X, minInt=%d, maxInt=%d\n",
			   (uint16_t)DstShortAddr,
			   (uint16_t)ClusterId,
			   (uint16_t)AttrId,
			   (uint8_t)AttrType,
			   (uint16_t)MinInterval,
			   (uint16_t)MaxInterval);

        eZCB_SendConfigureReportingCommand((uint16_t)DstShortAddr,
			                               (uint16_t)ClusterId,
                   			               (uint16_t)AttrId,
                            			   (uint8_t)AttrType,
                            			   (uint16_t)MinInterval,
                                           (uint16_t)MaxInterval);
    }
    else if (strcmp(cmd_str, READATTR) == 0)
    {
        int			DstShortAddr;
        sscanf(inputBuf + cmd_para_start_index, "0x%x", &DstShortAddr);
        printf("\tDst short addr=0x%04X\n", DstShortAddr);          
        ZDReadAttrReq(
                     (uint16_t)DstShortAddr,
                     gtDefaultParaOfSendReq.u8LocalEp,
                     gtDefaultParaOfSendReq.u8RemoteEp,
                     gtDefaultParaOfSendReq.u16ClustId,
                     1,
                     &gtDefaultParaOfSendReq.u16AttrId,
                     NULL);
    }
    else if (strcmp(cmd_str, TOGGLE) == 0)
    {
        int			DstShortAddr;
        sscanf(inputBuf + cmd_para_start_index, "0x%x", &DstShortAddr);
        printf("\tDst short addr=0x%04X\n", DstShortAddr);
        gu8CheckDefRespFlag = 0;
        ZDOnOffCmd((uint16_t)DstShortAddr, 
                    gtDefaultParaOfSendReq.u8LocalEp,
                    gtDefaultParaOfSendReq.u8RemoteEp,
                    E_CLD_ONOFF_CMD_TOGGLE,
                    NULL);
    }
    else if (strcmp(cmd_str, TOGGLETEST) == 0)
    {
        int DstShortAddr, TxTimes, TxInterval;
        uint8_t u8ZclSeqNum;

        sscanf(inputBuf + cmd_para_start_index, "0x%x %d %d", &DstShortAddr, &TxTimes, &TxInterval);
        printf("\tdstAddr=0x%04X, TxTimes=%d, TxInterval=%d\n", DstShortAddr, TxTimes, TxInterval);
        gu32NumOfTxCmd = 0;
        gu32NumOfAckedTxCmd = 0;
        gu8CheckDefRespFlag = 1;
        for (i = 0; i < TxTimes; i ++)
        {
            printf("\tSending toggle cmd #%d (%d acked) ... ", i + 1, gu32NumOfAckedTxCmd);
            ret = ZDOnOffCmd((uint16_t)DstShortAddr, 
                    gtDefaultParaOfSendReq.u8LocalEp,
                    gtDefaultParaOfSendReq.u8RemoteEp,
                    E_CLD_ONOFF_CMD_TOGGLE,
                    &u8ZclSeqNum);
                    
            if (ret != E_SL_OK)
            {
                printf("\t***send pkt failed.");
            }

            printf("done.\n");
            fflush(stdout);
            // add current tx transaction into pending ack list
            AddTxCmdToPendingAckList((uint16_t)DstShortAddr, u8ZclSeqNum);

            IOT_MSLEEP(TxInterval);            
        }

        // display testing result after 5 seconds
        IOT_SLEEP(5);
        gu8CheckDefRespFlag = 0;
        DisplayAndFreeTxCmdPendingAckList();        
    }
    else if (strcmp(cmd_str, SETTXPLEVEL) == 0)
    {
        int level;
        sscanf(inputBuf + cmd_para_start_index, "%d", &level);
        printf("\tTx priority level=%d\n", level);
        eSetTxPriorityLevel((uint8_t)level);         
    }
    else if (strcmp(cmd_str, SETRXPLEVEL) == 0)
    {
        int level;
        sscanf(inputBuf + cmd_para_start_index, "%d", &level);
        printf("\tRx priority =%s\n", level? "Enable":"Disable");
        eSetRxPriorityLevel((uint8_t)level);         
    }
    else if (strcmp(cmd_str, TXRXTEST) == 0)
    {
        int DstShortAddr, TxTimes, TxInterval;
        uint8_t u8ZclSeqNum;

        sscanf(inputBuf + cmd_para_start_index, "0x%x %d %d", &DstShortAddr, &TxTimes, &TxInterval);
        printf("\tdstAddr=0x%04X, TxTimes=%d, TxInterval=%d\n", DstShortAddr, TxTimes, TxInterval);
        gu32NumOfTxCmd = 0;
        gu32NumOfAckedTxCmd = 0;
        for (i = 0; i < TxTimes; i ++)
        {
            printf("\tSending read attr cmd #%d (%d acked) ... ", i + 1, gu32NumOfAckedTxCmd);
            ret = ZDReadAttrReq(
                               (uint16_t)DstShortAddr,
                               gtDefaultParaOfSendReq.u8LocalEp,
                               gtDefaultParaOfSendReq.u8RemoteEp,
                               gtDefaultParaOfSendReq.u16ClustId,
                               1,
                               &gtDefaultParaOfSendReq.u16AttrId,
                               &u8ZclSeqNum);
            if (ret != E_SL_OK)
            {
                printf("\t***send pkt failed.");                
            }

            printf("done.\n");
            fflush(stdout);
            // add current tx transaction into pending ack list
            AddTxCmdToPendingAckList((uint16_t)DstShortAddr, u8ZclSeqNum);

            IOT_MSLEEP(TxInterval);            
        }

        // display testing result after 5 seconds
        IOT_SLEEP(5);
        DisplayAndFreeTxCmdPendingAckList();        
    }
    else if (strcmp(cmd_str, QUIT_CMD) == 0)
    {
        return 1;
    }
    else if (strcmp(cmd_str, GETVERSION_CMD) == 0)
    {
    		//-send GetVersion command to ZCB
    		eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL);
    }
    else if (strcmp(cmd_str, WRITEATTR) == 0)
    {
    		int ZCL_MANUFACTURER_CODE = 0x1037;  // NXP
    		int			DstShortAddr,ClusterId,u8Direction,AttrId,AttrType;
    		//-char	 AttrDate_buf[16];
    		uint64_t	AttrDate;
    		sscanf(inputBuf + cmd_para_start_index, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%llx", 
               &DstShortAddr,
               &ClusterId,
               &u8Direction,
               &AttrId,
               &AttrType,
               &AttrDate);
        //-uint64_t	AttrDate = atoi(AttrDate_buf);        
        printf("\tdstAddr=0x%04X, ClusterId=0x%04X, u8Direction=0x%02X, AttrId=0x%04X, AttrType=0x%02X, AttrDate=0x%llx, \n", DstShortAddr, ClusterId, u8Direction, AttrId, AttrType, AttrDate);       
	    	teZcbStatus eStatus = eZCB_WriteAttributeRequest(
			  (uint16_t)DstShortAddr,                                       // ShortAddress
			  (uint16_t)ClusterId,                        // Cluster ID
			  (uint8_t)u8Direction,                                                     // Direction
			  1,                                                     // Manufacturer Specific
			  ZCL_MANUFACTURER_CODE,                                 // Manufacturer ID
			  (uint16_t)AttrId,  // Attr ID
			  (teZCL_ZCLAttributeType)AttrType,                                          // eType
			  (void *)&AttrDate );                          // &data
								    
		    if (E_ZCB_OK != eStatus)
		    {
		        DEBUG_PRINTF("eZCB_WriteAttributeRequest returned status %d\n", eStatus);
		    }
    }	
    else if (strcmp(cmd_str, "") != 0)
    {
        printf("\tUnkown cmd.\n");
    }
    
    return 0;
}

void *pvSerialReaderThread(void *p)	//-一个全新的线程处理函数
{
    tsSL_Message  sMessage;
    tsSL_Msg_Status *psMsgStatus;

    while (1)	//-周期性的读数据,直到有为止
    {
        /* Initialise buffer */
        memset(&sMessage, 0, sizeof(tsSL_Message));
        /* Initialise length to large value so CRC is skipped if end received */
        sMessage.u16Length = 0xFFFF;
        
        if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message) == E_SL_OK)
        {//-下面的处理是针对接收到的完好正确报文进行的
           
            if (verbosity >= 10)	//-调试信息的输出控制
            {
                char acBuffer[4096];
                int iPosition = 0, i;
                
                iPosition = sprintf(&acBuffer[iPosition], "Node->Host 0x%04X (Length % 4d)", sMessage.u16Type, sMessage.u16Length);
                for (i = 0; i < sMessage.u16Length; i++)
                {
                    iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", sMessage.au8Message[i]);
                }
                printf( "\n\t%s", acBuffer);	//-\t 的意思是 横向跳到下一制表符位置
            }

            switch (sMessage.u16Type)
            {
                case E_SL_MSG_LOG:
                    /* Log messages handled here first, and passsed to new thread in case user has added another handler */
                    sMessage.au8Message[sMessage.u16Length] = '\0';
                    uint8_t u8LogLevel = sMessage.au8Message[0];
                    char *pcMessage = (char *)&sMessage.au8Message[1];
                    printf( "\n\t<ZCB log, level %d>: %s\n", u8LogLevel, pcMessage);    
                    break;

                case E_SL_MSG_STATUS:
                    psMsgStatus = (tsSL_Msg_Status *)sMessage.au8Message;
                    set_txmsg_status(psMsgStatus);	//-根据接收到的报文进行标志位的设置
                    /* add some tips on execute result of some cmds */
                    //printf("\n msg status for 0x%04X ", htons(psMsgStatus->u16MessageType));
                    if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_ERASE_PERSISTENT_DATA)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tErase PDM sucessfully.\n");
                        else
                            printf("\t*** Erase PDM fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_SET_DEVICETYPE)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tSet device type sucessfully.\n");
                        else
                            printf("\t*** Set device type fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_SET_CHANNELMASK)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tSet channel mask sucessfully.\n");
                        else
                            printf("\t*** Set channel mask fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_PERMIT_JOINING_REQUEST)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tPermit join sucessfully.\n");
                        else
                            printf("\t*** Permit join fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_AHI_IPN_CONFIGURE)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tIPN config sucessfully.\n");
                        else
                            printf("\t*** IPN config fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_AHI_IPN_SET_TX_PRIORITY_RETRY)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tTx priority level config sucessfully.\n");
                        else
                            printf("\t*** Tx priority level config fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_AHI_IPN_SET_RX_PRIORITY)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tRx priority config sucessfully.\n");
                        else
                            printf("\t*** Rx priority config fail, status=%d.\n", psMsgStatus->eStatus);
                    }
                    else if (ntohs(psMsgStatus->u16MessageType) == E_SL_MSG_CONFIG_REPORTING_REQUEST)
                    {
                        if (psMsgStatus->eStatus == E_SL_MSG_STATUS_SUCCESS)
                            printf("\tConfig attr report cmd sucessfully.\n");
                        else
                            printf("\t*** Config attr report cmd fail, status=%d.\n", psMsgStatus->eStatus);
                    }

                    
                    break;
                    
                case E_SL_MSG_RESTART_FACTORY_NEW:
                    printf("\n\tZCB is in factory new state, need to form nwk before testing.\n");
                    CreateDetachedThread((void *)SendIPNConfigure, NULL);
                    break;
                    
                case E_SL_MSG_NON_RESTART_FACTORY_NEW:
                    printf("\n\tZCB Nwk already formed.\n");
                    CreateDetachedThread((void *)SendIPNConfigure, NULL);
                    break;

                case E_SL_MSG_NETWORK_JOINED_FORMED:
                    ZCB_HandleNetworkJoinedFormed(sMessage.au8Message);
                    break;
                    
                case E_SL_MSG_DEVICE_ANNOUNCE:
                    ZCB_HandleDeviceAnnounceMsg((void *)sMessage.au8Message);
                    break;

                case E_SL_MSG_MANAGEMENT_LQI_RESPONSE:
                    {
                        tsManagementLQIResponse *ptsMgmtLqiRsp = malloc(sizeof(tsManagementLQIResponse));
                        if(ptsMgmtLqiRsp)
                        {
                            memcpy(ptsMgmtLqiRsp, sMessage.au8Message, sizeof(tsManagementLQIResponse));
                            CreateDetachedThread((void *)ZCB_HandleMgmtLqiRsp, (void *)ptsMgmtLqiRsp);
                        }
                        else
                        {
                            printf("\n\tAllocate memory for mgmt lqi rsp failed.\n");
                        }                    
                        break;
                    }
                    
                case E_SL_MSG_READ_ATTRIBUTE_RESPONSE:
                    {
                        void *pAttrReadRsp = malloc(sMessage.u16Length);
                        if (pAttrReadRsp)
                        {
                            memcpy(pAttrReadRsp, sMessage.au8Message, sMessage.u16Length);
                            CreateDetachedThread((void *)ZCB_HandleReadAttrResp, pAttrReadRsp);
                        }
                        else
                        {
                            printf("\n\tAllocate memory for read attr rsp failed.");
                        }
                    }
                    break;
                case E_SL_MSG_ATTRIBUTE_REPORT:
                    ZCB_HandleReceivedAttrReport(sMessage.au8Message);
                    break;
                
                case E_SL_MSG_DEFAULT_RESPONSE:
                    {
                        void *pDefRsp = malloc(sMessage.u16Length);
                        if (pDefRsp)
                        {
                            memcpy(pDefRsp, sMessage.au8Message, sMessage.u16Length);
                            CreateDetachedThread((void *)ZCB_HandleDefaultResponse, pDefRsp);
                        }
                        else
                        {
                            printf("\n\tAllocate memory for default rsp failed.");
                        }
                    }
                    break;

                case E_SL_MSG_BIND_RESPONSE:
                    {
                        tsBindUnbindRsp *psBindRsp = (tsBindUnbindRsp *)sMessage.au8Message;
                        if(psBindRsp->u8Status == E_ZCB_OK)
                            printf("\n\tBind sucessfully.");
                        else
                            printf("\n\tBind failed, status=%d.", psBindRsp->u8Status);
                    }
                    break;

                case E_SL_MSG_CONFIG_REPORTING_RESPONSE:
                    {
                        tsConfigAttrReportRsp *psCfgAttrReportRsp = (tsConfigAttrReportRsp *)sMessage.au8Message;
                        if(psCfgAttrReportRsp->u8Status == E_ZCB_OK)
                            printf("\n\tConfig attr report sucessfully.");
                        else
                            printf("\n\tConfig attr report failed, status=%d.", psCfgAttrReportRsp->u8Status);                        
                    }
                    break;
                
                default:
                    break;
            }

            fflush(stdout);
        }
        else
        {
            printf("\n\t***eSL_ReadMessage failed.\n");
        }
    }

    printf("Exit reader thread\n");
    
    return NULL;
}

