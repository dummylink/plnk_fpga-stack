/****************************************************************************

  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com

  Project:      openPOWERLINK

  Description:  include file for DLL module

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

  2006/06/08 d.k.:   start of the implementation, version 1.00


****************************************************************************/

#ifndef _EPL_DLL_H_
#define _EPL_DLL_H_

#include "EplInc.h"
#include "EplFrame.h"

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------


#ifndef EPL_DLL_MAX_ASND_SERVICE_ID
#define EPL_DLL_MAX_ASND_SERVICE_ID (EPL_C_DLL_MAX_ASND_SERVICE_IDS + 1)
#endif

//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------

typedef enum
{
    kEplDllAsndNotDefined       = 0x00,
    kEplDllAsndIdentResponse    = 0x01,
    kEplDllAsndStatusResponse   = 0x02,
    kEplDllAsndNmtRequest       = 0x03,
    kEplDllAsndNmtCommand       = 0x04,
    kEplDllAsndSdo              = 0x05,
#if (EPL_DLL_PRES_CHAINING_CN != FALSE) || (EPL_DLL_PRES_CHAINING_MN != FALSE)
    kEplDllAsndSyncResponse     = 0x06
#endif
} tEplDllAsndServiceId;

typedef enum
{
    kEplDllAsndFilterNone     = 0x00,
    kEplDllAsndFilterLocal    = 0x01,  // receive only ASnd frames with local or broadcast node ID
    kEplDllAsndFilterAny      = 0x02,  // receive any ASnd frame

} tEplDllAsndFilter;

typedef enum
{
    kEplDllReqServiceNo         = 0x00,
    kEplDllReqServiceIdent      = 0x01,
    kEplDllReqServiceStatus     = 0x02,
    kEplDllReqServiceNmtRequest = 0x03,
#if (EPL_DLL_PRES_CHAINING_CN != FALSE) || (EPL_DLL_PRES_CHAINING_MN != FALSE)
    kEplDllReqServiceSync       = 0x06,
#endif
    kEplDllReqServiceUnspecified= 0xFF,

} tEplDllReqServiceId;

typedef enum
{
    kEplDllAsyncReqPrioNmt      = 0x07, // PRIO_NMT_REQUEST
    kEplDllAsyncReqPrio6        = 0x06,
    kEplDllAsyncReqPrio5        = 0x05,
    kEplDllAsyncReqPrio4        = 0x04,
    kEplDllAsyncReqPrioGeneric  = 0x03, // PRIO_GENERIC_REQUEST
    kEplDllAsyncReqPrio2        = 0x02, // till WSP 0.1.3: PRIO_ABOVE_GENERIC
    kEplDllAsyncReqPrio1        = 0x01, // till WSP 0.1.3: PRIO_BELOW_GENERIC
    kEplDllAsyncReqPrio0        = 0x00, // till WSP 0.1.3: PRIO_GENERIC_REQUEST

} tEplDllAsyncReqPriority;

typedef enum
{
    kEplDllAsyncBuffNmt         = 0x00,
    kEplDllAsyncBuffAsnd        = 0x01,
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
    kEplDllAsyncBuffVeth        = 0x02,
#endif
} tEplDllAsyncBufferNumber;

typedef struct
{
    unsigned int      m_uiFrameSize;
    tEplFrame       * m_pFrame;
//    tEplNetTime       m_NetTime;

} tEplFrameInfo;

typedef struct
{
    unsigned int        m_uiSizeOfStruct;
    BOOL                m_fAsyncOnly;   // do not need to register PRes-Frame
    unsigned int        m_uiNodeId;     // local node ID

    // 0x1F82: NMT_FeatureFlags_U32
    DWORD               m_dwFeatureFlags;
    // Cycle Length (0x1006: NMT_CycleLen_U32) in [us]
    DWORD               m_dwCycleLen;         // required for error detection
    // 0x1F98: NMT_CycleTiming_REC
    // 0x1F98.1: IsochrTxMaxPayload_U16
    unsigned int        m_uiIsochrTxMaxPayload; // const
    // 0x1F98.2: IsochrRxMaxPayload_U16
    unsigned int        m_uiIsochrRxMaxPayload; // const
    // 0x1F98.3: PResMaxLatency_U32
    DWORD               m_dwPresMaxLatency;   // const in [ns], only required for IdentRes
    // 0x1F98.4: PReqActPayloadLimit_U16
    unsigned int        m_uiPreqActPayloadLimit; // required for initialisation (+24 bytes)
    // 0x1F98.5: PResActPayloadLimit_U16
    unsigned int        m_uiPresActPayloadLimit; // required for initialisation of Pres frame (+24 bytes)
    // 0x1F98.6: ASndMaxLatency_U32
    DWORD               m_dwAsndMaxLatency;   // const in [ns], only required for IdentRes
    // 0x1F98.7: MultiplCycleCnt_U8
    unsigned int        m_uiMultiplCycleCnt;  // required for error detection
    // 0x1F98.8: AsyncMTU_U16
    unsigned int        m_uiAsyncMtu;         // required to set up max frame size
    // $$$ 0x1F98.9: Prescaler_U16
    // $$$ Multiplexed Slot

    // 0x1C14: DLL_LossOfFrameTolerance_U32 in [ns]
    DWORD               m_dwLossOfFrameTolerance;

    // 0x1F8A: NMT_MNCycleTiming_REC
    // 0x1F8A.1: WaitSoCPReq_U32 in [ns]
    DWORD               m_dwWaitSocPreq;

    // 0x1F8A.2: AsyncSlotTimeout_U32 in [ns]
    DWORD               m_dwAsyncSlotTimeout;

    // constant response latency for SyncRes in [ns]
    DWORD               m_dwSyncResLatency;

    // synchronization trigger (AppCbSync, cycle preparation)
    unsigned int        m_uiSyncNodeId;     // after PRes from CN with this node-ID (0 = SoC, 255 = SoA)
    BOOL                m_fSyncOnPrcNode;   // TRUE: CN is PRes chained; FALSE: conventional CN (PReq/PRes)

} tEplDllConfigParam;

typedef struct
{
    unsigned int      m_uiSizeOfStruct;
    DWORD             m_dwDeviceType;              // NMT_DeviceType_U32
    DWORD             m_dwVendorId;                // NMT_IdentityObject_REC.VendorId_U32
    DWORD             m_dwProductCode;             // NMT_IdentityObject_REC.ProductCode_U32
    DWORD             m_dwRevisionNumber;          // NMT_IdentityObject_REC.RevisionNo_U32
    DWORD             m_dwSerialNumber;            // NMT_IdentityObject_REC.SerialNo_U32
    QWORD             m_qwVendorSpecificExt1;
    DWORD             m_dwVerifyConfigurationDate; // CFM_VerifyConfiguration_REC.ConfDate_U32
    DWORD             m_dwVerifyConfigurationTime; // CFM_VerifyConfiguration_REC.ConfTime_U32
    DWORD             m_dwApplicationSwDate;       // PDL_LocVerApplSw_REC.ApplSwDate_U32 on programmable device or date portion of NMT_ManufactSwVers_VS on non-programmable device
    DWORD             m_dwApplicationSwTime;       // PDL_LocVerApplSw_REC.ApplSwTime_U32 on programmable device or time portion of NMT_ManufactSwVers_VS on non-programmable device
    DWORD             m_dwIpAddress;
    DWORD             m_dwSubnetMask;
    DWORD             m_dwDefaultGateway;
    BYTE              m_sHostname[32];
    BYTE              m_abVendorSpecificExt2[48];

} tEplDllIdentParam;

typedef struct
{
    unsigned int                m_uiNodeId;
    WORD                        m_wPresPayloadLimit;    // object 0x1F8D: NMT_PResPayloadLimitList_AU16
#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)
    WORD                        m_wPreqPayloadLimit;    // object 0x1F8B: NMT_MNPReqPayloadLimitList_AU16
    DWORD                       m_dwPresTimeoutNs;        // object 0x1F92: NMT_MNCNPResTimeout_AU32
#endif // (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_NMT_MN)) != 0)

} tEplDllNodeInfo;

typedef enum
{
    kEplDllNodeOpTypeIsochronous        = 0x00,
    kEplDllNodeOpTypeFilterPdo          = 0x01,
    kEplDllNodeOpTypeFilterHeartbeat    = 0x02,
    kEplDllNodeOpTypeSoftDelete         = 0x03,

} tEplDllNodeOpType;

typedef struct
{
    unsigned int        m_uiNodeId;
    tEplDllNodeOpType   m_OpNodeType;

} tEplDllNodeOpParam;

#if EPL_DLL_PRES_CHAINING_MN != FALSE
typedef struct
{
    unsigned int  m_uiNodeId;
    DWORD         m_dwSyncControl;
    DWORD         m_dwPResTimeFirst;
    DWORD         m_dwPResFallBackTimeout;

} tEplDllSyncRequest;
#endif

//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------



#endif  // #ifndef _EPL_DLL_H_


