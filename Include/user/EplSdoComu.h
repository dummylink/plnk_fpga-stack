/****************************************************************************

  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com

  Project:      openPOWERLINK

  Description:  include file for SDO Command Layer module

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

  2006/06/26 k.t.:   start of the implementation


****************************************************************************/

#ifndef _EPLSDOCOMU_H_
#define _EPLSDOCOMU_H_

#include "EplSdo.h"
#include "EplObd.h"
#include "EplSdoAc.h"
#include "user/EplObdu.h"
#include "user/EplSdoAsySequ.h"


//***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#ifndef EPL_MAX_SDO_COM_CON
#define EPL_MAX_SDO_COM_CON         5
#endif


//---------------------------------------------------------------------------
// typedefs
//---------------------------------------------------------------------------

// intern events
typedef enum
{
    kEplSdoComConEventSendFirst     = 0x00, // first frame to send
    kEplSdoComConEventRec           = 0x01, // frame received
    kEplSdoComConEventConEstablished= 0x02, // connection established
    kEplSdoComConEventConClosed     = 0x03, // connection closed
    kEplSdoComConEventAckReceived   = 0x04, // acknowledge received by lower layer
                                        // -> continue sending
    kEplSdoComConEventFrameSended   = 0x05, // lower has send a frame
    kEplSdoComConEventInitError     = 0x06, // error duringinitialisiation
                                            // of the connection
    kEplSdoComConEventTimeout       = 0x07, // timeout in lower layer
    kEplSdoComConEventTransferAbort = 0x08, // transfer abort by lower layer
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOC)) != 0)

    kEplSdoComConEventInitCon       = 0x09, // init connection (only client)
    kEplSdoComConEventAbort         = 0x0A, // abort sdo transfer (only client)
#endif


}tEplSdoComConEvent;

typedef enum
{
    kEplSdoComSendTypeReq      = 0x00,  // send a request
    kEplSdoComSendTypeAckRes   = 0x01,  // send a resonse without data
    kEplSdoComSendTypeRes      = 0x02,  // send response with data
    kEplSdoComSendTypeAbort    = 0x03   // send abort

}tEplSdoComSendType;

// state of the state maschine
typedef enum
{
    // General State
    kEplSdoComStateIdle             = 0x00, // idle state

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOS)) != 0)
    // Server States
    kEplSdoComStateServerSegmTrans  = 0x01, // send following frames
#endif


#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOC)) != 0)
    // Client States
    kEplSdoComStateClientWaitInit   = 0x10, // wait for init connection
                                            // on lower layer
    kEplSdoComStateClientConnected  = 0x11, // connection established
    kEplSdoComStateClientSegmTrans  = 0x12  // send following frames
#endif



} tEplSdoComState;


// control structure for transaction
typedef struct
{
    tEplSdoSeqConHdl    m_SdoSeqConHdl;     // if != 0 -> entry used
    tEplSdoComState     m_SdoComState;
    BYTE                m_bTransactionId;
    unsigned int        m_uiNodeId;         // NodeId of the target
                                            // -> needed to reinit connection
                                            //    after timeout
    tEplSdoTransType    m_SdoTransType;     // Auto, Expedited, Segmented
    tEplSdoServiceType  m_SdoServiceType;   // WriteByIndex, ReadByIndex
    tEplSdoType         m_SdoProtType;      // protocol layer: Auto, Udp, Asnd, Pdo
    BYTE*               m_pData;            // pointer to data
    unsigned int        m_uiTransSize;      // number of bytes
                                            // to transfer
    unsigned int        m_uiTransferredByte;// number of bytes
                                            // already transferred
    tEplSdoFinishedCb   m_pfnTransferFinished;// callback function of the
                                            // application
                                            // -> called in the end of
                                            //    the SDO transfer
    void*               m_pUserArg;         // user definable argument pointer

    DWORD               m_dwLastAbortCode;  // save the last abort code
#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOC)) != 0)
    // only for client
    unsigned int        m_uiTargetIndex;    // index to access
    unsigned int        m_uiTargetSubIndex; // subiondex to access

    // for future use
    unsigned int        m_uiTimeout;        // timeout for this connection

#endif

} tEplSdoComCon;

// instance table
typedef struct
{
    tEplSdoComCon       m_SdoComCon[EPL_MAX_SDO_COM_CON];

#if defined(WIN32) || defined(_WIN32)
    LPCRITICAL_SECTION  m_pCriticalSection;
    CRITICAL_SECTION    m_CriticalSection;
#endif

}tEplSdoComInstance;


//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------
tEplKernel PUBLIC EplSdoComInit(void);

tEplKernel PUBLIC EplSdoComAddInstance(void);

tEplKernel PUBLIC EplSdoComDelInstance(void);

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_SDOC)) != 0)

tEplKernel PUBLIC EplSdoComDefineCon(tEplSdoComConHdl*  pSdoComConHdl_p,
                                      unsigned int      uiTargetNodeId_p,
                                      tEplSdoType        ProtType_p);

tEplKernel PUBLIC EplSdoComInitTransferByIndex(tEplSdoComTransParamByIndex* pSdoComTransParam_p);

tEplKernel PUBLIC EplSdoComServerSendFrameIntern(tEplSdoComCon*     pSdoComCon_p,
                                           unsigned int       uiIndex_p,
                                           unsigned int       uiSubIndex_p,
                                           tEplSdoComSendType SendType_p);

unsigned int PUBLIC EplSdoComGetNodeId(tEplSdoComConHdl  SdoComConHdl_p);

tEplKernel PUBLIC EplSdoComUndefineCon(tEplSdoComConHdl  SdoComConHdl_p);

tEplKernel PUBLIC EplSdoComGetState(tEplSdoComConHdl SdoComConHdl_p,
                                    tEplSdoComFinished* pSdoComFinished_p);

tEplKernel PUBLIC EplSdoComSdoAbort(tEplSdoComConHdl SdoComConHdl_p,
                              DWORD           dwAbortCode_p);
#endif

// for future extention
/*
tEplKernel PUBLIC EplSdoComInitTransferAllByIndex(tEplSdoComTransParamAllByIndex* pSdoComTransParam_p);

tEplKernel PUBLIC EplSdoComInitTransferByName(tEplSdoComTransParamByName* pSdoComTransParam_p);

tEplKernel PUBLIC EplSdoComInitTransferFile(tEplSdoComTransParamFile* pSdoComTransParam_p);

*/





#endif  // #ifndef _EPLSDOCOMU_H_


