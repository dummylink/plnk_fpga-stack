/****************************************************************************
  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com
  (c) Bernecker + Rainer Industrie-Elektronik Ges.m.b.H.
      A-5142 Eggelsberg, B&R Strasse 1
      www.br-automation.com

  Project:      openPOWERLINK

  Description:  configuration file

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

****************************************************************************/


#ifndef _EPLCFG_H_
#define _EPLCFG_H_

#include "EplInc.h"



// =========================================================================
// generic defines which for whole EPL Stack
// =========================================================================
#define EPL_USE_DELETEINST_FUNC TRUE

// needed to support datatypes over 32 bit by global.h
#define USE_VAR64

// EPL_MAX_INSTANCES specifies count of instances of all EPL modules.
// If it is greater than 1 the first parameter of all
// functions is the instance number.
#define EPL_MAX_INSTANCES               1

// This defines the target hardware. Here is encoded wich CPU and wich external
// peripherals are connected. For possible values refere to target.h. If
// necessary value is not available EPL stack has to
// be adapted and tested.
//#define TARGET_HARDWARE                 TGTHW_PC_WRAPP

// use no FIFOs, make direct calls
//#define EPL_USE_SHAREDBUFF   FALSE

// do not use kernel part event queue (high priority), so make direct calls for kernel part events
#define EPL_EVENT_USE_KERNEL_QUEUE      FALSE

#ifndef BENCHMARK_MODULES
//#define BENCHMARK_MODULES       0 //0xEE800042L
#define BENCHMARK_MODULES       0xEE800043L
#endif

// Default defug level:
// Only debug traces of these modules will be compiled which flags are set in define DEF_DEBUG_LVL.
#ifndef DEF_DEBUG_LVL
#define DEF_DEBUG_LVL           0//0x40000000L //0xEC000000L
#endif
//   EPL_DBGLVL_OBD         =   0x00000004L
// * EPL_DBGLVL_ASSERT      =   0x20000000L
// * EPL_DBGLVL_ERROR       =   0x40000000L
// * EPL_DBGLVL_ALWAYS      =   0x80000000L

// EPL_MODULE_INTEGRATION defines all modules which are included in
// EPL application. Please add or delete modules for your application.

#define EPL_MODULE_INTEGRATION     (0 \
                                | EPL_MODULE_OBDK \
                                | EPL_MODULE_PDOU \
                                | EPL_MODULE_PDOK \
                                | EPL_MODULE_SDOS \
                                | EPL_MODULE_SDOC \
                                | EPL_MODULE_SDO_ASND \
                                | EPL_MODULE_DLLK \
                                | EPL_MODULE_DLLU \
                                | EPL_MODULE_NMT_CN \
                                | EPL_MODULE_NMTK \
                                | EPL_MODULE_NMTU \
                                | EPL_MODULE_LEDU \
                                )

/*                                | EPL_MODULE_PDOU \ */




// =========================================================================
// EPL ethernet driver (Edrv) specific defines
// =========================================================================

// switch this define to TRUE if Edrv supports fast tx frames
#define EDRV_FAST_TXFRAMES              FALSE
//#define EDRV_FAST_TXFRAMES              TRUE

// switch this define to TRUE if Edrv supports early receive interrupts
#define EDRV_EARLY_RX_INT               FALSE
//#define EDRV_EARLY_RX_INT               TRUE

// enables setting of several port pins for benchmarking purposes
#define EDRV_BENCHMARK                  TRUE
//#define EDRV_BENCHMARK                  TRUE // MCF_GPIO_PODR_PCIBR

// Call Tx handler (i.e. EplDllCbFrameTransmitted()) already if DMA has finished,
// otherwise call the Tx handler if frame was actually transmitted over ethernet.
#define EDRV_DMA_TX_HANDLER             FALSE
//#define EDRV_DMA_TX_HANDLER             TRUE

// number of used ethernet controller
#define EDRV_USED_ETH_CTRL              1

// openMAC supports auto-response
#define EDRV_AUTO_RESPONSE              TRUE


// increase the number of Tx buffers, because we are master
// and need one Tx buffer for each PReq and CN
// + SoC + SoA + MN PRes + NmtCmd + ASnd + IdentRes + StatusRes.
//#define EDRV_MAX_TX_BUFFERS             5

#define EDRV_AUTO_RESPONSE_DELAY         TRUE

#define EPL_DLL_PRES_CHAINING_CN        TRUE


// =========================================================================
// Data Link Layer (DLL) specific defines
// =========================================================================

// switch this define to TRUE if Edrv supports fast tx frames
// and DLL shall pass PRes as ready to Edrv after SoC
#define EPL_DLL_PRES_READY_AFTER_SOC    FALSE
//#define EPL_DLL_PRES_READY_AFTER_SOC    TRUE

// switch this define to TRUE if Edrv supports fast tx frames
// and DLL shall pass PRes as ready to Edrv after SoA
#define EPL_DLL_PRES_READY_AFTER_SOA    FALSE
//#define EPL_DLL_PRES_READY_AFTER_SOA    TRUE

// maximum count of Rx filter entries for PRes frames
#define EPL_DLL_PRES_FILTER_COUNT   3


#define EPL_DLL_PROCESS_SYNC        EPL_DLL_PROCESS_SYNC_ON_TIMER

// negative time shift of isochronous task in relation to SoC
#define EPL_DLL_SOC_SYNC_SHIFT_US       150

// Disable deferred release of rx-buffers until Edrv for openMAC supports it
#define EPL_DLL_DISABLE_DEFERRED_RXFRAME_RELEASE    TRUE

// Enable forwarding of SoC timestamps
#define EPL_DLL_SOCTIME_FORWARD         TRUE


// =========================================================================
// OBD specific defines
// =========================================================================

#define EPL_OBD_USE_KERNEL                TRUE

// switch this define to TRUE if Epl should compare object range
// automaticly
#define EPL_OBD_CHECK_OBJECT_RANGE          FALSE
//#define EPL_OBD_CHECK_OBJECT_RANGE          TRUE

// set this define to TRUE if there are strings or domains in OD, which
// may be changed in object size and/or object data pointer by its object
// callback function (called event kObdEvWrStringDomain)
//#define EPL_OBD_USE_STRING_DOMAIN_IN_RAM    FALSE
#define EPL_OBD_USE_STRING_DOMAIN_IN_RAM    TRUE

#define EPL_OBD_USE_VARIABLE_SUBINDEX_TAB TRUE


// =========================================================================
// Timer module specific defines
// =========================================================================

// if TRUE the high resolution timer module will be used
#define EPL_TIMER_USE_HIGHRES           FALSE



// =========================================================================
// SDO module specific defines
// =========================================================================

// do not increase the number of SDO channels, because we are not master
//#define EPL_SDO_MAX_CONNECTION_ASND 100
//#define EPL_MAX_SDO_SEQ_CON         100
//#define EPL_MAX_SDO_COM_CON         100
//#define EPL_SDO_MAX_CONNECTION_UDP  50


// =========================================================================
// API Layer specific defines
// =========================================================================

//#define EPL_API_PROCESS_IMAGE_SIZE_IN 0 //disable
//#define EPL_API_PROCESS_IMAGE_SIZE_OUT 0 //disable


#define CONFIG_USE_SDC_OBJECTS

#endif //_EPLCFG_H_



