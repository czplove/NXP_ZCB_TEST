/****************************************************************************
*
* MODULE:             SerialLink
*
* COMPONENT:          $RCSfile: SerialLink.c,v $
*
* REVISION:           $Revision: 43420 $
*
* DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
*
* AUTHOR:             Lee Mitchell
*
****************************************************************************
*
* This software is owned by NXP B.V. and/or its supplier and is protected
* under applicable copyright laws. All rights are reserved. We grant You,
* and any third parties, a license to use this software solely and
* exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
* You, and any third parties must reproduce the copyright and warranty notice
* and any other legend of ownership on each copy or partial copy of the 
* software.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.

* Copyright NXP B.V. 2012. All rights reserved
*
***************************************************************************/

/** \addtogroup zb
 * \file
 * \brief Serial Link layer
 */

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "SerialLink.h"
#include "Serial.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if DEBUG_SERIALLINK
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* DEBUG_SERIALLINK */

#define DBG_vPrintf(a,b,ARGS...) do {  if (a) printf("%s: " b, __FUNCTION__, ## ARGS); } while(0)
#define DBG_vAssert(a,b) do {  if (a && !(b)) printf(__FILE__ " %d : Asset Failed\n", __LINE__ ); } while(0)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    FALSE,
    TRUE,    
} bool;

typedef enum
{
    E_STATE_RX_WAIT_START,
    E_STATE_RX_WAIT_TYPEMSB,
    E_STATE_RX_WAIT_TYPELSB,
    E_STATE_RX_WAIT_LENMSB,
    E_STATE_RX_WAIT_LENLSB,
    E_STATE_RX_WAIT_CRC,
    E_STATE_RX_WAIT_DATA,
} teSL_RxState;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data);

static int iSL_TxByte(bool bSpecialCharacter, uint8_t u8Data);

static bool bSL_RxByte(uint8_t *pu8Data);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern int verbosity;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
//-读串口数据,如果没有立即返回,如果有那么进行最底层的转译,校验等,把最终报文进行返回
teSL_Status eSL_ReadMessage(uint16_t *pu16Type, uint16_t *pu16Length, uint16_t u16MaxLength, uint8_t *pu8Message)
{

    static teSL_RxState eRxState = E_STATE_RX_WAIT_START;
    static uint8_t u8CRC;
    uint8_t u8Data;
    static uint16_t u16Bytes;
    static bool bInEsc = FALSE;

    while(bSL_RxByte(&u8Data))	//-这里是目前整个程序唯一读一个字节的地方
    {
        DBG_vPrintf(DBG_SERIALLINK_COMMS, "0x%02x\n", u8Data);
        switch(u8Data)
        {

        case SL_START_CHAR:	//-读到了开始标志
            u16Bytes = 0;
            bInEsc = FALSE;
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "RX Start\n");
            eRxState = E_STATE_RX_WAIT_TYPEMSB;
            break;

        case SL_ESC_CHAR:
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "Got ESC\n");
            bInEsc = TRUE;
            break;

        case SL_END_CHAR:
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "Got END\n");
            
            if(*pu16Length > u16MaxLength)
            {
                /* Sanity check length before attempting to CRC the message */
                DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length > MaxLength\n");
                eRxState = E_STATE_RX_WAIT_START;
                break;
            }
            
            if(u8CRC == u8SL_CalculateCRC(*pu16Type, *pu16Length, pu8Message))
            {
#if DBG_SERIALLINK
                int i;
                DBG_vPrintf(DBG_SERIALLINK, "RX Message type 0x%04x length %d: { ", *pu16Type, *pu16Length);
                for (i = 0; i < *pu16Length; i++)
                {
                    printf("0x%02x ", pu8Message[i]);
                }
                printf("}\n");
#endif /* DBG_SERIALLINK */
                
                eRxState = E_STATE_RX_WAIT_START;
                return E_SL_OK;
            }
            DBG_vPrintf(DBG_SERIALLINK_COMMS, "CRC BAD\n");
            break;

        default:
            if(bInEsc)
            {
                u8Data ^= 0x10;
                bInEsc = FALSE;
            }

            switch(eRxState)
            {

                case E_STATE_RX_WAIT_START:
                    break;
                    

                case E_STATE_RX_WAIT_TYPEMSB:
                    *pu16Type = (uint16_t)u8Data << 8;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_TYPELSB:
                    *pu16Type += (uint16_t)u8Data;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_LENMSB:
                    *pu16Length = (uint16_t)u8Data << 8;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_LENLSB:
                    *pu16Length += (uint16_t)u8Data;
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length %d\n", *pu16Length);
                    if(*pu16Length > u16MaxLength)
                    {
                        DBG_vPrintf(DBG_SERIALLINK_COMMS, "Length > MaxLength\n");
                        eRxState = E_STATE_RX_WAIT_START;
                    }
                    else
                    {
                        eRxState++;
                    }
                    break;

                case E_STATE_RX_WAIT_CRC:
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "CRC %02x\n", u8Data);
                    u8CRC = u8Data;
                    eRxState++;
                    break;

                case E_STATE_RX_WAIT_DATA:
                    if(u16Bytes < *pu16Length)
                    {
                        DBG_vPrintf(DBG_SERIALLINK_COMMS, "Data\n");
                        pu8Message[u16Bytes++] = u8Data;
                    }
                    break;

                default:
                    DBG_vPrintf(DBG_SERIALLINK_COMMS, "Unknown state\n");
                    eRxState = E_STATE_RX_WAIT_START;
            }
            break;

        }

    }

    return E_SL_NOMESSAGE;
}


/****************************************************************************
*
* NAME: vSL_WriteRawMessage
*
* DESCRIPTION:
*
* PARAMETERS: Name        RW  Usage
*
* RETURNS:
* void
****************************************************************************/
teSL_Status eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)	//-实现数据的发送,会自动对有效数据组织成最终报文
{
    int n;
    uint8_t u8CRC;

    u8CRC = u8SL_CalculateCRC(u16Type, u16Length, pu8Data);	//-计算CRC数值,知道结果即可不需要深究

    //printf("\neSL_WriteMessage (0x%04X, %d, %02x)\n", u16Type, u16Length, u8CRC);

    if (verbosity >= 10)	//-log信息输出的控制
    {
        char acBuffer[4096];
        int iPosition = 0, i;
        
        iPosition = sprintf(&acBuffer[iPosition], "Host->Node 0x%04X (Length % 4d)", u16Type, u16Length);
        for (i = 0; i < u16Length; i++)
        {
            iPosition += sprintf(&acBuffer[iPosition], " 0x%02X ", pu8Data[i]);
        }
        printf( "%s\n", acBuffer);
    }
    //-这里开始对原始数据进行发送,转译不用应用层考虑
    /* Send start character */
    if (iSL_TxByte(TRUE, SL_START_CHAR) < 0) return E_SL_ERROR;

    /* Send message type */
    if (iSL_TxByte(FALSE, (u16Type >> 8) & 0xff) < 0) return E_SL_ERROR;
    if (iSL_TxByte(FALSE, (u16Type >> 0) & 0xff) < 0) return E_SL_ERROR;

    /* Send message length */
    if (iSL_TxByte(FALSE, (u16Length >> 8) & 0xff) < 0) return E_SL_ERROR;
    if (iSL_TxByte(FALSE, (u16Length >> 0) & 0xff) < 0) return E_SL_ERROR;

    /* Send message checksum */
    if (iSL_TxByte(FALSE, u8CRC) < 0) return E_SL_ERROR;

    /* Send message payload */  
    for(n = 0; n < u16Length; n++)
    {       
        if (iSL_TxByte(FALSE, pu8Data[n]) < 0) return E_SL_ERROR;
    }

    /* Send end character */
    if (iSL_TxByte(TRUE, SL_END_CHAR) < 0) return E_SL_ERROR;

    return E_SL_OK;
}

static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{
    int n;
    uint8_t u8CRC = 0;

    u8CRC ^= (u16Type >> 8) & 0xff;
    u8CRC ^= (u16Type >> 0) & 0xff;
    
    u8CRC ^= (u16Length >> 8) & 0xff;
    u8CRC ^= (u16Length >> 0) & 0xff;

    for(n = 0; n < u16Length; n++)
    {
        u8CRC ^= pu8Data[n];
    }
    return(u8CRC);
}

/****************************************************************************
*
* NAME: vSL_TxByte
*
* DESCRIPTION:
*
* PARAMETERS:  Name                RW  Usage
*								bSpecialCharacter		表示是否是特殊符号,是就不需要转化
* RETURNS:
* void
****************************************************************************/
static int iSL_TxByte(bool bSpecialCharacter, uint8_t u8Data)	//-原始数据一个字节一个字节的发送,如果需要的话会自动转译,所以上层不用考虑转译
{
    if(!bSpecialCharacter && (u8Data < 0x10))
    {
        u8Data ^= 0x10;

        if (eSerial_Write(SL_ESC_CHAR) != E_SERIAL_OK) return -1;
        //DBG_vPrintf(DBG_SERIALLINK_COMMS, " 0x%02x", SL_ESC_CHAR);
    }
    //DBG_vPrintf(DBG_SERIALLINK_COMMS, " 0x%02x", u8Data);

    return eSerial_Write(u8Data);
}


/****************************************************************************
*
* NAME: bSL_RxByte
*
* DESCRIPTION:
*
* PARAMETERS:  Name                RW  Usage
*
* RETURNS:
* void
****************************************************************************/
static bool bSL_RxByte(uint8_t *pu8Data)
{
    if (eSerial_Read(pu8Data) == E_SERIAL_OK)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
