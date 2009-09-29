
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */

/**
* @file OMX_Camera.c
*
* This file implements Camera OMX Component that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\camera\src\OMX_Camera.c
*
* @rev  0.1
*/

/* =================================================================================
*! Revision History
*! ===================================
*!
*!
*! 02-Dec-2005 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */

/****************************************************************
*  INCLUDE FILES
****************************************************************/

/* ----- System and Platform Files ----------------------------*/
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <linux/errno.h>
#include <sys/mman.h>
#include <videodev.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#if 1
    #include <time.h>
    #include <sys/time.h>
#endif

/*------- Program Header Files ----------------------------------------*/
#include "OMX_Camera.h"
#include "OMX_Camera_Utils.h"
#include <ResourceManagerProxyAPI.h>
#include <OMX_Clock.h>
#include <omap24xxvout.h>

extern int errno;
extern int bKernelBufRequested;

/*-------- Function Prototypes ---------------------------------*/
static OMX_ERRORTYPE Camera_SetCallbacks (OMX_HANDLETYPE hComp, 
                                          OMX_CALLBACKTYPE* pCallBacks, 
                                          OMX_PTR pAppData);

static OMX_ERRORTYPE Camera_GetComponentVersion (OMX_HANDLETYPE hComp, 
                                          OMX_STRING pComponentName, 
                                          OMX_VERSIONTYPE* pComponentVersion, 
                                          OMX_VERSIONTYPE* pSpecVersion, 
                                          OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE Camera_SendCommand (OMX_HANDLETYPE hComp, 
                                         OMX_COMMANDTYPE Cmd, 
                                         OMX_U32 nParam, 
                                         OMX_PTR pCmdData);

static OMX_ERRORTYPE Camera_GetParameter(OMX_HANDLETYPE hComp, 
                                         OMX_INDEXTYPE nParamIndex, 
                                         OMX_PTR ComponentParamStruct);

static OMX_ERRORTYPE Camera_SetParameter (OMX_HANDLETYPE hComp, 
                                          OMX_INDEXTYPE nParamIndex, 
                                          OMX_PTR ComponentParamStruct);

static OMX_ERRORTYPE Camera_GetConfig (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nConfigIndex, 
                                       OMX_PTR pComponentConfigStructure);

static OMX_ERRORTYPE Camera_SetConfig (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nConfigIndex, 
                                       OMX_PTR pComponentConfigStructure);

static OMX_ERRORTYPE Camera_EmptyThisBuffer (OMX_HANDLETYPE hComp, 
                                             OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE Camera_FillThisBuffer (OMX_HANDLETYPE hComp, 
                                            OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE Camera_GetState (OMX_HANDLETYPE hComp, 
                                      OMX_STATETYPE* pState);


static OMX_ERRORTYPE Camera_ComponentTunnelRequest (OMX_HANDLETYPE hComp, 
                                                    OMX_U32 nPort, 
                                                    OMX_HANDLETYPE hTunneledComp, 
                                                    OMX_U32 nTunneledPort, 
                                                    OMX_TUNNELSETUPTYPE* pTunnelSetup); 

static OMX_ERRORTYPE Camera_ComponentDeInit(OMX_HANDLETYPE pHandle);

static OMX_ERRORTYPE Camera_VerifyTunnelConnection(CAMERA_PORT_TYPE *pPort, 
                                                   OMX_HANDLETYPE hTunneledComp);


static OMX_ERRORTYPE Camera_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                      OMX_IN OMX_U32 nPortIndex,
                                      OMX_IN OMX_PTR pAppPrivate,
                                      OMX_IN OMX_U32 nSizeBytes,
                                      OMX_IN OMX_U8* pBuffer);

static OMX_ERRORTYPE Camera_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                                           OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                           OMX_IN OMX_U32 nPortIndex,
                                           OMX_IN OMX_PTR pAppPrivate,
                                           OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE Camera_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                       OMX_IN  OMX_U32 nPortIndex,
                                       OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE CreateCameraBuffer(OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate,
                                        OMX_U32 nPortIndex,OMX_S32 *pBufIndex,
                                        OMX_U32 nSizeBytes, OMX_U8* pBuffer, OMX_PTR pAppPrivate);

static OMX_ERRORTYPE Camera_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, 
                                              OMX_IN OMX_STRING cParameterName, 
                                              OMX_OUT OMX_INDEXTYPE* pIndexType);



/*-------- Function Implementations ---------------------------------*/

/*-------------------------------------------------------------------*/
/**
  * ComponentRoleEnum()
  *
  * Query component's role.
  *
  * @param hComponent           handle for this instance of the component
  * @param cRole                component role structure
  * @param nIndex               index.
  *
  * @retval OMX_ErrorNone       Success, ready to roll
  *         OMX_ErrorNoMore     Index beyond limit
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE ComponentRoleEnum(OMX_HANDLETYPE hComponent, 
                                       OMX_U8 *cRole, 
                                       OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    
    OMX_CAMERA_CHECK_CMD(hComponent, OMX_TRUE, OMX_TRUE);
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0)
    {
        strncpy((char *)cRole, 
                (char *)pComponentPrivate->sComponentRole.cRole, 
                OMX_MAX_STRINGNAME_SIZE - 1);
    }
    else 
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError, OMX_ErrorNoMore, "ComponentRoleEnum");
    }
    
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

    
/*-------------------------------------------------------------------*/
/**
  * IsTIOMXComponent()
  *
  * Check if the component is TI component.
  *
  * @param hTunneledComp Component Tunnel Pipe
  * 
  * @retval OMX_TRUE   Input is a TI component.
  *         OMX_FALSE  Input is a not a TI component.
  *
  **/
/*-------------------------------------------------------------------*/

static OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_STRING pTunnelcComponentName = NULL;
    OMX_VERSIONTYPE* pTunnelComponentVersion = NULL;
    OMX_VERSIONTYPE* pSpecVersion = NULL;
    OMX_VERSIONTYPE* pComponentUUID = NULL;
    OMX_BOOL bResult = OMX_TRUE;

    pTunnelcComponentName = malloc(128);
    if (pTunnelcComponentName == NULL) {
                eError = OMX_ErrorInsufficientResources; 
                printf("Error in camera OMX_ErrorInsufficientResources %d\n",__LINE__);
                goto EXIT;                               
    }

    pTunnelComponentVersion = malloc(sizeof(OMX_VERSIONTYPE));
    if (pTunnelComponentVersion == NULL) {
                printf("Error in camera OMX_ErrorInsufficientResources %d\n",__LINE__);
                eError = OMX_ErrorInsufficientResources; 
                goto EXIT;                               
    }

    pSpecVersion = malloc(sizeof(OMX_VERSIONTYPE));
    if (pSpecVersion == NULL) {
                printf("Error in camera OMX_ErrorInsufficientResources %d\n",__LINE__);
                eError = OMX_ErrorInsufficientResources; 
                goto EXIT;                               
    }

    pComponentUUID = malloc(sizeof(OMX_UUIDTYPE));
    if (pComponentUUID == NULL) {
                printf("Error in camera OMX_ErrorInsufficientResources %d\n",__LINE__);
                eError = OMX_ErrorInsufficientResources; 
                goto EXIT;                               
    }

#if 0
    eError = OMX_GetComponentVersion (hComp, pTunnelcComponentName, pTunnelComponentVersion, pSpecVersion, pComponentUUID);
    /* Check if tunneled component is a TI component */
    pSubstring = strstr(pTunnelcComponentName, "OMX.TI.");
    if(pSubstring == NULL) {
                bResult = OMX_FALSE;
    }
#endif

EXIT:
    free(pTunnelcComponentName);
    free(pTunnelComponentVersion);
    free(pSpecVersion);
    free(pComponentUUID);
 
    return bResult;
} /* End of IsTIOMXComponent */


/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentInit()
  *
  * Updates the component function pointer to the handle.
  * Opens camera & video devices.
  * Sets default parameters.
  * 
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;  
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
   
    /* RM proxy init */
#ifdef RESOURCE_MANAGER_ENABLED
    eError = RMProxy_NewInitalizeEx(OMX_COMPONENTTYPE_CAMERA);  
    OMX_CAMERA_IF_ERROR_BAIL(eError, "RMProxy_InitializeEx Failed\n");
#endif

    /*extern variable*/
    bKernelBufRequested = 0;
   
    /* Set component function pointers */
    pHandle->SetCallbacks           = Camera_SetCallbacks;
    pHandle->GetComponentVersion    = Camera_GetComponentVersion;
    pHandle->SendCommand            = Camera_SendCommand;
    pHandle->GetParameter           = Camera_GetParameter;
    pHandle->SetParameter           = Camera_SetParameter;
    pHandle->GetConfig              = Camera_GetConfig;
    pHandle->SetConfig              = Camera_SetConfig;
    pHandle->GetState               = Camera_GetState;
    pHandle->EmptyThisBuffer        = Camera_EmptyThisBuffer;
    pHandle->FillThisBuffer         = Camera_FillThisBuffer;
    pHandle->ComponentTunnelRequest = Camera_ComponentTunnelRequest;
    pHandle->ComponentDeInit        = Camera_ComponentDeInit;
    pHandle->AllocateBuffer         = Camera_AllocateBuffer;
    pHandle->UseBuffer              = Camera_UseBuffer;
    pHandle->FreeBuffer             = Camera_FreeBuffer;
    pHandle->GetExtensionIndex      = Camera_GetExtensionIndex;
    pHandle->ComponentRoleEnum      = ComponentRoleEnum;
    
    /*Allocate Component Private*/
    OMX_CAMERA_CALLOC(pHandle->pComponentPrivate,
                      void, 
                      1, 
                      OMX_CAMERA_COMPONENT_PRIVATE, 
                      eError);
          
          
    /* Make copy of component handle in private structure */
    ((OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
     pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('C','A','M',' '),
                                            PERF_ModuleLLMM |
                                            PERF_ModuleVideoEncode | PERF_ModuleImageEncode);
#endif

    /* Allocate memory for component name */
    OMX_CAMERA_CALLOC(pComponentPrivate->cComponentName,
                      char,
                      128,
                      OMX_U8, eError);
   
    /* Initialize component private stucture */
    eError = InitPrivateStructure(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "InitPrivateStructure Failed\n");

    /* Start component thread */
    eError = StartCameraComponentThread(hComp);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "*Error:StartCameraComponentThread Failed!");

    OMX_CAMERA_PRINTF_L3("Component Thread Started");
   
    /* Initialize video buffer mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mVideoBufMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
   
    /* Initialize pipe mutex for access of bPipe flag*/
    if (pthread_mutex_init(&(pComponentPrivate->mPipeMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
    
    /* Initialize pipe mutex for access of bPipe flag*/
    if (pthread_mutex_init(&(pComponentPrivate->mDriverMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
    
    /* Initialize state transition buffer mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mStateTransitionMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
   
    /* Initialize video buffer mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mFillThisBufferMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }

    /* initialize a condition variable to its default value */ 
    if (pthread_cond_init(&(pComponentPrivate->sConditionFillThisBuffer), NULL) != 0)
    {
        perror("Error at Initialize fill this buffer mutex condition");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
    
    /* Initialize video buffer mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mPortTransitionMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
    
    /* initialize a condition variable to its default value */ 
    if (pthread_cond_init(&(pComponentPrivate->sConditionPortTransition), NULL) != 0)
    {
        perror("Error at Initialize fill this buffer mutex condition");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
    
       /* Initialize video buffer mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mCameraPipeEmptyMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }

#ifdef __CAMERA_2A__    	
    /* initialize a condition variable to its default value*/
    if (pthread_cond_init(&(pComponentPrivate->sConditionFullLinkedList), NULL) != 0)
    {
        perror("Error at Initialize linked list mutex condition");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }

    /* Initialize linked list mutex */
    if (pthread_mutex_init(&(pComponentPrivate->mFullLinkedListMutex), NULL) != 0)
    {
        perror("Error at Initialize buffer mutex");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }
#endif
    
    /* initialize a condition variable to its default value */ 
    if (pthread_cond_init(&(pComponentPrivate->sConditionCameraPipeEmpty), NULL) != 0)
    {
        perror("Error at Initialize fill this buffer mutex condition");
        eError = OMX_ErrorHardware;
        goto OMX_CAMERA_BAIL_CMD;
    }

#if 0   
    pComponentPrivate->fileout = fopen("time.log", "w");
#endif

   /* Function executed properly */
   eError = OMX_ErrorNone;
   OMX_CAMERA_PRINTF_L3("ComponentInit Exit Properly");
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of OMX_ComponentInit */


/*-------------------------------------------------------------------*/
/**
  * Camera_SetCallbacks()
  *
  * This method will update application callbacks
  * the application.
  *
  * @param hComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param pAppData      application private data
  *
  * @retval OMX_ErrorNone              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_SetCallbacks (OMX_HANDLETYPE hComp, 
                                          OMX_CALLBACKTYPE* pCallBacks, 
                                          OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    /* Check parameter validity */
    OMX_CAMERA_CHECK_CMD(hComp, pCallBacks, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;

    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    /* Copy the application callback pointers to the component private structure */
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCallBackInfo), 
           pCallBacks,
           sizeof(OMX_CALLBACKTYPE),
           eError);

    /* Set application private data */
    pHandle->pApplicationPrivate = pAppData;

    /* Set component state as 'Loaded' */
    pComponentPrivate->eCurrentState = OMX_StateLoaded;
    
    /* Function executed properly */
    eError = OMX_ErrorNone;

OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_SetCallbacks */


/*-------------------------------------------------------------------*/
/**
  * Camera_GetComponentVersion()
  *
  * Retreives component version.
  *
  * @param pComp             Handle for this instance of the component
  * @param pComponentName    Application callbacks
  * @param pComponentVersion Pointer to store component version
  * @param pSpecVersion      Spec version
  * @param pComponentUUID    Component UUID
  *
  * @retval OMX_ErrorNone              Success, ready to roll
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_GetComponentVersion (OMX_HANDLETYPE hComp,
                                                 OMX_STRING pComponentName, 
                                                 OMX_VERSIONTYPE* pComponentVersion,
                                                 OMX_VERSIONTYPE* pSpecVersion,
                                                 OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    /* Check if parameters are null */
    OMX_CAMERA_CHECK_CMD(hComp, pComponentVersion, pComponentName);

    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

    /* Copy component version structure */
    OMX_CAMERA_STRCPY(pComponentName, 
                      pComponentPrivate->cComponentName, 
                      eError);
    
    /*Copy Component's Version*/ 
    OMX_CAMERA_MEMCPY(pComponentVersion, 
                      &(pComponentPrivate->nVersion.s), 
                      sizeof(pComponentPrivate->nVersion.s), 
                      eError);
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
       return eError;
} /* End of Camera_GetComponentVersion */


/*-------------------------------------------------------------------*/
/**
/  * Camera_SendCommand()
  *
  * Sends command from application to component.
  *
  * @param hComp   Handle for this instance of the component
  * @param nCommand     Command to be sent
  * @param nParam    Command parameters
  * @param pCmdData  Additional command data      
  *
  * @retval OMX_ErrorNone                  Success, ready to roll
  *         OMX_Error_BadParameter         The input parameter pointer is null
  *         OMX_ErrorInsufficientResources If pipe operation fails.
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_SendCommand (OMX_HANDLETYPE hComp, 
                                         OMX_COMMANDTYPE nCommand, 
                                         OMX_U32 nParam, 
                                         OMX_PTR pCmdData)
{
    OMX_U32 nIndex                                  = 0;
    OMX_U32 nPorts                                  = 0;
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* Check if component is already in invalid state */
    OMX_CAMERA_CHECK_COND(pComponentPrivate->eCurrentState == OMX_StateInvalid, 
                          eError,
                          OMX_ErrorInvalidState, 
                          "SendCommand call not possible from invalid state\n");

    /* Check if we are stoping the component to set the bStopping flag */
    if(nCommand == OMX_CommandStateSet)
    {
         pComponentPrivate->eDesiredState = nParam;
        if(nParam == OMX_StateIdle && 
           pComponentPrivate->eCurrentState == OMX_StateExecuting)
        {
            /* Stop buffer transfer if stop or pause command is received */
            pComponentPrivate->bStopping = OMX_TRUE;
        
            /* Send signal to unblock FillThisBuffer mutex */
			OMX_CAMERA_PRINTF_L4("mFillThisBufferMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
			OMX_CAMERA_PRINTF_L4("mFillThisBufferMutex: Locked");
            pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
			OMX_CAMERA_PRINTF_L4("Send Conditional Signal: sConditionFillThisBuffer");
            pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
			OMX_CAMERA_PRINTF_L4("mFillThisBufferMutex: Unlocked");
        }
				else if(nParam == OMX_StateExecuting && 
           pComponentPrivate->eCurrentState == OMX_StateIdle)
        {
            /* In case PP and CAM are sent to Executing from Idle, 
			bStopping has to be False before PP thread sends a FTB 
			to CAM. It is is achieved by initializing  this variable here,
			in the APP thread*/
            pComponentPrivate->bStopping = OMX_FALSE;                    
        }
        else if(nParam == OMX_StatePause)
        {
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTranistionMutex: Locked");
            eError = HandleCameraPauseState (pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraPauseState!");
            pthread_mutex_unlock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Unlock");
            
            eError = OMX_ErrorNone;
            goto OMX_CAMERA_BAIL_CMD;
        }
    }
    
    /* Check port index validity */
    else if(nCommand == OMX_CommandFlush)
    {
        OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nParam) &&  (nParam != -1), 
                              eError,
                              OMX_ErrorBadPortIndex,
                              "Error flushing wrong Port index ");
    }
    else if(nCommand == OMX_CommandPortDisable)
    {
        OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nParam) && (nParam != OMX_ALL), 
                              eError, 
                              OMX_ErrorBadPortIndex, 
                              "Error! Trying to disable unknown port\n");
        if(nParam == OMX_ALL)
        {
            /* All ports are to be disabled */
            nIndex = pComponentPrivate->sPortParam.nStartPortNumber;
            nPorts = pComponentPrivate->sPortParam.nStartPortNumber;
            nPorts += pComponentPrivate->sPortParam.nPorts;
            for( ; nIndex < nPorts ; nIndex++)
            {
                pPortDef = &(pComponentPrivate->sCompPorts[nIndex].pPortDef);
                pPortDef->bEnabled = OMX_FALSE;
                if (!pPortDef->bPopulated)
                {
                    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventCmdComplete,
                                             OMX_CommandPortDisable,
                                             nIndex,
                                             NULL);
                    eError = OMX_ErrorNone;
                    goto OMX_CAMERA_BAIL_CMD;
                }
            }
        }
        else
        {
            /* Disable the requested port */
            pPortDef = &(pComponentPrivate->sCompPorts[nParam].pPortDef);
            pPortDef->bEnabled = OMX_FALSE;
            if (!pPortDef->bPopulated)
            {
                OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete, 
                                         OMX_CommandPortDisable, 
                                         nParam, 
                                         NULL);
                eError = OMX_ErrorNone;
                goto OMX_CAMERA_BAIL_CMD;
            }
        }
    }
    else if (nCommand == OMX_CommandPortEnable)
    {
        OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nParam) && (nParam != OMX_ALL), 
                              eError, 
                              OMX_ErrorBadPortIndex, 
                              "Error! Trying to enable unknown port\n");
        
        if(nParam == OMX_ALL)
        {
            /* All ports are to be disabled */
            nIndex = pComponentPrivate->sPortParam.nStartPortNumber;
            nPorts = pComponentPrivate->sPortParam.nStartPortNumber;
            nPorts += pComponentPrivate->sPortParam.nPorts;
            for( ; nIndex < nPorts ; nIndex++)
            {
                pPortDef = &(pComponentPrivate->sCompPorts[nIndex].pPortDef);
                if (pPortDef->bPopulated && pPortDef->bEnabled)
                {
                    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventCmdComplete,
                                             OMX_CommandPortEnable,
                                             nIndex,
                                             NULL);
                    eError = OMX_ErrorNone;
                    goto OMX_CAMERA_BAIL_CMD;
                }
            }
        }
        else
        {
            /* Disable the requested port */
            pPortDef = &(pComponentPrivate->sCompPorts[nParam].pPortDef);
            if (pPortDef->bPopulated && pPortDef->bEnabled)
            {
                OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventCmdComplete, 
                                         OMX_CommandPortEnable, 
                                         nParam, 
                                         NULL);
                eError = OMX_ErrorNone;
                goto OMX_CAMERA_BAIL_CMD;
            }
        }
    }

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        nCommand,
                        nParam,
                        PERF_ModuleComponent);
#endif

    /* Write command to command pipe */
    OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nCmdPipe[1],
                                &nCommand,
                                sizeof(OMX_COMMANDTYPE)) != sizeof(OMX_COMMANDTYPE), 
                          eError, 
                          OMX_ErrorInsufficientResources, 
                          "Error while writing to the command pipe ");
    
    /* Write parameter to command data pipe */
    OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nCmdDataPipe[1], 
                                &nParam,
                                sizeof(OMX_U32)) != sizeof(OMX_U32), 
                          eError, 
                          OMX_ErrorInsufficientResources, 
                          "Error while writing to the command pipe ");
    
    /* Function executed properly */
    eError = OMX_ErrorNone;

OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_SendCommand */


/*-------------------------------------------------------------------*/
/**
  *  Camera_GetParameter()
  *
  *  Retrieve current parameter value.
  *
  * @param hComp                handle for this instance of the component
  * @param nParamIndex          parameter index
  * @param ComponentParamStruct structure for component parameters
  *
  * @retval OMX_ErrorNone       Success, ready to roll
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_GetParameter (OMX_HANDLETYPE hComp, 
                                          OMX_INDEXTYPE nParamIndex,
                                          OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE* pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, pCompParam, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    switch (nParamIndex)
    {
        case OMX_IndexParamVideoInit:
        {
            /* Cast the param pointer to appropriate structure type */
            OMX_PORT_PARAM_TYPE *pComponentParam = (OMX_PORT_PARAM_TYPE *)pCompParam;
            OMX_CAMERA_MEMCPY(pComponentParam,
                              &pComponentPrivate->sPortParam, 
                              sizeof(OMX_PORT_PARAM_TYPE), 
                              eError);
        }
            break;
        case OMX_IndexParamPortDefinition:
        {
                /* Cast the param pointer to appropriate structure type */
            OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
            OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(pComponentParam->nPortIndex),
                                  eError, 
                                  OMX_ErrorBadPortIndex, 
                                  "Camera Camera_GetParameter OMX_IndexParamPortDefinition error ");

             pPortDef = &(pComponentPrivate->sCompPorts[pComponentParam->nPortIndex].pPortDef);
             OMX_CAMERA_MEMCPY(pComponentParam, 
                               pPortDef,
                               sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
                               eError);
        }
            break;
        case OMX_IndexParamVideoPortFormat:
        {
            /* Cast the param pointer to appropriate structure type */
            OMX_VIDEO_PARAM_PORTFORMATTYPE *pComponentParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pCompParam;
            OMX_U32 nPortIndex = pComponentParam->nPortIndex;

            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nPortIndex),
                                  eError, OMX_ErrorBadPortIndex,
                                  "Camera_GetParameter OMX_ErrorBadPortIndex\n");

            if (pComponentParam->nIndex > pComponentPrivate->sCompPorts[nPortIndex].sPortFormat.nIndex)
            {
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorNoMore,
                                          "OMX_IndexParamVideoPortFormat");
            }
            else 
            {
                OMX_CAMERA_MEMCPY(pComponentParam,
                                  &(pComponentPrivate->sCompPorts[nPortIndex].sPortFormat), 
                                  sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE), 
                                  eError);
            }
        }
            break;
        case OMX_IndexParamCommonSensorMode:
        {
            /* Cast the param pointer to appropriate structure type */
            OMX_PARAM_SENSORMODETYPE *pComponentParam = (OMX_PARAM_SENSORMODETYPE *)pCompParam;
            /*
            eError = GetCameraSensorModeType(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error while Getting sensormode type");
            */
            OMX_CAMERA_MEMCPY(pComponentParam,
                              &pComponentPrivate->sSensorMode, 
                              sizeof(OMX_PARAM_SENSORMODETYPE), eError);
        }
            break;
        case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRIORITYMGMTTYPE *pComponentParam = (OMX_PRIORITYMGMTTYPE *)pCompParam;
            OMX_CAMERA_MEMCPY(pComponentParam,
                              &pComponentPrivate->sPriorityMgmt,
                              sizeof(OMX_PRIORITYMGMTTYPE),
                              eError);
        }
            break;
        case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pCompParam;

            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(pBuffSupplierParam->nPortIndex), 
                                  eError, 
                                  OMX_ErrorBadPortIndex, 
                                  "Camera Camera_GetParameter OMX_IndexParamCompBufferSupplier error\n");

            pBuffSupplierParam->eBufferSupplier = pComponentPrivate->sCompPorts[pBuffSupplierParam->nPortIndex].eSupplierSetting;
        }
            break;
        default:
        {
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedIndex, 
                                      "Camera Camera_GetParameter OMX_ErrorUnsupportedIndex\n");
        }
    } /* switch ends */

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_GetParameter */


/*-------------------------------------------------------------------*/
/**
  * Camera_SetParameter()
  *
  * Sets component parameters.
  *
  * @param hComp                handle for this instance of the component
  * @param nParamIndex          parameter index
  * @param pCompParam           parameter to be set.
  *
  * @retval OMX_ErrorNone                   Success, ready to roll
  *         OMX_ErrorInsufficientResources  Memory allocation failure
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_SetParameter (OMX_HANDLETYPE hComp, 
                                          OMX_INDEXTYPE nParamIndex,
                                          OMX_PTR pCompParam)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE* pHandle                      = (OMX_COMPONENTTYPE*)hComp;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, pCompParam, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
    pPortDef = &(pComponentPrivate->sCompPorts[pComponentParam->nPortIndex].pPortDef);
    
    OMX_CAMERA_PRINTF_L4("SetParam: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
    OMX_CAMERA_PRINTF_L4("SetParam: Locked");
    
    OMX_CAMERA_CHECK_COND((pComponentPrivate->eCurrentState != OMX_StateLoaded &&
                           pPortDef->bEnabled == OMX_TRUE), 
                          eError, 
                          OMX_ErrorIncorrectStateOperation,
                          "Setting Parameters is not allowed in Idle State\n");
    
    switch (nParamIndex)
    {
        case OMX_IndexParamVideoInit:
        {
            /* Cast the param pointer to appropriate structure type */
            OMX_PORT_PARAM_TYPE *pComponentParam = (OMX_PORT_PARAM_TYPE *)pCompParam;
            OMX_CAMERA_MEMCPY(&pComponentPrivate->sPortParam,
                              pComponentParam,
                              sizeof(OMX_PORT_PARAM_TYPE),
                              eError);
        }
        break;
        case OMX_IndexParamPortDefinition:
        {
            /* Cast the param pointer to appropriate structure type */
            OMX_PARAM_PORTDEFINITIONTYPE *pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
            
            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(pComponentParam->nPortIndex), 
                                  eError, 
                                  OMX_ErrorBadPortIndex, 
                                  "Setting parameter in a wrong index port ");
            OMX_CAMERA_MEMCPY(pPortDef, 
                              pComponentParam,
                              sizeof(OMX_PARAM_PORTDEFINITIONTYPE),
                              eError);
							  
			if(pPortDef->eDomain == OMX_PortDomainVideo){
				pPortDef->nBufferSize=(2*pPortDef->format.video.nFrameWidth*pPortDef->format.video.nFrameHeight);				
			}	
			else if(pPortDef->eDomain == OMX_PortDomainImage){
				pPortDef->nBufferSize=(2*pPortDef->format.image.nFrameWidth*pPortDef->format.image.nFrameHeight);
			}
			else
				OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorBadParameter, 
                                      "Camera Camera_SetParameter eDomain unsupported\n");
        }
            break;
        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *pComponentParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)pCompParam;
            OMX_U32 nPortIndex = pComponentParam->nPortIndex;

            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nPortIndex), 
                                  eError,
                                  OMX_ErrorBadPortIndex,
                                  "Wrong port index\n");

            if(pComponentParam->nIndex > pComponentPrivate->sCompPorts[nPortIndex].sPortFormat.nIndex)
            {
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorNoMore,
                                          "OMX_IndexParamVideoPortFormat");
            }
           else 
            {
                OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCompPorts[nPortIndex].sPortFormat),
                                  pCompParam, 
                                  sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE),
                                  eError);
            }
        }
             break;
        case OMX_IndexParamCommonSensorMode:
        {
             /* Cast the param pointer to appropriate structure type */
             OMX_PARAM_SENSORMODETYPE *pComponentParam = (OMX_PARAM_SENSORMODETYPE *)pCompParam;
             OMX_CAMERA_MEMCPY(&pComponentPrivate->sSensorMode,
                               pComponentParam,
                               sizeof(OMX_PARAM_SENSORMODETYPE),
                               eError);
        }
             break;
        case OMX_IndexParamPriorityMgmt:
        {
            OMX_CAMERA_MEMCPY(&pComponentPrivate->sPriorityMgmt, 
                              pCompParam,
                              sizeof(OMX_PRIORITYMGMTTYPE),
                              eError);
        }
            break;
        case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pCompParam;

            OMX_CAMERA_CHECK_COND(!IsCameraOutPort(pBuffSupplierParam->nPortIndex), 
                                  eError,
                                  OMX_ErrorBadPortIndex,
                                  "Wrong port Index ");

            /* Copy parameters to input port buffer supplier type */
            pComponentPrivate->sCompPorts[pBuffSupplierParam->nPortIndex].eSupplierSetting = pBuffSupplierParam->eBufferSupplier;
        }
             break; 
        case CameraCustomParamSetClockSource:
        {
            /* Set clock source handle */
            pComponentPrivate->hClock = (OMX_HANDLETYPE) pCompParam;
            /* Now pass your handle to clock component */
            OMX_SetParameter(pComponentPrivate->hClock, OMX_IndexCustomSetClient, (OMX_PTR)hComp);
            pComponentPrivate->bSynchronized = OMX_TRUE;
        }
            break;
        case CameraCustomParamIPP:
        {   
            /* Set capture or viewfinder mode */
            OMX_BOOL *pComponentParam = (OMX_BOOL *)pCompParam;
            /* Turn capturing OFF immediately */
            pComponentPrivate->bIPP = *pComponentParam;
        }
            break;
        case CameraCustomParamCameraDevice:
        {
            /* Set Camera Device */
            OMX_U32 *pComponentParam = (OMX_U32 *)pCompParam;
            pComponentPrivate->nCameraDevice = *pComponentParam;
        }
            break;
        default:
             OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                       OMX_ErrorUnsupportedIndex,
                                       "Unsupported Index\n");
    } /* switch ends */
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
    OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
    return eError;
} /* End of Camera_SetParameter */


/*-------------------------------------------------------------------*/
/**
  * Camera_GetConfig()
  *
  * Retrieves current configuration.
  *
  * @param hComp                    handle for this instance of the component
  * @param nConfigIndex             config param index
  * @param pComponentConfigStructure configuration structure
  *
  * @retval OMX_ErrorNone           Success, ready to roll
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_GetConfig (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CONFIG_RECTTYPE *pCropWindow                = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* Retrieve configuration */
    switch (nConfigIndex)
    {
        case OMX_IndexConfigCommonContrast:
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded)
            {
                eError = GetCameraContrast(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraContrast");
            }
            ((OMX_CONFIG_CONTRASTTYPE *)(pComponentConfigStructure))->nContrast = pComponentPrivate->sContrast.nContrast;
            break;
        case OMX_IndexConfigCaptureMode:
            ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->bContinuous = pComponentPrivate->sCaptureMode.bContinuous;
            ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->bFrameLimited = pComponentPrivate->sCaptureMode.bFrameLimited;
            ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->nFrameLimit = pComponentPrivate->sCaptureMode.nFrameLimit;
            break;
        case OMX_IndexConfigCommonBrightness:
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded)
            {
                eError = GetCameraBrightness(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraContrast");
            }
            ((OMX_CONFIG_BRIGHTNESSTYPE *)(pComponentConfigStructure))->nBrightness = pComponentPrivate->nCurrentBrightness;
            break;
        case CameraCustomConfigColorEffect:
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded)
            {
                eError = GetCameraColorEffect(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraColorEffect");
            }
            OMX_U32 *pComponentConfigParam = (OMX_U32 *)pComponentConfigStructure;
            *pComponentConfigParam = pComponentPrivate->nColorEffect;
            break;
        case OMX_IndexConfigCommonRotate:
            /* Camera doesn't support rotation */
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorUnsupportedIndex,
                                      "Camera doesn't support rotation");
        case OMX_IndexConfigCommonOutputCrop:
            pCropWindow = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;
            OMX_CAMERA_MEMCPY(pCropWindow,
                              &pComponentPrivate->sCropWindow, 
                              sizeof(OMX_CONFIG_RECTTYPE),
                              eError);
            break;
#ifdef __CAMERA_2A__            
        case OMX_IndexConfigCommonWhiteBalance:
            /*Get white balance value from 2A thread*/
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
               pComponentPrivate->cameraAlg)
            {
                eError = GetCameraWhiteBalance(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraWhiteBalance");
            }
            ((OMX_CONFIG_WHITEBALCONTROLTYPE *)(pComponentConfigStructure))->eWhiteBalControl = pComponentPrivate->sAWB.eWhiteBalControl;
            break;
        case OMX_IndexConfigCommonExposure:
            /*Get exposure value from 2A thread*/
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
               pComponentPrivate->cameraAlg)
            {
                eError = GetCameraExposure(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraExposure");
            }
            ((OMX_CONFIG_EXPOSURECONTROLTYPE *)(pComponentConfigStructure))->eExposureControl = pComponentPrivate->sAE.eExposureControl;
            break;
#endif            
        default:
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorBadParameter,
                                      "GetConfig BadParamter");
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_GetConfig */


/*-------------------------------------------------------------------*/
/**
  * Camera_SetConfig()
  *
  * Sends command to component thread to set new configuration.
  *
  * @param hComp                    handle for this instance of the component
  * @param nConfigIndex             config param index
  * @param pComponentConfigStructure configuration structure
  *
  * @retval OMX_ErrorNone           Success, ready to roll
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_SetConfig (OMX_HANDLETYPE hComp, 
                                       OMX_INDEXTYPE nConfigIndex,
                                       OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_BOOL bCapturingTemp;
	
    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    bCapturingTemp = pComponentPrivate->bCapturing;
	
    /*If we want to turn bCapturing to off, and is video mode*/
    if(nConfigIndex == OMX_IndexConfigCapturing &&
       ( *(OMX_BOOL *)pComponentConfigStructure == OMX_FALSE ) )
    {
	
	if(IsStillCaptureMode(pComponentPrivate) == OMX_TRUE)
        {   
#ifdef __CAMERA_2A__    	   
            if(pComponentPrivate->bLinkedListFull == OMX_FALSE)
            {   
		OMX_CAMERA_PRINTF_L4("Check Lock of FullLinkedList Mutex");	
	        pthread_mutex_lock(&(pComponentPrivate->mFullLinkedListMutex));
        	OMX_CAMERA_PRINTF_L4("Lock of FullLinkedList Mutex");
		OMX_CAMERA_PRINTF_L4("Wait for condition FullLinkedList mutex\n");
		pthread_cond_wait(&(pComponentPrivate->sConditionFullLinkedList),
				  &(pComponentPrivate->mFullLinkedListMutex));
		pComponentPrivate->bLinkedListFull = OMX_FALSE;
		pthread_mutex_unlock(&(pComponentPrivate->mFullLinkedListMutex));
		OMX_CAMERA_PRINTF_L4("Un Lock of FullLinkedList Mutex");
				
            }
#endif 	    
	}
	else
	{	
		
		pComponentPrivate->bCapturing = *(OMX_BOOL *)pComponentConfigStructure;
	
        	/*
			QueueBuffer_Video may be waiting for a FillThisBuffer from
			capture port. We send this signal to unblock that thread. 
		*/
        	OMX_CAMERA_PRINTF_L4("Check Lock of FillThisBuffer Mutex");
        	pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
        	OMX_CAMERA_PRINTF_L4("Lock of FillThisBuffer Mutex");
        	pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
        	OMX_CAMERA_PRINTF_L4("Send signal FillThisBuffer Mutex");
        	pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
        	OMX_CAMERA_PRINTF_L4("Un Lock of FillThisBuffer Mutex");
	}
    }
	
   
    OMX_CAMERA_PRINTF_L4("SetConfig: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
    OMX_CAMERA_PRINTF_L4("SetConfig: Locked");
    
    switch(nConfigIndex)
    {
        case OMX_IndexConfigCommonContrast:
        {
            pComponentPrivate->sContrast.nContrast = (OMX_S32)(((OMX_CONFIG_CONTRASTTYPE *)(pComponentConfigStructure))->nContrast);
            eError = SetCameraContrast(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error while Setting Contrast");
        }
            break;
        case OMX_IndexConfigCommonBrightness:
        {
            pComponentPrivate->nCurrentBrightness = (OMX_U32)(((OMX_CONFIG_BRIGHTNESSTYPE *)(pComponentConfigStructure))->nBrightness);
            eError = SetCameraBrightness(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error while Setting Brightness !");
        }
            break;
        case OMX_IndexConfigCommonRotate:
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorUnsupportedIndex,
                                      "OMX_IndexConfigCommonRotate not Supported");
        case OMX_IndexConfigCaptureMode:
        {
            pComponentPrivate->sCaptureMode.bContinuous = ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->bContinuous;
            pComponentPrivate->sCaptureMode.bFrameLimited = ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->bFrameLimited;
            pComponentPrivate->sCaptureMode.nFrameLimit= ((OMX_CONFIG_CAPTUREMODETYPE *)(pComponentConfigStructure))->nFrameLimit;
        }
            break;
        case OMX_IndexConfigCapturing:
        {
            /* Set capture or viewfinder mode */
            OMX_BOOL *pComponentParam = (OMX_BOOL *)pComponentConfigStructure;
            /* Turn capturing OFF immediately */
            if(bCapturingTemp == *pComponentParam)
            {
                break;
            }
            //If pComponentConfigStructure was false bCapturing was already set before sending the signal. 
			//It is a workaround to avoid a deadlock when PP sends a FTB.
            pComponentPrivate->bCapturing = *pComponentParam;
            OMX_CAMERA_PRINTF_L3("Setting pComponentPrivate->bCapturing=%d",pComponentPrivate->bCapturing);
            
#ifdef __PERF_INSTRUMENTATION__
            PERF_ReceivedCommand(pComponentPrivate->pPERF,
                                 nConfigIndex,
                                 pComponentPrivate->bCapturing,
                                 PERF_ModuleHLMM);
#endif
            eError = HandleConfigCapturing(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error while Setting Capturing!");
        }
            break;
        case OMX_IndexAutoPauseAfterCapture:
        {
            OMX_BOOL *pComponentParam = (OMX_BOOL *)pComponentConfigStructure;
            /* Set flag AutoPauseAfterCapture flag*/
            pComponentPrivate->bAutoPauseAfterCapture = *pComponentParam;
        }
            break;
        case OMX_IndexConfigCommonOutputCrop:
        {
            OMX_CONFIG_RECTTYPE *pCropWindow = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;
            OMX_CAMERA_MEMCPY(&pComponentPrivate->sCropWindow,
                              pCropWindow,
                              sizeof(OMX_CONFIG_RECTTYPE),
                              eError);

            eError = SetCameraCropWindow(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,
                                     "Error while setting crop window !");
        }
            break;
        case OMX_IndexConfigCommonDigitalZoom:
        {
            OMX_CONFIG_SCALEFACTORTYPE *pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE *)pComponentConfigStructure;
            OMX_CAMERA_MEMCPY(&pComponentPrivate->sScaleFactor,
                              pScaleFactor,
                              sizeof(OMX_CONFIG_SCALEFACTORTYPE),
                              eError);

            eError = SetCameraScaleWindow(pComponentPrivate,
                                          &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]));
            OMX_CAMERA_IF_ERROR_BAIL(eError,
                                     "Error while setting scale window !");
        }
            break;
        case CameraCustomConfigColorEffect:
        {
            OMX_U32 *pComponentParam = (OMX_U32 *)pComponentConfigStructure;
            pComponentPrivate->nColorEffect= *pComponentParam;
            {
                eError = SetCameraColorEffect(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError,
                                        "SetCameraColorEffect !");
            }
        }
            break;
#ifdef __CAMERA_2A__
        case OMX_IndexConfigCommonWhiteBalance:
        {
            pComponentPrivate->sAWB.eWhiteBalControl = ((OMX_CONFIG_WHITEBALCONTROLTYPE *)(pComponentConfigStructure))->eWhiteBalControl;
            /*Set white balance value from 2A thread*/
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
               pComponentPrivate->cameraAlg)
            {
                eError = SetCameraWhiteBalance(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at SetCameraWhiteBalance");
            }
        }
            break;
        case OMX_IndexConfigCommonExposure:
        {
            pComponentPrivate->sAE.eExposureControl = ((OMX_CONFIG_EXPOSURECONTROLTYPE *)(pComponentConfigStructure))->eExposureControl;
            /*Set exposure value from 2A thread*/
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
               pComponentPrivate->cameraAlg)
            {
                eError = SetCameraExposure(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at SetCameraExposure");
            }
        }
            break;
	case OMX_IndexConfigCommonFocusRegion:
	{
	    pComponentPrivate->bAutoFocus = *(OMX_BOOL *)pComponentConfigStructure;
	}
	break;
#endif
#ifdef __CAMERA_VSTAB__
        case OMX_IndexConfigCommonFrameStabilisation:
        {
            pComponentPrivate->sVSTAB.bStab = ((OMX_CONFIG_FRAMESTABTYPE *)(pComponentConfigStructure))->bStab;
            eError = SetCameraFrameStabilisation(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at SetCameraFrameStabilisation");
        }
            break;
#endif
    default:
        OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                  OMX_ErrorBadParameter,
                                  "SetConfig Error");
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
    OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
    return eError;
} /* End of Camera_SetConfig */


/*-------------------------------------------------------------------*/
/**
  * Camera_GetState()
  *
  * Retrieves current state.
  *
  * @param hComp         handle for this instance of the component
  * @param pState        pointer to store state
  *
  * @retval OMX_ErrorNone            Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_GetState (OMX_HANDLETYPE hComp, 
                                      OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError       = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, pState, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *)hComp;

    /* Retrieve current state */
    if(pHandle->pComponentPrivate)
    {
        *pState = ((OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->eCurrentState;
    }

OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_GetState */


/*-------------------------------------------------------------------*/
/**
  * Camera_EmptyThisBuffer()
  *
  * Pass the filled buffer from application to Camera component
  * for rendering.
  *
  * @param pComp         handle for this instance of the component
  * @param pBuffer       buffer pointer
  *
  * @retval OMX_ErrorNone            Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_EmptyThisBuffer (OMX_HANDLETYPE hComp, 
                                             OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    /* Note - Camera doesn't have any input port, so this function doesn't anything */
    
    return eError;
} /* End of Camera_EmptyThisBuffer */


/*-------------------------------------------------------------------*/
/**
  * Camera_FillThisBuffer()
  *
  * Passes empty buffer to component.
  *
  * @param hComp         handle for this instance of the component
    * @param pBuffer       buffer pointer
  *
  * @retval OMX_ErrorNone              Success, ready to roll
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_FillThisBuffer (OMX_HANDLETYPE hComp, 
                                            OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer      = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pPreviewBuffer     = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    CAMERA_PORT_TYPE *pThumbnailPort                = NULL;
    CAMERA_PORT_TYPE *pCapturePort                  = NULL;
    CAMERA_PORT_TYPE *pPreviewPort                  = NULL;
    
    /* Check parameter validity */
    OMX_CAMERA_CHECK_CMD(hComp, pBufferHeader, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *)hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pCapturePort = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    
    /* Check version */
    OMX_CAMERA_CHK_SIZE_VERSION(pBufferHeader,
                                OMX_BUFFERHEADERTYPE,
                                eError);
                                  
    /*Check if is an output port */
    OMX_CAMERA_CHECK_COND(!IsCameraOutPort(pBufferHeader->nOutputPortIndex),
                          eError,
                          OMX_ErrorBadPortIndex,
                          "Error ! Incorrect output port index\n");

    pCameraBuffer = pBufferHeader->pOutputPortPrivate;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBufferHeader->pBuffer,
                       pBufferHeader->nAllocLen,
                       PERF_ModuleHLMM);
#endif
    if(pComponentPrivate->bStopping == OMX_FALSE)
    {
        if(IsPreviewPort(pBufferHeader->nOutputPortIndex))
        {
            OMX_CAMERA_PRINTF_L3("Preview Buffer Received [%d]", 
                              pCameraBuffer->nIndex);
            
            /*
            Mutex will block the thread if the the camera is
            on a state transition, when the state transition is
            done it will unblock this mutex and process the buffer
            properly
            */
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTranistionMutex: Locked");
            
            /*
            Mutex will block the thread if the camera is attending
            a setconfig request from the IL Client, when the SetConfig
            is done the Thread will continue and process the buffer
            properly.
            */
            OMX_CAMERA_PRINTF_L4("SetConfig: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
            OMX_CAMERA_PRINTF_L4("SetConfig: Locked");
            
            if(pCameraBuffer->bFirstTime == OMX_FALSE)
            {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
                pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
            }
            
            if(pComponentPrivate->bCapturing == OMX_TRUE)
            {
                pCameraBuffer->bCapturing = OMX_TRUE;
            }
            else if(pCameraBuffer->bCapturing == OMX_TRUE)
            {
                pCameraBuffer->bCapturing = OMX_FALSE;
                OMX_CAMERA_PRINTF_L3("LastCapturing Buffers");
                pComponentPrivate->bLastCapturingBuffers = OMX_TRUE;
                pComponentPrivate->nLastCapturingBuffers++;				
                if(pComponentPrivate->nLastCapturingBuffers >= pPreviewPort->nBufferCount)
                {
                    OMX_CAMERA_PRINTF_L3("Turn off LastCapturingBuffers flag");
                    pComponentPrivate->bLastCapturingBuffers = OMX_FALSE;
                    pComponentPrivate->nLastCapturingBuffers = 0;
                }
               
            }
            
            if (pCameraBuffer->bPreviewFillThisBuffer == OMX_FALSE)
            {
                pCameraBuffer->bPreviewFillThisBuffer = OMX_TRUE;
                OMX_CAMERA_PRINTF_L3("Set PreviewFillThisBuffer flag to Buffer %d", 
                                   pCameraBuffer->nIndex);
            }
            else
            {
                OMX_CAMERA_PRINTF_L3("Duplicated Buffer %d", 
                                   pCameraBuffer->nIndex);
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorBadParameter,
                                          "Duplicated Buffer in Preview Port")
            }
            
            eError = QueueBuffer_Video(pComponentPrivate, pCameraBuffer);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error in QueueBuffer");
            if(pComponentPrivate->bExit == OMX_TRUE)
            {
                pComponentPrivate->bExit = OMX_FALSE;
                eError = OMX_ErrorNone;
                pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
                OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
                pthread_mutex_unlock(&(pComponentPrivate->mStateTransitionMutex));
                OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Unlock");
                goto OMX_CAMERA_BAIL_CMD;
            }
            
            /* Writing buffer to preview buffer pipe */
            OMX_CAMERA_PRINTF_L3("pCameraBuffer 0x%x\n    pCameraBuffer->eBufferOwner %d\n    nIndex [%d]\n    pComponentPrivate->bStopping %d\n    bCapturing %d",
                        pCameraBuffer,
                        pCameraBuffer->eBufferOwner,
                        pCameraBuffer->nIndex,
                        pComponentPrivate-> bStopping,
                        pCameraBuffer->bCapturing);
            
            OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
            OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
            OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nPreviewBufPipe[1], 
                                        &pCameraBuffer, 
                                        sizeof(OMX_CAMERA_COMPONENT_BUFFER*)) != sizeof(OMX_CAMERA_COMPONENT_BUFFER*), 
                                  eError,
                                  OMX_ErrorHardware, 
                                  "Error writing to  preview buffer pipe ");
            pCameraBuffer->bPipe = OMX_TRUE;
            OMX_CAMERA_PRINTF_L3("Preview Buffer[%d] sent through Pipe", pCameraBuffer->nIndex);
            pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
            OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
            
            pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
            OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
            
            pthread_mutex_unlock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Unlock");
        }
        else if(IsCapturePort(pBufferHeader->nOutputPortIndex))
        {
            
            OMX_CAMERA_PRINTF_L3("Capture Buffer Received [%d]=%p", 
                                  pCameraBuffer->nIndex,pCameraBuffer);
            
            if(IsStillCaptureMode(pComponentPrivate) == OMX_FALSE)
            {
                if(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE)
                {
                    pPreviewBuffer = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pCameraBuffer[pCameraBuffer->nIndex];
                    if (pPreviewBuffer->bCaptureFillThisBuffer == OMX_FALSE)
                    {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
						pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
                        pPreviewBuffer->bCaptureFillThisBuffer = OMX_TRUE;
                        OMX_CAMERA_PRINTF_L3("Set CaptureFillThisBuffer flag to Buffer %d", 
                                             pCameraBuffer->nIndex);
                    }
                    else
                    {
                        OMX_CAMERA_PRINTF_L3("Duplicated Buffer %d", 
                                             pCameraBuffer->nIndex);
                        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                                  OMX_ErrorBadParameter,
                                                  "Duplicated Buffer in Capture Port")
                    }
                    
                    /*
                    When buffers are shared it may occur that the preview port
                    has already recieved the FillThisBuffer and is waiting
                    (QueueBuffer_Video) for the FillThisBuffer of the capture
                    port. This will signal the other thread and allow to queue it.
                    */
                    OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Check Lock");
                    pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
                    OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Lock");
                    pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
                    OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Signal Condition");
                    pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
                    OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Check UnLock");
                }
                else
                {
                    pCameraBuffer->bCaptureFillThisBuffer = OMX_TRUE;
                    eError = QueueBuffer_Video(pComponentPrivate, pCameraBuffer);
                    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error in QueueBuffer");
                    if(pComponentPrivate->bExit == OMX_TRUE)
                    {
                        pComponentPrivate->bExit = OMX_FALSE;
                        eError = OMX_ErrorNone;
                        goto OMX_CAMERA_BAIL_CMD;
                    }
                }
            }
            else if(pComponentPrivate->bCapturing == OMX_TRUE)
            {
            
                /*
                Mutex will block the thread if the the camera is
                on a state transition, when the state transition is
                done it will unblock this mutex and process the buffer
                properly
                */
                OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Check Lock");
                pthread_mutex_lock(&(pComponentPrivate->mStateTransitionMutex));
                OMX_CAMERA_PRINTF_L4("StateTranistionMutex: Locked");
                pthread_mutex_unlock(&(pComponentPrivate->mStateTransitionMutex));
                OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Unlock");
                
                 /*
                Mutex will block the thread if the camera is attending
                a setconfig request from the IL Client, when the SetConfig
                is done the Thread will continue and process the buffer
                properly.
                */
                OMX_CAMERA_PRINTF_L4("SetConfig: Check Lock");
                pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
                OMX_CAMERA_PRINTF_L4("SetConfig: Locked");
                /* Writing buffer to still buffer pipe */
     
                pCameraBuffer = &(pComponentPrivate->StillBuffer[pCameraBuffer->nIndex]);
                              
                if(pCameraBuffer->bFirstTime == OMX_FALSE)
                {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
                    pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
                }
                
                /*
                Check if FillThisBuffers request has not exceded the numbers shots to take.
                If bOnseShot is disbale we take continuous shots until we turn off
                the bCapturing.
                if bOneShot is enable we only receive one, if bFrameLimit is enable we only
                receive nFrameLimit buffers.
                 */
                (pComponentPrivate->nStillFillThisBufferRcv)++;
                if((!pComponentPrivate->sSensorMode.bOneShot ||
                   (pComponentPrivate->sSensorMode.bOneShot &&
                    pComponentPrivate->nStillFillThisBufferRcv == 1)) ||
                   (pComponentPrivate->sCaptureMode.bContinuous ||
                    (pComponentPrivate->sCaptureMode.bFrameLimited &&
                    pComponentPrivate->nStillFillThisBufferRcv <= pComponentPrivate->sCaptureMode.nFrameLimit)))
                {
                    /* Queing Buffer to Camera Driver */
                    
                    eError = QueueBuffer_Still(pComponentPrivate, pCameraBuffer);
                    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error in QueueBuffer");
               
                    /* Put Buffer in Pipe */
                    OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
                    pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
                    OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
					OMX_CAMERA_PRINTF_L3("Sending Still Buffer : %p", pCameraBuffer);
                    OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nStillBufPipe[1],
                                            &pCameraBuffer,
                                            sizeof(OMX_CAMERA_COMPONENT_BUFFER*)) != sizeof(OMX_CAMERA_COMPONENT_BUFFER*), 
                                            eError,
                                            OMX_ErrorHardware,
                                            "Error writing to  capture buffer pipe");
                    pCameraBuffer->bPipe = OMX_TRUE;
                    pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
                    OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
                }
                else
                {
                    /*TODO: Save buffers on link list to later be process*/
                    OMX_CAMERA_PRINTF_ERROR("Ignoring Image Capture Buffer");
                }
                pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
                OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
            }
			else{
					OMX_CAMERA_PRINTF_L4("mVideoBufMutex: Check Lock");
				    pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
				    OMX_CAMERA_PRINTF_L4("mVideoBufMutex: Locked");

OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
					pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
					OMX_CAMERA_PRINTF_L3("Capture Buffer: %p, bCapturing = 0 , owner=%d",pCameraBuffer,pCameraBuffer->eBufferOwner);		
					
					pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
					OMX_CAMERA_PRINTF_L4("mVideoBufMutex: Unlocked");
			}
        }
        else if (IsThumbnailPort(pBufferHeader->nOutputPortIndex))
        {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
			pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
			OMX_CAMERA_PRINTF_L3("Tumbnail received\n");
            pThumbnailPort = &(pComponentPrivate->sCompPorts[OMXCAM_THUMBNAIL_PORT]);
            if (pThumbnailPort->pPortDef.bEnabled == OMX_TRUE &&
                pComponentPrivate->bThumbnailReady == OMX_TRUE)
            {
				OMX_CAMERA_PRINTF_L3("Tumbnail enabled\n");
                if(pThumbnailPort->hTunnelComponent == NULL)
                {
                    pCameraBuffer->eBufferOwner = BUFFER_WITH_CLIENT;
					OMX_CAMERA_PRINTF_L3("Send to IL Client Thumbnail Buffer");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pBufferHeader,pBuffer),
                                      PREF(pBufferHeader,nFilledLen),
                                      PERF_ModuleHLMM);
#endif
                    pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle, 
                                                                    pComponentPrivate->pHandle->pApplicationPrivate, 
                                                                    pBufferHeader);
                }
                else if (pThumbnailPort->eSupplierSetting == OMX_BufferSupplyInput)
                {
					OMX_CAMERA_PRINTF_L3("Send to Tunneled Component Thumbnail Buffer");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pBufferHeader,pBuffer),
                                      PREF(pBufferHeader,nFilledLen),
                                      PERF_ModuleLLMM);
#endif
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
					pCameraBuffer->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
                    eError = OMX_EmptyThisBuffer(pThumbnailPort->hTunnelComponent,
                                                 pBufferHeader);
                    OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
                }
				else if (pThumbnailPort->eSupplierSetting == OMX_BufferSupplyOutput)
                {
				OMX_CAMERA_PRINTF_L3("Tumbnail OMX_BufferSupplyOutput\n");
				}
            }
            else
            {
				OMX_CAMERA_PRINTF_L3("bThumbnailFillThisBuffer = OMX_TRUE");
                pComponentPrivate->bThumbnailFillThisBuffer = OMX_TRUE;
            }
        }
    }
    else
    {
        /*
        Send signal to mutex in WaitForBuffers when component is
        passing from Executing to Idle state.
        */
		OMX_CAMERA_PRINTF_L3("Receiving Buffer [%d]=%p",pCameraBuffer->nIndex,pCameraBuffer);
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
		pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
        OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Check Lock");
        pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
        OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Lock");
        pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
        OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Signal Condition");
        pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
        OMX_CAMERA_PRINTF_L4("FillThisBufferMutex: Check UnLock");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;                
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_FillThisBuffer */


/*-------------------------------------------------------------------*/
/**
  * Camera_ComponentDeInit()
  *
  * Deinitializes component by closing video device.
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_ErrorNone       Success, ready to roll.
  *         OMX_ErrorHardware  Failure in closing video device.
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE Camera_ComponentDeInit(OMX_HANDLETYPE hComp)
{
   OMX_ERRORTYPE eError                            = OMX_ErrorNone;
   OMX_COMPONENTTYPE *pHandle                      = NULL;
   OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;

   OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
   pHandle = (OMX_COMPONENTTYPE*) hComp;
   pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
OMX_CAMERA_PRINTF_L3("going to DeInit");

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif

#if 0
   /* Restore initial camera parameters */
   EXIT_IF_ERROR(SetCameraParameters(pComponentPrivate),"Error in SetCameraParameters");
#endif
   /* Stop component thread */
OMX_CAMERA_PRINTF_L3("going to StopCameraComponentThread");
   OMX_CAMERA_IF_ERROR_BAIL((eError = StopCameraComponentThread(hComp)), 
                            "Error in StopCameraComponentThread");

#if 0
    fclose(pComponentPrivate->fileout);
#endif

   /* Close all pipes */
OMX_CAMERA_PRINTF_L3("closing pipes");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nPreviewBufPipe[0]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nPreviewBufPipe");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nStillBufPipe[0]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nCaptureBufPipe");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nPreviewBufPipe[1]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nPreviewBufPipe");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nStillBufPipe[1]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nCaptureBufPipe");
OMX_CAMERA_PRINTF_L3("going to close nCmdPipe[0]");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nCmdPipe[0]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nCmdPipe");
OMX_CAMERA_PRINTF_L3("going to close nCmdPipe[1]");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nCmdPipe[1]) < 0,
                         eError,
                         OMX_ErrorInsufficientResources,
                         "Error closing nCmdPipe");
   OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nCmdDataPipe[0]) < 0,
                               eError, 
                               OMX_ErrorInsufficientResources,
                              "Error closing nCmdDataPipe");
   OMX_CAMERA_CHECK_COND(close (pComponentPrivate->nCmdDataPipe[1]) < 0,
                                eError,
                                OMX_ErrorInsufficientResources,
                                "Error closing nCmdDataPipe");
OMX_CAMERA_PRINTF_L3("closed all pipes");
   
    /*Unblock Thread that is in FillThisBuffer waiting for Capture FillThisBuffer*/
    /* Send signal to unblock FillThisBuffer mutex */
OMX_CAMERA_PRINTF_L3("lock mFillThisBufferMutex");
    pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
OMX_CAMERA_PRINTF_L3("signal sConditionFillThisBuffer");
    pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
OMX_CAMERA_PRINTF_L3("unlock mFillThisBufferMutex");
    pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
   
OMX_CAMERA_PRINTF_L3("destroy mVideoBufMutex");
    if (pthread_mutex_destroy(&(pComponentPrivate->mVideoBufMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
OMX_CAMERA_PRINTF_L3("destroy mPipeMutex");
    if (pthread_mutex_destroy(&(pComponentPrivate->mPipeMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
OMX_CAMERA_PRINTF_L3("mDriverMutex");
    if (pthread_mutex_destroy(&(pComponentPrivate->mDriverMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
OMX_CAMERA_PRINTF_L3("mStateTransitionMutex");
    if (pthread_mutex_destroy(&(pComponentPrivate->mStateTransitionMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
OMX_CAMERA_PRINTF_L3("mFillThisBufferMutex");
    if (pthread_mutex_destroy(&(pComponentPrivate->mFillThisBufferMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
    if (pthread_mutex_destroy(&(pComponentPrivate->mPortTransitionMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
    if (pthread_mutex_destroy(&(pComponentPrivate->mCameraPipeEmptyMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
    
    
    if (pthread_cond_destroy(&(pComponentPrivate->sConditionFillThisBuffer)) != 0)
    {
        perror("Error with pthread_cond_destroy");
    }
    
    if (pthread_cond_destroy(&(pComponentPrivate->sConditionPortTransition)) != 0)
    {
        perror("Error with pthread_cond_destroy");
    }
    
    if (pthread_cond_destroy(&(pComponentPrivate->sConditionCameraPipeEmpty)) != 0)
    {
        perror("Error with pthread_cond_destroy");
    }
#ifdef __CAMERA_2A__    
    if (pthread_cond_destroy(&(pComponentPrivate->sConditionFullLinkedList)) != 0)
    {
        perror("Error with pthread_cond_destroy");
    }
    if (pthread_mutex_destroy(&(pComponentPrivate->mFullLinkedListMutex)) != 0)
    {
        perror("Error with pthread_mutex_destroy");
    }
#endif
OMX_CAMERA_PRINTF_L3("free the component");
    
                         
   /* Free component name */
   OMX_CAMERA_FREE(pComponentPrivate->cComponentName);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERF,
                  PERF_BoundaryComplete | PERF_BoundaryCleanup);

    PERF_Done(pComponentPrivate->pPERF);
#endif

   OMX_CAMERA_FREE(pComponentPrivate);
#ifdef RESOURCE_MANAGER_ENABLED
   /* RM proxy De-init */    
   OMX_CAMERA_IF_ERROR_BAIL((eError = RMProxy_DeinitalizeEx(OMX_COMPONENTTYPE_CAMERA)), 
                            "Error while Deinitializing RM\n");
#endif
    
    /* Function executed properly */
    OMX_CAMERA_PRINTF_L3("Exit ComponentDeInit Properly");
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_ComponentDeInit */


/*-------------------------------------------------------------------*/
/**
  * Camera_VerifyTunnelConnection()
  *
  * Check if input port is compatible with output port 
  *
  * @param pPort            Output port (supplier)
  * @param hTunneledComp    Input port  (receiver)
  *
  * @retval OMX_ErrorNone                Success, ready to roll.
  *         OMX_ErrorPortsNotCompatible  Ports not compatible.
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE Camera_VerifyTunnelConnection(CAMERA_PORT_TYPE *pPort,
                                                   OMX_HANDLETYPE hTunneledComp)
{
    
   OMX_ERRORTYPE eError = OMX_ErrorUndefined;
   OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

   OMX_CAMERA_CHECK_CMD(pPort, hTunneledComp, OMX_TRUE);

   sPortDef.nSize                    = sizeof(sPortDef);
   sPortDef.nVersion.s.nVersionMajor = VERSIONMAJOR;
   sPortDef.nVersion.s.nVersionMinor = VERSIONMINOR;
   sPortDef.nPortIndex               = pPort->nTunnelPort;
  
   /* Get definition of the other port */
   eError=OMX_GetParameter(hTunneledComp, 
                           OMX_IndexParamPortDefinition,
                           &sPortDef);
   OMX_CAMERA_IF_ERROR_BAIL(eError,
                            "Error GetParameter OMX_IndexParamPortDefinition from hTunneledComp");

   /* COmpare domain specific parameters */
    switch(pPort->pPortDef.eDomain)
    {
    case OMX_PortDomainOther:
        if (pPort->pPortDef.format.other.eFormat!= sPortDef.format.other.eFormat)
        {
            pPort->hTunnelComponent = NULL;
            pPort->nTunnelPort      = 0;
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorPortsNotCompatible,
                                      "OMX_PortDomainOther Failed");
        }
        break;
    case OMX_PortDomainAudio:
        if (pPort->pPortDef.format.audio.eEncoding != sPortDef.format.audio.eEncoding)
        {
            pPort->hTunnelComponent = NULL;
            pPort->nTunnelPort      = 0;
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorPortsNotCompatible,
                                      "OMX_PortDomainAudio Failed");
        }
        break;
    case OMX_PortDomainVideo:
        if (pPort->pPortDef.format.video.eCompressionFormat != sPortDef.format.video.eCompressionFormat)
        {
            pPort->hTunnelComponent = NULL;
            pPort->nTunnelPort      = 0;
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorPortsNotCompatible,
                                      "OMX_PortDomainVideo Failed");
        }
        break;
    case OMX_PortDomainImage:
        if (pPort->pPortDef.format.image.eCompressionFormat != sPortDef.format.image.eCompressionFormat)
        {
            pPort->hTunnelComponent = NULL;
            pPort->nTunnelPort      = 0;
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorPortsNotCompatible,
                                      "OMX_PortDomainImage Failed");
        }
        break;
    default: 
        /* Our current port is not set up correctly */
        pPort->hTunnelComponent = NULL;
        pPort->nTunnelPort      = 0;
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorPortsNotCompatible,
                                  "Port is not set up correctly");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  ComponentTunnelRequest() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE Camera_ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                                    OMX_U32 nPort, 
                                                    OMX_HANDLETYPE hTunneledComp, 
                                                    OMX_U32 nTunneledPort, 
                                                    OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pPort                         = NULL;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    pPort = &(pComponentPrivate->sCompPorts[nPort]);

    if ((pTunnelSetup == NULL) || (hTunneledComp == NULL))
    {
        /* cancel previous tunnel */
        pPort->hTunnelComponent = 0;
        pPort->nTunnelPort = 0;
        pPort->eSupplierSetting = OMX_BufferSupplyUnspecified;
    }
    else 
    {
        /* Check port direction validity */
        OMX_CAMERA_CHECK_COND((pPort->pPortDef.eDir != OMX_DirInput) && 
                              (pPort->pPortDef.eDir != OMX_DirOutput), 
                              eError, 
                              OMX_ErrorBadParameter, 
                              "Camera tunnel OMX_ErrorBadParameter\n");
        
        /* Check if the other component is developed by TI */
        if(IsTIOMXComponent(hTunneledComp) != OMX_TRUE)
        {
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorTunnelingUnsupported,
                                      "ErrorTunnelingUnsupported\n");
        }

        pPort->hTunnelComponent = hTunneledComp;
        pPort->nTunnelPort = nTunneledPort;

        if (pPort->pPortDef.eDir == OMX_DirOutput)
        {
            /* Component port is the output (source of data) */
            pTunnelSetup->eSupplier = pPort->eSupplierSetting;
        }
        else
        {
             /* Component is the input (sink of data) */
             eError = Camera_VerifyTunnelConnection(pPort, hTunneledComp);
             OMX_CAMERA_CHECK_COND(eError != OMX_ErrorNone, 
                                   eError, 
                                   OMX_ErrorPortsNotCompatible, 
                                   "Error !! PP Camera_VerifyTunnelConnection failed\n");
            
             /* If specified obey port's preferences. Otherwise choose output */
             pPort->eSupplierSetting = pTunnelSetup->eSupplier;
             if (OMX_BufferSupplyUnspecified == pPort->eSupplierSetting)
             {
                 pPort->eSupplierSetting = pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
             }

             /* Tell the tunneled output port who the supplier is */
             OMX_CAMERA_INIT_SIZE_VERSION(&sBufferSupplier,
                                          OMX_PARAM_BUFFERSUPPLIERTYPE);
             sBufferSupplier.nPortIndex               = nTunneledPort;
             sBufferSupplier.eBufferSupplier          = pPort->eSupplierSetting;

             /* Set tunneled port's supplier settings */
             eError = OMX_SetParameter(hTunneledComp,
                                       OMX_IndexParamCompBufferSupplier,
                                       &sBufferSupplier);
             OMX_CAMERA_IF_ERROR_BAIL(eError, "Error SetParameter hTunneledComp");

             /* Verify tunneled port's supplier settings */
             eError = OMX_GetParameter(hTunneledComp,
                                       OMX_IndexParamCompBufferSupplier,
                                       &sBufferSupplier);
             OMX_CAMERA_IF_ERROR_BAIL(eError, 
                                      "Error GetParameter hTunneledComp");
             OMX_CAMERA_CHECK_COND(sBufferSupplier.eBufferSupplier != pPort->eSupplierSetting, 
                                   eError, 
                                   OMX_ErrorUndefined, 
                                   "OMX_IndexParamCompBufferSupplier failed to change setting\n");
        }
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_ComponentTunnelRequest */



/*----------------------------------------------------------------------------*/
/**
  *  UseBuffer() 
  *
  * 
  * 
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE Camera_UseBuffer(OMX_IN OMX_HANDLETYPE hComp,
                               OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                               OMX_IN OMX_U32 nPortIndex,
                               OMX_IN OMX_PTR pAppPrivate,
                               OMX_IN OMX_U32 nSizeBytes,
                               OMX_IN OMX_U8* pBuffer)
{   
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer      = NULL;
    OMX_S32 nBufIndex;

    OMX_CAMERA_CHECK_CMD(hComp, pBuffer, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer,
                       nSizeBytes,
                       PERF_ModuleHLMM);
#endif
    /* Check correct port */
    OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nPortIndex),
                          eError,
                          OMX_ErrorBadParameter,
                          "Wrong Port Index\n");

    /* Set port definition pointer */
    pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);

    /* Check if port is enabled */
    OMX_CAMERA_CHECK_COND(!pPortDef->bEnabled,
                          eError,
                          OMX_ErrorIncorrectStateOperation,
                          "Port not enabled\n");

    /* equal buffer sizes?*/
    OMX_CAMERA_CHECK_COND(nSizeBytes != pPortDef->nBufferSize,
                          eError,
                          OMX_ErrorBadParameter,
                          "Wrong buffer size\n");

    /* NMM TBD .. Is this a valid check ? (Check if port is already populated) */
    OMX_CAMERA_CHECK_COND(pPortDef->bPopulated,
                          eError,
                          OMX_ErrorBadParameter,
                          "Port already populated\n");
    /* Create and initialize buffer structures */
    if(nPortIndex == OMXCAM_PREVIEW_PORT || 
       nPortIndex == OMXCAM_THUMBNAIL_PORT ||
       (nPortIndex == OMXCAM_CAPTURE_PORT &&
       pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_FALSE) ||
       IsStillCaptureMode(pComponentPrivate))
    {
        OMX_CAMERA_PRINTF_L3("Allocate Preview Port Buffer: %x", pBuffer);
        eError = CreateCameraBuffer(pComponentPrivate,
                                    nPortIndex,
                                    &nBufIndex,
                                    nSizeBytes,
                                    pBuffer,
                                    pAppPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "CreateCameraBuffer error\n");

        pCameraBuffer = pComponentPrivate->sCompPorts[nPortIndex].pCameraBuffer[nBufIndex];
        pCameraBuffer->bSelfAllocated = OMX_FALSE;
        pCameraBuffer->pBufferStart = NULL;
        pCameraBuffer->bCaptureFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bPreviewFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bCapturing = OMX_FALSE;
        pCameraBuffer->bShared = OMX_FALSE;
        *ppBufferHdr = pCameraBuffer->pBufHeader;
		OMX_CAMERA_PRINTF_L3("Allocated PreviewBuffer[%d] = %p, pBuffer=%p",pCameraBuffer->nIndex,pCameraBuffer,pBuffer);

        /* Set buffer owner status */
        if(pComponentPrivate->sCompPorts[nPortIndex].hTunnelComponent != NULL)
        {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
            pCameraBuffer->eBufferOwner =  BUFFER_WITH_TUNNELEDCOMP;
        }
        else
        {
            pCameraBuffer->eBufferOwner = BUFFER_WITH_CLIENT;
        }
    }
    else
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorUnsupportedSetting,
                                  "UseBuffer is not supported for Caputure Port\n"); 
    } 

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:  
    return eError;
} /* End of Camera_UseBuffer */


/*----------------------------------------------------------------------------*/
/**
  *  CreateCameraBuffer() 
  *
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE CreateCameraBuffer(OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate, 
                                 OMX_U32 nPortIndex,
                                 OMX_S32 *pBufIndex, 
                                 OMX_U32 nSizeBytes, 
                                 OMX_U8* pBuffer,
                                 OMX_PTR pAppPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer = NULL;
    OMX_S32 nCount = 0;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    
    /* Set buffer index */
    *pBufIndex = nCount =  pComponentPrivate->sCompPorts[nPortIndex].nBufferCount;

    pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);

    /* Allocate commponent buffer structure */
    OMX_CAMERA_CALLOC(pComponentPrivate->sCompPorts[nPortIndex].pCameraBuffer[nCount],
                      OMX_CAMERA_COMPONENT_BUFFER,
                      1, 
                      OMX_CAMERA_COMPONENT_BUFFER,
                      eError);

    pCameraBuffer = pComponentPrivate->sCompPorts[nPortIndex].pCameraBuffer[nCount];
             
    /* Allocate buffer header structure */
    OMX_CAMERA_CALLOC(pCameraBuffer->pBufHeader,
                      OMX_BUFFERHEADERTYPE,
                      1,
                      OMX_BUFFERHEADERTYPE,
                      eError);
    OMX_CAMERA_INIT_SIZE_VERSION(pCameraBuffer->pBufHeader,
                                 OMX_BUFFERHEADERTYPE);
        
    pCameraBuffer->pBufHeader->pBuffer = pBuffer;
    pCameraBuffer->pBufHeader->nFilledLen           = 0;
    pCameraBuffer->pBufHeader->nAllocLen            = nSizeBytes;
    pCameraBuffer->nIndex                           = nCount;
    pCameraBuffer->bFirstTime                       = OMX_TRUE;
    pCameraBuffer->pBufHeader->pOutputPortPrivate   = pCameraBuffer;
    pCameraBuffer->pBufHeader->nOutputPortIndex     = nPortIndex;
    pCameraBuffer->pBufHeader->pAppPrivate          = pAppPrivate; 

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pCameraBuffer->pBufHeader->pBuffer,
                       pCameraBuffer->pBufHeader->nFilledLen,
                       PERF_ModuleHardware);
#endif

    pComponentPrivate->sCompPorts[nPortIndex].nBufferCount++;

    if(pComponentPrivate->sCompPorts[nPortIndex].nBufferCount == pPortDef->nBufferCountActual)
    {
        pPortDef->bPopulated = OMX_TRUE;
        OMX_CAMERA_PRINTF_L4("Lock PortTransitionMutex");
        pthread_mutex_lock(&(pComponentPrivate->mPortTransitionMutex));
        OMX_CAMERA_PRINTF_L4("Lock PortTransitionMutex Success");
        pthread_cond_signal(&(pComponentPrivate->sConditionPortTransition));
        OMX_CAMERA_PRINTF_L4("Signal ConditionPortTransition");
        pthread_mutex_unlock(&(pComponentPrivate->mPortTransitionMutex));
        OMX_CAMERA_PRINTF_L4("Unlock ConditionPortTransition");
    }
   
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD: 
    return eError;
}

/*----------------------------------------------------------------------------*/
/**
  *  AllocateBuffer() 
  *
  * 
  * 
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE Camera_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComp,
                                    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                    OMX_IN OMX_U32 nPortIndex,
                                    OMX_IN OMX_PTR pAppPrivate,
                                    OMX_IN OMX_U32 nSizeBytes)
{
    OMX_U32 nCount                                  = 0;
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_U8 *pBufferStart                            = NULL;
    OMX_U8 *pBufferAligned                          = NULL;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_CAMERA_COMPONENT_BUFFER  *pCameraBuffer     = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;  
    OMX_S32 nBufIndex;
    OMX_STATETYPE eCurrentState;
    
    /* Check non-NULL parameters*/
    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComp;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    /*output port?*/
    OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nPortIndex), 
                          eError, 
                          OMX_ErrorBadParameter,
                          "Wrong Port Index\n");

    pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
    eCurrentState = pComponentPrivate->eCurrentState;

    OMX_CAMERA_CHECK_COND((!pPortDef->bEnabled) && 
                          (eCurrentState != OMX_StateIdle) && 
                          (eCurrentState != OMX_StateExecuting) && 
                          (eCurrentState != OMX_StatePause), 
                          eError, OMX_ErrorIncorrectStateOperation,
                          "Incorrect State Operation\n");
                        
   /* Compare buffer size with the one in port defenition */
    OMX_CAMERA_CHECK_COND(nSizeBytes != pPortDef->nBufferSize,
                          eError,
                          OMX_ErrorBadParameter,
                          "Wrong nSizeBytes parameter\n");
   
    /* Check if port is already populated */
    OMX_CAMERA_CHECK_COND(pPortDef->bPopulated,
                          eError,
                          OMX_ErrorBadParameter,
                          "Port already populated\n");
                          
    if(nPortIndex == OMXCAM_PREVIEW_PORT ||
       nPortIndex == OMXCAM_THUMBNAIL_PORT ||
       (nPortIndex == OMXCAM_CAPTURE_PORT &&
       pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_FALSE)||
       IsStillCaptureMode(pComponentPrivate))
    {
        /* Create user space buffer - buffer is over allocated by 128 bytes */
        OMX_U32 nBufferLength = nSizeBytes;
        if (nSizeBytes & 0xfff)
        {
            nBufferLength = (nSizeBytes & 0xfffff000) + 0x1000;
        }
        
        OMX_CAMERA_CALLOC(pBufferStart,
                         OMX_U8,
                         (nBufferLength + 0x20 + 256),
                         OMX_U8, eError);
        /* Align buffer to 4K now because of change in camera driver  */
	pBufferAligned = pBufferStart;
        while ((((int)pBufferAligned) & 0xfff) != 0)
        {
            pBufferAligned++;  
        }
        /* Buffer pointer shifted to avoid DSP cache issues */
        //pBufferAligned += 128;
        if(nPortIndex == OMXCAM_PREVIEW_PORT)
        {
            OMX_CAMERA_PRINTF_L3("Allocate Preview Port Buffer: %p", pBufferAligned);
        }
        else
        {
            OMX_CAMERA_PRINTF_L3("Allocate Capture Port Buffer: %p", pBufferAligned);
        }
        /*Create and initialize buffer structures */
        eError = CreateCameraBuffer(pComponentPrivate,
                                    nPortIndex,
                                    &nBufIndex,
                                    nBufferLength,
                                    pBufferAligned,
                                    pAppPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "CreateCameraBuffer error\n");
        pCameraBuffer = pComponentPrivate->sCompPorts[nPortIndex].pCameraBuffer[nBufIndex];
        pCameraBuffer->bSelfAllocated = OMX_TRUE;
        pCameraBuffer->pBufferStart = pBufferStart;
        pCameraBuffer->bCaptureFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bPreviewFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bCapturing = OMX_FALSE;
        pCameraBuffer->bShared = OMX_FALSE;
        *ppBufferHdr = pCameraBuffer->pBufHeader;
    }
    else if(nPortIndex == OMXCAM_CAPTURE_PORT)
    {
        /* Check if Preview Port is already populated */
        OMX_CAMERA_CHECK_COND(!pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bPopulated, 
                              eError,
                              OMX_ErrorBadParameter,
                              "Preview Port is not populated\n");
        while(1)
        {
            pCameraBuffer = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pCameraBuffer[nCount];
            if(pCameraBuffer->bShared == OMX_FALSE)
            {
                OMX_CAMERA_PRINTF_L3("Share pBuffer with Capture Port: %p",
                                  pCameraBuffer->pBufHeader->pBuffer);
                pBufferAligned = pCameraBuffer->pBufHeader->pBuffer;
                pCameraBuffer->bShared = OMX_TRUE; 
                break;
            }
            nCount++;
            if(nCount == OMXCAM_MAX_NUM_BUFFERS)
            {
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorInsufficientResources,
                                          "Error: No Preview Buffer available to Share\n");
            }
        }
        eError = CreateCameraBuffer(pComponentPrivate,
                                    nPortIndex,
                                    &nBufIndex,
                                    nSizeBytes,
                                    pBufferAligned,
                                    pAppPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "CreateCameraBuffer error\n");
        pCameraBuffer = pComponentPrivate->sCompPorts[nPortIndex].pCameraBuffer[nBufIndex];
        pCameraBuffer->bSelfAllocated = OMX_FALSE;
        pCameraBuffer->pBufferStart = pBufferStart;
        pCameraBuffer->bCaptureFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bPreviewFillThisBuffer = OMX_FALSE;
        pCameraBuffer->bCapturing = OMX_FALSE;
        pCameraBuffer->bShared = OMX_TRUE;
        *ppBufferHdr = pCameraBuffer->pBufHeader;
    }
    else
    {
       OMX_CAMERA_SET_ERROR_BAIL(eError,
                                 OMX_ErrorBadParameter,
                                 "Error: Wrong Index Port\n");
    }

    /* Set buffer owner status */
    if(pComponentPrivate->sCompPorts[nPortIndex].hTunnelComponent != NULL)
    {
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
        pCameraBuffer->eBufferOwner =  BUFFER_WITH_COMPONENT;
    }
    else 
    {
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CLIENT;
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:  
    return eError;
}/* End of Camera_AllocateBuffer */


/*----------------------------------------------------------------------------*/
/**
  *  FreeBuffer() 
  *
  * 
  * 
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

OMX_ERRORTYPE Camera_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_U32 nPortBufferCount                        = 0;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_CAMERA_COMPONENT_BUFFER  *pPreviewBuffer    = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_CAMERA_COMPONENT_BUFFER  *pCameraBuffer     = NULL;

    OMX_CAMERA_CHECK_CMD(hComponent, pBuffer, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE*) hComponent;
    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pCameraBuffer = pBuffer->pOutputPortPrivate;

    /* Check the port index validity */
    OMX_CAMERA_CHECK_COND(!IsCameraOutPort(nPortIndex),
                          eError, 
                          OMX_ErrorBadParameter, 
                          "Wrong Port Index\n");

    pPortDef = &(pComponentPrivate->sCompPorts[nPortIndex].pPortDef);
    nPortBufferCount = pPortDef->nBufferCountActual;

    /* Decrement the buffer count */
    pComponentPrivate->sCompPorts[nPortIndex].nBufferCount--;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERF,
                      PREF(pCameraBuffer->pBufHeader,pBuffer),
                      PREF(pCameraBuffer->pBufHeader,nAllocLen),
                      PERF_ModuleHardware);
#endif


    if(nPortIndex == OMXCAM_PREVIEW_PORT || 
       nPortIndex == OMXCAM_THUMBNAIL_PORT ||
       IsStillCaptureMode(pComponentPrivate))
    {
    /* Free the buffer if self-allocated */
        if(pCameraBuffer->bSelfAllocated) 
        {
            OMX_CAMERA_FREE(pCameraBuffer->pBufferStart);
        }
    }
    else if(nPortIndex == OMXCAM_CAPTURE_PORT)
    {
        pPreviewBuffer = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pCameraBuffer[pCameraBuffer->nIndex];
        if(pPreviewBuffer->pBufHeader != NULL)
        {
            pPreviewBuffer->bShared = OMX_FALSE;
            OMX_CAMERA_PRINTF_L3("Un - Share pBuffer with Capture Port: %p",
                                 pCameraBuffer->pBufHeader->pBuffer);
        }
    }

    /* Free buffer structures */
    OMX_CAMERA_FREE(pCameraBuffer->pBufHeader);
    OMX_CAMERA_FREE(pCameraBuffer);

    if((pPortDef->bEnabled) && 
    ((pComponentPrivate->eCurrentState == OMX_StateIdle && 
         pComponentPrivate->eDesiredState != OMX_StateLoaded) ||
         pComponentPrivate->eCurrentState == OMX_StateExecuting ||
         pComponentPrivate->eCurrentState == OMX_StatePause))
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorPortUnpopulated,
                                 OMX_TI_ErrorMajor,
                                 "Port Unpopulated");
    }
    else if (!pPortDef->bEnabled && !pPortDef->bPopulated)
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate, 
                                 OMX_EventCmdComplete, 
                                 OMX_CommandPortDisable,
                                 nPortIndex,
                                 NULL);
    }

    if(pComponentPrivate->sCompPorts[nPortIndex].nBufferCount == 0)
    {
        pPortDef->bPopulated = OMX_FALSE;
        OMX_CAMERA_PRINTF_L4("Lock PortTransitionMutex");
        pthread_mutex_lock(&(pComponentPrivate->mPortTransitionMutex));
        OMX_CAMERA_PRINTF_L4("Lock PortTransitionMutex Success");
        pthread_cond_signal(&(pComponentPrivate->sConditionPortTransition));
        OMX_CAMERA_PRINTF_L4("Signal ConditionPortTransition");
        pthread_mutex_unlock(&(pComponentPrivate->mPortTransitionMutex));
        OMX_CAMERA_PRINTF_L4("Unlock ConditionPortTransition");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of Camera_FreeBuffer */

/*----------------------------------------------------------------------------*/
/**
  *  Camera_GetExtensionIndex() 
  *
  * 
  * 
  *
  * @param 
  * @param 
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*----------------------------------------------------------------------------*/

static OMX_ERRORTYPE Camera_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, 
                                              OMX_IN OMX_STRING cParameterName, 
                                              OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    CAMERA_CUSTOM_DEFINITION sCameraCustomIndex[] = 
                                   {{"OMX.TI.Camera.Param.Rendering", CameraCustomParamRendering},
                                    {"OMX.TI.Camera.Param.ClockSource", CameraCustomParamSetClockSource},
                                    {"OMX.TI.Camera.Param.IPP", CameraCustomParamIPP},
                                    {"OMX.TI.Camera.Config.ColorEffect", CameraCustomConfigColorEffect},
                                    {"OMX.TI.Camera.Param.CameraDevice", CameraCustomParamCameraDevice},
                                    {"",0x0}
                                   }; 
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    int nIndex = 0;
 
    if (!hComponent || !pIndexType) 
    {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    *pIndexType = CameraCustomInvalid;

    for (nIndex = 0; strlen((const char*)sCameraCustomIndex[nIndex].cCustomName); nIndex++) 
    {
        if (!strcmp((const char*)cParameterName, (const char*)(&(sCameraCustomIndex[nIndex].cCustomName))))
        {
            *pIndexType = sCameraCustomIndex[nIndex].nCustomIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    if(*pIndexType == CameraCustomInvalid)
    {
         eError = OMX_ErrorUnsupportedIndex;
    }

EXIT:
    return eError;
}

