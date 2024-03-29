/****************************************************************************

  (c) Bernecker + Rainer Industrie-Elektronik Ges.m.b.H.
      A-5142 Eggelsberg, B&R Strasse 1
      www.br-automation.com

  Project:      openPOWERLINK

  Description:  Virtual Ethernet Driver for NoOS

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

                $Author$

                $Revision$  $Date$

                $State$

                Build Environment:

----------------------------------------------------------------------------*/

#include "kernel/VirtualEthernet.h"
#include "kernel/EplDllk.h"
#include "kernel/EplDllkCal.h"
#include "user/EplNmtu.h"       //TODO: no clean user/kernel separation!
#include "VirtualEthernetApi.h"

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)

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

#ifndef EPL_VETH_TX_TIMEOUT
//#define EPL_VETH_TX_TIMEOUT (2*HZ)
#define EPL_VETH_TX_TIMEOUT 0       // d.k.: we use no timeout
#endif

#ifndef EPL_VETH_NUM_RX_BUFFERS
#error "Please define the number of pending RX buffers before using this driver!"
#endif

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

typedef struct _tVEthStatistics
{
    DWORD                  m_dwMsgfree;
    DWORD                  m_dwMsgsent;
    DWORD                  m_dwMsgrcv;
    DWORD                  m_dwTxBufferFull;
} tVEthStatistics;

typedef struct _tVEthInstance
{
    tEdrvRxBuffer          m_aVEthRxBuffer[EPL_VETH_NUM_RX_BUFFERS];
    BYTE                   m_bVethRxBuffWritePos;
    BYTE                   m_bVethRxBuffReadPos;
    BYTE                   m_abEthMac[6];

    DWORD                  m_dwIpAddress;
    DWORD                  m_dwSubnetMask;
    WORD                   m_wMtu;

    DWORD                  m_dwDefaultGateway;

    tVEthStatistics        m_Statistics;
} tVEthInstance;


//---------------------------------------------------------------------------
// module global vars
//---------------------------------------------------------------------------
static tVEthInstance VEthInstance_g;

#ifdef EPL_VETH_SEND_TEST
//Data from veth pinging 192.168.100.240
/*
 * Frame 22 (42 bytes on wire, 42 bytes captured)
Ethernet II, Src: Compex_61:e1:5e (00:80:48:61:e1:5e), Dst: Broadcast (ff:ff:ff:ff:ff:ff)
    Destination: Broadcast (ff:ff:ff:ff:ff:ff)
    Source: Compex_61:e1:5e (00:80:48:61:e1:5e)
    Type: ARP (0x0806)
Address Resolution Protocol (request)
    Hardware type: Ethernet (0x0001)
    Protocol type: IP (0x0800)
    Hardware size: 6
    Protocol size: 4
    Opcode: request (0x0001)
    Sender MAC address: Compex_61:e1:5e (00:80:48:61:e1:5e)
    Sender IP address: 192.168.100.240 (192.168.100.240)
    Target MAC address: 00:00:00_00:00:00 (00:00:00:00:00:00)
    Target IP address: 192.168.100.1 (192.168.100.1)
*/
static BYTE abNonEplData[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x0, 0x80, 0x48, 0x61, 0xe1, 0x5e,
        0x8, 0x6,
        0x0, 0x1,
        0x8, 0x0,
        0x6,
        0x4,
        0x0, 0x1,
        0x0, 0x12, 0x34, 0x56, 0x78, 0x9a,
        0xc0, 0xa8, 0x64, 0xf0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0xc0, 0xa8, 0x64, 0x1 };
#endif //VETH_SEND_TEST


//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------
static tEplKernel VEthRecvFrame(tEplFrameInfo * pFrameInfo_p, tEdrvReleaseRxBuffer*   pReleaseRxBuffer_p);
static tEplKernel VEthOpen(void);
static tEplKernel VEthClose(void);


//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    VEthAddInstance
//
// Description: Add a VEth instance
//
// Parameters: abSrcMac_p = The MAC Address to set
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthAddInstance(const BYTE abSrcMac_p[6])
{
    tEplKernel  Ret = kEplSuccessful;

    EPL_MEMSET(&VEthInstance_g, 0 , sizeof(VEthInstance_g));

    EPL_MEMCPY(VEthInstance_g.m_abEthMac, abSrcMac_p, sizeof(VEthInstance_g.m_abEthMac) );

    Ret = VEthOpen();
    if(Ret != kEplSuccessful)
    {
        EPL_DBGLVL_VETH_TRACE1("VEthOpen: returned 0x%02X\n", Ret);
    }

    return Ret;
}


//---------------------------------------------------------------------------
//
// Function:    VEthDelInstance
//
// Description: deletes the VEth instance
//
// Parameters:
//
// Returns:     tEplKernel              = error code
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthDelInstance(void)
{
    tEplKernel  Ret = kEplSuccessful;

    EPL_MEMSET(&VEthInstance_g, 0 , sizeof(VEthInstance_g));

    Ret = VEthClose();
    if(Ret != kEplSuccessful)
    {
        EPL_DBGLVL_VETH_TRACE1("VEthClose: returned 0x%02X\n", Ret);
    }

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthSetIpAddress
//
// Description: Set the IP-Address, SubnetMask and Mtu
//
// Parameters:  dwIpAddress_p = IP-Address to set
//              dwSubnetMask_p = Subnetmask to set
//              wMtu_p = MTU to set
//
// Returns:     tEplKernel  =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthSetIpAddress(DWORD dwIpAddress_p, DWORD dwSubnetMask_p, WORD wMtu_p)
{
    tEplKernel  Ret = kEplSuccessful;

    VEthInstance_g.m_dwIpAddress = dwIpAddress_p;
    VEthInstance_g.m_dwSubnetMask = dwSubnetMask_p;
    VEthInstance_g.m_wMtu = wMtu_p;

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthSetDefaultGateway
//
// Description:     Set the Default Gateway
//
// Parameters:      dwDefaultGateway_p = the default gateway to set
//
// Returns:         tEplKernel  =   Errorcode
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthSetDefaultGateway(DWORD dwDefaultGateway_p)
{
    tEplKernel  Ret = kEplSuccessful;

    VEthInstance_g.m_dwDefaultGateway = dwDefaultGateway_p;

    return Ret;
}

#ifdef EPL_VETH_SEND_TEST
//---------------------------------------------------------------------------
//
// Function:        VEthApiSendTest
//
// Description:     Send a ARP test frame to the master which responds
//
// Parameters:      void
//
// Returns:         kEplInvalidParam - maximum MTU is reached
//                  kEplDllAsyncTxBuffer - internal buffer is full
//                  kEplInvalidOperation - Send not allowed in this state
//                  kEplSuccessful - message sent successfully
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthApiSendTest(void)
{
tEplKernel  Ret = kEplSuccessful;

    //transmit test frame
    Ret = VEthApiXmit(abNonEplData,  sizeof(abNonEplData));

    return Ret;
}
#endif //EPL_VETH_SEND_TEST

//---------------------------------------------------------------------------
//
// Function:        VEthApiCheckAndForwardRxFrame
//
// Description:     Check if a new frame is in the receive queue
//
// Parameters:      ppEthData_p = point to packet buffer
//                  wEthDataSize_p = size of packet
//
// Returns:         kEplReject if no frame is available
//                  kEplSuccessful if a new frame is returned
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthApiCheckAndForwardRxFrame(BYTE **ppEthData_p, WORD *wEthDataSize_p)
{
    tEplKernel      Ret = kEplReject;

    if( (VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_pbBuffer != NULL)
     && (VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_uiRxMsgLen != 0) )
    {
        EPL_DBGLVL_VETH_TRACE0("VEthCheckAndForwardRxFrame: Rx here and forwarded!\n");
        //send the data
        *ppEthData_p = VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_pbBuffer;
        *wEthDataSize_p = VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_uiRxMsgLen;

        Ret = kEplSuccessful;
    }

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthApiReleaseRxFrame
//
// Description:     Release the active RX buffer
//
// Parameters:      void
//
// Returns:         kEplEdrvInvalidRxBuf if the current buffer is no available
//                  kEplSuccessful if the buffer is freed
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthApiReleaseRxFrame(void)
{
    tEplKernel      Ret = kEplSuccessful;
    tEdrvRxBuffer   localRxBuffer;

    // make rx buffer pointer local
    localRxBuffer.m_pbBuffer = VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_pbBuffer;
    localRxBuffer.m_uiRxMsgLen = VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_uiRxMsgLen;
    localRxBuffer.m_BufferInFrame = VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_BufferInFrame;

    // remove rx buffer from list
    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_BufferInFrame = 0;
    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_pbBuffer = NULL;
    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffReadPos].m_uiRxMsgLen = 0;

    VEthInstance_g.m_bVethRxBuffReadPos++;
    if (VEthInstance_g.m_bVethRxBuffReadPos == EPL_VETH_NUM_RX_BUFFERS)
    {
        VEthInstance_g.m_bVethRxBuffReadPos = 0;
    }

    Ret = EdrvReleaseRxBuffer(&localRxBuffer);
    if(Ret != kEplSuccessful)
    {
        EPL_DBGLVL_VETH_TRACE1("In %s() error while freeing the RX buffer!\n", __func__);
        goto Exit;
    }

    //set stats for device
    VEthInstance_g.m_Statistics.m_dwMsgfree++;

Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthApiXmit
//
// Description:     Transmit a TX frame
//
// Parameters:      pData_p = point to packet buffer
//                  wDataSize = size of packet
//
// Returns:         kEplInvalidParam - maximum MTU is reached
//                  kEplDllAsyncTxBuffer - internal buffer is full
//                  kEplInvalidOperation - Send not allowed in this state
//                  kEplSuccessful - message sent successfully
//
// State:
//
//---------------------------------------------------------------------------
tEplKernel PUBLIC VEthApiXmit(BYTE *pData_p, WORD wDataSize)
{
    tEplKernel      Ret = kEplSuccessful;
    tEplFrameInfo   FrameInfo;
    tEplNmtState    NmtState;

    //check POWERLINK state
    NmtState = EplNmtuGetNmtState();
    if(NmtState <= kEplNmtCsNotActive || NmtState == kEplNmtMsNotActive)
    {
        Ret =  kEplInvalidOperation;
        EPL_DBGLVL_VETH_TRACE1("VEthXmit: Error while transmitting! Invalid "
                "state: 0x%x!\n", NmtState);
        goto Exit;
    }

    //check MTU
    if(wDataSize > VEthInstance_g.m_wMtu)
    {
        Ret = kEplInvalidParam;
        EPL_DBGLVL_VETH_TRACE2("VEthXmit: Error while transmitting! MTU is set to "
                "%d and frame has size %d!\n", VEthInstance_g.m_wMtu, uiDataSize);
        goto Exit;
    }

    //save frame and size
    FrameInfo.m_pFrame = (tEplFrame *)pData_p;
    FrameInfo.m_uiFrameSize = wDataSize;

    //call send function on DLL
    Ret = EplDllkCalAsyncSend(&FrameInfo, kEplDllAsyncReqPrioGeneric, kEplDllAsyncBuffVeth);
    switch(Ret)
    {
        case kEplSuccessful:
            //set stats for the device
            VEthInstance_g.m_Statistics.m_dwMsgsent++;
            goto Exit;
        case kEplDllAsyncTxBufferFull:
            //buffer is full
            VEthInstance_g.m_Statistics.m_dwTxBufferFull++;
            goto Exit;
        default:
            EPL_DBGLVL_VETH_TRACE1("VEthXmit: EplDllkCalAsyncSend returned 0x%02X\n", Ret);
            goto Exit;
    }

Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthApiGetDefaultGateway
//
// Description:     Get the default gateway of the POWERLINK CN
//
// Parameters:
//
// Returns:         DWORD           the default gateway of the POWERLINK CN
//
// State:
//
//---------------------------------------------------------------------------
DWORD VEthApiGetDefaultGateway(void)
{
    return VEthInstance_g.m_dwDefaultGateway;
}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:        VEthRecvFrame
//
// Description:     Receive function from EplDllk module
//
// Parameters:      pFrameInfo_p = point to packet buffer
//                  pReleaseRxBuffer_p = release for the buffer
//
// Returns:         kEplSuccessful
//                  kEplNoResource = no buffer is available
//
// State:
//
//---------------------------------------------------------------------------
static tEplKernel VEthRecvFrame(tEplFrameInfo * pFrameInfo_p, tEdrvReleaseRxBuffer* pReleaseRxBuffer_p)
{
tEplKernel  Ret = kEplSuccessful;

    //BENCHMARK_MOD_03_SET(8);

    EPL_DBGLVL_VETH_TRACE1("VEthRecvFrame: FrameSize=%u\n", pFrameInfo_p->m_uiFrameSize);
    EPL_DBGLVL_VETH_TRACE1("VEthRecvFrame: SrcMAC=0x%llx\n", AmiGetQword48FromBe(pFrameInfo_p->m_pFrame->m_be_abSrcMac));

    if( (VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffWritePos].m_pbBuffer != NULL)
     || (VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffWritePos].m_uiRxMsgLen != 0) )
    {
        EPL_DBGLVL_VETH_TRACE0("VEthRecvFrame: No free buffer available!\n");
        Ret = kEplNoResource;
        goto Exit;
    }

    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffWritePos].m_BufferInFrame = kEdrvBufferLastInFrame;
    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffWritePos].m_pbBuffer = (BYTE *)pFrameInfo_p->m_pFrame;
    VEthInstance_g.m_aVEthRxBuffer[VEthInstance_g.m_bVethRxBuffWritePos].m_uiRxMsgLen = pFrameInfo_p->m_uiFrameSize;

    VEthInstance_g.m_bVethRxBuffWritePos++;
    if (VEthInstance_g.m_bVethRxBuffWritePos == EPL_VETH_NUM_RX_BUFFERS)
    {
        VEthInstance_g.m_bVethRxBuffWritePos = 0;
    }

    // set receive stats
    VEthInstance_g.m_Statistics.m_dwMsgrcv++;

    *pReleaseRxBuffer_p = kEdrvReleaseRxBufferLater;
    //BENCHMARK_MOD_03_RESET(8);

Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthOpen
//
// Description:     Open the Virtual Ethernet driver
//
// Parameters:      void
//
// Returns:
//
// State:
//
//---------------------------------------------------------------------------
static tEplKernel VEthOpen(void)
{
tEplKernel  Ret = kEplSuccessful;

    // register callback function in DLL
    Ret = EplDllkRegAsyncHandler(VEthRecvFrame);

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:        VEthClose
//
// Description:     Close the Virtual Ethernet driver
//
// Parameters:      void
//
// Returns:
//
// State:
//
//---------------------------------------------------------------------------
static tEplKernel VEthClose(void)
{
tEplKernel  Ret = kEplSuccessful;

    // deregister callback function in DLL
    Ret = EplDllkDeregAsyncHandler(VEthRecvFrame);

    return Ret;
}

#endif // (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_VETH)) != 0)
