/****************************************************************************

  (c) SYSTEC electronic GmbH, D-07973 Greiz, August-Bebel-Str. 29
      www.systec-electronic.com

  Project:      openPOWERLINK

  Description:  source file for api function of EplOBD-Module

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
                Microsoft VC7

  -------------------------------------------------------------------------

  Revision History:

  2006/06/02 k.t.:   start of the implementation, version 1.00
		     ->based on CANopen OBD-Modul

****************************************************************************/

#include "EplInc.h"
#include "kernel/EplObdk.h"         // function prototyps of the EplOBD-Modul
#include "EplSdoAc.h"

#if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDK)) != 0)

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

//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

// struct for instance table
INSTANCE_TYPE_BEGIN

    EPL_MCO_DECL_INSTANCE_MEMBER ()

    STATIC      tEplObdInitParam               INST_FAR    m_ObdInitParam;
    STATIC      tEplObdStoreLoadObjCallback    INST_NEAR   m_fpStoreLoadObjCallback;
    STATIC      tEplObdEntry                   INST_NEAR   m_DefaultObdEntry;
    STATIC      tEplObdParam                   INST_FAR    m_SaveObdParam;

INSTANCE_TYPE_END

// decomposition of float
typedef union
{
    tEplObdReal32   m_flRealPart;
    int             m_nIntegerPart;

} tEplObdRealParts;


//---------------------------------------------------------------------------
// module global vars
//---------------------------------------------------------------------------

// This macro replace the unspecific pointer to an instance through
// the modul specific type for the local instance table. This macro
// must defined in each modul.
//#define tEplPtrInstance             tEplInstanceInfo MEM*

EPL_MCO_DECL_INSTANCE_VAR ()

BYTE MEM            abEplObdTrashObject_g[8];


//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------

EPL_MCO_DEFINE_INSTANCE_FCT ()

static tEplKernel EplObdCallObjectCallback (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdCallback     fpCallback_p,
    tEplObdParam MEM*   pCbParam_p);

static tEplObdSize EplObdGetDataSizeIntern (tEplObdSubEntryPtr pSubIndexEntry_p);

static tEplObdSize EplObdGetStrLen (void* pObjData_p,
                                    tEplObdSize ObjLen_p,
                                    tEplObdType ObjType_p);

#if (EPL_OBD_CHECK_OBJECT_RANGE != FALSE)
static tEplKernel EplObdCheckObjectRange (
            tEplObdSubEntryPtr pSubindexEntry_p,
            void * pData_p);
#endif

static tEplKernel EplObdGetVarEntry (
            tEplObdSubEntryPtr    pSubindexEntry_p,
            tEplObdVarEntry MEM** ppVarEntry_p);

static tEplKernel  EplObdGetEntry (EPL_MCO_DECL_INSTANCE_PTR_
            tEplObdParam MEM*     pObdParam_p,
            tEplObdEntryPtr*      ppObdEntry_p,
            tEplObdSubEntryPtr*   ppObdSubEntry_p);

static tEplObdSize EplObdGetObjectSize (tEplObdSubEntryPtr pSubIndexEntry_p);

static tEplKernel EplObdGetIndexIntern (
    tEplObdInitParam MEM*     pInitParam_p,
    unsigned int              uiIndex_p,
    tEplObdEntryPtr*          ppObdEntry_p);

static tEplKernel EplObdGetSubindexIntern (
    tEplObdEntryPtr           pObdEntry_p,
    unsigned int              uiSubIndex_p,
    tEplObdSubEntryPtr*       ppObdSubEntry_p);

static tEplKernel EplObdAccessOdPartIntern (EPL_MCO_DECL_INSTANCE_PTR_
                            tEplObdPart     CurrentOdPart_p,
                            tEplObdEntryPtr pObdEnty_p,
                            tEplObdDir      Direction_p);

static CONST void*     EplObdGetObjectDefaultPtr (tEplObdSubEntryPtr pSubIndexEntry_p);
static void MEM*       EplObdGetObjectCurrentPtr (tEplObdSubEntryPtr pSubIndexEntry_p,
                                                  tEplObdAccess AccessType_p);

#if (EPL_OBD_USE_STORE_RESTORE != FALSE)

    static tEplKernel EplObdCallStoreCallback (EPL_MCO_DECL_INSTANCE_PTR_
        tEplObdCbStoreParam MEM* pCbStoreParam_p);

#endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)

static void EplObdCopyObjectData (
                        void MEM*       pDstData_p,
                        CONST void *    pSrcData_p,
                        tEplObdSize     ObjSize_p,
                        tEplObdType     ObjType_p);

static void * EplObdGetObjectDataPtrIntern (tEplObdSubEntryPtr pSubindexEntry_p,
                                            tEplObdAccess AccessType_p);

static tEplKernel EplObdIsNumericalIntern(tEplObdType Type_p,
                                        BOOL*         pfEntryNumerical_p);

static tEplKernel PUBLIC EplObdReadEntryPre (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr*        ppObdEntry_p,
    tEplObdSubEntryPtr*     ppSubEntry_p,
    void**                  ppSrcData_p);

static tEplKernel PUBLIC EplObdWriteEntryPre (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr*        ppObdEntry_p,
    tEplObdSubEntryPtr*     ppSubEntry_p,
    void**                  ppDstData_p);

static tEplKernel PUBLIC EplObdWriteEntryPost (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr         pObdEntry_p,
    tEplObdSubEntryPtr      pSubEntry_p,
    void*                   pDstData_p);



//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:    EplObdInit()
//
// Description: initializes the first instance
//
// Parameters:  pInitParam_p    = init parameter
//
// Return:      tEplKernel      =   errorcode
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdInit (EPL_MCO_DECL_PTR_INSTANCE_PTR_
                    tEplObdInitParam MEM* pInitParam_p)
{

tEplKernel Ret;
EPL_MCO_DELETE_INSTANCE_TABLE ();

    if (pInitParam_p == NULL)
    {
        Ret = kEplSuccessful;
        goto Exit;
    }

    Ret = EplObdAddInstance (EPL_MCO_PTR_INSTANCE_PTR_
        pInitParam_p);

Exit:
    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdAddInstance()
//
// Description: adds a new instance
//
// Parameters:  pInitParam_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdAddInstance (EPL_MCO_DECL_PTR_INSTANCE_PTR_
    tEplObdInitParam MEM* pInitParam_p)
{

EPL_MCO_DECL_INSTANCE_PTR_LOCAL
tEplKernel Ret;

    // check if pointer to instance pointer valid
    // get free instance and set the globale instance pointer
    // set also the instance addr to parameterlist
    EPL_MCO_CHECK_PTR_INSTANCE_PTR ();
    EPL_MCO_GET_FREE_INSTANCE_PTR ();
    EPL_MCO_SET_PTR_INSTANCE_PTR ();

    // save init parameters
    EPL_MEMCPY (&EPL_MCO_GLB_VAR (m_ObdInitParam), pInitParam_p, sizeof (tEplObdInitParam));

    // clear callback function for command LOAD and STORE
    EPL_MCO_GLB_VAR (m_fpStoreLoadObjCallback) = NULL;

    // clear default OD entry
    EPL_MEMSET(&EPL_MCO_GLB_VAR (m_DefaultObdEntry), 0, sizeof (EPL_MCO_GLB_VAR (m_DefaultObdEntry)));

    // clear OD handle storage
    EPL_MEMSET(&EPL_MCO_GLB_VAR (m_SaveObdParam), 0, sizeof (EPL_MCO_GLB_VAR (m_SaveObdParam)));

    // sign instance as used
    EPL_MCO_WRITE_INSTANCE_STATE (kStateUsed);

    // initialize object dictionary
    // so all all VarEntries will be initialized to trash object and default values will be set to current data
    Ret = EplObdAccessOdPart (EPL_MCO_INSTANCE_PTR_
        kEplObdPartAll, kEplObdDirInit);

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdDeleteInstance()
//
// Description: delete instance
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------
#if (EPL_USE_DELETEINST_FUNC != FALSE)
EPLDLLEXPORT tEplKernel PUBLIC EplObdDeleteInstance (EPL_MCO_DECL_INSTANCE_PTR)
{
    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    // sign instance as unused
    EPL_MCO_WRITE_INSTANCE_STATE (kStateUnused);

    return kEplSuccessful;

}
#endif // (EPL_USE_DELETEINST_FUNC != FALSE)


//---------------------------------------------------------------------------
//
// Function:    EplObdWriteEntry()
//
// Description: Function writes data to an OBD entry. Strings
//              are stored with added '\0' character.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pSrcData_p      =   Pointer to the data to write
//              Size_p          =   Size of the data in Byte
//
// Return:      tEplKernel      =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdWriteEntry (EPL_MCO_DECL_INSTANCE_PTR_
    unsigned int  uiIndex_p,
    unsigned int  uiSubIndex_p,
    void * pSrcData_p,
    tEplObdSize   Size_p)
{

tEplKernel              Ret;
tEplObdEntryPtr         pObdEntry;
tEplObdSubEntryPtr      pSubEntry;
tEplObdParam MEM        ObdParam;
void MEM*               pDstData;


    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    EPL_MEMSET(&ObdParam, 0, sizeof (ObdParam));
    ObdParam.m_uiIndex      = uiIndex_p;
    ObdParam.m_uiSubIndex   = uiSubIndex_p;
    ObdParam.m_pData        = pSrcData_p;
    ObdParam.m_TransferSize = Size_p;
    ObdParam.m_SegmentSize  = Size_p;
    ObdParam.m_ObdEvent     = kEplObdEvInitWrite;

    Ret = EplObdWriteEntryPre (EPL_MCO_INSTANCE_PTR_
                               &ObdParam,
                               &pObdEntry,
                               &pSubEntry,
                               &pDstData);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    Ret = EplObdWriteEntryPost (EPL_MCO_INSTANCE_PTR_
                                &ObdParam,
                                pObdEntry,
                                pSubEntry,
                                pDstData);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdReadEntry()
//
// Description: The function reads an object entry. The application
//              can always read the data even if attrib kEplObdAccRead
//              is not set. The attrib is only checked up for SDO transfer.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       = Index oof the OD entry to read
//              uiSubIndex_p    = Subindex to read
//              pDstData_p      = pointer to the buffer for data
//              Offset_p        = offset in data for read access
//              pSize_p         = IN: Size of the buffer
//                                OUT: number of readed Bytes
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdReadEntry (EPL_MCO_DECL_INSTANCE_PTR_
                                                unsigned int        uiIndex_p,
                                                unsigned int        uiSubIndex_p,
                                                void *              pDstData_p,
                                                tEplObdSize *       pSize_p)
{

tEplKernel              Ret;
tEplObdEntryPtr         pObdEntry;
tEplObdSubEntryPtr      pSubEntry;
tEplObdParam MEM        ObdParam;
void MEM*               pSrcData;


    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    if ((pDstData_p == NULL) || (pSize_p == NULL))
    {
        Ret = kEplInvalidParam;
        goto Exit;
    }

    EPL_MEMSET(&ObdParam, 0, sizeof (ObdParam));
    ObdParam.m_uiIndex      = uiIndex_p;
    ObdParam.m_uiSubIndex   = uiSubIndex_p;
    ObdParam.m_pData        = pDstData_p;
    ObdParam.m_TransferSize = *pSize_p;
    ObdParam.m_SegmentSize  = *pSize_p;

    Ret = EplObdReadEntryPre (EPL_MCO_INSTANCE_PTR_
                               &ObdParam,
                               &pObdEntry,
                               &pSubEntry,
                               &pSrcData);
    if (Ret != kEplSuccessful)
    {
        if (Ret == kEplObdSegmentReturned)
        {
            Ret = kEplObdValueLengthError;
        }
        goto Exit;
    }

    // read value from object
    EPL_MEMCPY (ObdParam.m_pData, pSrcData, ObdParam.m_SegmentSize);

    if ((ObdParam.m_Type == kEplObdTypVString)
        && (*pSize_p > ObdParam.m_SegmentSize))
    {   // space left to set the terminating null-character
        ((char MEM*) pDstData_p)[ObdParam.m_SegmentSize] = '\0';
    }
    *pSize_p = ObdParam.m_SegmentSize;

    // write address of destination data to structure of callback parameters
    // so callback function can change this data after reading
    ObdParam.m_pArg     = ObdParam.m_pData;
    ObdParam.m_ObdEvent = kEplObdEvPostRead;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry->m_fpCallback, &ObdParam);

Exit:

    return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplObdAccessOdPart()
//
// Description: restores default values of one part of OD
//
// Parameters:  ObdPart_p
//              Direction_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdAccessOdPart (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdPart ObdPart_p,
    tEplObdDir  Direction_p)
{

tEplKernel      Ret = kEplSuccessful;
BOOL            fPartFount;
tEplObdEntryPtr pObdEntry;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    //  part always has to be unequal to NULL
    pObdEntry = EPL_MCO_GLB_VAR (m_ObdInitParam.m_pGenericPart);
    ASSERTMSG (pObdEntry != NULL, "EplObdAccessOdPart(): no  OD part is defined!\n");

    // if ObdPart_p is not valid fPartFound keeps FALSE and function returns kEplObdIllegalPart
    fPartFount = FALSE;

    // access to  part
    if ((ObdPart_p & kEplObdPartGen) != 0)
    {
        fPartFount = TRUE;

        Ret = EplObdAccessOdPartIntern (EPL_MCO_INSTANCE_PTR_
            kEplObdPartGen, pObdEntry, Direction_p);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }
    }

    // access to manufacturer part
    pObdEntry = EPL_MCO_GLB_VAR (m_ObdInitParam.m_pManufacturerPart);

    if ( ((ObdPart_p & kEplObdPartMan) != 0) &&
         (pObdEntry != NULL) )
    {
        fPartFount = TRUE;

        Ret = EplObdAccessOdPartIntern (EPL_MCO_INSTANCE_PTR_
            kEplObdPartMan, pObdEntry, Direction_p);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }
    }

    // access to device part
    pObdEntry = EPL_MCO_GLB_VAR (m_ObdInitParam.m_pDevicePart);

    if ( ((ObdPart_p & kEplObdPartDev) != 0) &&
         (pObdEntry != NULL) )
    {
        fPartFount = TRUE;

        Ret = EplObdAccessOdPartIntern (EPL_MCO_INSTANCE_PTR_
            kEplObdPartDev, pObdEntry, Direction_p);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }
    }

    #if (defined (EPL_OBD_USER_OD) && (EPL_OBD_USER_OD != FALSE))
    {
        // access to user part
        pObdEntry = EPL_MCO_GLB_VAR (m_ObdInitParam.m_pUserPart);

        if ( ((ObdPart_p & kEplObdPartUsr) != 0) &&
             (pObdEntry != NULL) )
        {
            fPartFount = TRUE;

            Ret = EplObdAccessOdPartIntern (EPL_MCO_INSTANCE_PTR_
                kEplObdPartUsr, pObdEntry, Direction_p);
            if (Ret != kEplSuccessful)
            {
                goto Exit;
            }
        }
    }
    #endif

    // no access to an OD part was done? illegal OD part was specified!
    if (fPartFount == FALSE)
    {
        Ret = kEplObdIllegalPart;
    }

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdDefineVar()
//
// Description: defines a variable in OD
//
// Parameters:  pEplVarParam_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdDefineVar (EPL_MCO_DECL_INSTANCE_PTR_
    tEplVarParam MEM* pVarParam_p)
{

tEplKernel              Ret;
tEplObdVarEntry MEM*    pVarEntry;
tEplVarParamValid       VarValid;
tEplObdSubEntryPtr      pSubindexEntry;
tEplObdParam  MEM       ObdParam;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    ASSERT (pVarParam_p != NULL);   // is not allowed to be NULL

    ObdParam.m_uiIndex = pVarParam_p->m_uiIndex;
    ObdParam.m_uiSubIndex = pVarParam_p->m_uiSubindex;

    // get address of subindex entry
    Ret = EplObdGetEntry (EPL_MCO_INSTANCE_PTR_
        &ObdParam,
        NULL, &pSubindexEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get var entry
    Ret = EplObdGetVarEntry (pSubindexEntry, &pVarEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    VarValid =  pVarParam_p->m_ValidFlag;

    // copy only this values, which valid flag is set
    if ((VarValid & kVarValidSize) != 0)
    {
        if (pSubindexEntry->m_Type != kEplObdTypDomain)
        {
        tEplObdSize DataSize;

            // check passed size parameter
            DataSize = EplObdGetObjectSize(pSubindexEntry);
            if (DataSize != pVarParam_p->m_Size)
            {   // size of variable does not match
                Ret = kEplObdValueLengthError;
                goto Exit;
            }
        }
        else
        {   // size can be set only for objects of type DOMAIN
            pVarEntry->m_Size = pVarParam_p->m_Size;
        }
    }

    if ((VarValid & kVarValidData) != 0)
    {
       pVarEntry->m_pData = pVarParam_p->m_pData;

#ifdef EPL_MODULE_API_PDI
        // data will not be mapped to PDI by this function
       pVarEntry->m_fLinkedToPdi = FALSE;
#endif // EPL_MODULE_API_PDI

    }
/*
    #if (EPL_PDO_USE_STATIC_MAPPING == FALSE)
    {
        if ((VarValid & kVarValidCallback) != 0)
        {
           pVarEntry->m_fpCallback = pVarParam_p->m_fpCallback;
        }

        if ((VarValid & kVarValidArg) != 0)
        {
           pVarEntry->m_pArg = pVarParam_p->m_pArg;
        }
    }
    #endif
*/
    // Ret is already set to kEplSuccessful from ObdGetVarIntern()

Exit:

    return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplObdDefinePdiVar()
//
// Description: defines a variable in OD which is located in
//              Process Data Inteface (PDI). This variable can only be
//              accessed by mapping, but not by object access since it will
//              be located in a tripple buffer.
//
// Parameters:  pEplVarParam_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------
#ifdef EPL_MODULE_API_PDI
EPLDLLEXPORT tEplKernel PUBLIC EplObdDefinePdiVar (EPL_MCO_DECL_INSTANCE_PTR_
    tEplVarParam MEM* pVarParam_p)
{

tEplKernel              Ret;
tEplObdVarEntry MEM*    pVarEntry;
tEplVarParamValid       VarValid;
tEplObdSubEntryPtr      pSubindexEntry;
tEplObdParam  MEM       ObdParam;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    ASSERT (pVarParam_p != NULL);   // is not allowed to be NULL

    ObdParam.m_uiIndex = pVarParam_p->m_uiIndex;
    ObdParam.m_uiSubIndex = pVarParam_p->m_uiSubindex;

    // get address of subindex entry
    Ret = EplObdGetEntry (EPL_MCO_INSTANCE_PTR_
        &ObdParam,
        NULL, &pSubindexEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get var entry
    Ret = EplObdGetVarEntry (pSubindexEntry, &pVarEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    VarValid =  pVarParam_p->m_ValidFlag;

    // copy only this values, which valid flag is set
    if ((VarValid & kVarValidSize) != 0)
    {
        if (pSubindexEntry->m_Type != kEplObdTypDomain)
        {
        tEplObdSize DataSize;

            // check passed size parameter
            DataSize = EplObdGetObjectSize(pSubindexEntry);
            if (DataSize != pVarParam_p->m_Size)
            {   // size of variable does not match
                Ret = kEplObdValueLengthError;
                goto Exit;
            }
        }
        else
        {   // size can be set only for objects of type DOMAIN
            pVarEntry->m_Size = pVarParam_p->m_Size;
        }
    }

    if ((VarValid & kVarValidDataTPdo) != 0)
    {
        // data will be mapped to PDI
        pVarEntry->m_fLinkedToPdi = TRUE;
        pVarEntry->m_pDataMapTPdo = pVarParam_p->m_pDataMapTPdo;
    }
    if ((VarValid & kVarValidDataRPdo) != 0)
    {
        // data will be mapped to PDI
        pVarEntry->m_fLinkedToPdi = TRUE;
        pVarEntry->m_pDataMapRPdo = pVarParam_p->m_pDataMapRPdo;
    }
/*
    #if (EPL_PDO_USE_STATIC_MAPPING == FALSE)
    {
        if ((VarValid & kVarValidCallback) != 0)
        {
           pVarEntry->m_fpCallback = pVarParam_p->m_fpCallback;
        }

        if ((VarValid & kVarValidArg) != 0)
        {
           pVarEntry->m_pArg = pVarParam_p->m_pArg;
        }
    }
    #endif
*/
    // Ret is already set to kEplSuccessful from ObdGetVarIntern()

Exit:

    return Ret;

}
#endif // EPL_MODULE_API_PDI

//---------------------------------------------------------------------------
//
// Function:    EplObdGetObjectDataPtr()
//
// Description: It returnes the current data pointer. But if object is an
//              constant object it returnes the default pointer.
//
// Parameters:  uiIndex_p    =   Index of the entry
//              uiSubindex_p =   Subindex of the entry
//              AccessType_p =   request PDI address flag
//                               0: no PDI address requested
//                  kEplObdAccRead: TPDO PDI address requested
//                 kEplObdAccWrite: RPDO PDI address requested
//
// Return:      void *    = pointer to object data
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT void * PUBLIC EplObdGetObjectDataPtr (EPL_MCO_DECL_INSTANCE_PTR_
                                        unsigned int uiIndex_p,
                                        unsigned int uiSubIndex_p,
                                        tEplObdAccess AccessType_p)
 {
tEplKernel          Ret;
void *       pData;
tEplObdEntryPtr     pObdEntry;
tEplObdSubEntryPtr  pObdSubEntry;


    // get pointer to index structure
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam),
                                uiIndex_p,
                                &pObdEntry);
    if(Ret != kEplSuccessful)
    {
        pData = NULL;
        goto Exit;
    }

    // get pointer to subindex structure
    Ret = EplObdGetSubindexIntern (pObdEntry,
                                uiSubIndex_p,
                                &pObdSubEntry);
    if(Ret != kEplSuccessful)
    {
        pData = NULL;
        goto Exit;
    }

    // get Datapointer
    pData = EplObdGetObjectDataPtrIntern(pObdSubEntry, AccessType_p);

Exit:
    return pData;

}


#if (defined (EPL_OBD_USER_OD) && (EPL_OBD_USER_OD != FALSE))

//---------------------------------------------------------------------------
//
// Function:    EplObdRegisterUserOd()
//
// Description: function registers the user OD
//
// Parameters:  pUserOd_p   =pointer to user ODd
//
// Return:     tEplKernel = errorcode
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdRegisterUserOd (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdEntryPtr pUserOd_p)
{

    EPL_MCO_CHECK_INSTANCE_STATE ();

    EPL_MCO_GLB_VAR (m_ObdInitParam.m_pUserPart) = pUserOd_p;

    return kEplSuccessful;

}

#endif


//---------------------------------------------------------------------------
//
// Function:    EplObdInitVarEntry()
//
// Description: function to initialize VarEntry dependened on object type
//
// Parameters:  pVarEntry_p = pointer to var entry structure
//              Type_p      = object type
//              ObdSize_p   = size of object data
//
// Returns:     none
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT void PUBLIC EplObdInitVarEntry (EPL_MCO_DECL_INSTANCE_PTR_
                                             tEplObdVarEntry MEM* pVarEntry_p,
                                             tEplObdType Type_p, tEplObdSize ObdSize_p)
{
/*
    #if (EPL_PDO_USE_STATIC_MAPPING == FALSE)
    {
        // reset pointer to VAR callback and argument
        pVarEntry_p->m_fpCallback  = NULL;
        pVarEntry_p->m_pArg = NULL;
    }
    #endif
*/

// 10-dec-2004 r.d.: this function will not be used for strings
    if ((Type_p == kEplObdTypDomain))
//         (bType_p == kEplObdTypVString) /* ||
//         (bType_p == kEplObdTypOString) ||
//         (bType_p == kEplObdTypUString)    */ )
    {
        // variables which are defined as DOMAIN or VSTRING should not point to
        // trash object, because this trash object contains only 8 bytes. DOMAINS or
        // STRINGS can be longer.
        pVarEntry_p->m_pData = NULL;
        pVarEntry_p->m_Size  = 0;
    }
    else
    {
        // set address to variable data to trash object
        // This prevents an access violation if user forgets to call EplObdDefineVar()
        // for this variable but mappes it in a PDO.
        pVarEntry_p->m_pData = &abEplObdTrashObject_g[0];
#ifdef EPL_MODULE_API_PDI
        pVarEntry_p->m_pDataMapTPdo = &abEplObdTrashObject_g[0];
        pVarEntry_p->m_pDataMapRPdo = &abEplObdTrashObject_g[0];
#endif // EPL_MODULE_API_PDI
        pVarEntry_p->m_Size  = ObdSize_p;
    }

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetDataSize()
//
// Description: gets the data size of an object,
//              for string objects it returnes the string length
//              without terminating null-character
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = Instancepointer
//              uiIndex_p   =   Index
//              uiSubIndex_p=   Subindex
//
// Return:      tEplObdSize
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplObdSize PUBLIC EplObdGetDataSize(EPL_MCO_DECL_INSTANCE_PTR_
                                                  unsigned int uiIndex_p,
                                                  unsigned int uiSubIndex_p)
{
tEplKernel          Ret;
tEplObdSize         ObdSize;
tEplObdEntryPtr     pObdEntry;
tEplObdSubEntryPtr  pObdSubEntry;


    // get pointer to index structure
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam),
                                uiIndex_p,
                                &pObdEntry);
    if(Ret != kEplSuccessful)
    {
        ObdSize = 0;
        goto Exit;
    }

    // get pointer to subindex structure
    Ret = EplObdGetSubindexIntern (pObdEntry,
                                uiSubIndex_p,
                                &pObdSubEntry);
    if(Ret != kEplSuccessful)
    {
        ObdSize = 0;
        goto Exit;
    }

    // get size
    ObdSize = EplObdGetDataSizeIntern (pObdSubEntry);
Exit:
    return ObdSize;
}
//---------------------------------------------------------------------------
//
// Function:    EplObdGetNodeId()
//
// Description: function returns nodeid from entry 0x1F93
//
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR = Instancepointer
//
// Return:      unsigned int = Node Id
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT unsigned int PUBLIC EplObdGetNodeId(EPL_MCO_DECL_INSTANCE_PTR)
{
tEplKernel      Ret;
tEplObdSize     ObdSize;
BYTE            bNodeId;

    bNodeId = 0;
    ObdSize = sizeof(bNodeId);
    Ret = EplObdReadEntry(EPL_MCO_PTR_INSTANCE_PTR_
                            EPL_OBD_NODE_ID_INDEX,
                            EPL_OBD_NODE_ID_SUBINDEX,
                            &bNodeId,
                            &ObdSize);
    if(Ret != kEplSuccessful)
    {
        bNodeId = EPL_C_ADR_INVALID;
        goto Exit;
    }

Exit:
    return (unsigned int) bNodeId;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdSetNodeId()
//
// Description: function sets nodeid in entry 0x1F93
//
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = Instancepointer
//              uiNodeId_p  =   Node Id to set
//              NodeIdType_p=   Type on which way the Node Id was set
//
// Return:      tEplKernel = Errorcode
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdSetNodeId(EPL_MCO_DECL_PTR_INSTANCE_PTR_
                                         unsigned int uiNodeId_p,
                                         tEplObdNodeIdType NodeIdType_p)
{
tEplKernel  Ret;
tEplObdSize ObdSize;
BYTE        fHwBool;
BYTE        bNodeId;

    // check Node Id
    if(uiNodeId_p == EPL_C_ADR_INVALID)
    {
        Ret = kEplInvalidNodeId;
        goto Exit;
    }
    bNodeId = (BYTE)uiNodeId_p;
    ObdSize = sizeof(BYTE);
    // write NodeId to OD entry
    Ret = EplObdWriteEntry(EPL_MCO_PTR_INSTANCE_PTR_
                            EPL_OBD_NODE_ID_INDEX,
                            EPL_OBD_NODE_ID_SUBINDEX,
                            &bNodeId,
                            ObdSize);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // set HWBOOL-Flag in Subindex EPL_OBD_NODE_ID_HWBOOL_SUBINDEX
    switch (NodeIdType_p)
    {
        // type unknown
        case kEplObdNodeIdUnknown:
        {
            fHwBool = OBD_FALSE;
            break;
        }

        case kEplObdNodeIdSoftware:
        {
            fHwBool = OBD_FALSE;
            break;
        }

        case kEplObdNodeIdHardware:
        {
            fHwBool = OBD_TRUE;
            break;
        }

        default:
        {
            fHwBool = OBD_FALSE;
        }

    }   // end of switch (NodeIdType_p)

    // write flag
    ObdSize = sizeof(fHwBool);
    Ret = EplObdWriteEntry(EPL_MCO_PTR_INSTANCE_PTR
                            EPL_OBD_NODE_ID_INDEX,
                            EPL_OBD_NODE_ID_HWBOOL_SUBINDEX,
                            &fHwBool,
                            ObdSize);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplObdIsNumerical()
//
// Description: function checks if a entry is numerical or not
//
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = Instancepointer
//              uiIndex_p           = Index
//              uiSubIndex_p        = Subindex
//              pfEntryNumerical_p  = pointer to BOOL for returnvalue
//                                  -> TRUE if entry a numerical value
//                                  -> FALSE if entry not a numerical value
//
// Return:      tEplKernel = Errorcode
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdIsNumerical(EPL_MCO_DECL_INSTANCE_PTR_
                                        unsigned int  uiIndex_p,
                                        unsigned int  uiSubIndex_p,
                                        BOOL*         pfEntryNumerical_p)
{
tEplKernel          Ret;
tEplObdEntryPtr     pObdEntry;
tEplObdSubEntryPtr  pObdSubEntry;


    // get pointer to index structure
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam),
                                uiIndex_p,
                                &pObdEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get pointer to subindex structure
    Ret = EplObdGetSubindexIntern (pObdEntry,
                                uiSubIndex_p,
                                &pObdSubEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    Ret = EplObdIsNumericalIntern(pObdSubEntry->m_Type, pfEntryNumerical_p);


Exit:
    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetType()
//
// Description: function returns the data type of the specified entry
//
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = Instancepointer
//              uiIndex_p           = Index
//              uiSubIndex_p        = Subindex
//              pType_p             = pointer to tEplObdType for returnvalue
//
// Return:      tEplKernel = Errorcode
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdGetType(EPL_MCO_DECL_INSTANCE_PTR_
                                        unsigned int  uiIndex_p,
                                        unsigned int  uiSubIndex_p,
                                        tEplObdType*  pType_p)
{
tEplKernel          Ret;
tEplObdEntryPtr     pObdEntry;
tEplObdSubEntryPtr  pObdSubEntry;


    // get pointer to index structure
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam),
                                uiIndex_p,
                                &pObdEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get pointer to subindex structure
    Ret = EplObdGetSubindexIntern (pObdEntry,
                                uiSubIndex_p,
                                &pObdSubEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    *pType_p = pObdSubEntry->m_Type;

Exit:
    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdReadEntryToLe()
//
// Description: The function reads an object entry from the byteoder
//              of the system to the little endian byteorder for numerical values.
//              For other types a normal read will be processed. This is usefull for
//              the PDO and SDO module. The application
//              can always read the data even if attrib kEplObdAccRead
//              is not set. The attrib is only checked up for SDO transfer.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       = Index of the OD entry to read
//              uiSubIndex_p    = Subindex to read
//              pDstData_p      = pointer to the buffer for data
//              Offset_p        = offset in data for read access
//              pSize_p         = IN: Size of the buffer
//                                OUT: number of readed Bytes
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdReadEntryToLe (EPL_MCO_DECL_INSTANCE_PTR_
                                        tEplObdParam* pObdParam_p)
{
tEplKernel                      Ret;
tEplKernel                      RetCb;
tEplObdEntryPtr                 pObdEntry;
tEplObdSubEntryPtr              pSubEntry;
void *                          pSrcData;
tEplObdSize                     OrgSegmentSize;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    OrgSegmentSize = pObdParam_p->m_SegmentSize;

    Ret = EplObdReadEntryPre (EPL_MCO_INSTANCE_PTR_
                               pObdParam_p,
                               &pObdEntry,
                               &pSubEntry,
                               &pSrcData);
    if ((Ret != kEplSuccessful) && (Ret != kEplObdSegmentReturned))
    {
        goto Exit;
    }
    else
    {
        // kEplObdSegmentReturned is OK for local access, but
        // for abdopted access it should cause an exit (not an abort!?)
        Ret = kEplSuccessful;
    }

    // check if numerical type
    switch (pObdParam_p->m_Type)
    {
        //-----------------------------------------------
        // types without ami
        case kEplObdTypVString:
        case kEplObdTypOString:
        case kEplObdTypDomain:
        default:
        {
            // read value from object
            EPL_MEMCPY (pObdParam_p->m_pData, pSrcData, pObdParam_p->m_SegmentSize);

            if ((pObdParam_p->m_Type == kEplObdTypVString)
                && (OrgSegmentSize > pObdParam_p->m_SegmentSize))
            {   // space left to set the terminating null-character
                ((char MEM*) pObdParam_p->m_pData)[pObdParam_p->m_SegmentSize] = '\0';
            }
            break;
        }

        //-----------------------------------------------
        // numerical type which needs ami-write
        // 8 bit or smaller values
        case kEplObdTypBool:
        case kEplObdTypInt8:
        case kEplObdTypUInt8:
        {
            AmiSetByteToLe(pObdParam_p->m_pData, *((BYTE*)pSrcData));
            break;
        }

        // 16 bit values
        case kEplObdTypInt16:
        case kEplObdTypUInt16:
        {
            AmiSetWordToLe(pObdParam_p->m_pData, *((WORD*)pSrcData));
            break;
        }

        // 24 bit values
        case kEplObdTypInt24:
        case kEplObdTypUInt24:
        {
            AmiSetDword24ToLe(pObdParam_p->m_pData, *((DWORD*)pSrcData));
            break;
        }

        // 32 bit values
        case kEplObdTypInt32:
        case kEplObdTypUInt32:
        case kEplObdTypReal32:
        {
            AmiSetDwordToLe(pObdParam_p->m_pData, *((DWORD*)pSrcData));
            break;
        }

        // 40 bit values
        case kEplObdTypInt40:
        case kEplObdTypUInt40:
        {
            AmiSetQword40ToLe(pObdParam_p->m_pData, *((QWORD*)pSrcData));
            break;
        }

        // 48 bit values
        case kEplObdTypInt48:
        case kEplObdTypUInt48:
        {
            AmiSetQword48ToLe(pObdParam_p->m_pData, *((QWORD*)pSrcData));
            break;
        }

        // 56 bit values
        case kEplObdTypInt56:
        case kEplObdTypUInt56:
        {
            AmiSetQword56ToLe(pObdParam_p->m_pData, *((QWORD*)pSrcData));
            break;
        }

        // 64 bit values
        case kEplObdTypInt64:
        case kEplObdTypUInt64:
        case kEplObdTypReal64:
        {
            AmiSetQword64ToLe(pObdParam_p->m_pData, *((QWORD*)pSrcData));
            break;
        }

        // time of day
        case kEplObdTypTimeOfDay:
        case kEplObdTypTimeDiff:
        {
            AmiSetTimeOfDay(pObdParam_p->m_pData, ((tTimeOfDay*)pSrcData));
            break;
        }

    }// end of switch(pSubEntry->m_Type)


    // write address of destination data to structure of callback parameters
    // so callback function can change this data after reading
    pObdParam_p->m_pArg     = pObdParam_p->m_pData;
    pObdParam_p->m_ObdEvent = kEplObdEvPostReadLe;
    RetCb = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry->m_fpCallback, pObdParam_p);
    if (RetCb != kEplSuccessful)
    {
        Ret = RetCb;
    }

Exit:

    return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplObdWriteEntryFromLe()
//
// Description: Function writes data to an OBD entry from a source with
//              little endian byteorder to the od with system specuific
//              byteorder. Not numerical values will only by copied. Strings
//              are stored with added '\0' character.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pSrcData_p      =   Pointer to the data to write
//              Size_p          =   Size of the data in Byte
//
// Return:      tEplKernel      =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdWriteEntryFromLe (EPL_MCO_DECL_INSTANCE_PTR_
                                        tEplObdParam* pObdParam_p)
{
tEplKernel              Ret;
tEplObdEntryPtr         pObdEntry;
tEplObdSubEntryPtr      pSubEntry;
void MEM*               pDstData;
QWORD                   qwBuffer;
void*                   pBuffer = &qwBuffer;

    pObdParam_p->m_ObdEvent = kEplObdEvInitWriteLe;

    Ret = EplObdWriteEntryPre (EPL_MCO_INSTANCE_PTR_
                               pObdParam_p,
                               &pObdEntry,
                               &pSubEntry,
                               &pDstData);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }


    // check if numerical type
    switch (pObdParam_p->m_Type)
    {
        //-----------------------------------------------
        // types without ami
        default:
        {   // do nothing, i.e. use the given source pointer
            pBuffer = pObdParam_p->m_pData;
            break;
        }

        //-----------------------------------------------
        // numerical type which needs ami-write
        // 8 bit or smaller values
        case kEplObdTypBool:
        case kEplObdTypInt8:
        case kEplObdTypUInt8:
        {
            *((BYTE*)pBuffer) = AmiGetByteFromLe(pObdParam_p->m_pData);
            break;
        }

        // 16 bit values
        case kEplObdTypInt16:
        case kEplObdTypUInt16:
        {
            *((WORD*)pBuffer) = AmiGetWordFromLe(pObdParam_p->m_pData);
            break;
        }

        // 24 bit values
        case kEplObdTypInt24:
        case kEplObdTypUInt24:
        {
            *((DWORD*)pBuffer) = AmiGetDword24FromLe(pObdParam_p->m_pData);
            break;
        }

        // 32 bit values
        case kEplObdTypInt32:
        case kEplObdTypUInt32:
        case kEplObdTypReal32:
        {
            *((DWORD*)pBuffer) = AmiGetDwordFromLe(pObdParam_p->m_pData);
            break;
        }

        // 40 bit values
        case kEplObdTypInt40:
        case kEplObdTypUInt40:
        {
            *((QWORD*)pBuffer) = AmiGetQword40FromLe(pObdParam_p->m_pData);
            break;
        }

        // 48 bit values
        case kEplObdTypInt48:
        case kEplObdTypUInt48:
        {
            *((QWORD*)pBuffer) = AmiGetQword48FromLe(pObdParam_p->m_pData);
            break;
        }

        // 56 bit values
        case kEplObdTypInt56:
        case kEplObdTypUInt56:
        {
            *((QWORD*)pBuffer) = AmiGetQword56FromLe(pObdParam_p->m_pData);
            break;
        }

        // 64 bit values
        case kEplObdTypInt64:
        case kEplObdTypUInt64:
        case kEplObdTypReal64:
        {
            *((QWORD*)pBuffer) = AmiGetQword64FromLe(pObdParam_p->m_pData);
            break;
        }

        // time of day
        case kEplObdTypTimeOfDay:
        case kEplObdTypTimeDiff:
        {
            AmiGetTimeOfDay(pBuffer, ((tTimeOfDay*)pObdParam_p->m_pData));
            break;
        }

    }// end of switch(pSubEntry->m_Type)

    pObdParam_p->m_pData = pBuffer;

    Ret = EplObdWriteEntryPost (EPL_MCO_INSTANCE_PTR_
                                pObdParam_p,
                                pObdEntry,
                                pSubEntry,
                                pDstData);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

Exit:

    return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplObdGetAccessType()
//
// Description: Function returns accesstype of the entry
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pAccessTyp_p    =   pointer to buffer to store accesstype
//
// Return:      tEplKernel     =   errorcode
//
//
// State:
//
//---------------------------------------------------------------------------
EPLDLLEXPORT tEplKernel PUBLIC EplObdGetAccessType(EPL_MCO_DECL_INSTANCE_PTR_
                                        unsigned int uiIndex_p,
                                        unsigned int uiSubIndex_p,
                                        tEplObdAccess* pAccessTyp_p)

{
tEplKernel          Ret;
tEplObdEntryPtr     pObdEntry;
tEplObdSubEntryPtr  pObdSubEntry;


    // get pointer to index structure
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam),
                                uiIndex_p,
                                &pObdEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get pointer to subindex structure
    Ret = EplObdGetSubindexIntern (pObdEntry,
                                uiSubIndex_p,
                                &pObdSubEntry);
    if(Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // get accessType
    *pAccessTyp_p = pObdSubEntry->m_Access;


Exit:
    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplObdSearchVarEntry()
//
// Description: gets variable from OD
//
// Parameters:  uiIndex_p       =   index of the var entry to search
//              uiSubindex_p    =   subindex of var entry to search
//              ppVarEntry_p    =   pointer to the pointer to the varentry
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

tEplKernel PUBLIC EplObdSearchVarEntry (EPL_MCO_DECL_INSTANCE_PTR_
    unsigned int            uiIndex_p,
    unsigned int            uiSubindex_p,
    tEplObdVarEntry MEM**   ppVarEntry_p)
{

tEplKernel          Ret;
tEplObdSubEntryPtr  pSubindexEntry;
tEplObdParam  MEM   ObdParam;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    ObdParam.m_uiIndex = uiIndex_p;
    ObdParam.m_uiSubIndex = uiSubindex_p;

    // get address of subindex entry
    Ret = EplObdGetEntry (EPL_MCO_INSTANCE_PTR_
        &ObdParam, NULL, &pSubindexEntry);
    if (Ret == kEplSuccessful)
    {
        // get var entry
        Ret = EplObdGetVarEntry (pSubindexEntry, ppVarEntry_p);
    }

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdSetDefaultObdCallback()
//
// Description: function set address to default OD callbackfunction
//
// Parameters:  pfnCallback_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdSetDefaultObdCallback (EPL_MCO_DECL_INSTANCE_PTR_
                                                            tEplObdCallback pfnCallback_p)
{

    EPL_MCO_CHECK_INSTANCE_STATE ();

    // set new address of callback function
    EPL_MCO_GLB_VAR (m_DefaultObdEntry.m_fpCallback) = pfnCallback_p;

    return kEplSuccessful;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdSave0bdAccHdl()
//
// Description: function stores an OBD access handle and returns the storage
//              memory address to an user argument. Only one element can be
//              stored.
//
// Parameters:  ppStoreHdlPtr  [OUT] =  pointer to address were the OBD handle
//                                      memory address shall be stored
//              pObdAccParam_p [IN] =   OBD access handle
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdSave0bdAccHdl(tEplObdParam ** ppStoreHdlPtr_p,
                                                   tEplObdParam* pObdAccParam_p)
{
    if (*ppStoreHdlPtr_p != NULL)
    { // zero-initialized pointer required, otherwise its assumed that
      // the pointer is already in use
        return kEplInvalidParam;
    }

    // verify if storage is in use
    if (EPL_MCO_GLB_VAR (m_SaveObdParam).m_uiIndex == 0)
    {
        *ppStoreHdlPtr_p = &EPL_MCO_GLB_VAR (m_SaveObdParam);
    }
    else
    {   // storage is not empty
        pObdAccParam_p->m_dwAbortCode = EPL_SDOAC_OUT_OF_MEMORY;
        return kEplObdOutOfMemory;
    }

    EPL_MEMCPY(&EPL_MCO_GLB_VAR (m_SaveObdParam), pObdAccParam_p, sizeof (tEplObdParam));

    return kEplSuccessful;
}


//---------------------------------------------------------------------------
//
// Function:    EplObdDelete0bdAccHdl()
//
// Description: function resets a saved OBD access handle
//              and set memory pointer to 0.
//
// Parameters:  ppHdlPtr_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

EPLDLLEXPORT tEplKernel PUBLIC EplObdDelete0bdAccHdl(tEplObdParam ** ppHdlPtr_p)
{
    if ((*ppHdlPtr_p == NULL)                             ||
        (&EPL_MCO_GLB_VAR (m_SaveObdParam) != *ppHdlPtr_p)  )
    { // only one ObdParam (unique address) can be saved and deleted
        return kEplInvalidOperation;
    }

    // clear OD handle storage
    EPL_MEMSET(&EPL_MCO_GLB_VAR (m_SaveObdParam), 0, sizeof (EPL_MCO_GLB_VAR (m_SaveObdParam)));

    *ppHdlPtr_p = NULL;

    return kEplSuccessful;
}


//=========================================================================//
//                                                                         //
//          P R I V A T E   D E F I N I T I O N S                          //
//                                                                         //
//=========================================================================//

EPL_MCO_DECL_INSTANCE_FCT ()
//---------------------------------------------------------------------------
//
// Function:    EplObdCallObjectCallback()
//
// Description: calls callback function of an object or of a variable
//
// Parameters:  fpCallback_p
//              pCbParam_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdCallObjectCallback (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdCallback fpCallback_p,
    tEplObdParam MEM* pCbParam_p)
{

tEplKernel           Ret;
tEplObdCallback MEM  fpCallback;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    ASSERT (pCbParam_p != NULL);

    Ret = kEplSuccessful;

    // check address of callback function before calling it
    if (fpCallback_p != NULL)
    {
        // KEIL C51 V6.01 has a bug.
        // Therefore the parameter fpCallback_p has to be copied in local variable fpCallback.
        fpCallback = fpCallback_p;

        // call callback function for this object
        Ret = fpCallback (EPL_MCO_INSTANCE_PARAM_IDX_()
            pCbParam_p);
    }

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:    EplObdGetDataSizeIntern()
//
// Description: gets the data size of an object.
//              for string objects it returnes the string length
//              without terminating null-character
//
// Parameters:  pSubIndexEntry_p
//
// Return:      tEplObdSize
//
// State:
//
//---------------------------------------------------------------------------

static tEplObdSize EplObdGetDataSizeIntern (tEplObdSubEntryPtr pSubIndexEntry_p)
{

tEplObdSize DataSize = 0;
void MEM*   pData;

    if (pSubIndexEntry_p == NULL)
    {
        goto Exit;
    }

    // If OD entry is defined by macro EPL_OBD_SUBINDEX_ROM_VSTRING
    // then the current pointer is always NULL. The function
    // returns the length of default string.
    DataSize = EplObdGetObjectSize (pSubIndexEntry_p);

    if (pSubIndexEntry_p->m_Type == kEplObdTypVString)
    {
        // The pointer to current value can be received from EplObdGetObjectCurrentPtr()
        pData = ((void MEM*) EplObdGetObjectCurrentPtr (pSubIndexEntry_p, 0));
        if (pData != NULL)
        {
            DataSize = EplObdGetStrLen ((void *) pData, DataSize, pSubIndexEntry_p->m_Type);
        }

    }

Exit:
    return DataSize;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetStrLen()
//
// Description: The function calculates the length of string. The '\0'
//              character is NOT included!!
//
// Parameters:  pObjData_p          = pointer to string
//              ObjLen_p            = max. length of object entry
//              bObjType_p          = object type (VSTRING, ...)
//
// Returns:     string length
//
// State:
//
//---------------------------------------------------------------------------

static tEplObdSize EplObdGetStrLen (void * pObjData_p,
                                 tEplObdSize ObjLen_p,
                                 tEplObdType ObjType_p)
{

tEplObdSize    StrLen = 0;
BYTE *  pbString;

    if (pObjData_p == NULL)
    {
        goto Exit;
    }

    //----------------------------------------
    // Visible String: data format byte
    if (ObjType_p == kEplObdTypVString)
    {
        pbString = pObjData_p;

        for (StrLen = 0; StrLen < ObjLen_p; StrLen++)
        {
            if (*pbString == '\0')
            {
//                StrLen++;
                break;
            }

            pbString++;
        }
    }

    //----------------------------------------
    // other string types ...

Exit:
    return (StrLen);

}



#if (EPL_OBD_CHECK_OBJECT_RANGE != FALSE)

//---------------------------------------------------------------------------
//
// Function:    EplObdCheckObjectRange()
//
// Description: function to check value range of object data
//
// NOTICE: The pointer of data (pData_p) must point out to an even address,
//         if ObjType is unequal to kEplObdTypInt8 or kEplObdTypUInt8! But it is
//         always realiced because pointer m_pDefault points always to an
//         array of the SPECIFIED type.
//
// Parameters:  pSubindexEntry_p
//              pData_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdCheckObjectRange (
            tEplObdSubEntryPtr pSubindexEntry_p,
            void * pData_p)
{

tEplKernel      Ret;
const void *   pRangeData;

    //ASSERTMSG (pSubindexEntry_p != NULL,
    //    "EplObdCheckObjectRange(): no address to subindex struct!\n");

    Ret  = kEplSuccessful;
    if (pSubindexEntry_p == NULL)
    {
        goto Exit;
    }

    // check if data range has to be checked
    if ((pSubindexEntry_p->m_Access & kEplObdAccRange) == 0)
    {
        goto Exit;
    }

    // get address of default data
    pRangeData = pSubindexEntry_p->m_pDefault;

    // jump to called object type
    switch ((tEplObdType) pSubindexEntry_p->m_Type)
    {
        // -----------------------------------------------------------------
        // ObdType kEplObdTypBool will not be checked because there are only
        // two possible values 0 or 1.

        // -----------------------------------------------------------------
        // ObdTypes which has to be check up because numerical values
        case kEplObdTypInt8:

            // switch to lower limit
            pRangeData = ((tEplObdInteger8 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdInteger8 *) pData_p) < *((tEplObdInteger8 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdInteger8 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdInteger8 *) pData_p) > *((tEplObdInteger8 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypUInt8:

            // switch to lower limit
            pRangeData = ((tEplObdUnsigned8 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdUnsigned8 *) pData_p) < *((tEplObdUnsigned8 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdUnsigned8*) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdUnsigned8 *) pData_p) > *((tEplObdUnsigned8 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypInt16:

            // switch to lower limit
            pRangeData = ((tEplObdInteger16 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdInteger16 *) pData_p) < *((tEplObdInteger16 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdInteger16 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdInteger16 *) pData_p) > *((tEplObdInteger16 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypUInt16:

            // switch to lower limit
            pRangeData = ((tEplObdUnsigned16 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdUnsigned16 *) pData_p) < *((tEplObdUnsigned16 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdUnsigned16 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdUnsigned16 *) pData_p) > *((tEplObdUnsigned16 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypInt32:

            // switch to lower limit
            pRangeData = ((tEplObdInteger32 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdInteger32 *) pData_p) < *((tEplObdInteger32 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdInteger32 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdInteger32 *) pData_p) > *((tEplObdInteger32 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypUInt32:

            // switch to lower limit
            pRangeData = ((tEplObdUnsigned32 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdUnsigned32 *) pData_p) < *((tEplObdUnsigned32 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdUnsigned32 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdUnsigned32 *) pData_p) > *((tEplObdUnsigned32 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        case kEplObdTypReal32:

            // switch to lower limit
            pRangeData = ((tEplObdReal32 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdReal32 *) pData_p) < *((tEplObdReal32 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdReal32 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdReal32 *) pData_p) > *((tEplObdReal32 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt40:
        case kEplObdTypInt48:
        case kEplObdTypInt56:
        case kEplObdTypInt64:

            // switch to lower limit
            pRangeData = ((signed QWORD *) pRangeData) + 1;

            // check if value is to low
            if (*((signed QWORD *) pData_p) < *((signed QWORD *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((signed QWORD *) pRangeData) + 1;

            // check if value is to high
            if (*((signed QWORD *) pData_p) > *((signed QWORD *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        // -----------------------------------------------------------------
        case kEplObdTypUInt40:
        case kEplObdTypUInt48:
        case kEplObdTypUInt56:
        case kEplObdTypUInt64:

            // switch to lower limit
            pRangeData = ((unsigned QWORD *) pRangeData) + 1;

            // check if value is to low
            if (*((unsigned QWORD *) pData_p) < *((unsigned QWORD *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((unsigned QWORD *) pRangeData) + 1;

            // check if value is to high
            if (*((unsigned QWORD *) pData_p) > *((unsigned QWORD *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        // -----------------------------------------------------------------
        case kEplObdTypReal64:

            // switch to lower limit
            pRangeData = ((tEplObdReal64 *) pRangeData) + 1;

            // check if value is to low
            if (*((tEplObdReal64 *) pData_p) < *((tEplObdReal64 *) pRangeData))
            {
                Ret = kEplObdValueTooLow;
                break;
            }

            // switch to higher limit
            pRangeData = ((tEplObdReal64 *) pRangeData) + 1;

            // check if value is to high
            if (*((tEplObdReal64 *) pData_p) > *((tEplObdReal64 *) pRangeData))
            {
                Ret = kEplObdValueTooHigh;
            }

            break;

        // -----------------------------------------------------------------
        case kEplObdTypTimeOfDay:
        case kEplObdTypTimeDiff:
            break;

        // -----------------------------------------------------------------
        // ObdTypes kEplObdTypXString and kEplObdTypDomain can not be checkt because
        // they have no numerical value.
        default:

            Ret = kEplObdUnknownObjectType;
            break;
    }

Exit:

    return Ret;

}
#endif // (EPL_OBD_CHECK_OBJECT_RANGE != FALSE)

//---------------------------------------------------------------------------
//
// Function:    EplObdWriteEntryPre()
//
// Description: Function prepares write of data to an OBD entry. Strings
//              are stored with added '\0' character.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pSrcData_p      =   Pointer to the data to write
//              Size_p          =   Size of the data in Byte
//
// Return:      tEplKernel      =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel PUBLIC EplObdWriteEntryPre (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr*        ppObdEntry_p,
    tEplObdSubEntryPtr*     ppSubEntry_p,
    void**                  ppDstData_p)
{

tEplKernel              Ret;
tEplObdEntryPtr         pObdEntry;
tEplObdSubEntryPtr      pSubEntry;
tEplObdAccess           Access;
void MEM*               pDstData;
tEplObdSize             ObdSize;
BOOL                    fEntryNumerical;
tEplObdEvent            OrgObdEvent;

#if (EPL_OBD_USE_STRING_DOMAIN_IN_RAM != FALSE)
    tEplObdVStringDomain MEM    MemVStringDomain;
    void MEM*                   pCurrData;
#endif

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    OrgObdEvent = pObdParam_p->m_ObdEvent;

    //------------------------------------------------------------------------
    // get address of index and subindex entry
    Ret = EplObdGetEntry (EPL_MCO_INSTANCE_PTR_
        pObdParam_p, &pObdEntry, &pSubEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    Access = pObdParam_p->m_Access;

    // check if write access is allowed
    if ((Access & kEplObdAccConst) != 0)
    {
        Ret = kEplObdAccessViolation;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_WRITE_TO_READ_ONLY_OBJ;
        goto Exit;
    }

    if (((Access & kEplObdAccWrite) == 0) && (pObdParam_p->m_pRemoteAddress != NULL))
    {   // write access is not allowed for remote access via SDO
        Ret = kEplObdWriteViolation;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_WRITE_TO_READ_ONLY_OBJ;
        goto Exit;
    }

    ObdSize = EplObdGetObjectSize (pSubEntry);
    // get pointer to object data
    pDstData = (void MEM*) EplObdGetObjectDataPtrIntern (pSubEntry, 0);

    // 09-dec-2004 r.d.:
    //      Function EplObdWriteEntry() calls new event kEplObdEvWrStringDomain
    //      for String or Domain which lets called module directly change
    //      the data pointer or size. This prevents a recursive call to
    //      the callback function if it calls EplObdGetEntry().
    #if (EPL_OBD_USE_STRING_DOMAIN_IN_RAM != FALSE)
    if ((pObdParam_p->m_SegmentOffset == 0)
        && (pSubEntry != NULL)
        && ((pObdParam_p->m_Type == kEplObdTypVString)
            || (pObdParam_p->m_Type == kEplObdTypDomain)
            || (pObdParam_p->m_Type == kEplObdTypOString)))
    {
        // fill out new arg-struct
        MemVStringDomain.m_DownloadSize = pObdParam_p->m_TransferSize;
        MemVStringDomain.m_ObjSize      = ObdSize;
        MemVStringDomain.m_pData        = pDstData;

        if ((pObdParam_p->m_pRemoteAddress != NULL)
            && (pObdParam_p->m_Type == kEplObdTypVString))
        {
            // reserve one byte for 0-termination
            MemVStringDomain.m_DownloadSize++;
        }

        pObdParam_p->m_ObdEvent = kEplObdEvWrStringDomain;
        pObdParam_p->m_pArg     = &MemVStringDomain;
        //  call user callback
        Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
                pObdEntry->m_fpCallback, pObdParam_p);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }

        // write back new settings
        pCurrData = pSubEntry->m_pCurrent;
        if ((pObdParam_p->m_Type == kEplObdTypVString)
            ||(pObdParam_p->m_Type ==  kEplObdTypOString))
        {
            ((tEplObdVString MEM*) pCurrData)->m_Size    = MemVStringDomain.m_ObjSize;
            ((tEplObdVString MEM*) pCurrData)->m_pString = MemVStringDomain.m_pData;
        }
        else // if (pSdosTableEntry_p->m_bObjType == kEplObdTypDomain)
        {
        tEplObdVarEntry MEM*    pVarEntry = NULL;

            Ret = EplObdGetVarEntry(pSubEntry, &pVarEntry);
            if (Ret != kEplSuccessful)
            {
                goto Exit;
            }
            if (pVarEntry == NULL)
            {
                Ret = kEplObdAccessViolation;
                goto Exit;
            }

            pVarEntry->m_Size  = MemVStringDomain.m_ObjSize;
            pVarEntry->m_pData = (void MEM*) MemVStringDomain.m_pData;
        }

        // Because object size and object pointer are
        // adapted by user callback function, re-read
        // this values.
        ObdSize  = MemVStringDomain.m_ObjSize;
        pDstData = (void MEM*) MemVStringDomain.m_pData;
    }
    #endif //#if (OBD_USE_STRING_DOMAIN_IN_RAM != FALSE)

    // normalize the source data before calling the OD callback function
    if ((pObdParam_p->m_Type == kEplObdTypVString)
        && (((char MEM*) pObdParam_p->m_pData)[pObdParam_p->m_SegmentSize - 1] == '\0')
        && (pObdParam_p->m_SegmentSize != 0)                                           )
    {   // last byte of source string contains null character

        pObdParam_p->m_SegmentSize -= 1;

        // $$$ the transfer size is still too large
    }

    // call the OD callback function with kEplObdEvInitWrite/kEplObdEvInitWriteLe
    pObdParam_p->m_ObjSize  = ObdSize;
    pObdParam_p->m_pArg     = &pObdParam_p->m_ObjSize;
    pObdParam_p->m_ObdEvent = OrgObdEvent;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry->m_fpCallback, pObdParam_p);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // access violation if address to current value is NULL
    if (pDstData == NULL)
    {
        Ret = kEplObdAccessViolation;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_UNSUPPORTED_ACCESS;
        goto Exit;
    }

    ObdSize = pObdParam_p->m_ObjSize;

    if (pObdParam_p->m_TransferSize > ObdSize)
    {
        Ret = kEplObdValueLengthError;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_TOO_HIGH;
        goto Exit;
    }

    Ret = EplObdIsNumericalIntern(pObdParam_p->m_Type, &fEntryNumerical);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    if ((fEntryNumerical != FALSE)
        && ((pObdParam_p->m_SegmentOffset != 0)
            || (pObdParam_p->m_TransferSize != pObdParam_p->m_SegmentSize)
            || (pObdParam_p->m_SegmentSize != pObdParam_p->m_ObjSize)))
    {
        // type is numerical, therefore size has to fit, but it does not.
        Ret = kEplObdNumValSizeMismatch;

        if (pObdParam_p->m_TransferSize < ObdSize)
        {
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_TOO_LOW;
        }
        else
        {
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_NOT_MATCH;
        }

        goto Exit;
    }

    // check if offset given
    if (pObdParam_p->m_SegmentOffset != 0)
    {
        if ((pObdParam_p->m_SegmentOffset > pObdParam_p->m_ObjSize)
            || (pObdParam_p->m_SegmentSize > (pObdParam_p->m_ObjSize - pObdParam_p->m_SegmentOffset)))
        {   // segment does not fit into the object
            Ret = kEplObdValueLengthError;
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_TOO_HIGH;
            goto Exit;
        }

        pDstData = ((BYTE*) pDstData) + pObdParam_p->m_SegmentOffset;
    }

    // set output parameters
    *ppObdEntry_p = pObdEntry;
    *ppSubEntry_p = pSubEntry;
    *ppDstData_p = pDstData;

    // all checks are done
    // the caller may now convert the numerical source value to platform byte order in a temporary buffer

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdWriteEntryPost()
//
// Description: Function finishes write of data to an OBD entry. Strings
//              are stored with added '\0' character.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pSrcData_p      =   Pointer to the data to write
//              Size_p          =   Size of the data in Byte
//
// Return:      tEplKernel      =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel PUBLIC EplObdWriteEntryPost (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr         pObdEntry_p,
    tEplObdSubEntryPtr      pSubEntry_p,
    void*                   pDstData_p)
{

tEplKernel              Ret;


    // caller converted the source value to platform byte order
    // now the range of the value may be checked

    #if (EPL_OBD_CHECK_OBJECT_RANGE != FALSE)
    {
        // check data range
        Ret = EplObdCheckObjectRange (pSubEntry_p, pObdParam_p->m_pData);
        if (Ret != kEplSuccessful)
        {
            switch (Ret)
            {
                case kEplObdValueTooLow:
                {
                    pObdParam_p->m_dwAbortCode = EPL_SDOAC_VALUE_RANGE_TOO_LOW;
                    break;
                }

                case kEplObdValueTooHigh:
                {
                    pObdParam_p->m_dwAbortCode = EPL_SDOAC_VALUE_RANGE_TOO_HIGH;
                    break;
                }

                case kEplObdUnknownObjectType:
                {
                    pObdParam_p->m_dwAbortCode = EPL_SDOAC_UNSUPPORTED_ACCESS;
                    break;
                }

                default:
                {
                    break;
                }
            }

            goto Exit;
        }
    }
    #endif

    // now call user callback function to check value
    // write address of source data to structure of callback parameters
    // so callback function can check this data
    pObdParam_p->m_pArg     = pObdParam_p->m_pData;
    pObdParam_p->m_ObdEvent = kEplObdEvPreWrite;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry_p->m_fpCallback, pObdParam_p);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // copy object data to OBD
    EPL_MEMCPY (pDstData_p, pObdParam_p->m_pData, pObdParam_p->m_SegmentSize);

    // terminate string with 0
    if (pSubEntry_p->m_Type == kEplObdTypVString)
    {
        ((char MEM*) pDstData_p)[pObdParam_p->m_SegmentSize] = '\0';
    }

    // write address of destination to structure of callback parameters
    // so callback function can change data subsequently
    pObdParam_p->m_pArg     = pDstData_p;
    pObdParam_p->m_pData    = pDstData_p;
    pObdParam_p->m_ObdEvent = kEplObdEvPostWrite;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry_p->m_fpCallback, pObdParam_p);

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdReadEntryPre()
//
// Description: Function prepares write of data to an OBD entry. Strings
//              are stored with added '\0' character.
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_
//              uiIndex_p       =   Index of the OD entry
//              uiSubIndex_p    =   Subindex of the OD Entry
//              pSrcData_p      =   Pointer to the data to write
//              Size_p          =   Size of the data in Byte
//
// Return:      tEplKernel      =   Errorcode
//
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel PUBLIC EplObdReadEntryPre (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr*        ppObdEntry_p,
    tEplObdSubEntryPtr*     ppSubEntry_p,
    void**                  ppSrcData_p)
{

tEplKernel              Ret;
tEplObdEntryPtr         pObdEntry;
tEplObdSubEntryPtr      pSubEntry;
tEplObdAccess           Access;
void MEM*               pSrcData;
void MEM*               pDstData;
tEplObdSize             ObdSize;
BOOL                    fEntryNumerical;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    //------------------------------------------------------------------------
    // get address of index and subindex entry
    Ret = EplObdGetEntry (EPL_MCO_INSTANCE_PTR_
        pObdParam_p, &pObdEntry, &pSubEntry);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    Access = pObdParam_p->m_Access;

    if (((Access & kEplObdAccRead) == 0) &&
         (pObdParam_p->m_pRemoteAddress != NULL) &&
         ((Access & kEplObdAccConst) == 0)         )
    {   // read access is not allowed for remote access via SDO
        Ret = kEplObdReadViolation;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_READ_TO_WRITE_ONLY_OBJ;
        goto Exit;
    }

    // get pointer to object data
    pSrcData = EplObdGetObjectDataPtrIntern (pSubEntry, 0);

    // get size of data and check if application has reserved enough memory
    ObdSize = EplObdGetDataSizeIntern (pSubEntry);

    // save m_pData (it contains the pointer to the destination buffer)
    pDstData = pObdParam_p->m_pData;

    //------------------------------------------------------------------------
    // address of source data to structure of callback parameters
    // so callback function can change this data before reading
    pObdParam_p->m_ObjSize  = ObdSize;
    if (pSrcData == NULL)
    {
        pObdParam_p->m_pData    = NULL;
    }
    else
    {
        pObdParam_p->m_pData    = ((BYTE*) pSrcData) + pObdParam_p->m_SegmentOffset;
    }
    pObdParam_p->m_pArg     = pObdParam_p->m_pData;
    pObdParam_p->m_ObdEvent = kEplObdEvPreRead;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry->m_fpCallback, pObdParam_p);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    // check source pointer
    if (pObdParam_p->m_pData == NULL)
    {
        Ret = kEplObdAccessViolation;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_UNSUPPORTED_ACCESS;
        goto Exit;
    }

    Ret = EplObdIsNumericalIntern(pObdParam_p->m_Type, &fEntryNumerical);
    if (Ret != kEplSuccessful)
    {
        goto Exit;
    }

    if ((fEntryNumerical != FALSE)
        && ((pObdParam_p->m_SegmentOffset != 0)
            || (pObdParam_p->m_ObjSize > pObdParam_p->m_SegmentSize)
            || ((pObdParam_p->m_SegmentSize > pObdParam_p->m_ObjSize)
                && (pObdParam_p->m_TransferSize != 0))))
    {
        // type is numerical, therefore size has to fit, but it does not.
        Ret = kEplObdNumValSizeMismatch;

        if (pObdParam_p->m_TransferSize < ObdSize)
        {
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_TOO_LOW;
        }
        else
        {
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_NOT_MATCH;
        }

        goto Exit;
    }

    if (pObdParam_p->m_SegmentOffset > pObdParam_p->m_ObjSize)
    {
        Ret = kEplObdValueLengthError;
        pObdParam_p->m_dwAbortCode = EPL_SDOAC_DATA_TYPE_LENGTH_TOO_LOW;
        goto Exit;
    }

    // check if offset given and calc correct number of bytes to read
    if (pObdParam_p->m_SegmentOffset != 0)
    {
    tEplObdSize RemainingObjSize;

        Ret = kEplObdSegmentReturned;
        RemainingObjSize = pObdParam_p->m_ObjSize - pObdParam_p->m_SegmentOffset;

        if (pObdParam_p->m_SegmentSize > RemainingObjSize)
        {
            pObdParam_p->m_SegmentSize = RemainingObjSize;
        }
    }
    else if (pObdParam_p->m_SegmentSize < pObdParam_p->m_ObjSize)
    {   // segment offset == 0 and segment size is less than object size
        pObdParam_p->m_TransferSize = ObdSize;  // prepare segmented read size
        Ret = kEplObdSegmentReturned;
    }
    else
    {   // segment offset == 0 and segment size is greater or equal than object size
        pObdParam_p->m_SegmentSize = pObdParam_p->m_ObjSize;
    }

    // set output parameters
    *ppObdEntry_p = pObdEntry;
    *ppSubEntry_p = pSubEntry;
    *ppSrcData_p = pObdParam_p->m_pData;
    pObdParam_p->m_pData    = pDstData;

    // all checks are done
    // the caller may now convert the numerial source value in platform byte order to little endian byte order

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetObjectSize()
//
// Description: function to get size of object
//              The function determines if an object type an fixed data type (BYTE, WORD, ...)
//              or non fixed object (string, domain). This information is used to decide
//              if download data are stored temporary or not. For objects with fixed data length
//              and types a value range checking can process.
//              For strings the function returns the whole object size not the
//              length of string.
//
// Parameters:  pSubIndexEntry_p
//
// Return:      tEplObdSize
//
// State:
//
//---------------------------------------------------------------------------

static tEplObdSize EplObdGetObjectSize (tEplObdSubEntryPtr pSubIndexEntry_p)
{

tEplObdSize DataSize = 0;
void * pData;

    if (pSubIndexEntry_p == NULL)
    {
        goto Exit;
    }

    switch (pSubIndexEntry_p->m_Type)
    {
        // -----------------------------------------------------------------
        case kEplObdTypBool:

            DataSize = 1;
            break;

        // -----------------------------------------------------------------
        // ObdTypes which has to be check because numerical values
        case kEplObdTypInt8:
            DataSize = sizeof (tEplObdInteger8);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypUInt8:
            DataSize = sizeof (tEplObdUnsigned8);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt16:
            DataSize = sizeof (tEplObdInteger16);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypUInt16:
            DataSize = sizeof (tEplObdUnsigned16);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt32:
            DataSize = sizeof (tEplObdInteger32);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypUInt32:
            DataSize = sizeof (tEplObdUnsigned32);
            break;

        // -----------------------------------------------------------------
        case kEplObdTypReal32:
            DataSize = sizeof (tEplObdReal32);
            break;

        // -----------------------------------------------------------------
        // ObdTypes which has to be not checked because not NUM values
        case kEplObdTypDomain:
        {
        tEplObdVarEntry MEM*    pVarEntry = NULL;
        tEplKernel              Ret;

            Ret = EplObdGetVarEntry(pSubIndexEntry_p, &pVarEntry);
            if ((Ret == kEplSuccessful)
                && (pVarEntry != NULL))
            {
                DataSize = pVarEntry->m_Size;
            }
            break;
        }

        // -----------------------------------------------------------------
        case kEplObdTypVString:
        //case kEplObdTypUString:

            // If OD entry is defined by macro EPL_OBD_SUBINDEX_ROM_VSTRING
            // then the current pointer is always NULL. The function
            // returns the length of default string.
            pData = (void *) pSubIndexEntry_p->m_pCurrent;
            if ((void MEM*) pData != (void MEM*) NULL)
            {
                // The max. size of strings defined by STRING-Macro is stored in
                // tEplObdVString of current value.
                // (types tEplObdVString, tEplObdOString and tEplObdUString has the same members)
                DataSize = ((tEplObdVString MEM*) pData)->m_Size;
            }
            else
            {
                // The current position is not declared. The string
                // is located in ROM, therefore use default pointer.
                pData = (void *) pSubIndexEntry_p->m_pDefault;
                if ((CONST void ROM*) pData != (CONST void ROM*) NULL)
                {
                   // The max. size of strings defined by STRING-Macro is stored in
                   // tEplObdVString of default value.
                   DataSize = ((CONST tEplObdVString ROM*) pData)->m_Size;
                }
            }

            break;

        // -----------------------------------------------------------------
        case kEplObdTypOString:

            pData = (void *) pSubIndexEntry_p->m_pCurrent;
            if ((void MEM*) pData != (void MEM*) NULL)
            {
                // The max. size of strings defined by STRING-Macro is stored in
                // tEplObdVString of current value.
                // (types tEplObdVString, tEplObdOString and tEplObdUString has the same members)
                DataSize = ((tEplObdOString MEM*) pData)->m_Size;
            }
            else
            {
                // The current position is not declared. The string
                // is located in ROM, therefore use default pointer.
                pData = (void *) pSubIndexEntry_p->m_pDefault;
                if ((CONST void ROM*) pData != (CONST void ROM*) NULL)
                {
                   // The max. size of strings defined by STRING-Macro is stored in
                   // tEplObdVString of default value.
                   DataSize = ((CONST tEplObdOString ROM*) pData)->m_Size;
                }
            }
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt24:
        case kEplObdTypUInt24:

            DataSize = 3;
            break;


        // -----------------------------------------------------------------
        case kEplObdTypInt40:
        case kEplObdTypUInt40:

            DataSize = 5;
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt48:
        case kEplObdTypUInt48:

            DataSize = 6;
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt56:
        case kEplObdTypUInt56:

            DataSize = 7;
            break;

        // -----------------------------------------------------------------
        case kEplObdTypInt64:
        case kEplObdTypUInt64:
        case kEplObdTypReal64:

            DataSize = 8;
            break;

        // -----------------------------------------------------------------
        case kEplObdTypTimeOfDay:
        case kEplObdTypTimeDiff:

            DataSize = 6;
            break;

        // -----------------------------------------------------------------
        default:
            break;
    }

Exit:
    return DataSize;
}

//---------------------------------------------------------------------------
//
// Function:    EplObdGetObjectDefaultPtr()
//
// Description: function to get the default pointer (type specific)
//
// Parameters:  pSubIndexEntry_p    = pointer to subindex structure
//
// Returns:     (CONST void *)      = pointer to default value
//
// State:
//
//---------------------------------------------------------------------------

static CONST void* EplObdGetObjectDefaultPtr (tEplObdSubEntryPtr pSubIndexEntry_p)
{

CONST void*     pDefault;
tEplObdType     Type;

    ASSERTMSG (pSubIndexEntry_p != NULL, "EplObdGetObjectDefaultPtr(): pointer to SubEntry not valid!\n");

    // get address to default data from default pointer
    pDefault = pSubIndexEntry_p->m_pDefault;
    if (pDefault != NULL)
    {
        // there are some special types, whose default pointer always is NULL or has to get from other structure
        // get type from subindex structure
        Type = pSubIndexEntry_p->m_Type;

        // check if object type is a string value
        if ((Type == kEplObdTypVString) /* ||
            (Type == kEplObdTypUString) */ )
        {

            // EPL_OBD_SUBINDEX_RAM_VSTRING
            //    tEplObdSize         m_Size;       --> size of default string
            //    char *    m_pDefString; --> pointer to  default string
            //    char *    m_pString;    --> pointer to string in RAM
            //
            pDefault = ((tEplObdVStringDef *) pDefault)->m_pDefString;
        }
        else if(Type == kEplObdTypOString)
        {
             pDefault = ((tEplObdOStringDef *) pDefault)->m_pDefString;
        }
    }

    return pDefault;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetVarEntry()
//
// Description: gets a variable entry of an object
//
// Parameters:  pSubindexEntry_p
//              ppVarEntry_p
//
// Return:      tCopKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdGetVarEntry (
    tEplObdSubEntryPtr    pSubindexEntry_p,
    tEplObdVarEntry MEM** ppVarEntry_p)
{

tEplKernel Ret = kEplObdVarEntryNotExist;

    ASSERT (ppVarEntry_p != NULL);   // is not allowed to be NULL
    //ASSERT (pSubindexEntry_p != NULL);
    if (pSubindexEntry_p == NULL)
    {
        goto Exit;
    }

    // check VAR-Flag - only this object points to variables
    if ((pSubindexEntry_p->m_Access & kEplObdAccVar) != 0)
    {
        // check if object is an array
        if ((pSubindexEntry_p->m_Access & kEplObdAccArray) != 0)
        {
            *ppVarEntry_p = &((tEplObdVarEntry MEM*) pSubindexEntry_p->m_pCurrent)[pSubindexEntry_p->m_uiSubIndex - 1];
        }
        else
        {
            *ppVarEntry_p = (tEplObdVarEntry MEM*) pSubindexEntry_p->m_pCurrent;
        }

        Ret = kEplSuccessful;
    }

Exit:
    return Ret;

}

//---------------------------------------------------------------------------
//
// Function:    EplObdGetEntry()
//
// Description: gets a index entry from OD
//
// Parameters:  uiIndex_p       =   Index number
//              uiSubindex_p    =   Subindex number
//              ppObdEntry_p    =   pointer to the pointer to the entry
//              ppObdSubEntry_p =   pointer to the pointer to the subentry
//
// Return:      tEplKernel

//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel  EplObdGetEntry (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdParam MEM*       pObdParam_p,
    tEplObdEntryPtr*        ppObdEntry_p,
    tEplObdSubEntryPtr*     ppObdSubEntry_p)
{

tEplObdEntryPtr         pObdEntry;
tEplKernel              Ret;

    // check for all API function if instance is valid
    EPL_MCO_CHECK_INSTANCE_STATE ();

    //------------------------------------------------------------------------
    // get address of entry of index
    Ret = EplObdGetIndexIntern (&EPL_MCO_GLB_VAR (m_ObdInitParam), pObdParam_p->m_uiIndex, &pObdEntry);
    if (Ret != kEplSuccessful)
    {
        // prepare for default OBD access
        if (EPL_MCO_GLB_VAR (m_DefaultObdEntry.m_fpCallback) == NULL)
        {   // no default OD callback function registered
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_OBJECT_NOT_EXIST;
            goto Exit;
        }
        pObdEntry = &EPL_MCO_GLB_VAR (m_DefaultObdEntry);
        pObdEntry->m_uiIndex = pObdParam_p->m_uiIndex;

        *ppObdSubEntry_p = NULL;

        pObdParam_p->m_Access = kEplObdAccRW;
        pObdParam_p->m_Type = kEplObdTypDomain;
    }
    else
    {
        //------------------------------------------------------------------------
        // get address of entry of subindex
        Ret = EplObdGetSubindexIntern (pObdEntry, pObdParam_p->m_uiSubIndex, ppObdSubEntry_p);
        if (Ret != kEplSuccessful)
        {
            pObdParam_p->m_dwAbortCode = EPL_SDOAC_SUB_INDEX_NOT_EXIST;
            goto Exit;
        }

#ifdef EPL_MODULE_API_PDI
        if ((pObdParam_p->m_uiIndex >= 0x2000)
#ifdef CONFIG_APP_OBJECTS_PRESET_AT_PCP_ONLY
             && (pObdParam_p->m_uiIndex != 0x5011)
#ifdef CONFIG_USE_SDC_OBJECTS
             && (pObdParam_p->m_uiIndex != 0x5020)
#endif //CONFIG_USE_SDC_OBJECTS
#endif // CONFIG_MAN_DEV_OBJECTS_PRESET_AT_PCP
            )
        {
            // prepare for default OBD access
            if (EPL_MCO_GLB_VAR (m_DefaultObdEntry.m_fpCallback) == NULL)
            {   // no default OD callback function registered
                pObdParam_p->m_dwAbortCode = EPL_SDOAC_OBJECT_NOT_EXIST;
                goto Exit;
            }
            pObdEntry = &EPL_MCO_GLB_VAR (m_DefaultObdEntry);
            pObdEntry->m_uiIndex = pObdParam_p->m_uiIndex;
        }
#endif  // EPL_MODULE_API_PDI

        pObdParam_p->m_Access = (*ppObdSubEntry_p)->m_Access;
        pObdParam_p->m_Type = (*ppObdSubEntry_p)->m_Type;
    }


    //------------------------------------------------------------------------
    // call callback function to inform user/stack that an object will be searched
    // if the called module returnes an error then we abort the searching with kEplObdIndexNotExist
    pObdParam_p->m_pArg       = NULL;
    pObdParam_p->m_ObdEvent   = kEplObdEvCheckExist;
    Ret = EplObdCallObjectCallback (EPL_MCO_INSTANCE_PTR_
        pObdEntry->m_fpCallback, pObdParam_p);
    if (Ret != kEplSuccessful)
    {
        Ret = kEplObdIndexNotExist;
        goto Exit;
    }

    //------------------------------------------------------------------------
    // it is allowed to set ppObdEntry_p to NULL
    // if so, no address will be written to calling function
    if (ppObdEntry_p != NULL)
    {
        *ppObdEntry_p = pObdEntry;
    }

Exit:

    return Ret;

}
//---------------------------------------------------------------------------
//
// Function:    EplObdGetObjectCurrentPtr()
//
// Description: function to get Current pointer (type specific)
//
// Parameters:  pSubindexEntry_p = pointer to subindex entry
//              AccessType_p     = request PDI address flag
//                                 0: no PDI address requested
//                    kEplObdAccRead: TPDO PDI address requested
//                   kEplObdAccWrite: RPDO PDI address requested
//
// Return:      void MEM*
//
// State:
//
//---------------------------------------------------------------------------

static void MEM* EplObdGetObjectCurrentPtr (tEplObdSubEntryPtr pSubIndexEntry_p,
                                            tEplObdAccess AccessType_p)
{

void MEM*       pData;
unsigned int    uiArrayIndex;
tEplObdSize     Size;

    pData = pSubIndexEntry_p->m_pCurrent;

    // check if constant object
    if (pData != NULL)
    {
        // check if object is an array
        if ((pSubIndexEntry_p->m_Access & kEplObdAccArray) != 0)
        {
            // calculate correct data pointer
            uiArrayIndex = pSubIndexEntry_p->m_uiSubIndex - 1;
            if ((pSubIndexEntry_p->m_Access & kEplObdAccVar) != 0)
            {
                Size = sizeof (tEplObdVarEntry);
            }
            else
            {
                Size = EplObdGetObjectSize (pSubIndexEntry_p);
            }
            pData = ((BYTE MEM*) pData) + (Size * uiArrayIndex);
        }

        // check if VarEntry
        if ((pSubIndexEntry_p->m_Access & kEplObdAccVar) != 0)
        {

#ifdef EPL_MODULE_API_PDI
            if ( ((tEplObdVarEntry MEM*) pData)->m_fLinkedToPdi == FALSE)
            {
                // user variables not linked to PDI memory, return ordinary data pointer
                // normalize access type
                AccessType_p = 0;
            }


            switch (AccessType_p)
            {

                case kEplObdAccRead:
                {
                    // TPDO address requested
                    // The data pointer is stored in VarEntry->m_pDataMapTPdo
                    pData = ((tEplObdVarEntry MEM*) pData)->m_pDataMapTPdo;
                    break;
                }

                case kEplObdAccWrite:
                {
                    // RPDO address requested
                    // The data pointer is stored in VarEntry->m_pDataMapRPdo
                    pData = ((tEplObdVarEntry MEM*) pData)->m_pDataMapRPdo;
                    break;
                }

                case 0:
                {
                    // No PDI address present or requested
                    // The data pointer is stored in VarEntry->m_pData
                    pData = ((tEplObdVarEntry MEM*) pData)->m_pData;

                    break;
                }

                default:
                {
                    // invalid case
                    pData = NULL;
                    break;
                }
            }
#else
            // The data pointer is stored in VarEntry->m_pData
            pData = ((tEplObdVarEntry MEM*) pData)->m_pData;

#endif // EPL_MODULE_API_PDI
        }

        // the default pointer is stored for strings in tEplObdVString
        else if ((pSubIndexEntry_p->m_Type == kEplObdTypVString) /* ||
            (pSubIndexEntry_p->m_Type == kEplObdTypUString)    */ )
        {
            pData = (void MEM*) ((tEplObdVString MEM*) pData)->m_pString;
        }
        else if (pSubIndexEntry_p->m_Type == kEplObdTypOString)
        {
            pData = (void MEM*) ((tEplObdOString MEM*) pData)->m_pString;
        }
    }

    return pData;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetIndexIntern()
//
// Description: gets a index entry from OD
//
// Parameters:  pInitParam_p
//              uiIndex_p
//              ppObdEntry_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdGetIndexIntern (
                        tEplObdInitParam MEM*    pInitParam_p,
                        unsigned int             uiIndex_p,
                        tEplObdEntryPtr*         ppObdEntry_p)
{

tEplObdEntryPtr pObdEntry;
tEplKernel      Ret;
unsigned int    uiIndex;

#if (defined (EPL_OBD_USER_OD) && (EPL_OBD_USER_OD != FALSE))

unsigned int  nLoop;

    // if user OD is used then objekts also has to be searched in user OD
    // there is less code need if we do this in a loop
    nLoop = 2;

#endif

    ASSERTMSG (ppObdEntry_p != NULL, "EplObdGetIndexIntern(): pointer to index entry is NULL!\n");

    Ret = kEplObdIndexNotExist;

    // get start address of OD part
    // start address depends on object index because
    // object dictionary is divided in 3 parts
    if ((uiIndex_p >= 0x1000) && (uiIndex_p < 0x2000))
    {
        pObdEntry = pInitParam_p->m_pGenericPart;
    }
    else if ((uiIndex_p >= 0x2000) && (uiIndex_p < 0x6000))
    {
        pObdEntry = pInitParam_p->m_pManufacturerPart;
    }

    // index range 0xA000 to 0xFFFF is reserved for DSP-405
    // DS-301 defines that range 0x6000 to 0x9FFF (!!!) is stored if "store" was written to 0x1010/3.
    // Therefore default configuration is OBD_INCLUDE_A000_TO_DEVICE_PART = FALSE.
    // But a CANopen Application which does not implement dynamic OD or user-OD but wants to use static objets 0xA000...
    // should set OBD_INCLUDE_A000_TO_DEVICE_PART to TRUE.

#if (EPL_OBD_INCLUDE_A000_TO_DEVICE_PART == FALSE)
    else if ((uiIndex_p >= 0x6000) && (uiIndex_p < 0x9FFF))
#else
    else if ((uiIndex_p >= 0x6000) && (uiIndex_p < 0xFFFF))
#endif
    {
        pObdEntry = pInitParam_p->m_pDevicePart;
    }


#if (defined (EPL_OBD_USER_OD) && (EPL_OBD_USER_OD != FALSE))

    // if index does not match in static OD then index only has to be searched in user OD
    else
    {
        // begin from first entry of user OD part
        pObdEntry = pInitParam_p->m_pUserPart;

        // no user OD is available
        if (pObdEntry == NULL)
        {
            goto Exit;
        }

        // loop must only run once
        nLoop = 1;
    }

    do
    {

#else

        // no user OD is available
        // so other object can be found in OD
        else
        {
            Ret = kEplObdIllegalPart;
            goto Exit;
        }

#endif

        // note:
        // The end of Index table is marked with m_uiIndex = 0xFFFF.
        // If this function will be called with wIndex_p = 0xFFFF, entry
        // should not be found. Therefor it is important to use
        // while{} instead of do{}while !!!

        // get first index of index table
        uiIndex = pObdEntry->m_uiIndex;

        // search Index in OD part
        while (uiIndex != EPL_OBD_TABLE_INDEX_END)
        {
            // go to the end of this function if index is found
            if (uiIndex_p == uiIndex)
            {
                // write address of OD entry to calling function
                *ppObdEntry_p = pObdEntry;
                Ret = kEplSuccessful;
                goto Exit;
            }

            // objects are sorted in OD
            // if the current index in OD is greater than the index which is to search then break loop
            // in this case user OD has to be search too
            if (uiIndex_p < uiIndex)
            {
                break;
            }

            // next entry in index table
            pObdEntry++;

            // get next index of index table
            uiIndex = pObdEntry->m_uiIndex;
        }

#if (defined (EPL_OBD_USER_OD) && (EPL_OBD_USER_OD != FALSE))

        // begin from first entry of user OD part
        pObdEntry = pInitParam_p->m_pUserPart;

        // no user OD is available
        if (pObdEntry == NULL)
        {
            goto Exit;
        }

        // switch next loop for user OD
        nLoop--;

    } while (nLoop > 0);

#endif

    // in this line Index was not found

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdGetSubindexIntern()
//
// Description: gets a subindex entry from a index entry
//
// Parameters:  pObdEntry_p
//              bSubIndex_p
//              ppObdSubEntry_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdGetSubindexIntern (
    tEplObdEntryPtr           pObdEntry_p,
    unsigned int              uiSubIndex_p,
    tEplObdSubEntryPtr*       ppObdSubEntry_p)
{

tEplObdSubEntryPtr pSubEntry;
unsigned int       nSubIndexCount;
tEplKernel         Ret;

    ASSERTMSG (pObdEntry_p != NULL, "EplObdGetSubindexIntern(): pointer to index is NULL!\n");
    ASSERTMSG (ppObdSubEntry_p != NULL, "EplObdGetSubindexIntern(): pointer to subindex is NULL!\n");

    Ret = kEplObdSubindexNotExist;

    // get start address of subindex table and count of subindices
    pSubEntry     = pObdEntry_p->m_pSubIndex;
    nSubIndexCount =  pObdEntry_p->m_uiCount;
    ASSERTMSG ((pSubEntry != NULL) && (nSubIndexCount > 0),
        "ObdGetSubindexIntern(): invalid subindex table within index table!\n");   // should never be NULL

    // search subindex in subindex table
    while (nSubIndexCount > 0)
    {
        // check if array is found
        if ((pSubEntry->m_Access & kEplObdAccArray) != 0)
        {
            // check if subindex is in range
            if (uiSubIndex_p < pObdEntry_p->m_uiCount)
            {
                // update subindex number (subindex entry of an array is always in RAM !!!)
                pSubEntry->m_uiSubIndex = uiSubIndex_p;
                *ppObdSubEntry_p = pSubEntry;
                Ret = kEplSuccessful;
                goto Exit;
            }
        }

        // go to the end of this function if subindex is found
        else if (uiSubIndex_p == pSubEntry->m_uiSubIndex)
        {
            *ppObdSubEntry_p = pSubEntry;
            Ret = kEplSuccessful;
            goto Exit;
        }

        // objects are sorted in OD
        // if the current subindex in OD is greater than the subindex which is to search then break loop
        // in this case user OD has to be search too
        if (uiSubIndex_p < pSubEntry->m_uiSubIndex)
        {
            break;
        }

        pSubEntry++;
        nSubIndexCount--;
    }

    // in this line SubIndex was not fount

Exit:

    return Ret;

}


//---------------------------------------------------------------------------
//
// Function:    EplObdSetStoreLoadObjCallback()
//
// Description: function set address to callbackfunction for command Store and Load
//
// Parameters:  fpCallback_p
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------
#if (EPL_OBD_USE_STORE_RESTORE != FALSE)
EPLDLLEXPORT tEplKernel PUBLIC EplObdSetStoreLoadObjCallback (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdStoreLoadObjCallback fpCallback_p)
{

    EPL_MCO_CHECK_INSTANCE_STATE ();

    // set new address of callback function
    EPL_MCO_GLB_VAR (m_fpStoreLoadObjCallback) = fpCallback_p;

    return kEplSuccessful;

}
#endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)


//---------------------------------------------------------------------------
//
// Function:    EplObdAccessOdPartIntern()
//
// Description: runs through OD and executes a job
//
// Parameters:  CurrentOdPart_p
//              pObdEnty_p
//              Direction_p     = what is to do (load values from flash or EEPROM, store, ...)
//
// Return:      tEplKernel
//
// State:
//
//---------------------------------------------------------------------------

static tEplKernel EplObdAccessOdPartIntern (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdPart     CurrentOdPart_p,
    tEplObdEntryPtr pObdEnty_p,
    tEplObdDir      Direction_p)
{

tEplObdSubEntryPtr          pSubIndex;
unsigned int                nSubIndexCount;
tEplObdAccess               Access;
void MEM*                   pDstData;
CONST void*                 pDefault;
tEplObdSize                 ObjSize;
tEplKernel                  Ret;
tEplObdCbStoreParam MEM     CbStore;
tEplObdVarEntry MEM*        pVarEntry = 0;

    ASSERT (pObdEnty_p != NULL);

    Ret = kEplSuccessful;

    // prepare structure for STORE RESTORE callback function
    CbStore.m_bCurrentOdPart = (BYTE) CurrentOdPart_p;
    CbStore.m_pData          = NULL;
    CbStore.m_ObjSize        = 0;

    // command of first action depends on direction to access
    #if (EPL_OBD_USE_STORE_RESTORE != FALSE)
    if (Direction_p == kEplObdDirLoad)
    {
        CbStore.m_bCommand = (BYTE) kEplObdCommOpenRead;

        // call callback function for previous command
        Ret = EplObdCallStoreCallback (EPL_MCO_INSTANCE_PTR_
            &CbStore);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }

        // set command for index and subindex loop
        CbStore.m_bCommand = (BYTE) kEplObdCommReadObj;
    }
    else if (Direction_p == kEplObdDirStore)
    {
        CbStore.m_bCommand = (BYTE) kEplObdCommOpenWrite;

        // call callback function for previous command
        Ret = EplObdCallStoreCallback (EPL_MCO_INSTANCE_PTR_
            &CbStore);
        if (Ret != kEplSuccessful)
        {
            goto Exit;
        }

        // set command for index and subindex loop
        CbStore.m_bCommand = (BYTE) kEplObdCommWriteObj;
    }
    #endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)

    // we should not restore the OD values here
    // the next NMT command "Reset Node" or "Reset Communication" resets the OD data
    if (Direction_p != kEplObdDirRestore)
    {
        // walk through OD part till end is found
        while (pObdEnty_p->m_uiIndex != EPL_OBD_TABLE_INDEX_END)
        {
            // get address to subindex table and count of subindices
            pSubIndex     = pObdEnty_p->m_pSubIndex;
            nSubIndexCount = pObdEnty_p->m_uiCount;
            ASSERT ((pSubIndex != NULL) && (nSubIndexCount > 0));    // should never be NULL

            // walk through subindex table till all subinices were restored
            while (nSubIndexCount != 0)
            {
                Access = (tEplObdAccess) pSubIndex->m_Access;

                // get pointer to current and default data
                pDefault = EplObdGetObjectDefaultPtr (pSubIndex);
                pDstData = EplObdGetObjectCurrentPtr (pSubIndex, 0);

                // NOTE (for kEplObdTypVString):
                //      The function returnes the max. number of bytes for a
                //      current string.
                //      r.d.: For stings the default-size will be read in other lines following (kEplObdDirInit).
                ObjSize  = EplObdGetObjectSize (pSubIndex);

                // switch direction of OD access
                switch (Direction_p)
                {
                    // --------------------------------------------------------------------------
                    // VarEntry structures has to be initialized
                    case kEplObdDirInit:

                        // If VAR-Flag is set, m_pCurrent means not address of data
                        // but address of tEplObdVarEntry. Address of data has to be get from
                        // this structure.
                        if ((Access & kEplObdAccVar) != 0)
                        {
                            EplObdGetVarEntry (pSubIndex, &pVarEntry);
                            EplObdInitVarEntry (pVarEntry, pSubIndex->m_Type, ObjSize);
/*
                            if ((Access & kEplObdAccArray) == 0)
                            {
                                EplObdInitVarEntry (pSubIndex->m_pCurrent, pSubIndex->m_Type, ObjSize);
                            }
                            else
                            {
                                EplObdInitVarEntry ((tEplObdVarEntry MEM*) (((BYTE MEM*) pSubIndex->m_pCurrent) + (sizeof (tEplObdVarEntry) * pSubIndex->m_uiSubIndex)),
                                    pSubIndex->m_Type, ObjSize);
                            }
*/
                            // at this time no application variable is defined !!!
                            // therefore data can not be copied.
                            break;
                        }
                        else if (pSubIndex->m_Type == kEplObdTypVString)
                        {
                            // If pointer m_pCurrent is not equal to NULL then the
                            // string was defined with EPL_OBD_SUBINDEX_RAM_VSTRING. The current
                            // pointer points to struct tEplObdVString located in MEM.
                            // The element size includes the max. number of
                            // bytes. The element m_pString includes the pointer
                            // to string in MEM. The memory location of default string
                            // must be copied to memory location of current string.

                            pDstData = pSubIndex->m_pCurrent;
                            if (pDstData != NULL)
                            {
                                // 08-dec-2004: code optimization !!!
                                //              entries ((tEplObdVStringDef ROM*) pSubIndex->m_pDefault)->m_pString
                                //              and ((tEplObdVStringDef ROM*) pSubIndex->m_pDefault)->m_Size were read
                                //              twice. thats not necessary!

                                // For copying data we have to set the destination pointer to the real RAM string. This
                                // pointer to RAM string is located in default string info structure. (translated r.d.)
                                pDstData = (void MEM*) ((tEplObdVStringDef ROM*) pSubIndex->m_pDefault)->m_pString;
                                ObjSize  = ((tEplObdVStringDef ROM*) pSubIndex->m_pDefault)->m_Size;


                                ((tEplObdVString MEM*) pSubIndex->m_pCurrent)->m_pString = pDstData;
                                ((tEplObdVString MEM*) pSubIndex->m_pCurrent)->m_Size    = ObjSize;
                            }

                        }
                        else if(pSubIndex->m_Type == kEplObdTypOString)
                        {
                            pDstData = pSubIndex->m_pCurrent;
                            if (pDstData != NULL)
                            {
                                // 08-dec-2004: code optimization !!!
                                //              entries ((tEplObdOStringDef ROM*) pSubIndex->m_pDefault)->m_pString
                                //              and ((tEplObdOStringDef ROM*) pSubIndex->m_pDefault)->m_Size were read
                                //              twice. thats not necessary!

                                // For copying data we have to set the destination pointer to the real RAM string. This
                                // pointer to RAM string is located in default string info structure. (translated r.d.)
                                pDstData = (void MEM*) ((tEplObdOStringDef ROM*) pSubIndex->m_pDefault)->m_pString;
                                ObjSize  = ((tEplObdOStringDef ROM*) pSubIndex->m_pDefault)->m_Size;


                                ((tEplObdOString MEM*) pSubIndex->m_pCurrent)->m_pString = pDstData;
                                ((tEplObdOString MEM*) pSubIndex->m_pCurrent)->m_Size    = ObjSize;
                            }

                        }


                        // no break !! because copy of data has to done too.

                    // --------------------------------------------------------------------------
                    // all objects has to be restored with default values
                    case kEplObdDirRestore:

                        // 09-dec-2004 r.d.: optimization! the same code for kEplObdDirRestore and kEplObdDirLoad
                        //                   is replaced to function ObdCopyObjectData() with a new parameter.


                        // restore object data for init phase
                        EplObdCopyObjectData (pDstData, pDefault, ObjSize, pSubIndex->m_Type);
                        break;

                    // --------------------------------------------------------------------------
                    // objects with attribute kEplObdAccStore has to be load from EEPROM or from a file
                    case kEplObdDirLoad:

                        // restore object data for init phase
                        EplObdCopyObjectData (pDstData, pDefault, ObjSize, pSubIndex->m_Type);

                        // no break !! because callback function has to be called too.

                    // --------------------------------------------------------------------------
                    // objects with attribute kEplObdAccStore has to be stored in EEPROM or in a file
                    case kEplObdDirStore:

                        // when attribute kEplObdAccStore is set, then call callback function
                        #if (EPL_OBD_USE_STORE_RESTORE != FALSE)
                        if ((Access & kEplObdAccStore) != 0)
                        {
                            // fill out data pointer and size of data
                            CbStore.m_pData    = pDstData;
                            CbStore.m_ObjSize  = ObjSize;

                            // call callback function for read or write object
                            Ret = EplObdCallStoreCallback (EPL_MCO_INSTANCE_PTR_
                                &CbStore);
                            if (Ret != kEplSuccessful)
                            {
                                goto Exit;
                            }
                        }
                        #endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)
                        break;


                    // --------------------------------------------------------------------------
                    // if OD Builder key has to be checked no access to subindex and data should be made
                    case kEplObdDirOBKCheck:

                        // no break !! because we want to break the second loop too.


                    // --------------------------------------------------------------------------
                    // unknown Direction
                    default:

                        // so we can break the second loop earler
                        nSubIndexCount = 1;
                        break;
                }

                nSubIndexCount--;

                // next subindex entry
                if ((Access & kEplObdAccArray) == 0)
                {
                    pSubIndex++;
                    if ((nSubIndexCount > 0)
                        && ((pSubIndex->m_Access & kEplObdAccArray) != 0))
                    {
                        // next subindex points to an array
                        // reset subindex number
                        pSubIndex->m_uiSubIndex = 1;
                    }
                }
                else
                {
                    if (nSubIndexCount > 0)
                    {
                        // next subindex points to an array
                        // increment subindex number
                        pSubIndex->m_uiSubIndex++;
                    }
                }
            }

            // next index entry
            pObdEnty_p++;
        }
    }

    // -----------------------------------------------------------------------------------------
    // command of last action depends on direction to access
    if (Direction_p == kEplObdDirOBKCheck)
    {

        goto Exit;
    }
    #if (EPL_OBD_USE_STORE_RESTORE != FALSE)
    else
    {
        if (Direction_p == kEplObdDirLoad)
        {
            CbStore.m_bCommand = (BYTE) kEplObdCommCloseRead;
        }
        else if (Direction_p == kEplObdDirStore)
        {
            CbStore.m_bCommand = (BYTE) kEplObdCommCloseWrite;
        }
        else if (Direction_p == kEplObdDirRestore)
        {
            CbStore.m_bCommand = (BYTE) kEplObdCommClear;
        }
        else
        {
            goto Exit;
        }

        // call callback function for last command
        Ret = EplObdCallStoreCallback (EPL_MCO_INSTANCE_PTR_
            &CbStore);
    }
    #endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)

//    goto Exit;

Exit:

    return Ret;

}


// ----------------------------------------------------------------------------
// Function:    EplObdCopyObjectData()
//
// Description: checks pointers to object data and copy them from source to destination
//
// Parameters:  pDstData_p              = destination pointer
//              pSrcData_p              = source pointer
//              ObjSize_p               = size of object
//              ObjType_p               =
//
// Returns:     tEplKernel              = error code
// ----------------------------------------------------------------------------

static void EplObdCopyObjectData (
            void MEM*           pDstData_p,
            CONST void *        pSrcData_p,
            tEplObdSize         ObjSize_p,
            tEplObdType         ObjType_p)
{


tEplObdSize StrSize = 0;


    // it is allowed to set default and current address to NULL (nothing to copy)
    if (pDstData_p != NULL)
    {

        if (ObjType_p == kEplObdTypVString)
        {
            // The function calculates the really number of characters of string. The
            // object entry size can be bigger as string size of default string.
            // The '\0'-termination is NOT included. A string with no characters has a
            // size of 0.
            StrSize = EplObdGetStrLen ((void *) pSrcData_p, ObjSize_p, kEplObdTypVString);

            // If the string length is greater than or equal to the entry size in OD then only copy
            // entry size - 1 and always set the '\0'-termination.
            if (StrSize >= ObjSize_p)
            {
                StrSize = ObjSize_p - 1;
            }
        }

        if (pSrcData_p != NULL)
        {
            // copy data
            EPL_MEMCPY (pDstData_p, pSrcData_p, ObjSize_p);

            if (ObjType_p == kEplObdTypVString)
            {
                ((char MEM*) pDstData_p)[StrSize] = '\0';
            }
        }
    }

}


//---------------------------------------------------------------------------
//
// Function:    EplObdIsNumericalIntern()
//
// Description: function checks if a entry is numerical or not
//
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = Instancepointer
//              Type_p              = Data type of the object
//              pfEntryNumerical_p  = pointer to BOOL for returnvalue
//                                  -> TRUE if entry a numerical value
//                                  -> FALSE if entry not a numerical value
//
// Return:      tEplKernel = Errorcode
//
// State:
//
//---------------------------------------------------------------------------
static tEplKernel EplObdIsNumericalIntern(tEplObdType Type_p,
                                        BOOL*         pfEntryNumerical_p)
{
tEplKernel          Ret = kEplSuccessful;


    // get Type
    if((Type_p == kEplObdTypVString)
        || (Type_p == kEplObdTypOString)
        || (Type_p == kEplObdTypDomain))
    {   // not numerical types
        *pfEntryNumerical_p = FALSE;
    }
    else
    {   // numerical types
        *pfEntryNumerical_p = TRUE;
    }

    return Ret;

}


// -------------------------------------------------------------------------
// function to classify object type (fixed/non fixed)
// -------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Function:    EplObdCallStoreCallback()
//
// Description: checks address to callback function and calles it when unequal
//              to NULL
//
// Parameters:  EPL_MCO_DECL_INSTANCE_PTR_ = (instance pointer)
//              pCbStoreParam_p        = address to callback parameters
//
// Returns:     tEplKernel             = error code
// ----------------------------------------------------------------------------
#if (EPL_OBD_USE_STORE_RESTORE != FALSE)
static tEplKernel EplObdCallStoreCallback (EPL_MCO_DECL_INSTANCE_PTR_
    tEplObdCbStoreParam MEM* pCbStoreParam_p)
{

tEplKernel Ret = kEplSuccessful;

    ASSERT (pCbStoreParam_p != NULL);

    // check if function pointer is NULL - if so, no callback should be called
    if (EPL_MCO_GLB_VAR (m_fpStoreLoadObjCallback) != NULL)
    {
        Ret = EPL_MCO_GLB_VAR (m_fpStoreLoadObjCallback) (EPL_MCO_INSTANCE_PARAM_IDX_()
            pCbStoreParam_p);
    }

    return Ret;

}
#endif // (EPL_OBD_USE_STORE_RESTORE != FALSE)
//---------------------------------------------------------------------------
//
// Function:    EplObdGetObjectDataPtrIntern()
//
// Description: Function gets the data pointer of an object.
//              It returnes the current data pointer. But if object is an
//              constant object it returnes the default pointer.
//
// Parameters:  pSubindexEntry_p = pointer to subindex entry
//              AccessType_p     = request PDI address flag
//                                 0: no PDI address requested
//                    kEplObdAccRead: TPDO PDI address requested
//                   kEplObdAccWrite: RPDO PDI address requested
//
// Return:      void *    = pointer to object data
//
// State:
//
//---------------------------------------------------------------------------

static void * EplObdGetObjectDataPtrIntern (tEplObdSubEntryPtr pSubindexEntry_p,
                                            tEplObdAccess AccessType_p)
{

void * pData = NULL;
tEplObdAccess Access;

    //ASSERTMSG (pSubindexEntry_p != NULL, "EplObdGetObjectDataPtrIntern(): pointer to SubEntry not valid!\n");
    if (pSubindexEntry_p == NULL)
    {
        goto Exit;
    }

    // there are are some objects whose data pointer has to get from other structure
    // get access type for this object
    Access = pSubindexEntry_p->m_Access;

    // If object has access type = const,
    // only the default value exists.
    if ((Access & kEplObdAccConst) != 0)
    {
        // The pointer to default value can be received from ObdGetObjectDefaultPtr()
        pData = ((void *) EplObdGetObjectDefaultPtr (pSubindexEntry_p));
    }
    else
    {
        // The pointer to current value can be received from ObdGetObjectCurrentPtr()
        pData = EplObdGetObjectCurrentPtr (pSubindexEntry_p, AccessType_p);
    }

Exit:
    return pData;

}
#endif // end of #if(((EPL_MODULE_INTEGRATION) & (EPL_MODULE_OBDK)) != 0)
// EOF

