/****************************************************************************

  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com

  Project:      openPOWERLINK

  Description:  include file for Epl-Obd-Userspace-module

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

  2006/06/19 k.t.:   start of the implementation


****************************************************************************/

#ifndef _EPLOBDU_H_
#define _EPLOBDU_H_

#include "EplObd.h"


//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// typedef
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// function prototypes
//---------------------------------------------------------------------------

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) != 0)

#if EPL_OBD_USE_KERNEL != FALSE
#error "EPL OBDu module enabled, but OBD_USE_KERNEL == TRUE"
#endif

EPLDLLEXPORT tEplKernel PUBLIC EplObduWriteEntry (unsigned int  uiIndex_p,
                                unsigned int  uiSubIndex_p,
                                void * pSrcData_p,
                                tEplObdSize   Size_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduReadEntry (unsigned int        uiIndex_p,
                                unsigned int        uiSubIndex_p,
                                void *       pDstData_p,
                                tEplObdSize *pSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduAccessOdPart (tEplObdPart ObdPart_p,
                                tEplObdDir Direction_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduDefineVar (tEplVarParam MEM* pVarParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void* PUBLIC EplObduGetObjectDataPtr (unsigned int uiIndex_p,
                                        unsigned int uiSubIndex_p,
                                        tEplObdAccess AccessType_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduRegisterUserOd (
                                        tEplObdEntryPtr pUserOd_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT void PUBLIC EplObduInitVarEntry (tEplObdVarEntry MEM* pVarEntry_p,
                                        BYTE bType_p,
                                        tEplObdSize ObdSize_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplObdSize PUBLIC EplObduGetDataSize(unsigned int uiIndex_p,
                                        unsigned int uiSubIndex_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT unsigned int PUBLIC EplObduGetNodeId(void);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduSetNodeId(unsigned int uiNodeId_p,
                                         tEplObdNodeIdType NodeIdType_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduGetAccessType(unsigned int uiIndex_p,
                                        unsigned int   uiSubIndex_p,
                                        tEplObdAccess* pAccessTyp_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduReadEntryToLe (
                                        tEplObdParam* pObdParam_p);
// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduWriteEntryFromLe (
                                        tEplObdParam* pObdParam_p);

// ---------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObduSearchVarEntry (EPL_MCO_DECL_INSTANCE_PTR_
                                        unsigned int            uiIndex_p,
                                        unsigned int            uiSubindex_p,
                                        tEplObdVarEntry MEM**   ppVarEntry_p);

#elif EPL_OBD_USE_KERNEL != FALSE
#include "kernel/EplObdk.h"

#define EplObduWriteEntry       EplObdWriteEntry

#define EplObduReadEntry        EplObdReadEntry

#define EplObduAccessOdPart     EplObdAccessOdPart

#define EplObduDefineVar        EplObdDefineVar

#define EplObduGetObjectDataPtr EplObdGetObjectDataPtr

#define EplObduRegisterUserOd   EplObdRegisterUserOd

#define EplObduInitVarEntry     EplObdInitVarEntry

#define EplObduGetDataSize      EplObdGetDataSize

#define EplObduGetNodeId        EplObdGetNodeId

#define EplObduSetNodeId        EplObdSetNodeId

#define EplObduGetAccessType    EplObdGetAccessType

#define EplObduReadEntryToLe    EplObdReadEntryToLe

#define EplObduWriteEntryFromLe EplObdWriteEntryFromLe

#define EplObduSearchVarEntry   EplObdSearchVarEntry

#define EplObduIsNumerical      EplObdIsNumerical

#define EplObduGetType          EplObdGetType

#define EplObduSetDefaultObdCallback EplObdSetDefaultObdCallback

#define EplObduSave0bdAccHdl    EplObdSave0bdAccHdl

#define EplObduDelete0bdAccHdl  EplObdDelete0bdAccHdl

#endif // #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDU)) != 0)

#endif  // #ifndef _EPLOBDU_H_


