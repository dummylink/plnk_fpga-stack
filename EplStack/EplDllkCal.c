/****************************************************************************

  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com

  Project:      openPOWERLINK

  Description:  source file for kernel DLL Communication Abstraction Layer module

  License:

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    3. Neither the name of SYSTEC electronic GmbH nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without prior written permission. For written
       permission, please contact info@systec-electronic.com.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    Severability Clause:

        If a provision of this License is or becomes illegal, invalid or
        unenforceable in any jurisdiction, that shall not affect:
        1. the validity or enforceability in that jurisdiction of any other
           provision of this License; or
        2. the validity or enforceability in other jurisdictions of that or
           any other provision of this License.

  -------------------------------------------------------------------------

                $RCSfile$

                $Author$

                $Revision$  $Date$

                $State$

                Build Environment:
                    GCC V3.4

  -------------------------------------------------------------------------

  Revision History:

  2006/06/15 d.k.:   start of the implementation, version 1.00

****************************************************************************/

#include "kernel/EplDllkCal.h"
#include "kernel/EplDllk.h"
#include "kernel/EplEventk.h"
#include "user/EplSdoAsySequ.h"

#include "EplDllCal.h"
#if EPL_USE_SHAREDBUFF != FALSE
#include "SharedBuff.h"
#endif

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)
/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#if (EPL_DLL_PRES_CHAINING_MN != FALSE) \
    && (EPL_USE_SHAREDBUFF == FALSE)
#error "DLLCal module needs SharedBuffer for PRC MN"
#endif


//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// module global vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------


/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          C L A S S  EplDllkCal                                          */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Description:
//
//
/***************************************************************************/


//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#if EPL_DLL_PRES_CHAINING_MN != FALSE
#define EPL_DLLKCAL_MAX_QUEUES  6   // CnGenReq, CnNmtReq, {MnGenReq, MnNmtReq}, MnIdentReq, MnStatusReq, SyncReq
#else
#define EPL_DLLKCAL_MAX_QUEUES  5   // CnGenReq, CnNmtReq, {MnGenReq, MnNmtReq}, MnIdentReq, MnStatusReq
#endif

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

typedef struct
{
#if EPL_USE_SHAREDBUFF != FALSE
//    tShbInstance    m_ShbInstanceRx;        // FIFO for Rx ASnd frames
    tShbInstance              m_ShbInstanceTxNmt;       // FIFO for Tx frames with NMT request priority
    tShbInstance              m_ShbInstanceTxGenAsnd;   // FIFO for Asnd Tx frames with generic priority
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    tShbInstance              m_ShbInstanceTxGenVeth;   // FIFO for Veth Tx frames with generic priority

    tEplDllAsyncBufferNumber  m_ShbCurrentBuffer;       // Switch between Asnd and Veth Tx buffer
#endif
#else
    unsigned int    m_uiFrameSizeNmt;
    BYTE            m_abFrameNmt[1500];
    unsigned int    m_uiFrameSizeGen;
    BYTE            m_abFrameGen[1500];
#endif

    tEplDllkCalStatistics   m_Statistics;

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)
    // IdentRequest queue with CN node IDs
    unsigned int    m_auiQueueIdentReq[EPL_D_NMT_MaxCNNumber_U8 + 1];   // 1 entry is reserved to distinguish between full and empty
    volatile unsigned int    m_uiWriteIdentReq;
    volatile unsigned int    m_uiReadIdentReq;

    // StatusRequest queue with CN node IDs
    unsigned int    m_auiQueueStatusReq[EPL_D_NMT_MaxCNNumber_U8 + 1];  // 1 entry is reserved to distinguish between full and empty
    volatile unsigned int    m_uiWriteStatusReq;
    volatile unsigned int    m_uiReadStatusReq;

    unsigned int    m_auiQueueCnRequests[254 * 2];
        // first 254 entries represent the generic requests of the corresponding node
        // second 254 entries represent the NMT requests of the corresponding node
    unsigned int    m_uiNextQueueCnRequest;
    unsigned int    m_uiNextRequestQueue;

#if EPL_DLL_PRES_CHAINING_MN != FALSE
    tShbInstance    m_ShbInstanceTxSync;   // FIFO for SyncRequests
#endif

#endif

} tEplDllkCalInstance;

//---------------------------------------------------------------------------
// local vars
//---------------------------------------------------------------------------

// if no dynamic memory allocation shall be used
// define structures statically
static tEplDllkCalInstance     EplDllkCalInstance_g;

// SDO flow control mechanism prevents event posting (workaround)
static WORD wAsycSdoRxCnt_l = 0;

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------


//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAddInstance()
//
// Description: add and initialize new instance of DLL CAL module
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAddInstance(void)
{
tEplKernel      Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError       ShbError;
unsigned int    fShbNewCreated;

    ShbError = ShbCirAllocBuffer (EPL_DLLCAL_BUFFER_SIZE_TX_NMT, EPL_DLLCAL_BUFFER_ID_TX_NMT,
        &EplDllkCalInstance_g.m_ShbInstanceTxNmt, &fShbNewCreated);
    // returns kShbOk, kShbOpenMismatch, kShbOutOfMem or kShbInvalidArg

    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }

    ShbError = ShbCirAllocBuffer (EPL_DLLCAL_BUFFER_SIZE_TX_GEN_ASND, EPL_DLLCAL_BUFFER_ID_TX_GEN_ASND,
        &EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, &fShbNewCreated);
    // returns kShbOk, kShbOpenMismatch, kShbOutOfMem or kShbInvalidArg

    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    ShbError = ShbCirAllocBuffer (EPL_DLLCAL_BUFFER_SIZE_TX_GEN_VETH, EPL_DLLCAL_BUFFER_ID_TX_GEN_VETH,
        &EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, &fShbNewCreated);
    // returns kShbOk, kShbOpenMismatch, kShbOutOfMem or kShbInvalidArg

    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }

    EplDllkCalInstance_g.m_ShbCurrentBuffer = kEplDllAsyncBuffAsnd;
#endif


#if EPL_DLL_PRES_CHAINING_MN != FALSE
    ShbError = ShbCirAllocBuffer (EPL_DLLCAL_BUFFER_SIZE_TX_SYNC, EPL_DLLCAL_BUFFER_ID_TX_SYNC,
        &EplDllkCalInstance_g.m_ShbInstanceTxSync, &fShbNewCreated);
    // returns kShbOk, kShbOpenMismatch, kShbOutOfMem or kShbInvalidArg

    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }
#endif

#else
    EplDllkCalInstance_g.m_uiFrameSizeNmt = 0;
    EplDllkCalInstance_g.m_uiFrameSizeGen = 0;
#endif

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalDelInstance()
//
// Description: deletes instance of DLL CAL module
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalDelInstance(void)
{
tEplKernel      Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError       ShbError;

    ShbError = ShbCirReleaseBuffer (EplDllkCalInstance_g.m_ShbInstanceTxNmt);
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }
    EplDllkCalInstance_g.m_ShbInstanceTxNmt = NULL;

    ShbError = ShbCirReleaseBuffer (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd);
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }
    EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd = NULL;

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    ShbError = ShbCirReleaseBuffer (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth);
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }
    EplDllkCalInstance_g.m_ShbInstanceTxGenVeth = NULL;
#endif

#if EPL_DLL_PRES_CHAINING_MN != FALSE
    ShbError = ShbCirReleaseBuffer (EplDllkCalInstance_g.m_ShbInstanceTxSync);
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
    }
    EplDllkCalInstance_g.m_ShbInstanceTxSync = NULL;
#endif

#else
    EplDllkCalInstance_g.m_uiFrameSizeNmt = 0;
    EplDllkCalInstance_g.m_uiFrameSizeGen = 0;
#endif

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalProcess
//
// Description: process the passed configuration
//
// Parameters:  pEvent_p                = event containing configuration options
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalProcess(tEplEvent * pEvent_p)
{
tEplKernel      Ret = kEplSuccessful;

    switch (pEvent_p->m_EventType)
    {
        case kEplEventTypeDllkServFilter:
        {
        tEplDllCalAsndServiceIdFilter*  pServFilter;

            pServFilter = (tEplDllCalAsndServiceIdFilter*) pEvent_p->m_pArg;
            Ret = EplDllkSetAsndServiceIdFilter(pServFilter->m_ServiceId, pServFilter->m_Filter);
            break;
        }

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)
        case kEplEventTypeDllkIssueReq:
        {
        tEplDllCalIssueRequest*  pIssueReq;

            pIssueReq = (tEplDllCalIssueRequest*) pEvent_p->m_pArg;
            Ret = EplDllkCalIssueRequest(pIssueReq->m_Service, pIssueReq->m_uiNodeId, pIssueReq->m_bSoaFlag1);
            break;
        }
#endif

#if EPL_NMT_MAX_NODE_ID > 0
        case kEplEventTypeDllkConfigNode:
        {
        tEplDllNodeInfo*    pNodeInfo;

            pNodeInfo = (tEplDllNodeInfo*) pEvent_p->m_pArg;
            Ret = EplDllkConfigNode(pNodeInfo);
            break;
        }

        case kEplEventTypeDllkAddNode:
        {
        tEplDllNodeOpParam*    pNodeOpParam;

            pNodeOpParam = (tEplDllNodeOpParam*) pEvent_p->m_pArg;
            Ret = EplDllkAddNode(pNodeOpParam);
            break;
        }

        case kEplEventTypeDllkDelNode:
        {
        tEplDllNodeOpParam*    pNodeOpParam;

            pNodeOpParam = (tEplDllNodeOpParam*) pEvent_p->m_pArg;
            Ret = EplDllkDeleteNode(pNodeOpParam);
            break;
        }
#endif // EPL_NMT_MAX_NODE_ID > 0

        case kEplEventTypeDllkIdentity:
        {
        tEplDllIdentParam*  pIdentParam;

            pIdentParam = (tEplDllIdentParam*) pEvent_p->m_pArg;
            if (pIdentParam->m_uiSizeOfStruct > pEvent_p->m_uiSize)
            {
                pIdentParam->m_uiSizeOfStruct = pEvent_p->m_uiSize;
            }
            Ret = EplDllkSetIdentity(pIdentParam);
            break;
        }

        case kEplEventTypeDllkConfig:
        {
        tEplDllConfigParam* pConfigParam;

            pConfigParam = (tEplDllConfigParam*) pEvent_p->m_pArg;
            if (pConfigParam->m_uiSizeOfStruct > pEvent_p->m_uiSize)
            {
                pConfigParam->m_uiSizeOfStruct = pEvent_p->m_uiSize;
            }
            Ret = EplDllkConfig(pConfigParam);
            break;
        }

        default:
        {
            Ret = kEplInvalidEvent;
            break;
        }
    }

//Exit:
    return Ret;
}


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncGetTxCount()
//
// Description: returns count of Tx frames of FIFO with highest priority
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncGetTxCount(tEplDllAsyncReqPriority * pPriority_p, unsigned int * puiCount_p)
{
tEplKernel  Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError       ShbError;
unsigned long   ulFrameCountNmt;
unsigned long   ulFrameCountAsnd;
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
  unsigned long   ulFrameCountVeth;
#endif

    // get frame count of Tx FIFO with NMT request priority
    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxNmt, &ulFrameCountNmt);
    // returns kShbOk, kShbInvalidArg

    // error handling
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
        goto Exit;
    }

    if (ulFrameCountNmt > EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountNmt)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountNmt = ulFrameCountNmt;
    }

    if (ulFrameCountNmt != 0)
    {   // NMT requests are in queue
        *pPriority_p = kEplDllAsyncReqPrioNmt;
        *puiCount_p = (unsigned int) ulFrameCountNmt;
        goto Exit;
    }

    // get frame count of Asnd Tx FIFO with generic priority
    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, &ulFrameCountAsnd);
    // returns kShbOk, kShbInvalidArg

    // error handling
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
        goto Exit;
    }

    if (ulFrameCountAsnd > EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGenAsnd)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGenAsnd = ulFrameCountAsnd;
    }

    *pPriority_p = kEplDllAsyncReqPrioGeneric;
    *puiCount_p = (unsigned int) ulFrameCountAsnd;

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    // get frame count of Veth Tx FIFO with generic priority
    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, &ulFrameCountVeth);
    // returns kShbOk, kShbInvalidArg

    // error handling
    if (ShbError != kShbOk)
    {
        Ret = kEplNoResource;
        goto Exit;
    }

    if (ulFrameCountVeth > EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGenVeth)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGenVeth = ulFrameCountVeth;
    }

    *pPriority_p = kEplDllAsyncReqPrioGeneric;
    // return the highest count of both fifo's
    *puiCount_p = (unsigned int) ((ulFrameCountVeth < ulFrameCountAsnd) ? ulFrameCountAsnd : ulFrameCountVeth);
#endif

Exit:
#else
    if (EplDllkCalInstance_g.m_uiFrameSizeNmt > 0)
    {
        *pPriority_p = kEplDllAsyncReqPrioNmt;
        *puiCount_p = 1;
    }
    else if (EplDllkCalInstance_g.m_uiFrameSizeGen > 0)
    {
        *pPriority_p = kEplDllAsyncReqPrioGeneric;
        *puiCount_p = 1;
    }
    else
    {
        *pPriority_p = kEplDllAsyncReqPrioGeneric;
        *puiCount_p = 0;
    }
#endif

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncGetTxFrame()
//
// Description: returns Tx frames from FIFO with specified priority
//
// Parameters:  pFrame_p                = IN: pointer to buffer
//              puiFrameSize_p          = IN: max size of buffer
//                                        OUT: actual size of frame
//              Priority_p              = IN: priority
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncGetTxFrame(void * pFrame_p, unsigned int * puiFrameSize_p, tEplDllAsyncReqPriority Priority_p)
{
tEplKernel      Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError       ShbError;
unsigned long   ulFrameSize;

    switch (Priority_p)
    {
        case kEplDllAsyncReqPrioNmt:    // NMT request priority
            ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxNmt, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
            // returns kShbOk, kShbDataTruncated, kShbInvalidArg, kShbNoReadableData
            break;

        default:    // generic priority
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) == 0)
            ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
            // returns kShbOk, kShbDataTruncated, kShbInvalidArg, kShbNoReadableData
#else
            switch(EplDllkCalInstance_g.m_ShbCurrentBuffer)
            {
                case kEplDllAsyncBuffAsnd:
                    ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
                    if(ShbError == kShbNoReadableData)
                    {   // nothing to read in Asnd queue -> try Veth frames!
                        ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
                    }
                    break;
                case kEplDllAsyncBuffVeth:
                    ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
                    if(ShbError == kShbNoReadableData)
                    {   // nothing to read in Veth queue -> try Asnd frames!
                        ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, (BYTE *) pFrame_p, *puiFrameSize_p, &ulFrameSize);
                    }
                    break;
                default:
                    ShbError = kShbBufferInvalid;
                    break;
            }
#endif
            break;
    }

    // error handling
    if (ShbError != kShbOk)
    {
        if (ShbError == kShbNoReadableData)
        {
            Ret = kEplDllAsyncTxBufferEmpty;
        }
        else
        {   // other error
            Ret = kEplNoResource;
        }
        goto Exit;
    }

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    EplDllkCalInstance_g.m_ShbCurrentBuffer = (EplDllkCalInstance_g.m_ShbCurrentBuffer == kEplDllAsyncBuffAsnd)
            ? kEplDllAsyncBuffVeth : kEplDllAsyncBuffAsnd;
#endif

    *puiFrameSize_p = (unsigned int) ulFrameSize;

Exit:
#else
    switch (Priority_p)
    {
        case kEplDllAsyncReqPrioNmt:    // NMT request priority
            *puiFrameSize_p = min(*puiFrameSize_p, EplDllkCalInstance_g.m_uiFrameSizeNmt);
            EPL_MEMCPY(pFrame_p, EplDllkCalInstance_g.m_abFrameNmt, *puiFrameSize_p);
            EplDllkCalInstance_g.m_uiFrameSizeNmt = 0;
            break;

        default:    // generic priority
            *puiFrameSize_p = min(*puiFrameSize_p, EplDllkCalInstance_g.m_uiFrameSizeGen);
            EPL_MEMCPY(pFrame_p, EplDllkCalInstance_g.m_abFrameGen, *puiFrameSize_p);
            EplDllkCalInstance_g.m_uiFrameSizeGen = 0;
            break;
    }

#endif

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncFrameReceived()
//
// Description: passes ASnd frame to receive FIFO.
//              It will be called only for frames with registered AsndServiceIds.
//
// Parameters:  none
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel EplDllkCalAsyncFrameReceived(tEplFrameInfo * pFrameInfo_p)
{
tEplKernel  Ret = kEplSuccessful;
tEplEvent   Event;
BYTE bAsndServiceId;

    // application has enabled sdo sequence flow control
    if (fEplSdoAsySequEnableRetrasmFlowCntrl_g)
    {
        // Get ASnd service ID
        bAsndServiceId = (BYTE) AmiGetByteFromLe(&pFrameInfo_p->m_pFrame->m_Data.m_Asnd.m_le_bServiceId);

        // prevent only SDO frames from being forwarded
        // this workaround is needed because otherwise the Shared Buffer will overflow if
        // a "retransmission request flow control" is issued by this node for every received SDO sequence frame
        if (bAsndServiceId == kEplDllAsndSdo)
        {
            // TODO: prevent forwarding of already received sequences ? -> timer necessary for sending "ACKs"
            // get SDO scon from received frame
            // bSdoSendSeqNum = (BYTE) AmiGetByteFromLe(&pFrameInfo_p->m_pFrame->m_Data.m_Asnd.m_Payload.m_SdoSequenceFrame.m_le_bSendSeqNumCon);

            // forward (and acknowledge) only every 3rd received sequence to reduce load
            wAsycSdoRxCnt_l++;
            if (wAsycSdoRxCnt_l % 3)
            {
                // do not post this event -> exit
                return Ret;
            }
        }
    }

    Event.m_EventSink = kEplEventSinkDlluCal;
    Event.m_EventType = kEplEventTypeAsndRx;
    Event.m_pArg = pFrameInfo_p->m_pFrame;
    Event.m_uiSize = pFrameInfo_p->m_uiFrameSize;

    Ret = EplEventkPost(&Event);
    if (Ret != kEplSuccessful)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulCurRxFrameCount++;
    }
    else
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxRxFrameCount++;
    }



    return Ret;
}


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncSend()
//
// Description: puts the given frame into the transmit FIFO with the specified
//              priority.
//
// Parameters:  pFrameInfo_p            = frame info structure
//              Priority_p              = priority
//              Buffer_p                = buffer to send (SDO or Veth)
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncSend(tEplFrameInfo * pFrameInfo_p, tEplDllAsyncReqPriority Priority_p, tEplDllAsyncBufferNumber Buffer_p)
{
tEplKernel  Ret = kEplSuccessful;
tEplEvent       Event;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError   ShbError;
tEplKernel  ShbRet = kEplSuccessful;

    switch (Priority_p)
    {
        case kEplDllAsyncReqPrioNmt:    // NMT request priority
            ShbError = ShbCirWriteDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxNmt, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
            // returns kShbOk, kShbExceedDataSizeLimit, kShbBufferFull, kShbInvalidArg
            break;

        default:    // generic priority
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) == 0)
            ShbError = ShbCirWriteDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
            // returns kShbOk, kShbExceedDataSizeLimit, kShbBufferFull, kShbInvalidArg
#else
            switch(Buffer_p)
            {
                case kEplDllAsyncBuffAsnd:    // Message is an Asnd frame
                    ShbError = ShbCirWriteDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
                    break;

                case kEplDllAsyncBuffVeth:    // Message is an Veth frame
                    ShbError = ShbCirWriteDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
                    break;

                default:
                    ShbError = kShbInvalidArg;
                    break;
            }
#endif
            break;

    }

    // error handling
    switch (ShbError)
    {
        case kShbOk:
            break;

        case kShbExceedDataSizeLimit:
            ShbRet = kEplDllAsyncTxBufferFull;
            break;

        case kShbBufferFull:
            ShbRet = kEplDllAsyncTxBufferFull;
            break;

        case kShbInvalidArg:
        default:
            ShbRet = kEplNoResource;
            break;
    }

#else

    switch (Priority_p)
    {
        case kEplDllAsyncReqPrioNmt:    // NMT request priority
            if (EplDllkCalInstance_g.m_uiFrameSizeNmt == 0)
            {
                EPL_MEMCPY(EplDllkCalInstance_g.m_abFrameNmt, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
                EplDllkCalInstance_g.m_uiFrameSizeNmt = pFrameInfo_p->m_uiFrameSize;
            }
            else
            {
                Ret = kEplDllAsyncTxBufferFull;
                goto Exit;
            }
            break;

        default:    // generic priority
            if (EplDllkCalInstance_g.m_uiFrameSizeGen == 0)
            {
                EPL_MEMCPY(EplDllkCalInstance_g.m_abFrameGen, pFrameInfo_p->m_pFrame, pFrameInfo_p->m_uiFrameSize);
                EplDllkCalInstance_g.m_uiFrameSizeGen = pFrameInfo_p->m_uiFrameSize;
            }
            else
            {
                Ret = kEplDllAsyncTxBufferFull;
                goto Exit;
            }
            break;
    }

#endif

    // post event to DLL
    Event.m_EventSink = kEplEventSinkDllk;
    Event.m_EventType = kEplEventTypeDllkFillTx;
    EPL_MEMSET(&Event.m_NetTime, 0x00, sizeof(Event.m_NetTime));
    Event.m_pArg = &Priority_p;
    Event.m_uiSize = sizeof(Priority_p);
    Ret = EplEventkPost(&Event);

#if EPL_USE_SHAREDBUFF == FALSE
Exit:
#endif
    return (( ShbRet != kEplSuccessful) ? ShbRet : Ret);
}


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncClearBuffer()
//
// Description: clears the transmit buffer
//
// Parameters:  (none)
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncClearBuffer(void)
{
tEplKernel  Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError   ShbError;

    ShbError = ShbCirResetBuffer (EplDllkCalInstance_g.m_ShbInstanceTxNmt, 1000, NULL);
    ShbError = ShbCirResetBuffer (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, 1000, NULL);
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    ShbError = ShbCirResetBuffer (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, 1000, NULL);
#endif

#else
    EplDllkCalInstance_g.m_uiFrameSizeNmt = 0;
    EplDllkCalInstance_g.m_uiFrameSizeGen = 0;
#endif

//    EPL_MEMSET(&EplDllkCalInstance_g.m_Statistics, 0, sizeof (tEplDllkCalStatistics));
    return Ret;
}


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncClearQueues()
//
// Description: clears the transmit buffer
//
// Parameters:  (none)
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)
tEplKernel EplDllkCalAsyncClearQueues(void)
{
tEplKernel  Ret = kEplSuccessful;
#if EPL_DLL_PRES_CHAINING_MN != FALSE
tShbError   ShbError;

    ShbError = ShbCirResetBuffer (EplDllkCalInstance_g.m_ShbInstanceTxSync, 1000, NULL);
#endif

    // clear MN asynchronous queues
    EplDllkCalInstance_g.m_uiNextQueueCnRequest = 0;
    EplDllkCalInstance_g.m_uiNextRequestQueue = 0;
    EplDllkCalInstance_g.m_uiReadIdentReq = 0;
    EplDllkCalInstance_g.m_uiWriteIdentReq = 0;
    EplDllkCalInstance_g.m_uiReadStatusReq = 0;
    EplDllkCalInstance_g.m_uiWriteStatusReq = 0;

    return Ret;
}
#endif


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalGetStatistics()
//
// Description: returns statistics of the asynchronous queues.
//
// Parameters:  ppStatistics            = statistics structure
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalGetStatistics(tEplDllkCalStatistics ** ppStatistics)
{
tEplKernel  Ret = kEplSuccessful;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError   ShbError;

    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxNmt, &EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountNmt);
    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxGenAsnd, &EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountGenAsnd);
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxGenVeth, &EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountGenVeth);
#endif
//    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceRx, &EplDllkCalInstance_g.m_Statistics.m_ulCurRxFrameCount);

#else
    if (EplDllkCalInstance_g.m_uiFrameSizeNmt > 0)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountNmt = 1;
    }
    else
    {
        EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountNmt = 0;
    }
    if (EplDllkCalInstance_g.m_uiFrameSizeGen > 0)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountGen = 1;
    }
    else
    {
        EplDllkCalInstance_g.m_Statistics.m_ulCurTxFrameCountGen = 0;
    }
#endif

    *ppStatistics = &EplDllkCalInstance_g.m_Statistics;
    return Ret;
}


#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalIssueRequest()
//
// Description: issues a StatusRequest or a IdentRequest to the specified node.
//
// Parameters:  Service_p               = request service ID
//              uiNodeId_p              = node ID
//              bSoaFlag1_p             = flag1 for this node (transmit in SoA and PReq)
//                                        If 0xFF this flag is ignored.
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalIssueRequest(tEplDllReqServiceId Service_p, unsigned int uiNodeId_p, BYTE bSoaFlag1_p)
{
tEplKernel  Ret = kEplSuccessful;

    if (bSoaFlag1_p != 0xFF)
    {
        Ret = EplDllkSetFlag1OfNode(uiNodeId_p, bSoaFlag1_p);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }
    }

    // add node to appropriate request queue
    switch (Service_p)
    {
        case kEplDllReqServiceIdent:
        {
            if (((EplDllkCalInstance_g.m_uiWriteIdentReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueIdentReq))
                == EplDllkCalInstance_g.m_uiReadIdentReq)
            {   // queue is full
                Ret = kEplDllAsyncTxBufferFull;
                goto Exit;
            }
            EplDllkCalInstance_g.m_auiQueueIdentReq[EplDllkCalInstance_g.m_uiWriteIdentReq] = uiNodeId_p;
            EplDllkCalInstance_g.m_uiWriteIdentReq =
                (EplDllkCalInstance_g.m_uiWriteIdentReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueIdentReq);
            break;
        }

        case kEplDllReqServiceStatus:
        {
            if (((EplDllkCalInstance_g.m_uiWriteStatusReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueStatusReq))
                == EplDllkCalInstance_g.m_uiReadStatusReq)
            {   // queue is full
                Ret = kEplDllAsyncTxBufferFull;
                goto Exit;
            }
            EplDllkCalInstance_g.m_auiQueueStatusReq[EplDllkCalInstance_g.m_uiWriteStatusReq] = uiNodeId_p;
            EplDllkCalInstance_g.m_uiWriteStatusReq =
                (EplDllkCalInstance_g.m_uiWriteStatusReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueStatusReq);
            break;
        }

        default:
        {
            Ret = kEplDllInvalidParam;
            goto Exit;
        }
    }

Exit:
    return Ret;
}


//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncGetSoaRequest()
//
// Description: returns next request for SoA. This function is called by DLLk module.
//
// Parameters:  pReqServiceId_p         = pointer to request service ID
//                                        IN: available request for MN NMT or generic request queue (Flag2.PR)
//                                            or kEplDllReqServiceNo if queues are empty
//                                        OUT: next request
//              puiNodeId_p             = OUT: pointer to node ID of next request
//                                             = EPL_C_ADR_INVALID, if request is self addressed
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncGetSoaRequest(tEplDllReqServiceId* pReqServiceId_p, unsigned int* puiNodeId_p, tEplSoaPayload* pSoaPayload_p)
{
tEplKernel      Ret = kEplSuccessful;
unsigned int    uiCount;

#if EPL_DLL_PRES_CHAINING_MN == FALSE
    UNUSED_PARAMETER(pSoaPayload_p);
#endif

    for (uiCount = EPL_DLLKCAL_MAX_QUEUES; uiCount > 0; uiCount--)
    {
        switch (EplDllkCalInstance_g.m_uiNextRequestQueue)
        {
            case 0:
            {   // CnGenReq
                for (;EplDllkCalInstance_g.m_uiNextQueueCnRequest < (tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2);
                    EplDllkCalInstance_g.m_uiNextQueueCnRequest++)
                {
                    if (EplDllkCalInstance_g.m_auiQueueCnRequests[EplDllkCalInstance_g.m_uiNextQueueCnRequest] > 0)
                    {   // non empty queue found
                        // remove one request from queue
                        EplDllkCalInstance_g.m_auiQueueCnRequests[EplDllkCalInstance_g.m_uiNextQueueCnRequest]--;
                        *puiNodeId_p = EplDllkCalInstance_g.m_uiNextQueueCnRequest + 1;
                        *pReqServiceId_p = kEplDllReqServiceUnspecified;
                        EplDllkCalInstance_g.m_uiNextQueueCnRequest++;
                        if (EplDllkCalInstance_g.m_uiNextQueueCnRequest >= (tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2))
                        {   // last node reached
                            // continue with CnNmtReq queue at next SoA
                            EplDllkCalInstance_g.m_uiNextRequestQueue = 1;
                        }
                        goto Exit;
                    }
                }
                // all CnGenReq queues are empty -> continue with CnNmtReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 1;
                break;
            }

            case 1:
            {   // CnNmtReq
                for (;EplDllkCalInstance_g.m_uiNextQueueCnRequest < tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests);
                    EplDllkCalInstance_g.m_uiNextQueueCnRequest++)
                {
                    if (EplDllkCalInstance_g.m_auiQueueCnRequests[EplDllkCalInstance_g.m_uiNextQueueCnRequest] > 0)
                    {   // non empty queue found
                        // remove one request from queue
                        EplDllkCalInstance_g.m_auiQueueCnRequests[EplDllkCalInstance_g.m_uiNextQueueCnRequest]--;
                        *puiNodeId_p = EplDllkCalInstance_g.m_uiNextQueueCnRequest + 1 - (tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2);
                        *pReqServiceId_p = kEplDllReqServiceNmtRequest;
                        EplDllkCalInstance_g.m_uiNextQueueCnRequest++;
                        if (EplDllkCalInstance_g.m_uiNextQueueCnRequest > tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests))
                        {   // last node reached
                            // restart CnGenReq queue
                            EplDllkCalInstance_g.m_uiNextQueueCnRequest = 0;
                            // continue with MnGenReq queue at next SoA
                            EplDllkCalInstance_g.m_uiNextRequestQueue = 2;
                        }
                        goto Exit;
                    }
                }
                // restart CnGenReq queue
                EplDllkCalInstance_g.m_uiNextQueueCnRequest = 0;
                // all CnNmtReq queues are empty -> continue with MnGenReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 2;
                break;
            }

            case 2:
            {   // MnNmtReq and MnGenReq
                // next queue will be MnIdentReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 3;
                if (*pReqServiceId_p != kEplDllReqServiceNo)
                {
                    *puiNodeId_p = EPL_C_ADR_INVALID;   // DLLk must exchange this with the actual node ID
                    goto Exit;
                }
                break;
            }

            case 3:
            {   // MnIdentReq
                // next queue will be MnStatusReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 4;
                if (EplDllkCalInstance_g.m_uiReadIdentReq != EplDllkCalInstance_g.m_uiWriteIdentReq)
                {   // queue is not empty
                    *puiNodeId_p = EplDllkCalInstance_g.m_auiQueueIdentReq[EplDllkCalInstance_g.m_uiReadIdentReq];
                    EplDllkCalInstance_g.m_uiReadIdentReq =
                        (EplDllkCalInstance_g.m_uiReadIdentReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueIdentReq);
                    *pReqServiceId_p = kEplDllReqServiceIdent;
                    goto Exit;
                }
                break;
            }

            case 4:
            {   // MnStatusReq
#if EPL_DLL_PRES_CHAINING_MN != FALSE
                // next queue will be MnSyncReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 5;
#else
                // next queue will be CnGenReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 0;
#endif
                if (EplDllkCalInstance_g.m_uiReadStatusReq != EplDllkCalInstance_g.m_uiWriteStatusReq)
                {   // queue is not empty
                    *puiNodeId_p = EplDllkCalInstance_g.m_auiQueueStatusReq[EplDllkCalInstance_g.m_uiReadStatusReq];
                    EplDllkCalInstance_g.m_uiReadStatusReq =
                        (EplDllkCalInstance_g.m_uiReadStatusReq + 1) % tabentries (EplDllkCalInstance_g.m_auiQueueStatusReq);
                    *pReqServiceId_p = kEplDllReqServiceStatus;
                    goto Exit;
                }
                break;
            }

#if EPL_DLL_PRES_CHAINING_MN != FALSE
            case 5:
            {   // MnSyncReq
            unsigned long       ulSyncReqCount = 0;
            unsigned long       ulSyncReqSize = 0;
            tEplDllSyncRequest  SyncRequest;
            tShbError           ShbError;

                // next queue will be CnGenReq queue
                EplDllkCalInstance_g.m_uiNextRequestQueue = 0;
                ShbError = ShbCirGetReadBlockCount(EplDllkCalInstance_g.m_ShbInstanceTxSync, &ulSyncReqCount);
                if (ShbError != kShbOk)
                {
                    Ret = kEplNoResource;
                    goto Exit;
                }
                if (ulSyncReqCount > 0)
                {
                    ShbError = ShbCirReadDataBlock (EplDllkCalInstance_g.m_ShbInstanceTxSync, (BYTE *) &SyncRequest, sizeof (SyncRequest), &ulSyncReqSize);
                    if (ShbError != kShbOk)
                    {
                        Ret = kEplNoResource;
                        goto Exit;
                    }

                    if (ulSyncReqSize > memberoffs(tEplDllSyncRequest, m_dwSyncControl))
                    {
                        AmiSetDwordToLe(&pSoaPayload_p->m_SyncRequest.m_le_dwSyncControl, SyncRequest.m_dwSyncControl);
                        if ((SyncRequest.m_dwSyncControl & EPL_SYNC_PRES_MODE_SET) != 0)
                        {
                        tEplDllNodeOpParam  NodeOpParam;

                            NodeOpParam.m_OpNodeType = kEplDllNodeOpTypeIsochronous;
                            NodeOpParam.m_uiNodeId = SyncRequest.m_uiNodeId;

                            Ret = EplDllkAddNode(&NodeOpParam);
                            if (Ret != kEplSuccessful)
                            {
                                goto Exit;
                            }
                        }
                        if ((SyncRequest.m_dwSyncControl & EPL_SYNC_PRES_MODE_RESET) != 0)
                        {
                        tEplDllNodeOpParam  NodeOpParam;

                            NodeOpParam.m_OpNodeType = kEplDllNodeOpTypeIsochronous;
                            NodeOpParam.m_uiNodeId = SyncRequest.m_uiNodeId;

                            Ret = EplDllkDeleteNode(&NodeOpParam);
                            if (Ret != kEplSuccessful)
                            {
                                goto Exit;
                            }
                        }
                    }
                    if (ulSyncReqSize > memberoffs(tEplDllSyncRequest, m_dwPResTimeFirst))
                    {
                        AmiSetDwordToLe(&pSoaPayload_p->m_SyncRequest.m_le_dwPResTimeFirst, SyncRequest.m_dwPResTimeFirst);
                    }
                    if (ulSyncReqSize > memberoffs(tEplDllSyncRequest, m_dwPResFallBackTimeout))
                    {
                        AmiSetDwordToLe(&pSoaPayload_p->m_SyncRequest.m_le_dwPResFallBackTimeout, SyncRequest.m_dwPResFallBackTimeout);
                    }

                    if ((SyncRequest.m_dwSyncControl & EPL_SYNC_DEST_MAC_ADDRESS_VALID) != 0)
                    {
                        Ret = EplDllkGetCnMacAddress(SyncRequest.m_uiNodeId, &pSoaPayload_p->m_SyncRequest.m_be_abDestMacAddress[0]);
                        if (Ret != kEplSuccessful)
                        {
                            goto Exit;
                        }
                    }

                    *puiNodeId_p = SyncRequest.m_uiNodeId;
                    *pReqServiceId_p = kEplDllReqServiceSync;
                    goto Exit;
                }
                break;
            }
#endif
        }
    }

Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplDllkCalAsyncSetPendingRequests()
//
// Description: sets the pending asynchronous frame requests of the specified node.
//              This will add the node to the asynchronous request scheduler.
//
// Parameters:  uiNodeId_p              = node ID
//              AsyncReqPrio_p          = asynchronous request priority
//              uiCount_p               = count of asynchronous frames
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel EplDllkCalAsyncSetPendingRequests(unsigned int uiNodeId_p, tEplDllAsyncReqPriority AsyncReqPrio_p, unsigned int uiCount_p)
{
tEplKernel  Ret = kEplSuccessful;

    // add node to appropriate request queue
    switch (AsyncReqPrio_p)
    {
        case kEplDllAsyncReqPrioNmt:
        {
            uiNodeId_p--;
            if (uiNodeId_p >= (tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2))
            {
                Ret = kEplDllInvalidParam;
                goto Exit;
            }
            uiNodeId_p += tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2;
            EplDllkCalInstance_g.m_auiQueueCnRequests[uiNodeId_p] = uiCount_p;
            break;
        }

        default:
        {
            uiNodeId_p--;
            if (uiNodeId_p >= (tabentries (EplDllkCalInstance_g.m_auiQueueCnRequests) / 2))
            {
                Ret = kEplDllInvalidParam;
                goto Exit;
            }
            EplDllkCalInstance_g.m_auiQueueCnRequests[uiNodeId_p] = uiCount_p;
            break;
        }
    }

Exit:
    return Ret;
}
#endif //(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Callback handler for new data signaling
//---------------------------------------------------------------------------

#if EPL_USE_SHAREDBUFF != FALSE
/*static void  EplDllkCalTxNmtSignalHandler (
    tShbInstance pShbRxInstance_p,
    unsigned long ulDataSize_p)
{
tEplKernel      Ret = kEplSuccessful;
tEplEvent       Event;
tEplDllAsyncReqPriority Priority;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError   ShbError;
unsigned long   ulBlockCount;


    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxNmt, &ulBlockCount);
    if (ulBlockCount > EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountNmt)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountNmt = ulBlockCount;
    }

#endif

    // post event to DLL
    Priority = kEplDllAsyncReqPrioNmt;
    Event.m_EventSink = kEplEventSinkDllk;
    Event.m_EventType = kEplEventTypeDllkFillTx;
    EPL_MEMSET(&Event.m_NetTime, 0x00, sizeof(Event.m_NetTime));
    Event.m_pArg = &Priority;
    Event.m_uiSize = sizeof(Priority);
    Ret = EplEventkPost(&Event);

}

static void  EplDllkCalTxGenSignalHandler (
    tShbInstance pShbRxInstance_p,
    unsigned long ulDataSize_p)
{
tEplKernel      Ret = kEplSuccessful;
tEplEvent       Event;
tEplDllAsyncReqPriority Priority;
#if EPL_USE_SHAREDBUFF != FALSE
tShbError   ShbError;
unsigned long   ulBlockCount;


    ShbError = ShbCirGetReadBlockCount (EplDllkCalInstance_g.m_ShbInstanceTxGen, &ulBlockCount);
    if (ulBlockCount > EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGen)
    {
        EplDllkCalInstance_g.m_Statistics.m_ulMaxTxFrameCountGen = ulBlockCount;
    }

#endif

    // post event to DLL
    Priority = kEplDllAsyncReqPrioGeneric;
    Event.m_EventSink = kEplEventSinkDllk;
    Event.m_EventType = kEplEventTypeDllkFillTx;
    EPL_MEMSET(&Event.m_NetTime, 0x00, sizeof(Event.m_NetTime));
    Event.m_pArg = &Priority;
    Event.m_uiSize = sizeof(Priority);
    Ret = EplEventkPost(&Event);

}
*/
#endif


#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_DLLK)) != 0)

// EOF

