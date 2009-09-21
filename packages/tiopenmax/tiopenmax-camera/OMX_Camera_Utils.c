/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */

/**
* @file OMX_Camera_Utils.c
*
* This file implements utility functions called by application as well as
* component thread.
*
* @path  $(CSLPATH)\camera\src\OMX_Camera_Utils.c
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
#include <sys/types.h>
#include <malloc.h>
#include <memory.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sched.h>
#include <videodev.h>
#include <pthread.h> 
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <dlfcn.h>

/*------- Program Header Files ----------------------------------------*/
#include "OMX_Camera.h"
#include "OMX_Camera_Utils.h"
#include "OMX_Camera_Thread.h"
#include <ResourceManagerProxyAPI.h>
#include <OMX_Clock.h>
#include <omap24xxvout.h>

int bKernelBufRequested = 0;
extern int errno;


int usec1 = 0;
int sec1 = 0 ;
int count1 = 0;
struct timeval sTime1;
struct timezone sTimeZone1;





#ifdef __CAMERA_IPP__
static const IPP_EENFAlgoDynamicParams IPPEENFAlgoDynamicParamsArray [MAXIPPDynamicParams] = {
 {sizeof(IPP_EENFAlgoDynamicParams), 0, 180, 18, 120, 8, 40}//2
};
#endif


/*-------------------------------------------------------------------*/
/**
  * IsCameraOutPort()
  *
  * Checks if the the argument is an output port of the camera.
  *
  * @param nPortIndex  port index to be checked
  *
  * @retval       1                  success, ready to roll
  *               0                          otherwise
  **/
/*-------------------------------------------------------------------*/
inline OMX_BOOL IsCameraOutPort(OMX_U32 nPortIndex)
{
   return ((nPortIndex >= OMXCAM_PREVIEW_PORT) && (nPortIndex <= OMXCAM_THUMBNAIL_PORT));
}

/*-------------------------------------------------------------------*/
/**
  * IsPreviewPort()
  *
  * Checks if the the argument is preview port index of the camera.
  *
  * @param nPortIndex  port index to be checked
  *
  * @retval       1                  success, ready to roll
  *               0                          otherwise
  **/
/*-------------------------------------------------------------------*/
inline OMX_BOOL IsPreviewPort(OMX_U32 nPortIndex)
{
   return (nPortIndex == OMXCAM_PREVIEW_PORT);
}

/*-------------------------------------------------------------------*/
/**
  * IsCapturePort()
  *
  * Checks if the the argument is capture port index of the camera.
  *
  * @param nPortIndex  port index to be checked
  *
  * @retval       1                  success, ready to roll
  *               0                          otherwise
  **/
/*-------------------------------------------------------------------*/
inline OMX_BOOL IsCapturePort(OMX_U32 nPortIndex)
{
   return (nPortIndex == OMXCAM_CAPTURE_PORT);
}

/*-------------------------------------------------------------------*/
/**
  * IsThumbnailPort()
  *
  * Checks if the the argument is capture port index of the camera.
  *
  * @param nPortIndex  port index to be checked
  *
  * @retval       1                  success, ready to roll
  *               0                          otherwise
  **/
/*-------------------------------------------------------------------*/
inline OMX_BOOL IsThumbnailPort(OMX_U32 nPortIndex)
{
   return (nPortIndex == OMXCAM_THUMBNAIL_PORT);
}

/*-------------------------------------------------------------------*/
/**
  * IsStillCaptureMode()
  *
  * Checks if the the camera is working on Still Capture.
  *
  * @param nPortIndex  port index to be checked
  *
  * @retval       1                  success, ready to roll
  *               0                          otherwise
  **/
/*-------------------------------------------------------------------*/
inline OMX_BOOL IsStillCaptureMode(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    return (pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pPortDef.eDomain == OMX_PortDomainImage);
}


OMX_S32 ioctlCamera(int fildes, int request, void* ptr, OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_S32 eError;
    
    /*OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");*/
    /*PRINTF("fildes %d request %x", fildes, request);*/
    eError = ioctl(fildes,request,ptr);
    /*pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");*/
    
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * StopCameraComponentThread()
  *
  * Called by application thread, waits until component thread terminates.
  *
  * @param hComp  handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE StopCameraComponentThread(OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_COMMANDTYPE nCommand;
    
    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) hComp;

    pComponentPrivate =(OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* Send -1 to stop component thread */
    nCommand = OMXCAM_CMD_STOPTHREAD;

#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingCommand(pComponentPrivate->pPERF,
                        nCommand, 0,
                        PERF_ModuleComponent);
#endif
    OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nCmdPipe[1],
                                &nCommand, 
                                sizeof(OMX_STATETYPE)) != sizeof(OMX_STATETYPE), 
                          eError, 
                          OMX_ErrorInsufficientResources, 
                          "Error writing to nCmdPipe");

    /* Makes sure component thread is terminated */
    OMX_CAMERA_CHECK_COND(pthread_join(pComponentPrivate->sComponentThread,
                                       (void*)&eError), 
                          eError,
                          OMX_ErrorUndefined,
                          "error pthread_join in stopCameraComponent");
    
/* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of StopCameraComponentThread */


/*-------------------------------------------------------------------*/
/**
  * StartCameraComponentThread()
  *
  * Called by application thread.
  * Creates commad & data pipes and starts component thread.
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the pipe creation fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE StartCameraComponentThread(OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eError                            = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_CAMERA_CHECK_CMD(hComp, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) hComp;

    pComponentPrivate = (OMX_CAMERA_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    /* Create preview buffer pipe */
    OMX_CAMERA_CHECK_COND(pipe(pComponentPrivate->nPreviewBufPipe),
                          eError,
                          OMX_ErrorInsufficientResources,
                          "Error opening pipe nPreviewBufPipe\n");

    /* Create capture buffer pipe */
    OMX_CAMERA_CHECK_COND(pipe(pComponentPrivate->nStillBufPipe),
                          eError,
                          OMX_ErrorInsufficientResources,
                          "Error opening pipe nStillBufPipe\n");

    /* Create the pipe used to send commands to the thread */
    OMX_CAMERA_CHECK_COND(pipe(pComponentPrivate->nCmdPipe), 
                          eError, 
                          OMX_ErrorInsufficientResources, 
                          "Error opening pipe nCmdPipe\n");

   /* Create the pipe used to send commands to the thread */
   OMX_CAMERA_CHECK_COND(pipe(pComponentPrivate->nCmdDataPipe), 
                         eError, 
                         OMX_ErrorInsufficientResources, 
                         "Error opening pipe nCmdDataPipe\n");

   /* Create the Component Thread */
   eError = pthread_create(&(pComponentPrivate->sComponentThread), 
                           NULL, 
                           OMX_Camera_Thread, 
                           pComponentPrivate);

   OMX_CAMERA_CHECK_COND(eError || !pComponentPrivate->sComponentThread, 
                         eError, 
                         OMX_ErrorInsufficientResources, 
                         "Error Create the Component Thread\n");

#ifdef __PERF_INSTRUMENTATION__
    PERF_ThreadCreated(pComponentPrivate->pPERF,
                       pComponentPrivate->sComponentThread,
                       PERF_FOURCC('C','A','M','T'));
#endif

    /* Function executed properly */
    eError = OMX_ErrorNone;    
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of StartCameraComponentThread */


/*-------------------------------------------------------------------*/
/**
  * HandleCameraIdleState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraIdleState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_S32 nCount;
    OMX_U32 nIndex                              = 0;
    OMX_U32 nStartPortNumber                    = 0;
    OMX_U32 nPortNumber                         = 0;
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                  = NULL;
    CAMERA_PORT_TYPE *pPreviewPort              = NULL;
    CAMERA_PORT_TYPE *pPortType                 = NULL;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef      = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
#ifdef __CAMERA_2A__
    int (*Init2A) (Camera2AInterface** interface, int camHandle, uint8 enable3PTuning);
    int (*Release2A) (Camera2AInterface** interface );
#endif

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    nStartPortNumber = pComponentPrivate->sPortParam.nStartPortNumber;
    nPortNumber = nStartPortNumber + pComponentPrivate->sPortParam.nPorts;

    if(pComponentPrivate->eCurrentState == OMX_StateExecuting ||
       pComponentPrivate->eCurrentState == OMX_StatePause)
    {
        /* Stop 2A algorithm */
#ifdef __CAMERA_2A__
        OMX_CAMERA_STOP_2A(pComponentPrivate);
#endif
        /*Reset flags of buffers of the preview port.*/
        for(nCount = 0; nCount < pPreviewPort->nBufferCount; ++nCount)
        {
            pCameraBuffer = pPreviewPort->pCameraBuffer[nCount];
			printf("Accessing bCapturing:OMX_Camera_Utils.c-HandleCameraIdleState()\n");
            pCameraBuffer->bCapturing = OMX_FALSE;
            pCameraBuffer->bCaptureFillThisBuffer = OMX_FALSE;
            pCameraBuffer->bPreviewFillThisBuffer = OMX_FALSE;
            pCameraBuffer->bFirstTime = OMX_TRUE;
        }
        
        /* Flush Data Pipes */
        eError = FlushDataPipes(pComponentPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "Error in CleanDataPipes");
        
        /* Flush all Camera V4L2 Buffers */
        OMX_CAMERA_FLUSH_V4L2_BUFFERS(pPreviewPort);

        /* Used for setting clock start time */
        pComponentPrivate->bFirstCameraBuffer = OMX_TRUE;
        
        /* Return buffers */
        for(nIndex = nStartPortNumber; nIndex < nPortNumber; nIndex++)
        {
            pPortDef = &(pComponentPrivate->sCompPorts[nIndex].pPortDef);
			hTunnelComponent = pComponentPrivate->sCompPorts[nIndex].hTunnelComponent;
            if (pPortDef->bEnabled == OMX_TRUE)
            {
                OMX_CAMERA_PRINTF_L3("Returning buffers of port %d", nIndex);
                /* Tunneled mode and Supplier*/
                if(hTunnelComponent != NULL &&
                   pComponentPrivate->sCompPorts[nIndex].eSupplierSetting == OMX_BufferSupplyOutput)
                {
                    eError = WaitForBuffers(pComponentPrivate,
                                            &(pComponentPrivate->sCompPorts[nIndex]));
                    OMX_CAMERA_IF_ERROR_BAIL(eError,"Camera_ReturnBuffers Error");
                }
                /* If camera port is non supplier, return all possessed buffers */
                else
                {
                    eError = ReturnBuffers(pComponentPrivate,
                                           &(pComponentPrivate->sCompPorts[nIndex]));
                    OMX_CAMERA_IF_ERROR_BAIL(eError,"Camera_ReturnBuffers Error");
                }
            }
        } /* For loop ends */
    
        /* Turn the straming mode of camera driver OFF */
        OMX_CAMERA_TURN_OFF_STREAMING(pComponentPrivate);
		
		
        pComponentPrivate->eCurrentState = OMX_StateIdle;
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateExecuting to OMX_StateIdle\n");
    } 
    else if(pComponentPrivate->eCurrentState == OMX_StateLoaded ||
            pComponentPrivate->eCurrentState == OMX_StateWaitForResources)
    {
        /* Open camera device */
        /*MT Sensor*/
        if(pComponentPrivate->nCameraDevice == 0)
        {
            __PERF_V4L2m_CMD(pComponentPrivate, Sending, "\0\0\0\0");
            pComponentPrivate->nCameraDev = open(VIDEO_DEVICE_PRIMARY, O_RDWR); 
            __PERF_V4L2m_CMD(pComponentPrivate, Sent, "\0\0\0\0");
#ifdef __CAMERA_2A__
            __PERF_2Am_CMD(pComponentPrivate, Sending, 0x0);
            void* error;
            OMX_PTR libHandle = NULL;
            /* Load 2A library */
						
#ifdef __TI_FRAMEWORK__
            OMX_CAMERA_PRINTF_L6("Load TI Framework");
            libHandle = dlopen("libCameraAlg.so", RTLD_LAZY);
#else
            OMX_CAMERA_PRINTF_L6("Load MMS Framework");
            libHandle = dlopen("libMMS3AFW.so", RTLD_LAZY);
#endif
            if ( !libHandle )
            {
                if((error = dlerror()) != NULL)
                {
                    fputs(error, stderr);
                }
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorComponentNotFound,
                                          "Error at dlopen for 2A");
            }
            pComponentPrivate->libHandle = libHandle;
    
            __PERF_2Am_CMD(pComponentPrivate, Sending, 0x1);
            Init2A = dlsym (libHandle, "Init2A");
            if((error = dlerror()) != NULL)
            {
                fputs(error, stderr);
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorInvalidComponent,
                                          "Error at dlsym for Init2A");
            }
            pComponentPrivate->Init2A = Init2A;

            __PERF_2Am_CMD(pComponentPrivate, Sending, 0x1);
            Release2A = dlsym (libHandle, "Release2A");
            if ((error = dlerror()) != NULL)
            {
                fputs(error, stderr);
                OMX_CAMERA_SET_ERROR_BAIL(eError,
                                          OMX_ErrorInvalidComponent,
                                          "Error at dlsym for Release2A");
            }
            pComponentPrivate->Release2A = Release2A;

            OMX_CAMERA_PRINTF_L6("2A Lib Loaded");
            __PERF_2Am_CMD(pComponentPrivate, Received, 0x0);
            
            __PERF_2A_CMD(pComponentPrivate, Sending, 0x2);
            eError = Init2A(&(pComponentPrivate->cameraAlg),
                            pComponentPrivate->nCameraDev, 0);
            __PERF_2A_CMD(pComponentPrivate, Sent, 0x2);
            OMX_CAMERA_PRINTF_L6("Init2A: Successful");
            if (eError < 0)
            {
                OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                          OMX_ErrorHardware,
                                          "Error at Init2A");
            }
#endif
        }
        /* OV sensor */
        else if(pComponentPrivate->nCameraDevice == 1)
        {
            __PERF_V4L2m_CMD(pComponentPrivate, Sending, "\0\0\0\0");
            pComponentPrivate->nCameraDev = open(VIDEO_DEVICE_SECONDARY,O_RDWR); 
            __PERF_V4L2m_CMD(pComponentPrivate, Sent, "\0\0\0\0");
#ifdef __CAMERA_2A__
            pComponentPrivate->cameraAlg = NULL;
#endif
        }
        else
        {
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsoported Camera Device");
        }

        OMX_CAMERA_CHECK_COND(pComponentPrivate->nCameraDev <= 0, 
                              eError, 
                              OMX_ErrorHardware, 
                              "*Error:Open camera device Failed!");
        OMX_CAMERA_PRINTF_L2("Camera device opened");
        
#ifdef __CAMERA_2A__
        /* Initialize 2A Component */
        if(pComponentPrivate->cameraAlg)
        {
#ifdef __TI_FRAMEWORK__
            /* Set Settings */
            eError = SetCameraWhiteBalance(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"SetCameraWhiteBalance Error");
            eError = SetCameraExposure(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"SetCameraExposure Error");
#endif
        }
#endif

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                      PERF_BoundaryStart | PERF_BoundarySetup);
#endif
        /* Initialize camera driver */
        eError = InitCameraResources(pComponentPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError,"InitCameraResources Error");
        /* Make sure all enabled ports are populated */
        for(nIndex = nStartPortNumber; nIndex < nPortNumber; nIndex++)
        {
            pPortType = &(pComponentPrivate->sCompPorts[nIndex]);
            pPortDef = &(pPortType->pPortDef);
            
            /* Check if the port is enabled */
            if(pPortDef->bEnabled == OMX_TRUE)
            {
                OMX_CAMERA_PRINTF_L3("Population of Port %d", nIndex);
                eError = PortPopulation (pComponentPrivate, 
                                         pPortType);
                OMX_CAMERA_IF_ERROR_BAIL(eError,"Error at Population of Port");
            }
        }/* for loop ends */ 

       /* Change state to Idle */
       pComponentPrivate->eCurrentState = OMX_StateIdle;
#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                      PERF_BoundaryComplete | PERF_BoundarySetup);
#endif
        /* Notify state change */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateLoaded or OMX_StateWaitForResources to OMX_StateIdle");
    }
    else if(pComponentPrivate->eCurrentState == OMX_StateInvalid)
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorIncorrectStateTransition,
                                 OMX_TI_ErrorSevere,
                                 "Invalid State");
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}/*End of HandleCameraIdleState*/


/*-------------------------------------------------------------------*/
/**
  * HandleCameraExecutingState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraExecutingState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                  = NULL;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    CAMERA_PORT_TYPE *pPreviewPort              = NULL;
	OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
    int nBufferCount							= 0;
	int nCount;
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    hTunnelComponent = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].hTunnelComponent;
	nBufferCount = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].nBufferCount;
	
    if(pComponentPrivate->eCurrentState == OMX_StateIdle)
    {

#ifdef __PERF_INSTRUMENTATION__
        PERF_Boundary(pComponentPrivate->pPERFcomp,
                      PERF_BoundaryStart | PERF_BoundarySteadyState);
#endif
        /* Command to transition Executing state */
        pComponentPrivate->bStopping = OMX_FALSE;

        /* Start 2A algorithm */
#ifdef __CAMERA_2A__
        OMX_CAMERA_START_2A(pComponentPrivate);
#endif        
        /* Turn on streaming */
        OMX_CAMERA_TURN_ON_STREAMING(pComponentPrivate);
        
		/* Queue and query all buffers to Camera driver in video preview mode */
		nCount = 0;
		for(nCount = 0; nCount < nBufferCount; ++nCount)
		{		
		eError = QueryQueueBuffer_Video(pComponentPrivate, pPreviewPort->pCameraBuffer[nCount]);    
		OMX_CAMERA_IF_ERROR_BAIL(eError, "Error trying to queue al buffers!");
        }
        /* Change state to Executing */
        pComponentPrivate->eCurrentState = OMX_StateExecuting;

        /* Notify state change */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateIdle or OMX_StatePause to OMX_StateExecuting ");
    }
    else if(pComponentPrivate->eCurrentState == OMX_StatePause)
    {
        OMX_CAMERA_PRINTF_L3("Pause to Execting");
	
	#ifdef __CAMERA_2A__
	        /* Start 2A thread */
	        OMX_CAMERA_START_2A(pComponentPrivate);
	#endif
	
        /* Turn ON Streaming */
        OMX_CAMERA_TURN_ON_STREAMING(pComponentPrivate)
        
        pComponentPrivate->eCurrentState = OMX_StateExecuting;
        /* Notify state change */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateIdle or OMX_StatePause to OMX_StateExecuting ");
        
        /*Return all store buffers in link list*/
        OMX_CAMERA_RETURN_STORE_BUFFERS(pComponentPrivate);
        
    }
    else
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorIncorrectStateTransition,
                                 OMX_TI_ErrorMinor,
                                 "Invalid State\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;   
OMX_CAMERA_BAIL_CMD:
    return eError;

}


/*-------------------------------------------------------------------*/
/**
  * HandleCameraPauseState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE HandleCameraPauseState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    /*OMX_S32 nCount                              = 0;*/
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle                  = NULL;
    /*OMX_S32 nOutputBufferCount                  = 0;*/
    CAMERA_PORT_TYPE *pPreviewPort              = NULL;
    CAMERA_PORT_TYPE *pCapturePort              = NULL;
    
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    pCapturePort = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERFcomp,
                  PERF_BoundaryComplete | PERF_BoundarySteadyState);
#endif

    if(pComponentPrivate->eCurrentState == OMX_StateExecuting)
    {
        
#ifdef __CAMERA_2A__
        /* Stop 2A Thread */
        OMX_CAMERA_STOP_2A(pComponentPrivate);
#endif
        if(pComponentPrivate->bCapturing == OMX_TRUE)
        {
            /*Unblock Thread that is in FillThisBuffer waiting for Capture FillThisBuffer*/
            /* Send signal to unblock FillThisBuffer mutex */
            __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MFTB");
            pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
            __PERF_SYS_CMD(pComponentPrivate, Sending, "CSI:CFTB");  
            pthread_cond_signal(&(pComponentPrivate->sConditionFillThisBuffer));
            __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MFTB");
            pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
            __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MFTB");
        }
        
        /* Wait for all buffers be dequeue from driver */
        OMX_CAMERA_PRINTF_L3("Pausing");
        /*
        This logic will check if there are buffers queue to the Camera Driver
        If there are buffers inside, the mutex will wait for the signal that
        the driver queue is empty. This signal is generated on the DequeueBuffer_Still
        and DequeueBuffer_Video functions.
        */
        if(CameraQueueStatus(pComponentPrivate, pPreviewPort) ||
           CameraQueueStatus(pComponentPrivate, pCapturePort))
        {
            OMX_CAMERA_PRINTF_L4("Check Lock CameraDriverEmpty mutex\n");
            __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MCDE");  
            pthread_mutex_lock(&(pComponentPrivate->mCameraPipeEmptyMutex));
            OMX_CAMERA_PRINTF_L4("Wait for condition CameraDriverEmpty mutex\n");
            __PERF_SYS_CMD(pComponentPrivate, Sending, "CWA:CCDE");  
            pthread_cond_wait(&(pComponentPrivate->sConditionCameraPipeEmpty),
                              &(pComponentPrivate->mCameraPipeEmptyMutex));
            __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MCDE");  
            pthread_mutex_unlock(&(pComponentPrivate->mCameraPipeEmptyMutex));
            __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MCDE");
            OMX_CAMERA_PRINTF_L4("Unlock CameraPipeEmpty mutex\n");
        }
        OMX_CAMERA_FLUSH_V4L2_BUFFERS(pPreviewPort);
        OMX_CAMERA_FLUSH_V4L2_BUFFERS(pCapturePort);
        OMX_CAMERA_PRINTF_L2("No Buffers on Driver");
        
        /* Turn OFF streaming */
        OMX_CAMERA_TURN_OFF_STREAMING(pComponentPrivate);
     
        pComponentPrivate->eCurrentState = OMX_StatePause;
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateExecuting to OMX_StatePause\n");
    }
    else if(pComponentPrivate->eCurrentState == OMX_StateIdle)
    {
        pComponentPrivate->eCurrentState = OMX_StatePause;
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateIdle to OMX_StatePause\n");
    }
    else
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorIncorrectStateTransition,
                                 OMX_TI_ErrorMinor,
                                 "Invalid State\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;   
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * HandleCameraLoadedState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraLoadedState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 nIndex                          = 0;
    OMX_U32 nPortNumber                     = 0;
    OMX_U32 nStartPortNumber                = 0;
    OMX_ERRORTYPE eError                    = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle              = NULL;
    CAMERA_PORT_TYPE *pPortType  = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    nStartPortNumber = pComponentPrivate->sPortParam.nStartPortNumber;
    nPortNumber = nStartPortNumber + pComponentPrivate->sPortParam.nPorts;

#ifdef __CAMERA_2A__
   int (*Release2A) (Camera2AInterface** interface);
   Release2A = pComponentPrivate->Release2A;
#endif

#ifdef __PERF_INSTRUMENTATION__
    PERF_Boundary(pComponentPrivate->pPERFcomp,
                  PERF_BoundaryStart | PERF_BoundaryCleanup);
#endif
    if(pComponentPrivate->eCurrentState == OMX_StateIdle || 
       pComponentPrivate->eCurrentState == OMX_StateWaitForResources)
    {
        bKernelBufRequested = 0;
        
#ifdef __CAMERA_VSTAB__
        if(pComponentPrivate->sVSTAB.bStab == OMX_TRUE)
        {
            pComponentPrivate->sVSTAB.bStab = OMX_FALSE;
            //close vstab lib
            vStabLib_Close(&(pComponentPrivate->sVSTAB_Context));
            OMX_CAMERA_PRINTF_L7("vStabLib_Close: Closing VSTAB");
        }

#endif
        /* Make sure all enabled ports are unpopulated */
        for(nIndex = nStartPortNumber; nIndex < nPortNumber; nIndex++)
        {
            pPortType = &(pComponentPrivate->sCompPorts[nIndex]);
            pPortDef = &(pPortType->pPortDef);
            /* Check if the port is enabled */
            if(pPortDef->bEnabled == OMX_TRUE)
            {
                OMX_CAMERA_PRINTF_L3("Port Unpopulation %d", nIndex);
                eError = PortUnpopulation(pComponentPrivate,pPortType);
                OMX_CAMERA_IF_ERROR_BAIL(eError,"PortUnpopulation Error");
            }
        }/* For loop ends */

#ifdef __CAMERA_2A__
        if(pComponentPrivate->cameraAlg)
        {
            OMX_CAMERA_PRINTF_L6("Release2A...");
            __PERF_2A_CMD(pComponentPrivate, Sending, 0x5);
            OMX_CAMERA_CHECK_COND(Release2A(&(pComponentPrivate->cameraAlg)) != 0,
                                  eError,
                                  OMX_ErrorHardware,
                                  "Error in Release2A");
            __PERF_2A_CMD(pComponentPrivate, Received, 0x5);
            OMX_CAMERA_PRINTF_L6("Release2A Sucessful");
        
        
            /*dlclose on 2A lib */
            OMX_CAMERA_CHECK_COND(dlclose(pComponentPrivate->libHandle) < 0,
                                  eError,
                                  OMX_ErrorHardware,
                                  "2A lib didn't unload properly");
            OMX_CAMERA_PRINTF_L2("2A lib closed");
        }
#endif

        /* Close camera device properly */
        OMX_CAMERA_CHECK_COND(close(pComponentPrivate->nCameraDev) < 0,
                              eError,
                              OMX_ErrorHardware,
                              "Camera device didn't get close properly");
        OMX_CAMERA_PRINTF_L2("Camera device closed");
        
        /* Change state from Idle to Loaded */
        pComponentPrivate->eCurrentState = OMX_StateLoaded;
        /* Notify state change */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateIdle or OMX_StateWaitForResources to OMX_StateLoaded\n");
    }
    else 
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorIncorrectStateTransition,
                                 OMX_TI_ErrorMinor,
                                 "Invalid Transition to OMX_StateLoaded\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;  
    OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * HandleCameraWaitForResourcesState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraWaitForResourcersState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{

    OMX_ERRORTYPE eError        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle  = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;	

    if ( pComponentPrivate->eCurrentState == OMX_StateLoaded )
    {
        pComponentPrivate->eCurrentState = OMX_StateWaitForResources;
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandStateSet,
                                 pComponentPrivate->eCurrentState,
                                 "Transitioning from OMX_StateLoaded to OMX_StateWaitForResources\n");

    }
    else
    {
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorIncorrectStateTransition,
                                 OMX_TI_ErrorMinor,
                                 "Invalid Transition to OMX_StateWaitForResources\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * HandleCameraInvalidState()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraInvalidState (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle  = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    pComponentPrivate->eCurrentState = OMX_StateInvalid; 

    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventError,
                             OMX_ErrorInvalidState,
                             OMX_TI_ErrorSevere,
                             "Component in Invalid State\n");

    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                             OMX_EventCmdComplete,
                             OMX_CommandStateSet,
                             pComponentPrivate->eCurrentState,
                             "Transitioning to OMX_StateInvalid\n");

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * HandleCameraStateTransition()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraStateTransition (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_U32 nCommandData;
    OMX_COMPONENTTYPE *pHandle;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;

    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nCmdDataPipe[0], 
                               &nCommandData, 
                               sizeof(nCommandData)) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "error reading nCmdDataPipe");
        
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         OMX_CommandStateSet, nCommandData,
                         PERF_ModuleLLMM);
#endif
        
    if(pComponentPrivate->eCurrentState == nCommandData)
    {
       OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                OMX_EventError,
                                OMX_ErrorSameState,
                                OMX_TI_ErrorMinor,
                                "Same State transition\n");
       goto OMX_CAMERA_BAIL_CMD;
    }

    switch(nCommandData)
    {
        case OMX_StateIdle:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraIdleState");
            eError = HandleCameraIdleState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraIdleState error");
            break;
        case OMX_StateExecuting:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraExecutingState");
            eError = HandleCameraExecutingState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraExecutingState error");
            break;
        case OMX_StatePause:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraPauseState");
            eError = HandleCameraPauseState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraPauseState error");
            break;
        case OMX_StateLoaded:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraLoadedState");
            eError = HandleCameraLoadedState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraLoadedState error");
            break;
        case OMX_StateWaitForResources:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraWaitForResourcersState");
            eError = HandleCameraWaitForResourcersState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraWaitForResourcersState error");
            break;
        case OMX_StateInvalid:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraInvalidState");
            eError = HandleCameraInvalidState(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraInvalidState error");
            break;
        default:
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorInvalidState,
                                     OMX_TI_ErrorMinor,
                                     "Undefined State\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * HandleCameraFlushCmd()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraFlushCmd (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError        = OMX_ErrorUndefined;
    OMX_U32 nCommandData;
    OMX_COMPONENTTYPE *pHandle  = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *)pComponentPrivate->pHandle;

    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nCmdDataPipe[0],
                               &nCommandData, 
                               sizeof(nCommandData)) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "error reading nCmdDataPipe");
                          
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         OMX_CommandFlush, nCommandData,
                         PERF_ModuleLLMM);
#endif
             
    if((nCommandData == OMXCAM_PREVIEW_PORT) || (nCommandData == -1))
    {
        /* Flush Buffers from V4L2 */
        OMX_CAMERA_FLUSH_V4L2_BUFFERS(&(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]));
        
        /* Flush Preview Buffer Pipe */
        OMX_CAMERA_FLUSH_PIPE_BUFFERS(pComponentPrivate->nPreviewBufPipe[0],
                                      &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]));
        
        /* Return Buffers with bPreviewFillThisBuffer Done flag*/
        eError = ReturnBuffers(pComponentPrivate,
                               &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]));
        OMX_CAMERA_IF_ERROR_BAIL(eError,"Camera_ReturnBuffers Error");
        
        /* Send Event */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandFlush,
                                 OMXCAM_PREVIEW_PORT,
                                 "Flush done\n");
    }
    if((nCommandData == OMXCAM_CAPTURE_PORT) || (nCommandData == -1))
    {
        /* If Still Image Mode*/
        if(IsStillCaptureMode(pComponentPrivate))
        {
            /* Flush Buffers from V4L2 */
            OMX_CAMERA_FLUSH_V4L2_BUFFERS(&(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
            
            /* Flush Preview Buffer Pipe */
            OMX_CAMERA_FLUSH_PIPE_BUFFERS(pComponentPrivate->nStillBufPipe[0],
                                          &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
        }

        /* Return Buffers with bPreviewFillThisBuffer Done flag*/
        eError = ReturnBuffers(pComponentPrivate,
                               &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
        OMX_CAMERA_IF_ERROR_BAIL(eError,"Camera_ReturnBuffers Error");
        
        /* Send Event */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandFlush,
                                 OMXCAM_CAPTURE_PORT,
                                 "Flush done\n");
    }
    if((nCommandData == OMXCAM_THUMBNAIL_PORT) || (nCommandData == -1))
    {
        
        if(pComponentPrivate->bThumbnailFillThisBuffer == OMX_TRUE)
        {
            /* Return buffer */
            eError = ReturnBuffers(pComponentPrivate,
                                   &(pComponentPrivate->sCompPorts[OMXCAM_THUMBNAIL_PORT]));
            OMX_CAMERA_IF_ERROR_BAIL(eError,"Camera_ReturnBuffers Error");
            
            /* Clear bFillThisBufferThumbnail flag */
            pComponentPrivate->bThumbnailFillThisBuffer = OMX_FALSE;
        }
            
        /* Send Event */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete,
                                 OMX_CommandFlush,
                                 OMXCAM_THUMBNAIL_PORT,
                                 "Flush done\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
    OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * PortUnpopulation()
  *
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE PortUnpopulation (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                CAMERA_PORT_TYPE *pPortType)
{ 
    OMX_S32 nCount                                   = 0;
    OMX_ERRORTYPE eError                             = OMX_ErrorUndefined;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pPortDef = &(pPortType->pPortDef);
    
    if(pPortType->pPortDef.nPortIndex == OMXCAM_PREVIEW_PORT)
    {
        if(pPortType->hTunnelComponent == NULL)
        {
            /* send error event. Non tunneled preview port not supported */ 
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorUnsupportedIndex,
                                     OMX_TI_ErrorMinor,
                                     "Non tunneled preview port not supported\n");
        }
        else
        {
            if(pPortType->eSupplierSetting == OMX_BufferSupplyOutput)
            {
                /* send error event. tunneled supplier  preview port not supported */ 
                OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorUnsupportedIndex,
                                         OMX_TI_ErrorMinor,
                                         "Tunneled supplier preview port not supported\n");
            
            }
            else
            {
                /*Preview tunneled Non Supplier*/
                OMX_CAMERA_WAIT_PORT_UNPOPULATION(pComponentPrivate, pPortDef);
            }
        }
    }
    else if(pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT ||
            pPortType->pPortDef.nPortIndex == OMXCAM_THUMBNAIL_PORT)
    {
        if(pPortType->hTunnelComponent == NULL)
        {
            OMX_CAMERA_WAIT_PORT_UNPOPULATION(pComponentPrivate, pPortDef);
        }
        else
        {
            if(pPortType->eSupplierSetting == OMX_BufferSupplyOutput)
            {
                OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer;
                OMX_U32 nBufferCount = pPortType->nBufferCount;
                for(nCount = 0; nCount < nBufferCount; nCount++)
                {    
                    pPortType->nBufferCount--;
                    pCameraBuffer = pPortType->pCameraBuffer[nCount];

#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCameraBuffer->pBufHeader,pBuffer),
                                      0,
                                      PERF_ModuleLLMM);
#endif
                    eError = OMX_FreeBuffer(pPortType->hTunnelComponent, 
                                            pPortType->nTunnelPort, 
                                            pCameraBuffer->pBufHeader);
                    /*OMX_CAMERA_IF_ERROR_BAIL(eError, 
                                             "OMX_FreeBuffer Failed\n");*/
                    if(eError != OMX_ErrorNone)
                    {
                        OMX_CAMERA_PRINTF_ERROR ("Warning: FreeBuffer error %x on Tunneled Component %p - port %d",
                                                 eError,
                                                 pPortType->hTunnelComponent,
                                                 (int)pPortType->pPortDef.nPortIndex);
                    }
                    /*
                      If camera is working on image capture mode or thumbnail it
                      should free the buffer and buffer header. If the camera is working
                      on video mode it should just should release the buffer header
                      because the buffer is shared with preview port
                    */
                    if(IsStillCaptureMode(pComponentPrivate)||
                       pPortType->pPortDef.nPortIndex == OMXCAM_THUMBNAIL_PORT)
                    {
                        OMX_CAMERA_FREE(pCameraBuffer->pBufferStart);
                    }
                    OMX_CAMERA_FREE(pCameraBuffer);
                }
                pPortDef->bPopulated = OMX_FALSE;
            }
            else
            {
                if (pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT)
                {
                    /* send error event. tunneled non - supplier  capture port not supported */ 
                    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventError,
                                             OMX_ErrorPortsNotCompatible,
                                             OMX_TI_ErrorMinor,
                                             "Tunneled supplier preview port  not supported\n");
                }
                else
                {
                    /*Thumbnail tunneled Non Supplier. Wait to buffer gets un populated*/
                    pPortDef = &(pPortType->pPortDef);
                    OMX_CAMERA_WAIT_PORT_UNPOPULATION(pComponentPrivate, pPortDef);
                }
            }
        }
    }
    else
    {
        /* send error event. Index out of range */ 
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorUnsupportedIndex,
                                 OMX_TI_ErrorMinor,
                                 "No Camera Port with that nPortIndex\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * HandleCameraPortDisable()
  *
  * 
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraPortDisable (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 nIndex              = 0;
    OMX_U32 nPortNumber         = 0;
    OMX_U32 nStartPortNumber    = 0;
    OMX_U32 nPortIndex          = 0;
    OMX_ERRORTYPE eError        = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pPortType = NULL;
    
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    nStartPortNumber = pComponentPrivate->sPortParam.nStartPortNumber;
    nPortNumber = nStartPortNumber + pComponentPrivate->sPortParam.nPorts;
 
    /* read command parameter */
    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nCmdDataPipe[0],
                               &nPortIndex, 
                               sizeof(nPortIndex)) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "Error reading nCmdDataPipe");   

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         OMX_CommandPortDisable, nPortIndex,
                         PERF_ModuleLLMM);
#endif
    pPortType = &(pComponentPrivate->sCompPorts[nPortIndex]);
    
    if (nPortIndex == -1)
    {
        /* Disable all ports */
        for(nIndex = nStartPortNumber; nIndex < nPortNumber; nIndex++)
        {
            OMX_CAMERA_PRINTF_L3("Disable Port %d", nIndex);
            pPortType->pPortDef.bEnabled = OMX_FALSE;
            eError = PortUnpopulation(pComponentPrivate, pPortType);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "DisableCameraPort error");
            /* return cmdcomplete event if input unpopulated */ 
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete, 
                                     OMX_CommandPortDisable,
                                     nPortIndex,
                                     NULL);
        } /* For loop ends */
    }
    else
    {
        /* Disable the requested port */
        OMX_CAMERA_PRINTF_L3("Disable Port %d", nIndex);
        pPortType->pPortDef.bEnabled = OMX_FALSE;
        eError = PortUnpopulation(pComponentPrivate, pPortType);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "DisableCameraPort error");
        /* return cmdcomplete event if input unpopulated */ 
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete, 
                                 OMX_CommandPortDisable,
                                 nPortIndex,
                                 NULL);
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * PortPopulation()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE PortPopulation (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate, 
                                CAMERA_PORT_TYPE *pPortType)
{
    OMX_U32 nCount = 0;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pPreviewBuffer  = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCaptureBuffer = NULL;
    OMX_PTR pBufferStart = NULL;
    OMX_PTR pBufferAligned = NULL;
OMX_CAMERA_PRINTF_L3("PortPopulation");
        
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, pPortType, OMX_TRUE);
    pPortDef = &(pPortType->pPortDef);
    if(pPortType->pPortDef.nPortIndex ==  OMXCAM_PREVIEW_PORT)
    {
        if(pPortType->hTunnelComponent == NULL)
        {
            /* send error event. Non tunneled preview port not supported */
            /*Preview tunneled Non Supplier*/
            pPortType->pPortDef.bEnabled = OMX_FALSE;
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorUnsupportedIndex,
                                     OMX_TI_ErrorMajor,
                                     "Non tunneled preview port not supported\n");
        }
        else
        {
            if(pPortType->eSupplierSetting == OMX_BufferSupplyOutput)
            {
                /*Preview tunneled Non Supplier*/
                pPortType->pPortDef.bEnabled = OMX_FALSE;
                /* send error event. tunneled supplier  preview port not supported */ 
                OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                         OMX_EventError,
                                         OMX_ErrorPortsNotCompatible,
                                         OMX_TI_ErrorMajor,
                                         "Tunneled supplier preview port  not supported\n");
            }
            else
            {
                /*Preview tunneled Non Supplier*/
                OMX_CAMERA_WAIT_PORT_POPULATION(pComponentPrivate, pPortDef);
            }
        }
    }
    else if(pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT ||
            pPortType->pPortDef.nPortIndex == OMXCAM_THUMBNAIL_PORT)
    {
        if(pPortType->hTunnelComponent == NULL)
        {
            /* Capture Port. Non-Tunneled. */
            /* Wait for port to be populated */
            /*Preview tunneled Non Supplier*/
            OMX_CAMERA_PRINTF_L3("Non Tunneled Port. Waiting for buffer population");
            OMX_CAMERA_WAIT_PORT_POPULATION(pComponentPrivate, pPortDef);
        }
        else
        {
            if(pPortType->eSupplierSetting == OMX_BufferSupplyOutput)
            {
                OMX_CAMERA_PRINTF_L3("Tunnled Port. Buffer Owner. Buffer allocation");
                /*TODO: Capture  tunneled port - Supplier */
                for(nCount = 0; nCount < pPortDef->nBufferCountActual; nCount++)
                {
                    /* Allocate component buffer structure */
                    OMX_CAMERA_CALLOC(pPortType->pCameraBuffer[nCount],
                                      OMX_CAMERA_COMPONENT_BUFFER, 
                                      1, 
                                      OMX_CAMERA_COMPONENT_BUFFER, 
                                      eError);
                    if(IsStillCaptureMode(pComponentPrivate) ||
                       pPortType->pPortDef.nPortIndex == OMXCAM_THUMBNAIL_PORT ||
                       (pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT &&
                        pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_FALSE))
                    {
                        OMX_U32 nBufferLength = pPortDef->nBufferSize;
                        if (pPortDef->nBufferSize & 0xfff)
                        {
                            nBufferLength = (pPortDef->nBufferSize & 0xfffff000) + 0x1000;
                        }
                        
                        /* Create user space buffer - buffer is over allocated by 128 bytes */
                        OMX_CAMERA_CALLOC(pBufferStart,
                                          OMX_PTR,
                                          (nBufferLength + 0x20 + 256), 
                                          OMX_U8, 
                                          eError);
                            
                        /* Align buffer to 4K boundary */
			pBufferAligned = pBufferStart;
                        while ((((int)pBufferAligned) & 0xfff) != 0)
                        {
                            pBufferAligned++;
                        }
                        /* Buffer pointer shifted to avoid DSP cache issues */
                        //pBufferAligned += 128;

                        if (pBufferStart == NULL)
                        {
                           OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                                     OMX_ErrorHardware, 
                                                     "Can't Allocate Memory!");
                        }

                        /* Initialize component buffer structure */
                        pCaptureBuffer = pPortType->pCameraBuffer[nCount];
                        pCaptureBuffer->nIndex = nCount;
                        pCaptureBuffer->bSelfAllocated = OMX_TRUE;
                        pCaptureBuffer->pBufferStart  = pBufferStart;
OMX_CAMERA_PRINTF_L3("pCaptureBuffer: 0x%08x", pCaptureBuffer);
                        pCaptureBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
                        pCaptureBuffer->bFirstTime = OMX_TRUE;
                        /* Pass buffer pointers to tunnled component */
#ifdef __PERF_INSTRUMENTATION__
                        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                          PREF(pCaptureBuffer->pBufHeader,pBuffer),
                                          pPortDef->nBufferSize,
                                          PERF_ModuleLLMM);
#endif
                        eError = OMX_UseBuffer(pPortType->hTunnelComponent,
                                               &(pCaptureBuffer->pBufHeader),
                                               pPortType->nTunnelPort,
                                               NULL,
                                               nBufferLength,
                                               pBufferAligned);
                        OMX_CAMERA_IF_ERROR_BAIL(eError, 
                                                 "OMX_UseBuffer Failed\n");
                        
                        OMX_CAMERA_PRINTF_L3("UseBuffer send to Tunneled Component");

                        /* Initialize output port specific parameters from buffer header */
                        pCaptureBuffer->pBufHeader->pOutputPortPrivate = pCaptureBuffer;
                        pCaptureBuffer->pBufHeader->nOutputPortIndex = pPortType->pPortDef.nPortIndex;
                                
                        pPortType->nBufferCount++;
                    }
                    else
                    {
                        if(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE &&
                           pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bPopulated == OMX_TRUE)
                        {
                            pPreviewBuffer = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pCameraBuffer[nCount];
                            pCaptureBuffer = pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pCameraBuffer[nCount];
#ifdef __PERF_INSTRUMENTATION__
                            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                              PREF(pPreviewBuffer->pBufHeader,pBuffer),
                                              pPortDef->nBufferSize,
                                              PERF_ModuleLLMM);
#endif
                            eError = OMX_UseBuffer(pPortType->hTunnelComponent,
                                                   &(pCaptureBuffer->pBufHeader),
                                                   pPortType->nTunnelPort,
                                                   NULL,
                                                   pPortDef->nBufferSize,
                                                   pPreviewBuffer->pBufHeader->pBuffer);
                            OMX_CAMERA_IF_ERROR_BAIL(eError, 
                                                     "OMX_UseBuffer Failed\n");

                            /* Initialize output port specific parameters from buffer header */
							pCaptureBuffer->pBufHeader->pOutputPortPrivate = pCaptureBuffer;
                            pCaptureBuffer->pBufHeader->nOutputPortIndex = pPortType->pPortDef.nPortIndex;
                            pCaptureBuffer->nIndex = nCount;
                            pCaptureBuffer->bSelfAllocated = OMX_TRUE;
                            pCaptureBuffer->pBufferStart  = pBufferStart;
OMX_CAMERA_PRINTF_L3("pCaptureBuffer: 0x%08x", pCaptureBuffer);
                            pCaptureBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
                            pCaptureBuffer->bFirstTime = OMX_TRUE;
                            pPortType->nBufferCount++;
                        }
                        else
                        {
                            /* send error event. tunneled non - supplier  capture port not supported */
                            pPortDef->bEnabled = OMX_FALSE;
                            pPortDef->bPopulated = OMX_FALSE;
                            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                                     OMX_EventError,
                                                     OMX_ErrorPortUnpopulated,
                                                     OMX_TI_ErrorMajor,
                                                     "Preview Port need to be enable and populated first");
                            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                                      OMX_ErrorNone,
                                                      "Preview Port need to be enable and populated first");
                        }
                    }
                }
                pPortDef->bPopulated = OMX_TRUE;
            }
            else
            {
                OMX_CAMERA_PRINTF_L3("Tunnled Port. Tunneled Component Buffer Owner. Waiting Buffer Population");
                if (pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT &&
                    pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE)
                {
                    /* send error event. tunneled non - supplier  capture port not supported */
                    pPortDef->bEnabled = OMX_FALSE;
                    OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                             OMX_EventError,
                                             OMX_ErrorPortsNotCompatible,
                                             OMX_TI_ErrorMajor,
                                             "Tunneled supplier preview port  not supported\n");
                }
                else
                {
                    /*
                    Thumbnail tunneled Non Supplier or Capture Port when Preview port is
                    disable. Wait to buffer gets populated
                    */
                    OMX_CAMERA_WAIT_PORT_POPULATION(pComponentPrivate, pPortDef);
                }
            }
        }
    }
    else
    {
        /* send error event. Index out of range */
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventError,
                                 OMX_ErrorUnsupportedIndex,
                                 OMX_TI_ErrorMinor,
                                 "No Camera Port with that nPortIndex\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD: 
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * HandleCameraPortEnable()
  *
  * 
  * 
  *
  * @param hComp handle for this instance of the component
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraPortEnable (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 nIndex                          = 0;
    OMX_U32 nPortIndex                      = 0;
    OMX_U32 nPortNumber                     = 0;
    OMX_ERRORTYPE eError                    = OMX_ErrorUndefined;
    OMX_U32 nStartPortNumber                = 0;
    OMX_COMPONENTTYPE *pHandle              = NULL;    
    CAMERA_PORT_TYPE *pPortType             = NULL;
    
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    nStartPortNumber = pComponentPrivate->sPortParam.nStartPortNumber;
    nPortNumber = nStartPortNumber + pComponentPrivate->sPortParam.nPorts;

    /* read command parameter */
    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nCmdDataPipe[0],
                               &nPortIndex,
                               sizeof(nPortIndex)) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "error reading nCmdDataPipe");

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                         OMX_CommandPortEnable, nPortIndex,
                         PERF_ModuleLLMM);
#endif

    pPortType = &(pComponentPrivate->sCompPorts[nPortIndex]);
    
    if (nPortIndex == OMX_ALL) 
    {
        /* Enable all ports */
        for(nIndex = nStartPortNumber; nIndex < nPortNumber; nIndex++)
        {
            /* Enable Port */
            pPortType->pPortDef.bEnabled = OMX_TRUE;
            /* Populate Port */
            /* Check if the component is different from Loaded state */
            if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
               pComponentPrivate->eCurrentState != OMX_StateWaitForResources)
            {
                eError = PortPopulation(pComponentPrivate, pPortType);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "DisableCameraPort error");
            }
            /* Return cmdcomplete event if input unpopulated */ 
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                      OMX_EventCmdComplete, 
                                      OMX_CommandPortEnable,
                                      nPortIndex,
                                      NULL);
        } /* For loop ends */
    }
    else
    {
        /* Enable the requested port */
         pPortType->pPortDef.bEnabled = OMX_TRUE;
        /* Populate Port */
        if(pComponentPrivate->eCurrentState != OMX_StateLoaded &&
           pComponentPrivate->eCurrentState != OMX_StateWaitForResources)
        {
            eError = PortPopulation(pComponentPrivate, pPortType);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "DisableCameraPort error");
        }
        /* Return cmdcomplete event if input unpopulated */ 
        OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                 OMX_EventCmdComplete, 
                                 OMX_CommandPortEnable,
                                 nPortIndex,
                                 NULL);
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * HandleCameraCmd()
  *
  * Takes appropriate actions based on commands received from application.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the pipe creation fails
  *         OMX_ErrorNoMore                when component thread must be terminated.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleCameraCmd (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_COMMANDTYPE nCommand;
    OMX_ERRORTYPE eError        = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle  = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    
    /* Read command received from pipe */
    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nCmdPipe[0],
                               &nCommand,
                               sizeof(nCommand)) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "error reading nCmdPipe");
    switch(nCommand)
    {
        case OMX_CommandStateSet:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraStateTransition");
             OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Locked");
            OMX_CAMERA_PRINTF_L4("SetConfig: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
            OMX_CAMERA_PRINTF_L4("SetConfig: Locked");
            pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
            OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
            eError = HandleCameraStateTransition(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"HandleCameraStateTransition error");
            pthread_mutex_unlock(&(pComponentPrivate->mStateTransitionMutex));
            OMX_CAMERA_PRINTF_L4("StateTransitionMutex: Unlock");
            break;
        case OMX_CommandFlush:
            OMX_CAMERA_PRINTF_L3("Enters to HandleCameraFlushCmd");
            eError = HandleCameraFlushCmd(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"HandleCameraFlushCmd error");
            break;
       case OMX_CommandPortDisable:
            OMX_CAMERA_PRINTF_L3("OMX_CommandPortDisable");
            eError = HandleCameraPortDisable(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"HandleCameraPortDisable error");
            break;
        case OMX_CommandPortEnable:
            OMX_CAMERA_PRINTF_L3("HandleCameraPortEnable");
            eError = HandleCameraPortEnable(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "HandleCameraPortEnable error");
            break;
        case OMXCAM_CMD_STOPTHREAD:
#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedCommand(pComponentPrivate->pPERFcomp,
                             nCommand, 0,
                             PERF_ModuleLLMM);
#endif
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorNoMore,
                                      "Stoping Camera Thread!");
            break;
        default:
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventError,
                                     OMX_ErrorUnsupportedIndex,
                                     OMX_TI_ErrorMinor,
                                     "Unknown Command\n");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} 


/*-------------------------------------------------------------------*/
/**
  * HandleConfigCapturing()
  *
  * Takes appropriate actions based on commands received from application.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone                  success, ready to roll
  *         OMX_ErrorInsufficientResources if the pipe creation fails
  *         OMX_ErrorNoMore                when component thread must be terminated.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleConfigCapturing (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 nCount                              = 0;
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pPreviewPort              = NULL;
    CAMERA_PORT_TYPE *pCapturePort              = NULL;
    CAMERA_PORT_TYPE *pThumbnailPort            = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef      = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
    
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    pCapturePort = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);
    pThumbnailPort = &(pComponentPrivate->sCompPorts[OMXCAM_THUMBNAIL_PORT]);
    
    if(pComponentPrivate->bCapturing == OMX_FALSE)
    {
        

        if(pComponentPrivate->bAutoPauseAfterCapture == OMX_TRUE)
        {
            pComponentPrivate->eCurrentState = OMX_StatePause;
            OMX_CAMERA_EVENT_HANDLER(pComponentPrivate,
                                     OMX_EventCmdComplete,
                                     OMX_CommandStateSet,
                                     pComponentPrivate->eCurrentState,
                                     "Transitioning from OMX_StateIdle to OMX_StatePause\n");
        }
      
        if(IsStillCaptureMode(pComponentPrivate))
        {
            /*
            This logic will check if there are buffers queue to the Camera Driver
            If there are buffers inside, the mutex will wait for the signal that
            the driver queue is empty. This signal is generated on the DequeueBuffer_Still
            and DequeueBuffer_Video functions.
            */
            if(CameraQueueStatus_Still(pComponentPrivate, pCapturePort))
            {
                OMX_CAMERA_PRINTF_L4("Check Lock CameraPipeEmpty mutex");
                __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MCDE");
                pthread_mutex_lock(&(pComponentPrivate->mCameraPipeEmptyMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sending, "CWA:CCDE");
                OMX_CAMERA_PRINTF_L4("Wait for condition CameraPipeEmpty mutex");
                pthread_cond_wait(&(pComponentPrivate->sConditionCameraPipeEmpty),
                                  &(pComponentPrivate->mCameraPipeEmptyMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MCDE");
                pthread_mutex_unlock(&(pComponentPrivate->mCameraPipeEmptyMutex));
                OMX_CAMERA_PRINTF_L4("CameraPipeEmpty mutex unlock");
                __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MCDE");
            }
            
            OMX_CAMERA_PRINTF_L2("Flush!");
            OMX_CAMERA_FLUSH_V4L2_BUFFERS_STILL(pCapturePort);
            OMX_CAMERA_PRINTF_L2("No Buffers on Driver!");
            
            /*Stop 2A algorithm*/
#ifdef __CAMERA_2A__
            OMX_CAMERA_STOP_2A(pComponentPrivate);
#endif
            
#ifdef __CAMERA_IPP__
            if(pComponentPrivate->bIPP == OMX_TRUE)
            {
                /*DeInit Image Processing Pipe */
                eError = DeInitIPP(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error DeInitIPP");
            }
#endif
            
            /*Turn off Streaming mode */
            OMX_CAMERA_TURN_OFF_STREAMING(pComponentPrivate);
            
            /* Workaround: Copy Back  */
            for(nCount = 0; nCount < pCapturePort->nBufferCount; ++nCount)
            {
                OMX_CAMERA_MEMCPY(pCapturePort->pCameraBuffer[nCount],
                                  &(pComponentPrivate->StillBuffer[nCount]),
                                  sizeof(OMX_CAMERA_COMPONENT_BUFFER),
                                  eError);
                pCameraBuffer = pCapturePort->pCameraBuffer[nCount];
                pCameraBuffer->bFirstTime = OMX_TRUE;
            }
			            
            /*Change resolution to preview*/
            bKernelBufRequested = 0;
			OMX_CAMERA_PRINTF_L3("Return to Preview Mode");
            eError = InitCameraPreview(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
            
            if(pComponentPrivate->bAutoPauseAfterCapture == OMX_FALSE)
            {
			                /* Start 2A algorithm */
			#ifdef __CAMERA_2A__
			                OMX_CAMERA_START_2A(pComponentPrivate);
			#endif
				  
                /*Turn on streaming mode */
                OMX_CAMERA_TURN_ON_STREAMING(pComponentPrivate);
                
                /*Return all store buffers in link list*/
                OMX_CAMERA_RETURN_STORE_BUFFERS(pComponentPrivate);
                
            }
        }
        /* Send Thumbnail frame to Client or Tunneled Component  */
        else if(pThumbnailPort->pPortDef.bEnabled == OMX_TRUE &&
                pComponentPrivate->bThumbnailReady == OMX_TRUE)
        {
            if(pThumbnailPort->hTunnelComponent != NULL &&
               pThumbnailPort->eSupplierSetting == OMX_BufferSupplyOutput)
            {
				OMX_CAMERA_PRINTF_L3("Send to Tunneled Component Thumbnail Buffer");
				OMX_CAMERA_PRINTF_L3("pCameraBuffer=%p pBufHeader=%p ",pThumbnailPort->pCameraBuffer[0],pThumbnailPort->pCameraBuffer[0]->pBufHeader);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,pBuffer),
                                  PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,nFilledLen),
                                  PERF_ModuleLLMM);
#endif
                eError = OMX_EmptyThisBuffer(pThumbnailPort->hTunnelComponent,
                                             pThumbnailPort->pCameraBuffer[0]->pBufHeader);                                             
#ifdef __PERF_INSTRUMENTATION__
                PERF_SentFrame(pComponentPrivate->pPERFcomp,
                               PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,pBuffer),
                               PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,nFilledLen),
                               PERF_ModuleLLMM);
#endif
                OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
OMX_CAMERA_PRINTF_L3("pThumbnailPort->pCameraBuffer[0]: 0x%08x\n", pThumbnailPort->pCameraBuffer[0]);
                pThumbnailPort->pCameraBuffer[0]->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
            }
            else if (pComponentPrivate->bThumbnailFillThisBuffer == OMX_TRUE)
            {
                pThumbnailPort->pCameraBuffer[0]->eBufferOwner = BUFFER_WITH_CLIENT;
				OMX_CAMERA_PRINTF_L3("Send to IL Client Thumbnail Buffer");
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,pBuffer),
                                  PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,nFilledLen),
                                  PERF_ModuleLLMM);
#endif
                pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle, 
                                                                pComponentPrivate->pHandle->pApplicationPrivate, 
                                                                pThumbnailPort->pCameraBuffer[0]->pBufHeader);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SentFrame(pComponentPrivate->pPERFcomp,
                               PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,pBuffer),
                               PREF(pThumbnailPort->pCameraBuffer[0]->pBufHeader,nFilledLen),
                               PERF_ModuleLLMM);
#endif
                pComponentPrivate->bThumbnailFillThisBuffer = OMX_FALSE;
            }
        }

       
    }
    else
    {

        	
        if(IsStillCaptureMode(pComponentPrivate))
        {
												/*Reset Flags to count buffers dequeue and fillthisbuffer received*/
            pComponentPrivate->nStillBuffersDequeue = 0;
            pComponentPrivate->nStillFillThisBufferRcv = 0;
            
            /*Stop 2A algorithm*/
#ifdef __CAMERA_2A__
	    if( pComponentPrivate->bAutoFocus == OMX_TRUE )
	    {
		pComponentPrivate->cameraAlg->StartAF(pComponentPrivate->cameraAlg->pPrivateHandle);
            }
            OMX_CAMERA_STOP_2A(pComponentPrivate);
#endif
            
            /*
            This logic will check if buffers are in the queue of the Camera Driver
            If there are buffers inside, the mutex will wait for the signal that
            the driver queue is empty. This signal is generated on the QueueBuffer_Still
            and QueueBuffer_Video functions.
            */
            OMX_CAMERA_PRINTF_L3("Enabling Capturing Still Image");
            if(CameraQueueStatus(pComponentPrivate, pPreviewPort))
            {
                OMX_CAMERA_PRINTF_L4("Check Lock CameraDriverEmpty mutex\n");
                 __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MCDE");
                pthread_mutex_lock(&(pComponentPrivate->mCameraPipeEmptyMutex));
                OMX_CAMERA_PRINTF_L4("Wait for condition CameraDriverEmpty mutex\n");
                __PERF_SYS_CMD(pComponentPrivate, Sending, "CWA:CCDE"); 
                pthread_cond_wait(&(pComponentPrivate->sConditionCameraPipeEmpty),
                                  &(pComponentPrivate->mCameraPipeEmptyMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MCDE");
                pthread_mutex_unlock(&(pComponentPrivate->mCameraPipeEmptyMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MCDE");
                OMX_CAMERA_PRINTF_L4("Unlock CameraDriverEmpty mutex\n");
            }
            
            OMX_CAMERA_FLUSH_V4L2_BUFFERS(pPreviewPort);
            OMX_CAMERA_PRINTF_L2("No Buffers on Driver");
            /* Turn 0FF Streaming Mode */
            OMX_CAMERA_TURN_OFF_STREAMING(pComponentPrivate);
            
            /* Setup Still Image Resolution and queue buffers*/
			
            eError = InitStillCapture(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error InitStillCapture");
	    
            /* Start 2A algorithm */
#ifdef __CAMERA_2A__
			            OMX_CAMERA_START_2A(pComponentPrivate);

#endif      
      
            /* Turn ON Streaming Mode */
            OMX_CAMERA_TURN_ON_STREAMING(pComponentPrivate);
            
#ifdef __CAMERA_IPP__
            if(pComponentPrivate->bIPP == OMX_TRUE)
            {
                /* Initialize the IPP */
                eError = InitIPP(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error InitIPP");
            
                /* Populate IPP args */
                eError = PopulateArgsIPP(pComponentPrivate);
                OMX_CAMERA_IF_ERROR_BAIL(eError, "Error PopulateArgsIPP");
            }
#endif
            
            /* Writing buffers to still buffer pipe */
            /* Check if capture port is enabled, tunneled and supplier */
            pPortDef = &(pCapturePort->pPortDef);
            if((pPortDef->bEnabled == OMX_TRUE) && 
               (pCapturePort->hTunnelComponent != NULL) &&
               (pCapturePort->eSupplierSetting == OMX_BufferSupplyOutput))
            {
                
                OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
                pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
                OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
                for(nCount = 0; nCount < pPortDef->nBufferCountActual;nCount++)
                {
                    OMX_CAMERA_PRINTF_L3("Sending Still Buffer throug pipe [%d] : %p",
									pCapturePort->pCameraBuffer[nCount]->nIndex,
                                       pCapturePort->pCameraBuffer[nCount]);
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCapturePort->pCameraBuffer[nCount]->pBufHeader,pBuffer),
                                      PREF(pCapturePort->pCameraBuffer[nCount]->pBufHeader,nFilledLen),
                                      PERF_ModuleHardware);
#endif
                    OMX_CAMERA_CHECK_COND(write(pComponentPrivate->nStillBufPipe[1],
                                                &(pCapturePort->pCameraBuffer[nCount]),
                                                sizeof(OMX_CAMERA_COMPONENT_BUFFER*)) != sizeof(OMX_CAMERA_COMPONENT_BUFFER*), 
                                          eError,
                                          OMX_ErrorHardware,
                                          "Error writing to  capture buffer pipe ");
                    pCapturePort->pCameraBuffer[nCount]->bPipe = OMX_TRUE;
                }
                pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
                OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
            }
            else
            {
                /*TODO: Write to pipe Store Buffers on link list*/
            }
        }
        else
        {
            /*
            Reset intial count for buffer send when Capture Port is Tunneled and
            is buffer owner.
            */
            pComponentPrivate->nInitialTunneledCaptureBuffer = 0;
            
            if (pThumbnailPort->pPortDef.bEnabled == OMX_TRUE){
                /*Set Thumbnail Port flag to take first frame of next coming frame*/
                pComponentPrivate->bThumbnailReady = OMX_FALSE;
            }
        }
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * HandleStillDataBuf()
  *
  * Fill capture port buffer with captured data and send the buffer out.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandleStillDataBuf (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError                          = OMX_ErrorUndefined;
    OMX_HANDLETYPE hTunnelComponent               = NULL;
    OMX_BUFFERHEADERTYPE *pBufHeader              = NULL;
    OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer    = NULL;
    CAMERA_PORT_TYPE* pPortType                   = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pPortType = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);
    
    OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
    
    /* Read from capture buffer pipe */
    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nStillBufPipe[0],
                               &pCameraBuffer,
                               sizeof(pCameraBuffer)) == -1, 
                          eError, 
                          OMX_ErrorHardware,
                          "Error reading to nFilledBufpipe ");
#ifdef __PERF_INSTRUMENTATION__
                    PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                                      pCameraBuffer,
                                      sizeof(pCameraBuffer),
                                      PERF_ModuleHardware);
#endif
    pCameraBuffer = &(pComponentPrivate->StillBuffer[pCameraBuffer->nIndex]);
    pCameraBuffer->bPipe = OMX_FALSE;
    pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
    
    if(pCameraBuffer->nIndex < pComponentPrivate->nBuffersSupportedByDriver)
    {
        eError = DequeueBuffer_Driver(pComponentPrivate, pCameraBuffer, OMXCAM_CAPTURE_PORT);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "eError Dequeue Buffer from Driver");
    
#ifdef __CAMERA_IPP__
        if(pComponentPrivate->bIPP == OMX_TRUE)
        {
            eError = ProcessBufferIPP(pComponentPrivate, pCameraBuffer);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error DeInitIPP");
        }
#endif
    }
    else
    {
        /*
        If the buffer was not succesfully asigned by the driver
        through the request function we send and
        empty buffer. If we try to dequeue a buffer that was not
        requested and query to the driver will get and error from
        the driver.
        TODO:Check if this implementation is the best to handle this
        situation
        */
        OMX_CAMERA_PRINTF_ERROR("Sending back emtpy buffer %d",
                                (int)pCameraBuffer->nIndex);
        pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT_OUT_PIPE;
        pCameraBuffer->pBufHeader->nFilledLen = 0;
    }
    
    if(pCameraBuffer->bFirstTime)
    {
        pCameraBuffer->bFirstTime = OMX_FALSE;
    }
   
    /*
    If Camera Driver Queue is empty it will send a signal. This is used for
    changing from preview mode to capture mode, or going to pause state.
    */
    if(!CameraQueueStatus_Still(pComponentPrivate, pPortType))
    {
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Check Lock");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MCDE");
        pthread_mutex_lock(&(pComponentPrivate->mCameraPipeEmptyMutex));
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Lock");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "CSI:CCDE");
        pthread_cond_signal(&(pComponentPrivate->sConditionCameraPipeEmpty));
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MCDE");
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Signal Condition");
        pthread_mutex_unlock(&(pComponentPrivate->mCameraPipeEmptyMutex));
        __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MCDE");
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: UnLock");
    }
    
    pBufHeader = pCameraBuffer->pBufHeader;
    hTunnelComponent = pComponentPrivate->sCompPorts[pBufHeader->nOutputPortIndex].hTunnelComponent;
    /* If tunneling, pass the buffer to tunneled component */
    if(hTunnelComponent != NULL)
    {
        if(!pComponentPrivate->bStopping)
        {
			OMX_CAMERA_PRINTF_L3("Send to Tunneled Component Capture Buffer [%d]",pCameraBuffer->nIndex);
#ifdef __PERF_INSTRUMENTATION__
            PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                              PREF(pBufHeader,pBuffer),
                              PREF(pBufHeader,nFilledLen),
                              PERF_ModuleLLMM);
#endif
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
            pCameraBuffer->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
            eError = OMX_EmptyThisBuffer(hTunnelComponent, pBufHeader);
#ifdef __PERF_INSTRUMENTATION__
            PERF_SentFrame(pComponentPrivate->pPERFcomp,
                           PREF(pBufHeader,pBuffer),
                           PREF(pBufHeader,nFilledLen),
                           PERF_ModuleLLMM);
#endif
            OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
            sched_yield();
        }
    }
    else
    {
        /*
        Notify application about completeion of buffer processing & return
        the free buffer
        */
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CLIENT;

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          PREF(pBufHeader,pBuffer),
                          PREF(pBufHeader,nFilledLen),
                          PERF_ModuleHLMM);
#endif
        pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle, 
                                                        pComponentPrivate->pHandle->pApplicationPrivate, 
                                                        pBufHeader);
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       PREF(pBufHeader,pBuffer),
                       PREF(pBufHeader,nFilledLen),
                       PERF_ModuleHLMM);
#endif
		    OMX_CAMERA_PRINTF_L3("Send to IL Client Capture Buffer [%d]", pCameraBuffer->nIndex);
    }
    /*
    If the Camera had finished to send all the Image Capture Buffers will
    reset the bCapturing flag. If bOneShot is enable it will only return one buffer.
    If bFrameLimited is enable it will reset when the nFrameLimit is reach.
    */
    (pComponentPrivate->nStillBuffersDequeue)++;
    if((pComponentPrivate->sSensorMode.bOneShot &&
       pComponentPrivate->nStillBuffersDequeue == 1) ||
       (pComponentPrivate->sCaptureMode.bFrameLimited &&
       pComponentPrivate->nStillBuffersDequeue == pComponentPrivate->sCaptureMode.nFrameLimit))
    {
        /* Mutex for bCapturing */
        OMX_CAMERA_PRINTF_L3("Turn off Capturing\n");
        OMX_CAMERA_PRINTF_L4("SetConfig: Check Lock");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MVB_"); 
        pthread_mutex_lock(&(pComponentPrivate->mVideoBufMutex));
        __PERF_SYS_CMD(pComponentPrivate, Sent, "MLK:MVB_");
        OMX_CAMERA_PRINTF_L4("SetConfig: Locked");
        if(pComponentPrivate->bCapturing == OMX_TRUE)
        {
												printf("Accessing bCapturing:OMX_Camera_Utils.c-HandleStillDataBuf()\n");
            pComponentPrivate->bCapturing = OMX_FALSE;
            /* IMPORTANT!!!
              Need to flush buffers from pipe.
              HandleConfigCapturing to be executed on Component thread (Commonly is
              executed on application thread), and the only thread
              that reads the pipe is the same Component thread so it will hang
              waiting for the pipe to be empty.
            */
            OMX_CAMERA_FLUSH_PIPE_BUFFERS_STILL(pComponentPrivate->nStillBufPipe[0],
                                                &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
            eError = HandleConfigCapturing(pComponentPrivate);
            OMX_CAMERA_IF_ERROR_BAIL(eError, "Error Turning off bCapturing");
        }
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MVB_");  
        pthread_mutex_unlock(&(pComponentPrivate->mVideoBufMutex));
        __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MVB_");  
        OMX_CAMERA_PRINTF_L4("SetConfig: Unlock");
    }
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
   return eError;
} /* end of HandleStillDataBuf */


/*-------------------------------------------------------------------*/
/**
  * HandlePreviewDataBuf()
  *
  * Notify application that buffer it sent is processed.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE HandlePreviewDataBuf(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
    
    OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");

    OMX_CAMERA_PRINTF_L3("Enters to HandlePreviewDataBuf");
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Read preview buffer */
    OMX_CAMERA_CHECK_COND(read(pComponentPrivate->nPreviewBufPipe[0],
                               &pCameraBuffer,
                               sizeof(pCameraBuffer)) == -1,
                          eError,
                          OMX_ErrorHardware,
                          "Error reading nPreviewBufPipe ");

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                       PREF(PREF(pCameraBuffer, pBufHeader), pBuffer),
                       PREF(PREF(pCameraBuffer, pBufHeader), nFilledLen),
                       PERF_ModuleLLMM);
#endif
    pCameraBuffer->bPipe = OMX_FALSE;
    pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");

    eError = DequeueBuffer_Video(pComponentPrivate, pCameraBuffer);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at DequeueBuffer");
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of HandlePreviewDataBuf */

/*-------------------------------------------------------------------*/
/**
  * QueueBuffer_Video()
  *
  * Will Queue the buffer to the driver. It will validate if the system 
  * and the buffer can be Queue to the driver.
  *
  * @param pComponentPrivate component private data structure.
  * @param pCameraBuffer private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE QueueBuffer_Video (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                 OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer)
{
    OMX_ERRORTYPE eError  = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pCapturePort                  = NULL;
    CAMERA_PORT_TYPE *pPreviewPort					= NULL;
	
    OMX_CAMERA_PRINTF_L3("Enters to QueueBuffer_Video");
    pCapturePort = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
	
    if(pComponentPrivate->eCurrentState == OMX_StatePause ||
       (IsStillCaptureMode(pComponentPrivate) &&
        pComponentPrivate->bCapturing == OMX_TRUE))
    {
        OMX_CAMERA_PRINTF_L3("Queue: Component in Pause / Or Taking Still Image - Storing Buffer %d",
                           pCameraBuffer->nIndex);
        OMX_CAMERA_LINK_LIST_ADD_NODE(pComponentPrivate, pCameraBuffer);
#ifdef __CAMERA_2A__	
	if(pComponentPrivate->sLinkList.nElements  == pPreviewPort->pPortDef.nBufferCountActual 
	   && pComponentPrivate->bLinkedListFull != OMX_TRUE)
	{
		OMX_CAMERA_PRINTF_L4("Check Lock of FullLinkedList Mutex");
		pthread_mutex_lock(&(pComponentPrivate->mFullLinkedListMutex));	
		OMX_CAMERA_PRINTF_L4("Lock of FullLinkedList Mutex");	
		OMX_CAMERA_PRINTF_L4("Sending signal of FullLinkedList \n");
		pthread_cond_signal(&(pComponentPrivate->sConditionFullLinkedList));
		pComponentPrivate->bLinkedListFull = OMX_TRUE;	
		pthread_mutex_unlock(&(pComponentPrivate->mFullLinkedListMutex));
		OMX_CAMERA_PRINTF_L4("Un Lock of FullLinkedList Mutex");
	}	
#endif
        pComponentPrivate->bExit = OMX_TRUE;	
        eError = OMX_ErrorNone;
        goto OMX_CAMERA_BAIL_CMD;
    }
    else if(pComponentPrivate->bCapturing == OMX_TRUE &&
            pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE)
    {	
		
        if (pCameraBuffer->bCapturing == OMX_TRUE &&
            pCapturePort->hTunnelComponent != NULL &&
            pCapturePort->eSupplierSetting == OMX_BufferSupplyOutput &&
            pComponentPrivate->nInitialTunneledCaptureBuffer < pCapturePort->nBufferCount)
        {
			pComponentPrivate->nInitialTunneledCaptureBuffer++;
            pCameraBuffer->bCaptureFillThisBuffer = OMX_TRUE;
        }
        else if(pCameraBuffer->bCapturing == OMX_TRUE)
        {
            OMX_CAMERA_PRINTF_L3("wait for capture fill this buffer");
            if(pCameraBuffer->bCaptureFillThisBuffer == OMX_FALSE)
            {
                __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MFTB");
                pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sending, "CWA:CFTB");
                pthread_cond_wait(&(pComponentPrivate->sConditionFillThisBuffer),
                                  &(pComponentPrivate->mFillThisBufferMutex));
                if(pCameraBuffer->bCaptureFillThisBuffer == OMX_TRUE)
                
                {
                    OMX_CAMERA_PRINTF_L3("Correct Capture FillThisBuffer");
                }
                else if(pComponentPrivate->bCapturing == OMX_FALSE)
                {
                   OMX_CAMERA_PRINTF_L3("bCapturing Off. Stop Polling Flags");
                }
                else if(pComponentPrivate->eDesiredState == OMX_StatePause ||
                        pComponentPrivate->eDesiredState == OMX_StateIdle)
                {
                    OMX_CAMERA_PRINTF_L3("Component has recieve Pause or Stop Command\n");
                    if(pComponentPrivate->eDesiredState == OMX_StatePause)
                    {
                        OMX_CAMERA_PRINTF_L3("Component Passing to Pause - Storing Buffer %d",
                                           pCameraBuffer->nIndex);
                        OMX_CAMERA_LINK_LIST_ADD_NODE(pComponentPrivate,
                                                      pCameraBuffer);
                    }
                    else
                    {
                        OMX_CAMERA_PRINTF_L3("Component Passing to Idle - Ignore Buffer %d",
                                          pCameraBuffer->nIndex);
                    }
                    /* Function executed properly */
                    __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MFTB");
                    pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
                    __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MFTB");  
                    pComponentPrivate->bExit = OMX_TRUE;
                    eError = OMX_ErrorNone;
                    goto OMX_CAMERA_BAIL_CMD;
                }
                else
                {
                    /*Buffer send by tunneled component or client is out order, or 
                    the Camera is turning off the bCapturing */
                    OMX_CAMERA_PRINTF_L3("Warning - Capture FillThisBuffer is Out of Sync");
                    __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MFTB");
                    pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
                    __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MFTB");
                    pComponentPrivate->bExit = OMX_TRUE;
                    eError = OMX_ErrorNone;
                    goto OMX_CAMERA_BAIL_CMD;
                }
                __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MFTB");  
                pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
                __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MFTB");  
            }
        }
        OMX_CAMERA_PRINTF_L3("Shared buffer Ready to Queue - pBufferHeader: %p",
                          pCameraBuffer->pBufHeader);
    }

    if(!pCameraBuffer->bFirstTime)
    {
        
        OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
        pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
        OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");
        
        /* Queue empty preview buffer to camera driver */
        struct v4l2_buffer cfilledbuffer;
        cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
        cfilledbuffer.index  = pCameraBuffer->nIndex;
		cfilledbuffer.flags = 0;
		cfilledbuffer.m.userptr = (unsigned long)(pCameraBuffer->pBufHeader->pBuffer);			
		
		if(pComponentPrivate->sVSTAB.bStab == OMX_TRUE)
		{
			cfilledbuffer.length = (2* pComponentPrivate->sVSTAB_Params.myInFrame.x * pComponentPrivate->sVSTAB_Params.myInFrame.y);			
		}
		
		else
		{			
			cfilledbuffer.length = (2*pPreviewPort->pPortDef.format.video.nFrameWidth * pPreviewPort->pPortDef.format.video.nFrameHeight);
			
		}				       
	
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          cfilledbuffer.m.userptr,
                          cfilledbuffer.length,
                          PERF_ModuleHardware);
#endif
					
        OMX_CAMERA_PRINTF_L2("[VIDIOC_QBUF][%p][%d]",
                             cfilledbuffer.m.userptr,
                             cfilledbuffer.index);
			     
	eError = ioctlCamera(pComponentPrivate->nCameraDev,
                             VIDIOC_QUERYBUF,
                             &cfilledbuffer,
                             pComponentPrivate);
        OMX_CAMERA_CHECK_COND(eError == -1,
                              eError,
                              OMX_ErrorHardware,
                              "VIDIOC_QUERYBUF");
			      			     
        if( ioctlCamera(pComponentPrivate->nCameraDev,
                        VIDIOC_QBUF,
                        &cfilledbuffer,
                        pComponentPrivate) == -1) 
        {
            OMX_CAMERA_PRINTF_ERROR("VIDIOC_QBUF %d\n", (int)pCameraBuffer->nIndex);
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorHardware,
                                      "Error at VIDIOC_QBUF");
        }
			
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CAMERA_DRIVER;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       cfilledbuffer.m.userptr,
                       cfilledbuffer.length,
                       PERF_ModuleHardware);
#endif
        
        pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
        OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
        
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of QueueBuffer_Video */

/*-------------------------------------------------------------------*/
/**
  * QueryQueueBuffer_Video()
  *
  * Will Query and Queue a  preview buffer to the driver.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/

inline OMX_ERRORTYPE QueryQueueBuffer_Video (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                 OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer)
{

    OMX_ERRORTYPE eError  = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pPreviewPort	= &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);

	OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");
        
        /* Queue empty preview buffer to camera driver */
        struct v4l2_buffer cfilledbuffer;
        cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
        cfilledbuffer.index  = pCameraBuffer->nIndex;
				
		if(pComponentPrivate->sVSTAB.bStab == OMX_TRUE)
		{
			cfilledbuffer.length = (2* pComponentPrivate->sVSTAB_Params.myInFrame.x * pComponentPrivate->sVSTAB_Params.myInFrame.y);			
		}
		
		else
		{			
			cfilledbuffer.length = (2*pPreviewPort->pPortDef.format.video.nFrameWidth * pPreviewPort->pPortDef.format.video.nFrameHeight);
			
		}				       
	
	
		eError = ioctlCamera(pComponentPrivate->nCameraDev,			
                             VIDIOC_QUERYBUF, 	
                             &cfilledbuffer,	
                             pComponentPrivate);						
        OMX_CAMERA_CHECK_COND(eError == -1, 								
                              eError, 										
                              OMX_ErrorHardware, 							
                              "VIDIOC_QUERYBUF");							
		
		cfilledbuffer.flags = 0;
		cfilledbuffer.m.userptr = (unsigned long)(pCameraBuffer->pBufHeader->pBuffer);	

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          cfilledbuffer.m.userptr,
                          cfilledbuffer.length,
                          PERF_ModuleHardware);
#endif
			
	    OMX_CAMERA_PRINTF_L2("[VIDIOC_QBUF][%p][%d]",
                             cfilledbuffer.m.userptr,
                             cfilledbuffer.index);
        if( ioctlCamera(pComponentPrivate->nCameraDev,
                        VIDIOC_QBUF,
                        &cfilledbuffer,
                        pComponentPrivate) == -1) 
        {
            OMX_CAMERA_PRINTF_ERROR("VIDIOC_QBUF %d\n", (int)pCameraBuffer->nIndex);
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorHardware,
                                      "Error at VIDIOC_QBUF");
        }
			
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CAMERA_DRIVER;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       cfilledbuffer.m.userptr,
                       cfilledbuffer.length,
                       PERF_ModuleHardware);
#endif
        
        pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
        OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
        
        
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * QueueBuffer_Still()
  *
  * Will Queue the buffer to the driver. It will validate if the system 
  * and the buffer can be Queue to the driver.
  *
  * @param pComponentPrivate component private data structure.
  * @param pCameraBuffer private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE QueueBuffer_Still (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                 OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer)
{
    OMX_ERRORTYPE eError  = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pCapturePort = NULL;
    
    OMX_CAMERA_PRINTF_L3("Enters to QueueBuffer_Still");
    pCapturePort = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]);
    
    OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");
    /* Queue empty preview buffer to camera driver */
    if(pCameraBuffer->bFirstTime == OMX_FALSE &&
       pCameraBuffer->nIndex < pComponentPrivate->nBuffersSupportedByDriver)
    {
        struct v4l2_buffer cfilledbuffer;
        cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
        cfilledbuffer.index  = pCameraBuffer->nIndex;
        cfilledbuffer.length = pCameraBuffer->pBufHeader->nAllocLen;
        cfilledbuffer.m.userptr = (unsigned int)(pCameraBuffer->pBufHeader->pBuffer);
        cfilledbuffer.flags = 0;
    
#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          cfilledbuffer.m.userptr,
                          cfilledbuffer.length,
                          PERF_ModuleHardware);
#endif
        OMX_CAMERA_PRINTF_L2("[VIDIOC_QBUF][%p][%d]",
                             cfilledbuffer.m.userptr,
                             cfilledbuffer.index);
		
        while(ioctlCamera(pComponentPrivate->nCameraDev,
                          VIDIOC_QBUF,
                          &cfilledbuffer,
                          pComponentPrivate) < 0) 
        {
            OMX_CAMERA_PRINTF_L2("[VIDIOC_QBUF][%p][%d]\n",
                                 pCameraBuffer->pBufHeader->pBuffer,
                                 pCameraBuffer->nIndex);
            perror("STILL VIDIOC_QBUF:");
            /*OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorHardware,
                                      "Error at VIDIOC_QBUF");*/
            __PERF_V4L2_CMD(pComponentPrivate, Sending, "QBUF");
        }
		
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CAMERA_DRIVER;
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       cfilledbuffer.m.userptr,
                       cfilledbuffer.length,
                       PERF_ModuleHardware);
#endif
    }
    
    pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
/*OMX_CAMERA_BAIL_CMD:*/
    return eError;
} /* End of QueueBuffer */

/*-------------------------------------------------------------------*/
/**
  * DequeueBuffer()
  *
  * Will Dequeue Buffer and do proper actions depending of system 
  * status.
  * @param pComponentPrivate component private data structure.
  * @param pCameraBuffer buffer private data structure
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE DequeueBuffer_Video (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                   OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer)
{
    OMX_ERRORTYPE eError                        = OMX_ErrorUndefined;
    OMX_HANDLETYPE hTunnelComponent             = NULL;
    CAMERA_PORT_TYPE *pThumbnailPort            = NULL;
    OMX_BUFFERHEADERTYPE *pBufHeader            = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef      = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCaptureBuffer = NULL;
				CAMERA_PORT_TYPE *pPreviewPort              = NULL;
    long nCurrentMediaTime                      = 0;
				OMX_U32 nThumbnailFrameSize                 = 0;
			
    OMX_TIME_CONFIG_TIMESTAMPTYPE sClockStartTime;
    OMX_TIME_CONFIG_TIMESTAMPTYPE sCurrentMediaTime;

    OMX_CAMERA_PRINTF_L3("Enters to DequeueBuffer");
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);    
    pPortDef = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef);
    pThumbnailPort = &(pComponentPrivate->sCompPorts[OMXCAM_THUMBNAIL_PORT]);
    pBufHeader = pCameraBuffer->pBufHeader;
    eError = DequeueBuffer_Driver(pComponentPrivate, pCameraBuffer, OMXCAM_PREVIEW_PORT);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "eError Dequeue Buffer from Driver");
    
    /*
    If Camera Driver Queue is empty it will send a signal. This is used for
    changing from preview mode to capture mode, or going to pause state.
    */
    if(!CameraQueueStatus(pComponentPrivate, &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT])))
    {
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Check Lock");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MLK:MCDE");  
        pthread_mutex_lock(&(pComponentPrivate->mCameraPipeEmptyMutex));
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Lock");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "CSI:CCDE");
        pthread_cond_signal(&(pComponentPrivate->sConditionCameraPipeEmpty));
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Signal Condition");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "MUL:MCDE");
        pthread_mutex_unlock(&(pComponentPrivate->mCameraPipeEmptyMutex));
        __PERF_SYS_CMD(pComponentPrivate, Sent, "MUL:MCDE");
        OMX_CAMERA_PRINTF_L4("CameraDriverEmptyMutex: Check UnLock");
        
    }

    /* Store the latest preview buffer index */
    pComponentPrivate->nCurrentPreviewIndex = pCameraBuffer->nIndex;

    if(pCameraBuffer->bFirstTime)
    {
        pCameraBuffer->bFirstTime = OMX_FALSE;
    }    
    /* Copy First frame for thumbnail frame*/
    if(pComponentPrivate->bThumbnailReady == OMX_FALSE &&
       pThumbnailPort->pPortDef.bEnabled == OMX_TRUE &&
       pThumbnailPort->pPortDef.bPopulated == OMX_TRUE)
    {
#ifdef __PERF_INSTRUMENTATION__
        PERF_XferingFrame(pComponentPrivate->pPERFcomp,
                          pCameraBuffer->pBufHeader->pBuffer,
                          pCameraBuffer->pBufHeader->nFilledLen,
                          PERF_ModuleMemory, PERF_ModuleMemory);
#endif
								////VSTAB FIX/////								
        pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);    
        nThumbnailFrameSize =( (pPreviewPort->pPortDef.format.video.nFrameHeight)*
                               (pPreviewPort->pPortDef.format.video.nFrameWidth) );
        ///////////////////

								OMX_CAMERA_MEMCPY(pThumbnailPort->pCameraBuffer[0]->pBufHeader->pBuffer,
                          pBufHeader->pBuffer,
                          nThumbnailFrameSize,
                          eError);
 
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       pCameraBuffer->pBufHeader->pBuffer,
                       pCameraBuffer->pBufHeader->nFilledLen,
                       PERF_ModuleMemory);
#endif
        pThumbnailPort->pCameraBuffer[0]->pBufHeader->nFilledLen = pBufHeader->nFilledLen;
        pComponentPrivate->bThumbnailReady = OMX_TRUE;
    }

    if(pComponentPrivate->eCurrentState == OMX_StatePause ||
       pComponentPrivate->eDesiredState == OMX_StatePause)
    {
        OMX_CAMERA_PRINTF_L3("Component in Pause / Or Taking Still Image - Storing Buffer %d",
                           pCameraBuffer->nIndex);
        OMX_CAMERA_LINK_LIST_ADD_NODE(pComponentPrivate, pCameraBuffer);
        eError = OMX_ErrorNone;
        goto OMX_CAMERA_BAIL_CMD;
    }

#ifdef __CAMERA_VSTAB__
    if(pComponentPrivate->sVSTAB.bStab == OMX_TRUE)
    {
        vsStabResults vsResults;


#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          pCameraBuffer->pBufHeader->pBuffer,
                          0,
                          PERF_ModuleAlgorithm);
#endif
								
        eError = vStabLib_StabilizeFrame2(&(pComponentPrivate->sVSTAB_Context),
                                          pCameraBuffer->pBufHeader->pBuffer,
                                          0,
                                          &vsResults);							

		pBufHeader->nFilledLen =(pComponentPrivate->sVSTAB_Params.myOutFrame.x*pComponentPrivate->sVSTAB_Params.myOutFrame.y*2);
        OMX_CAMERA_PRINTF_L7("Called vStabLib_StabilizeFrame2 status[%d]", eError);
#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                           pCameraBuffer->pBufHeader->pBuffer,
                           0,
                           PERF_ModuleAlgorithm);
#endif
        
    }
#endif

    if(pComponentPrivate->bStopping == OMX_FALSE)
    {
        /* Send filled buffer to IL client or tunnled component in Preview Port*/
        if(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE &&
           pCameraBuffer->bPreviewFillThisBuffer == OMX_TRUE)
        {
            hTunnelComponent = pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].hTunnelComponent;
            pCameraBuffer->bPreviewFillThisBuffer = OMX_FALSE;
            if(hTunnelComponent != NULL)
            {
                /* Send buffer to tunneled component */
                OMX_CAMERA_PRINTF_L3("Send to Tunneled Component Preview Buffer[%d]:%p", pCameraBuffer->nIndex,pCameraBuffer);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pBufHeader,pBuffer),
                                  PREF(pBufHeader,nFilledLen),
                                  PERF_ModuleLLMM);
#endif
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x\n", pCameraBuffer);
                pCameraBuffer->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
                eError = OMX_EmptyThisBuffer(hTunnelComponent, pBufHeader);
                OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
                sched_yield();
            }
            else
            {
				OMX_CAMERA_PRINTF_L3("Send to IL Client Preview Buffer %d\n",
                    pCameraBuffer->nIndex);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pCameraBuffer->pBufHeader,pBuffer),
                                  PREF(pCameraBuffer->pBufHeader,nFilledLen),
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                pCameraBuffer->pBufHeader);
                pCameraBuffer->eBufferOwner = BUFFER_WITH_CLIENT;
            }
        }
        
        /* Send filled buffer to IL client or tunnled component in Capture Port*/
			
        if(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pPortDef.bEnabled == OMX_TRUE &&
       	   pComponentPrivate->bCapturing &&
           (pCameraBuffer->bCapturing == OMX_TRUE ||
            pComponentPrivate->bLastCapturingBuffers == OMX_TRUE) &&
           pCameraBuffer->bCaptureFillThisBuffer == OMX_TRUE)
        {
            hTunnelComponent = pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].hTunnelComponent;
            /* Clear bCaptureFillThisBuffer Flag*/
            pCameraBuffer->bCaptureFillThisBuffer = OMX_FALSE;
            
            if(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef.bEnabled == OMX_TRUE)
            {
                /* If Preview port is enabled, the buffer is shared. We need corresponding capture buffer*/
                pCaptureBuffer = pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pCameraBuffer[pCameraBuffer->nIndex];
                /* Set captured framed length */
                pCaptureBuffer->pBufHeader->nFilledLen = pCameraBuffer->pBufHeader->nFilledLen;
            }
            else
            {
                /* If Preview port is disable, the buffer is not shared*/
                pCaptureBuffer = pCameraBuffer;
            }
            
            if(pComponentPrivate->bSynchronized)
            {
                /* Get audio time stamp from clock */
                if(pComponentPrivate->bFirstCameraBuffer)
                {
                    OMX_CAMERA_PRINTF_L3("HandleCaptureDataBuf %d\n",__LINE__);
                    sClockStartTime.nSize = sizeof(OMX_TIME_CONFIG_TIMESTAMPTYPE);
                    sClockStartTime.nVersion.s.nVersionMajor = 0x01;
                    sClockStartTime.nVersion.s.nVersionMinor = 0x00;
                    sClockStartTime.nTimestamp = (OMX_TICKS) nCurrentMediaTime; 
    
                    eError = OMX_SetConfig(pComponentPrivate->hClock,
                                           OMX_IndexConfigTimeClientStartTime,
                                           (void *)(&sClockStartTime));
                    OMX_CAMERA_IF_ERROR_BAIL(eError, 
                                             "Error in OMX_SetConfig function\n");
                    
                    pComponentPrivate->bFirstCameraBuffer = OMX_FALSE;
                }
                else
                {
                    eError = OMX_GetConfig(pComponentPrivate->hClock,
                                           OMX_IndexConfigTimeCurrentMediaTime,
                                           (void *)(&sCurrentMediaTime));    
                    
                    OMX_CAMERA_IF_ERROR_BAIL(eError,
                                             "Error in OMX_SetConfig function\n");
                    nCurrentMediaTime = (long)sCurrentMediaTime.nTimestamp;
                }
                pCaptureBuffer->pBufHeader->nTimeStamp = (OMX_TICKS) nCurrentMediaTime;
            }
            
            /* If tunneling, pass the buffer to tunneled component */
            if(hTunnelComponent != NULL)
            {
                OMX_STATETYPE eTunnelCurrentState;
                eError = OMX_GetState(hTunnelComponent, &(eTunnelCurrentState));
                OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_GetState upon Tunneled component Failed\n");
                if(eTunnelCurrentState == OMX_StateExecuting)
                {
                    OMX_CAMERA_PRINTF_L3("Send to Tunneled Component Capture Buffer [%d]",
                                         pCameraBuffer->nIndex);
#ifdef __PERF_INSTRUMENTATION__
                    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                      PREF(pCaptureBuffer->pBufHeader,pBuffer),
                                      PREF(pCaptureBuffer->pBufHeader,nFilledLen),
                                      PERF_ModuleLLMM);
#endif
OMX_CAMERA_PRINTF_L3("pCaptureBuffer: 0x%08x\n", pCaptureBuffer);
                    pCaptureBuffer->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
                    eError = OMX_EmptyThisBuffer(hTunnelComponent, pCaptureBuffer->pBufHeader);
                    OMX_CAMERA_IF_ERROR_BAIL(eError,"OMX_EmptyThisBuffer Failed\n");
                    sched_yield();
                }
            }
            else
            {
                OMX_CAMERA_PRINTF_L3("Send to IL Client Capture Buffer [%d]",
                                     pCameraBuffer->nIndex);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pCaptureBuffer->pBufHeader,pBuffer),
                                  PREF(pCaptureBuffer->pBufHeader,nFilledLen),
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                pCaptureBuffer->pBufHeader);
                pCaptureBuffer->eBufferOwner = BUFFER_WITH_CLIENT;
            }
        }
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of DequeueBuffer */



/*-------------------------------------------------------------------*/
/**
  * DequeueBuffer_Driver()
  *
  * Will Dequeue Buffer from driver and check if is the corresponding buffer 
  * @param pComponentPrivate component private data structure.
  * @param pCameraBuffer buffer private data structure
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/

inline OMX_ERRORTYPE DequeueBuffer_Driver (OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                           OMX_CAMERA_COMPONENT_BUFFER* pCameraBuffer,
                                           CAMERA_COMP_PORT_TYPE ePort)
{
    OMX_ERRORTYPE eError  = OMX_ErrorNone;
    struct v4l2_buffer cfilledbuffer;
	/* sleep for a while to let other threads resume */
	struct timespec timeout, timerem;
	timeout.tv_nsec = 10000000;
	timeout.tv_sec = 0;
	
    
    OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");
    
    /* Dequeue Buffer from driver*/
    cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
    if(pCameraBuffer->eBufferOwner == BUFFER_WITH_CAMERA_DRIVER)
    {
#ifdef __PERF_INSTRUMENTATION__
        PERF_RequestingFrame(pComponentPrivate->pPERFcomp,
                             pCameraBuffer->pBufHeader->pBuffer,
                             pCameraBuffer->pBufHeader->nAllocLen,
                             PERF_ModuleHardware);
#endif
        
        if(!(IsStillCaptureMode(pComponentPrivate) && 
           ePort == OMXCAM_CAPTURE_PORT))
        {
            pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
            OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
    
            /* Polling algorithm to avoid stall condition on Camera Driver */
            struct pollfd dqpoll;
            dqpoll.fd = pComponentPrivate->nCameraDev;
            dqpoll.events = POLLIN;
            int retVal = 0;
	
			
            do
            {
                retVal = poll(&dqpoll, 1, 1000);
                if(retVal < 0)
                {                    
                    OMX_CAMERA_PRINTF_L2("poll function fails waiting for buffer:[%p][%d]",
                                        pCameraBuffer->pBufHeader->pBuffer,
                                        pCameraBuffer->nIndex);		    
                    perror("Poll error");
		    eError = OMX_ErrorHardware;
		    goto OMX_CAMERA_BAIL_CMD;		    
                }
		if(retVal == 0)
		{
		    printf("TIME OUT poll()\n");
		    eError = OMX_ErrorTimeout;
		    goto OMX_CAMERA_BAIL_CMD;		    
		}
				
            }
            while (retVal < 0);  /* pass through on timeout */
			
            OMX_CAMERA_PRINTF_L4("DriverMutex: Check Lock");
            pthread_mutex_lock(&(pComponentPrivate->mDriverMutex));
            OMX_CAMERA_PRINTF_L4("DriverMutex: Locked");
        
            /*If the client thread has flush the driver we need to skip the de-queue*/
            if(pCameraBuffer->eBufferOwner == BUFFER_FLUSHED)
            {
                OMX_CAMERA_PRINTF_L2("Buffer already return by flush, skip de-queue");
OMX_CAMERA_PRINTF_L3("pCameraBuffer: 0x%08x", pCameraBuffer);
                pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT;
                pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
                OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
                eError = OMX_ErrorNone;
                goto OMX_CAMERA_BAIL_CMD;
            }
        }
        
        while (ioctlCamera(pComponentPrivate->nCameraDev,
                           VIDIOC_DQBUF,
                           &cfilledbuffer,
                           pComponentPrivate) < 0)
        {
            OMX_CAMERA_PRINTF_L2("[VIDIOC_DQBUF][%p][%d]",
                                  pCameraBuffer->pBufHeader->pBuffer,
                                  pCameraBuffer->nIndex);
            perror("VIDIOC_DQBUF Driver");
           // sched_yield();
		   nanosleep(&timeout, &timerem);
        }		
        //sched_yield();
		
		nanosleep(&timeout, &timerem);
		
#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                           cfilledbuffer.m.userptr,
                           cfilledbuffer.length,
                           PERF_ModuleHardware);
#endif
        OMX_CAMERA_PRINTF_L2("[VIDIOC_DQBUF][%p][%d]",
                             cfilledbuffer.m.userptr,
                             cfilledbuffer.index);
                             
        /* Check if the buffer received is the one expected */
        if ((void*)cfilledbuffer.m.userptr != pCameraBuffer->pBufHeader->pBuffer)
        {
            OMX_CAMERA_PRINTF_ERROR("Warning:Received different Buffer from driver [%p]",
                                    (void*)cfilledbuffer.m.userptr);
            OMX_CAMERA_SET_ERROR_BAIL(eError, OMX_ErrorHardware,"DequeueBuffer_Driver");
        }
        
        pCameraBuffer->eBufferOwner = BUFFER_WITH_COMPONENT_OUT_PIPE;
        pCameraBuffer->pBufHeader->nFilledLen = cfilledbuffer.bytesused;
    }
    else
    {
        OMX_CAMERA_PRINTF_ERROR("Warning:Buffer to dequeue not inside driver[%p]",
                                pCameraBuffer->pBufHeader->pBuffer);
        OMX_CAMERA_PRINTF_ERROR("Warning: Buffer to dequeue is on %d\n", pCameraBuffer->eBufferOwner);
        pCameraBuffer->pBufHeader->nFilledLen = 0;
    }
    
    pthread_mutex_unlock(&(pComponentPrivate->mDriverMutex));
    OMX_CAMERA_PRINTF_L4("DriverMutex: Unlock");
    
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * InitPrivateStructure()
  *
  * Notify application that buffer it sent is processed.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if reading from pipe fails.
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE InitPrivateStructure(OMX_CAMERA_COMPONENT_PRIVATE* pComponentPrivate)
{
    OMX_U32 nIndex                                      = 0;
    OMX_ERRORTYPE eError                                = OMX_ErrorUndefined;
    OMX_STRING cCameraName                              = "OMX.TI.Camera"; 
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef              = NULL;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat    = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Set component version */
    OMX_CAMERA_INIT_SIZE_VERSION(pComponentPrivate, OMX_CAMERA_COMPONENT_PRIVATE);

    /* Set port param type */
    pComponentPrivate->sPortParam.nSize    = sizeof(OMX_PORT_PARAM_TYPE);
    pComponentPrivate->sPortParam.nVersion = pComponentPrivate->nVersion;
    pComponentPrivate->sPortParam.nPorts   = OMXCAM_NUM_PORTS;
    pComponentPrivate->sPortParam.nStartPortNumber = OMXCAM_PREVIEW_PORT;

    /* Set default contrast and brightness */
    pComponentPrivate->sContrast.nContrast  = OMXCAM_DEFAULT_CONTRAST;
    pComponentPrivate->nCurrentBrightness = OMXCAM_DEFAULT_BRIGHTNESS;
    
    /*Set default color effect*/
    pComponentPrivate->nColorEffect = COLOR_FX_COLOR_LEVEL;
    
    /* Set default for white balance */
    pComponentPrivate->sAWB.nSize = sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE);
    pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlAuto;
    
    /* Set default for exposure*/
    pComponentPrivate->sAE.nSize = sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE);
    pComponentPrivate->sAE.eExposureControl = OMX_ExposureControlAuto;
#ifdef __CAMERA_2A__
    /*set default for Autofocus */
    pComponentPrivate->bAutoFocus = OMX_FALSE;
    pComponentPrivate->bLinkedListFull = OMX_FALSE;	
#endif
    
    /* Set default scale windows*/
    pComponentPrivate->sScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
    pComponentPrivate->sScaleFactor.xWidth = FACTOR_1X;
    pComponentPrivate->sScaleFactor.xHeight = FACTOR_1X;
    
    /*
    Set to 0 Buffers send when Capture Port is tunneled and is buffer owner.
    Variable used to control first 4 EmptyThisBuffer send to tunneled component
    */
    pComponentPrivate->nInitialTunneledCaptureBuffer = 0;
         
    /* Set component name */
    OMX_CAMERA_STRCPY(pComponentPrivate->cComponentName, cCameraName, eError);

    /* Set sensor settings */
    OMX_FRAMESIZETYPE sFrameSize = {sizeof(OMX_FRAMESIZETYPE),
                                   pComponentPrivate->nVersion,
                                   OMXCAM_PREVIEW_PORT, 
                                   OMXCAM_DEFAULT_WIDTH, 
                                   OMXCAM_DEFAULT_HEIGHT};
    pComponentPrivate->sSensorMode.nSize      = sizeof(OMX_PARAM_SENSORMODETYPE);
    pComponentPrivate->sSensorMode.nVersion   = pComponentPrivate->nVersion;
    pComponentPrivate->sSensorMode.nPortIndex = nIndex;
    pComponentPrivate->sSensorMode.nFrameRate = 30;
    pComponentPrivate->sSensorMode.bOneShot   = OMX_FALSE;
    pComponentPrivate->sSensorMode.sFrameSize = sFrameSize;

    /*  Set capture mode settings */
    pComponentPrivate->sCaptureMode.nSize = sizeof(OMX_CONFIG_CAPTUREMODETYPE);
    pComponentPrivate->sCaptureMode.nVersion = pComponentPrivate->nVersion;
    pComponentPrivate->sCaptureMode.nPortIndex = OMXCAM_CAPTURE_PORT;
    pComponentPrivate->sCaptureMode.bContinuous = OMX_TRUE;
    pComponentPrivate->sCaptureMode.bFrameLimited = OMX_FALSE;
    pComponentPrivate->sCaptureMode.nFrameLimit = 0;

    /* Set default crop window */
    pComponentPrivate->sCropWindow.nLeft      = 0;
    pComponentPrivate->sCropWindow.nTop       = 0;
    pComponentPrivate->sCropWindow.nWidth     = OMXCAM_MAX_FRAMEWIDTH;
    pComponentPrivate->sCropWindow.nHeight    = OMXCAM_MAX_FRAMEHEIGHT;

    /* By default no clock input expected */
    pComponentPrivate->bSynchronized = OMX_FALSE;

    /* Component priority */
    pComponentPrivate->sPriorityMgmt.nSize          = sizeof(OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->sPriorityMgmt.nVersion       = pComponentPrivate->nVersion;
    pComponentPrivate->sPriorityMgmt.nGroupPriority = 0;
    pComponentPrivate->sPriorityMgmt.nGroupID       = 0;

    pComponentPrivate->bFirstCameraBuffer   = OMX_TRUE;
    pComponentPrivate->bCapturing           = OMX_FALSE;
    pComponentPrivate->bAutoPauseAfterCapture = OMX_FALSE;
    pComponentPrivate->bImageCaptureMode    = OMX_TRUE;
    pComponentPrivate->bStopping            = 0;
    pComponentPrivate->nPreviewBufCount     = 0;
    pComponentPrivate->nVidCaptureBufCount  = 0;
    pComponentPrivate->bProcessingCaptuteBuf= 0;
    pComponentPrivate->nCurrentPreviewIndex = 0;
    pComponentPrivate->nCurrentCaptureIndex = 0;
    pComponentPrivate->bCopyInProgress      = OMX_FALSE;
    pComponentPrivate->bStreaming           = OMX_FALSE;
    pComponentPrivate->alg2A = OMX_FALSE;
    
    /*To process last Capturing process */
    pComponentPrivate->bLastCapturingBuffers = OMX_FALSE;
    pComponentPrivate->nLastCapturingBuffers = 0;
    
    /* Flag for IPP */
    pComponentPrivate->bIPP = OMX_FALSE;

    OMX_CAMERA_STRCPY((char *)pComponentPrivate->sComponentRole.cRole,
                      (char *)"camera.yuv",
                      eError);

    /* Initialize all port structures */
    for(nIndex = 0; nIndex < pComponentPrivate->sPortParam.nPorts; nIndex++)
    {
         pComponentPrivate->sCompPorts[nIndex].nBufferCount = 0;
         pPortDef = &(pComponentPrivate->sCompPorts[nIndex].pPortDef);
         pPortDef->nSize                           = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
         pPortDef->nVersion                        = pComponentPrivate->nVersion;
         pPortDef->nPortIndex                      = nIndex; 
         pPortDef->bEnabled                        = OMX_TRUE;
         pPortDef->bPopulated                      = OMX_FALSE;
         pPortDef->eDir                            = OMX_DirOutput;
         pPortDef->eDomain                         = OMX_PortDomainVideo;
         pPortDef->nBufferCountMin                 = 1;
         if(nIndex == OMXCAM_THUMBNAIL_PORT)
         {
             pPortDef->nBufferCountActual          = OMXCAM_NUM_BUFFERS_THUMBNAIL;
         }
         else
         {
             pPortDef->nBufferCountActual          = OMXCAM_NUM_BUFFERS;
         }
         
         pPortDef->format.video.nFrameWidth        = OMXCAM_DEFAULT_WIDTH;
         pPortDef->format.video.nFrameHeight       = OMXCAM_DEFAULT_HEIGHT;
         pPortDef->nBufferSize                     = (OMXCAM_DEFAULT_WIDTH*OMXCAM_DEFAULT_HEIGHT) * 2;
         pPortDef->format.video.eColorFormat       = OMX_COLOR_FormatYCbYCr;
         pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
 
         /* video port format */
         pVideoPortFormat = &(pComponentPrivate->sCompPorts[nIndex].sPortFormat);
         pVideoPortFormat->nSize              = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
         pVideoPortFormat->nVersion           = pComponentPrivate->nVersion;
         pVideoPortFormat->nPortIndex         = nIndex;
         pVideoPortFormat->nIndex             = 1;
         pVideoPortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
         pVideoPortFormat->eColorFormat       = OMX_COLOR_FormatYCbYCr;
         pVideoPortFormat->xFramerate = 30;
 
        /* Buffer supplier setting */
         if(!nIndex)
         {
             pComponentPrivate->sCompPorts[nIndex].eSupplierSetting = OMX_BufferSupplyInput;
         }
        else
         {
            pComponentPrivate->sCompPorts[nIndex].eSupplierSetting = OMX_BufferSupplyOutput;
         }
         pComponentPrivate->sCompPorts[nIndex].hTunnelComponent = NULL;
    }
    
    OMX_CAMERA_LINK_LIST_INIT(pComponentPrivate);

    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * InitCameraResources()
  *
  * Allocate & initialize resources. Set default configurations.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE InitCameraResources(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
#if 0
    /*get initial camera parameters*/
    OMX_CAMERA_IF_ERROR_BAIL(GetCameraParameters(pComponentPrivate), "Error GetCameraParameters");
#endif
	//if(pComponentPrivate->sVSTAB.bStab == OMX_FALSE){
		eError = InitCameraPreview(pComponentPrivate);
		OMX_CAMERA_IF_ERROR_BAIL(eError, "Error InitCameraPreview");
	//}

    /* Set default contrast */
    eError = SetCameraContrast(pComponentPrivate);
    if(eError != OMX_ErrorUnsupportedSetting){
        OMX_CAMERA_IF_ERROR_BAIL(eError, "Error SetCameraContrast\n");
    }

    /* Set default brightness */
    eError = SetCameraBrightness(pComponentPrivate);
    if(eError != OMX_ErrorUnsupportedSetting){
        OMX_CAMERA_IF_ERROR_BAIL(eError,"Error SetCameraBrightness\n");
    }

    /* Set default sensor frame rate */
    eError = SetCameraSensorModeType(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error SetCameraSensorModeType\n");

    /* Set default color effect */
    eError = SetCameraColorEffect(pComponentPrivate);
    if(eError != OMX_ErrorUnsupportedSetting){
        OMX_CAMERA_IF_ERROR_BAIL(eError, "Error SetCameraColorEffect");
    }

#if 0
    /* Set default crop */
    eError = SetCameraCropWindow(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError,"Error SetCameraCropWindow");
#endif

    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of InitCameraResources */


/*-------------------------------------------------------------------*/
/**
  * InitStillCapture()
  *
  * Initialize camera driver for preview.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE InitStillCapture(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    struct v4l2_format sFrameFormat;
    struct v4l2_capability capability;
    struct v4l2_requestbuffers creqbuf;
    OMX_ERRORTYPE eError                    = OMX_ErrorUndefined;  
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef  = NULL;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
    OMX_U32 i;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pPortDef = &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pPortDef);
    
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:QCAP");
    if (ioctlCamera(pComponentPrivate->nCameraDev,
                    VIDIOC_QUERYCAP,
                    &capability,
                    pComponentPrivate) < 0)
    {
        perror("VIDIOC_QUERYCAP");
        OMX_CAMERA_SET_ERROR_BAIL(eError, OMX_ErrorHardware, "VIDIOC_QUERYCAP");
    }

    if (capability.capabilities & V4L2_CAP_STREAMING)
    {
        OMX_CAMERA_PRINTF_L2("[VIDIOC_QUERYCAP] The driver is capable of Streaming!");
    }
    else
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorHardware,
                                  "The driver is not capable of Streaming!");
    }

    /* 
    Note: In the beginning camera sensor is going to be configurred with
    preview port's configuration 
    */
    switch (pPortDef->format.image.eColorFormat)
    {
        case OMX_COLOR_FormatYCbYCr:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;												
            break;
        case OMX_COLOR_FormatCbYCrY:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;												
            break;
        case OMX_COLOR_Format16bitRGB565:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;												
            break;
        default:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;												
            break;
    }


    /* Set camera driver resolution and color format */
    sFrameFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:GFMT");
    eError = ioctlCamera(pComponentPrivate->nCameraDev,
                         VIDIOC_G_FMT,
                         &sFrameFormat,
                         pComponentPrivate);
    if (eError < 0)
    {
        perror("VIDIOC_G_FMT");
        return -1;
    }

    OMX_CAMERA_PRINTF_L2("[VIDIOC_G_FMT] Camera Image width = %d, Image height = %d, size = %d",
           sFrameFormat.fmt.pix.width, sFrameFormat.fmt.pix.height,
           sFrameFormat.fmt.pix.sizeimage);
    
    /* Preview port is always in video domain */
    sFrameFormat.fmt.pix.width  = pPortDef->format.image.nFrameWidth;
    sFrameFormat.fmt.pix.height = pPortDef->format.image.nFrameHeight;

    /* Set camera driver resolution and color format */
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:SFMT");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_S_FMT,
                                      &sFrameFormat,
                                      pComponentPrivate) < 0, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_S_FMT");
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:GFMT");
    
    OMX_CAMERA_PRINTF_L2("[VIDIOC_S_FMT] Camera Image width = %d, Image height = %d, size = %d",
                         sFrameFormat.fmt.pix.width, sFrameFormat.fmt.pix.height,
                         sFrameFormat.fmt.pix.sizeimage);

    eError = ioctlCamera(pComponentPrivate->nCameraDev,
                         VIDIOC_G_FMT,
                         &sFrameFormat,
                         pComponentPrivate);
    __PERF_SYS_CMD(pComponentPrivate, Sent, "CAM:GFMT");
    if (eError < 0)
    {
        perror("VIDIOC_G_FMT");
        return -1;
    }

    OMX_CAMERA_PRINTF_L2("[VIDIOC_G_FMT] Camera Image width = %d, Image height = %d, size = %d",
           sFrameFormat.fmt.pix.width, sFrameFormat.fmt.pix.height,
           sFrameFormat.fmt.pix.sizeimage);
	
	/*Necessary  VIDIOC_S_PARM after changing resolution*/
	SetCameraSensorModeType(pComponentPrivate);
    
    /* Apply Zoom factor */
    eError = SetCameraScaleWindow(pComponentPrivate,
                                  &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
    OMX_CAMERA_IF_ERROR_BAIL(eError,
                             "Error while setting scale window !");


    /* Request buffers */
    creqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    creqbuf.memory = V4L2_MEMORY_USERPTR;
    creqbuf.count = pPortDef->nBufferCountActual;
    OMX_CAMERA_PRINTF_L2("[VIDIOC_REQBUFS] Requesting %d buffers of type %s", creqbuf.count, 
    (creqbuf.memory == V4L2_MEMORY_USERPTR) ? "V4L2_MEMORY_USERPTR" :"V4L2_MEMORY_MMAP");

    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:RQBF");
    if (ioctlCamera(pComponentPrivate->nCameraDev,
                    VIDIOC_REQBUFS,
                    &creqbuf,
                    pComponentPrivate) < 0)
    {
        perror("VIDEO_REQBUFS");
        return -1;
    }
    
    OMX_CAMERA_PRINTF_L2("[VIDIOC_REQBUFS] Camera Driver allowed buffers reqbuf.count = %d",
                         creqbuf.count);
    if(pPortDef->nBufferCountActual != creqbuf.count)
    {
        OMX_CAMERA_PRINTF_ERROR("CAMDRV: Only using %d/%d buffers requested",
                                (int)creqbuf.count,
                                (int)pPortDef->nBufferCountActual);
    }
     pComponentPrivate->nBuffersSupportedByDriver = creqbuf.count;
     
    /* mmap driver memory or allocate user memory, and queue each buffer */ 
    for (i = 0; i < creqbuf.count; ++i)
    {
        struct v4l2_buffer buffer;
        buffer.type = creqbuf.type;
        buffer.memory = creqbuf.memory;
        buffer.index = i;
        __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:QRBF");
		
        if (ioctlCamera(pComponentPrivate->nCameraDev,
                        VIDIOC_QUERYBUF,
                        &buffer,
                        pComponentPrivate) < 0)
        {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }
		
        pCameraBuffer = pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pCameraBuffer[i];
        pCameraBuffer->bFirstTime = OMX_TRUE;
        pCameraBuffer->nIndex = i;
        buffer.length = pCameraBuffer->pBufHeader->nAllocLen;
        buffer.m.userptr = (unsigned int)(pCameraBuffer->pBufHeader->pBuffer);
        OMX_CAMERA_PRINTF_L2("[VIDIOC_QUERYBUF] User Buffer [%d].start = %x  length = %d",
               (int) i,
               (unsigned int)(pCameraBuffer->pBufHeader->pBuffer),
               (int) pCameraBuffer->pBufHeader->nAllocLen);

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                          buffer.m.userptr,
                          buffer.length,
                          PERF_ModuleHardware);
#endif
		
        OMX_CAMERA_PRINTF_L2("[VIDIOC_QBUF][%p][%d]",
                             buffer.m.userptr,
                             buffer.index);
		
        if (ioctlCamera(pComponentPrivate->nCameraDev,
                        VIDIOC_QBUF,
                        &buffer,
                        pComponentPrivate) < 0)
        {
            perror("INIT STILL CAMERA VIDIOC_QBUF");
            return -1;
        }
		
#ifdef __PERF_INSTRUMENTATION__
        PERF_SentFrame(pComponentPrivate->pPERFcomp,
                       buffer.m.userptr,
                       buffer.length,
                       PERF_ModuleHardware);
#endif
        pCameraBuffer->eBufferOwner = BUFFER_WITH_CAMERA_DRIVER;
         
        
    }
    
    for(i = 0; i < pPortDef->nBufferCountActual; ++i)
    {
        pCameraBuffer = pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pCameraBuffer[i];
        pCameraBuffer->nIndex = i;

        //pComponentPrivate->StillBuffer = &pCameraBuffer;
							
        /* Workaround Copy pCameraBuffess to internal array*/
        OMX_CAMERA_MEMCPY(&(pComponentPrivate->StillBuffer[i]),
                          pCameraBuffer,
                          sizeof(OMX_CAMERA_COMPONENT_BUFFER),
                          eError);
    }
   
    
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of InitStillCapture */

/*-------------------------------------------------------------------*/
/**
  * InitCameraPreview()
  *
  * Initialize camera driver for preview.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  *         OMX_ErrorInsufficientResources  if malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE InitCameraPreview(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_format sFrameFormat;
    struct v4l2_capability sCapability;
    struct v4l2_requestbuffers sReqBuf;
	
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = NULL;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Note: In the beginning camera sensor is going to be configurred with preview port's configuration */
    pPortDef = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT].pPortDef);
    switch ((pPortDef->eDomain == OMX_PortDomainImage)? pPortDef->format.image.eColorFormat: 
                                                        pPortDef->format.video.eColorFormat)
    {
        case OMX_COLOR_FormatYCbYCr:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;												
            break;
        case OMX_COLOR_FormatCbYCrY:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;												
            break;
        case OMX_COLOR_Format16bitRGB565:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;												
            break;
        default:
            sFrameFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;												
    }

    /* Preview port is always in video domain */
    sFrameFormat.fmt.pix.width  = pPortDef->format.video.nFrameWidth;
    sFrameFormat.fmt.pix.height = pPortDef->format.video.nFrameHeight;

    /* Check camera driver's streming capabilities */
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCAP)");
     __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:QRCA");
    if(ioctlCamera(pComponentPrivate->nCameraDev,
                   VIDIOC_QUERYCAP,
                   &sCapability,
                   pComponentPrivate) == -1) 
    {
            OMX_CAMERA_PRINTF_L2(" VIDIOC_QUERYCAP not implemented ");
            OMX_CAMERA_SET_ERROR_BAIL(eError,
                                      OMX_ErrorHardware,
                                      "Camera doesn't support streaming");
    }
    else 
    {
        OMX_CAMERA_CHECK_COND(!(sCapability.capabilities & V4L2_CAP_STREAMING), 
                              eError, 
                              OMX_ErrorHardware, 
                              "Error ! - Camera driver is not capable of Streaming ");
    }

    /* Set camera driver resolution and color format */
	
    sFrameFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:SFMT");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_S_FMT,
                                      &sFrameFormat,
                                      pComponentPrivate) < 0, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_S_FMT");
    
	  OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_S_FMT) Camera Image width = %d, Image height = %d, size = %d",
                         sFrameFormat.fmt.pix.width, sFrameFormat.fmt.pix.height,
                         sFrameFormat.fmt.pix.sizeimage);
	
	/*Necessary  VIDIOC_S_PARM after change resolution*/
	SetCameraSensorModeType(pComponentPrivate);
	
    /* Check if the camera driver can accept 'nBufferCountActual' number of buffers*/
    sReqBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sReqBuf.memory = V4L2_MEMORY_USERPTR;
    sReqBuf.count  = pPortDef->nBufferCountActual; 

    /* Request buffers */
    if(!bKernelBufRequested)
    {
        OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_REQBUFS) Requesting %d buffers of type %s", sReqBuf.count, 
                             (sReqBuf.memory == V4L2_MEMORY_USERPTR) ? "V4L2_MEMORY_USERPTR" :"V4L2_MEMORY_MMAP");
        __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:RQBF");
        if(ioctlCamera(pComponentPrivate->nCameraDev,
                       VIDIOC_REQBUFS,
                       &sReqBuf,
                       pComponentPrivate) == -1)
        {
			OMX_CAMERA_SET_ERROR_BAIL(eError,
									OMX_ErrorHardware,
                                    "Camera doesn't request buffers");
        }
		
		OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_REQBUFS) Camera Driver allowed buffers reqbuf.count = %d",
                            sReqBuf.count);

		
        bKernelBufRequested = 1;
    }
    __PERF_SYS_CMD(pComponentPrivate, Sent, "CAM:RQBF");
	

    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
} /* End of InitCameraPreview */


/*-------------------------------------------------------------------*/
/**
  * SetCameraContrast() 
  *
  * Set camera contrast.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraContrast(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    int level;
    int nDriverContrastRange;
    struct v4l2_control control;
    struct v4l2_queryctrl queryctrl;
    int fd;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    fd = pComponentPrivate->nCameraDev;
    
    /* Query contrast limit */
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_CONTRAST)");
    queryctrl.id = V4L2_CID_CONTRAST;
    if (ioctlCamera(fd, VIDIOC_QUERYCTRL, &queryctrl, pComponentPrivate) == -1)
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorUnsupportedSetting,
                                  "Contrast is not supported!");
    }
    OMX_CAMERA_PRINTF_L2("[Maximum:%d][Minimum:%d]",
                         (int)queryctrl.maximum,
                         (int)queryctrl.minimum);

    nDriverContrastRange = queryctrl.maximum - queryctrl.minimum;
    level = (int)((pComponentPrivate->sContrast.nContrast - (-100))* nDriverContrastRange / OMXCAM_CONTRAST_RANGE);
    if (level < queryctrl.minimum || level > queryctrl.maximum)
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError, OMX_ErrorHardware, "Out of range!");
    }
    
    control.id = V4L2_CID_CONTRAST;
    control.value = level;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_S_CTRL, V4L2_CID_CONTRAST) - Set to %d",
                         (int)control.value);
    if (ioctlCamera(fd, VIDIOC_S_CTRL, &control, pComponentPrivate) == -1) 
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorHardware,
                                  "VIDIOC_S_CTRL failed!");
    }
    
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * GetCameraContrast() 
  *
  * Get camera contrast.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraContrast(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    int nDriverContrastRange;
    struct v4l2_control control;
    struct v4l2_queryctrl queryctrl;
    int fd;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    fd = pComponentPrivate->nCameraDev;

    /* Query contrast limit */
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_CONTRAST)");
    queryctrl.id = V4L2_CID_CONTRAST;
    if (ioctlCamera(fd, VIDIOC_QUERYCTRL, &queryctrl, pComponentPrivate) == -1)
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorUnsupportedSetting,
                                  "Contrast is not supported!");
    }
    OMX_CAMERA_PRINTF_L2("[Maximum:%d][Minimum:%d]",
                         (int)queryctrl.maximum,
                         (int)queryctrl.minimum);

    /* Query contrast actual contrast*/
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_G_CTRL,V4L2_CID_CONTRAST)");
    control.id = V4L2_CID_CONTRAST;
    if (ioctlCamera(fd, VIDIOC_G_CTRL, &control, pComponentPrivate) == -1)
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError,
                                  OMX_ErrorHardware,
                                  "VIDIOC_G_CTRL failed!");
    }
    OMX_CAMERA_PRINTF_L2("Contrast Level:%d",(int)control.value);

    nDriverContrastRange = queryctrl.maximum - queryctrl.minimum;
    pComponentPrivate->sContrast.nContrast =
        (int)(control.value * OMXCAM_CONTRAST_RANGE/nDriverContrastRange - 100);
    if (pComponentPrivate->sContrast.nContrast < -100 ||
        pComponentPrivate->sContrast.nContrast > 100)
    {
        OMX_CAMERA_SET_ERROR_BAIL(eError, OMX_ErrorHardware, "Out of range!");
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}



/*-------------------------------------------------------------------*/
/**
  * SetCameraBrightness() 
  *
  * Set camera brightness.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraBrightness(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    int nDriverBrightnessRange;
    struct v4l2_control sControl;
    struct v4l2_queryctrl sQueryctrl;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Check if brightness supported */
    sQueryctrl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_BRIGHTNESS)");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_QUERYCTRL,
                                      &sQueryctrl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorUnsupportedSetting, 
                          "Brightness is not supported!");
     OMX_CAMERA_PRINTF_L2("[Maximum:%d][Minimum:%d]",
                         (int)sQueryctrl.maximum,
                         (int)sQueryctrl.minimum);

    
    nDriverBrightnessRange = sQueryctrl.maximum - sQueryctrl.minimum;
    /* Mapping range (0-100%) to (0, 15) */
    sControl.value =
        (int)(sQueryctrl.minimum +
        (int) (pComponentPrivate->nCurrentBrightness * nDriverBrightnessRange / OMXCAM_BRIGHTNESS_RANGE));

    /* Check limits */
    OMX_CAMERA_CHECK_COND((sControl.value < sQueryctrl.minimum) ||
                          (sControl.value > sQueryctrl.maximum),
                          eError, 
                          OMX_ErrorBadParameter, 
                          "Brightness out of range!");

    /* Set new brightness value */
    sControl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_S_CTRL, V4L2_CID_BRIGHTNESS) - Set to %d",
                         (int)sControl.value);
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_S_CTRL,
                                      &sControl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_S_CTRL failed!");
    
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * GetCameraBrightness() 
  *
  * Get camera brightness.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraBrightness(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    int nDriverBrightnessRange;
    struct v4l2_control sControl;
    struct v4l2_queryctrl sQueryctrl;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Check if brightness supported */
    sQueryctrl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_BRIGHTNESS)");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_QUERYCTRL,
                                      &sQueryctrl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorUnsupportedSetting, 
                          "Brightness is not supported!");
     OMX_CAMERA_PRINTF_L2("[Maximum:%d][Minimum:%d]",
                         (int)sQueryctrl.maximum,
                         (int)sQueryctrl.minimum);
    
    /* Retrieve brightness */
    sControl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_G_CTRL,V4L2_CID_BRIGHTNESS)");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_G_CTRL,
                                      &sControl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_G_CTRL failed");
    OMX_CAMERA_PRINTF_L2("Brightness Level:%d",(int)sControl.value);
    
    nDriverBrightnessRange = sQueryctrl.maximum - sQueryctrl.minimum;
    /* Mapping range (0-15) to (0, 100%) */
    pComponentPrivate->nCurrentBrightness =
    (sControl.value - sQueryctrl.minimum) * OMXCAM_BRIGHTNESS_RANGE/nDriverBrightnessRange;
    /* Check limits */
    OMX_CAMERA_CHECK_COND((pComponentPrivate->nCurrentBrightness < 0) ||
                          (pComponentPrivate->nCurrentBrightness > 100),
                          eError, 
                          OMX_ErrorBadParameter, 
                          "Brightness out of range!");
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * GetCameraColorEffect() 
  *
  * Get camera color effect.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraColorEffect(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_control sControl;
    struct v4l2_queryctrl sQueryctrl;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Check if brightness supported */
    sQueryctrl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_PRIVATE_BASE)");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_QUERYCTRL,
                                      &sQueryctrl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorUnsupportedSetting, 
                          "Color effect is not supported!");
    
    /* Retrieve brightness */
    //sControl.id = V4L2_CID_PRIVATE_ISP_COLOR_FX;

    sControl.id = V4L2_CID_COLORFX;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_G_CTRL,V4L2_CID_PRIVATE_ISP_COLOR_FX)");
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_G_CTRL,
                                      &sControl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_G_CTRL failed");
    OMX_CAMERA_PRINTF_L2("Color effect Level:%d",(int)sControl.value);
    pComponentPrivate->nColorEffect = sControl.value;
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * SetCameraColorEffect() 
  *
  * Set camera color effect.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraColorEffect(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_control sControl;
    struct v4l2_queryctrl sQueryctrl;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Check if brightness supported */
    sQueryctrl.id = V4L2_CID_BRIGHTNESS;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_QUERYCTRL, V4L2_CID_PRIVATE_BASE)");
		
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_QUERYCTRL,
                                      &sQueryctrl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorUnsupportedSetting, 
                          "Color effect is not supported!");
    
    /* Retrieve brightness */
    sControl.id = V4L2_CID_PRIVATE_ISP_COLOR_FX;
    sControl.value = pComponentPrivate->nColorEffect;
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_S_CTRL,V4L2_CID_PRIVATE_ISP_COLOR_FX) Set to %d",
                         (int)sControl.value);
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_S_CTRL,
                                      &sControl,
                                      pComponentPrivate) == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_S_CTRL failed");
	
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

#ifdef __CAMERA_2A__
/*-------------------------------------------------------------------*/
/**
  * SetWhiteBalance() 
  *
  * Set WhiteBalance.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraWhiteBalance(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_WHITEBALCONTROLTYPE eWhiteBalControl;
    
    pComponentPrivate->cameraAlg->ReadSettings(pComponentPrivate->cameraAlg->pPrivateHandle, 
                                               &(pComponentPrivate->cam2aSettings));
    
    eWhiteBalControl = pComponentPrivate->sAWB.eWhiteBalControl;
    switch (eWhiteBalControl)
    {
        case OMX_WhiteBalControlOff:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_MANUAL;
            pComponentPrivate->cam2aSettings.awb.gain_blue = 0x30;
            pComponentPrivate->cam2aSettings.awb.gain_red = 0X30;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;
            break;
        case OMX_WhiteBalControlAuto:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_AUTO;
            break;
        case OMX_WhiteBalControlSunLight:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_DAYLIGHT;
            /*pComponentPrivate->cam2aSettings.ae.exposure_time = 30000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;*/
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        case OMX_WhiteBalControlCloudy:
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
            break;
        case OMX_WhiteBalControlShade:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_CLOUDY;
            /*pComponentPrivate->cam2aSettings.ae.exposure_time = 30000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;*/
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        case OMX_WhiteBalControlTungsten:
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
        case OMX_WhiteBalControlFluorescent:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_FLUORESCENT;
            /*pComponentPrivate->cam2aSettings.ae.exposure_time = 30000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;*/
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        case OMX_WhiteBalControlIncandescent:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_INCANDESCENT;
            /*pComponentPrivate->cam2aSettings.ae.exposure_time = 30000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;*/
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        case OMX_WhiteBalControlFlash:
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
        case OMX_WhiteBalControlHorizon:
            pComponentPrivate->cam2aSettings.awb.mode = WHITE_BALANCE_MODE_WB_HORIZON;
            /*pComponentPrivate->cam2aSettings.ae.exposure_time = 30000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;*/
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        default :
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
    }
    
    OMX_CAMERA_PRINTF_L6("WriteSettings: Set values for 2A");
    pComponentPrivate->cameraAlg->WriteSettings(pComponentPrivate->cameraAlg->pPrivateHandle,
                                                &(pComponentPrivate->cam2aSettings));
    
    /*Confirm that new setting is save properly in 2A thread*/
    eError = GetCameraWhiteBalance(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraWhiteBalance");
    OMX_CAMERA_CHECK_COND(eWhiteBalControl != pComponentPrivate->sAWB.eWhiteBalControl,
                          eError,
                          OMX_ErrorHardware,
                          "Unable to verify new white balance setting")
                                                
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * SetExposure() 
  *
  * Set WhiteBalance.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraExposure(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_EXPOSURECONTROLTYPE eExposureControl;
    
    pComponentPrivate->cameraAlg->ReadSettings(pComponentPrivate->cameraAlg->pPrivateHandle, 
                                               &(pComponentPrivate->cam2aSettings));
    
    eExposureControl = pComponentPrivate->sAE.eExposureControl;
    switch (eExposureControl)
    {
        case OMX_ExposureControlOff:
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_MANUAL;
            pComponentPrivate->cam2aSettings.ae.exposure_time = 20000;
            pComponentPrivate->cam2aSettings.ae.gain = 0x40;
            break;
        case OMX_ExposureControlAuto:
            pComponentPrivate->cam2aSettings.ae.mode = EXPOSURE_MODE_EXP_AUTO;
            break;
        default :
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
    }
    
    OMX_CAMERA_PRINTF_L6("WriteSettings");
    pComponentPrivate->cameraAlg->WriteSettings(pComponentPrivate->cameraAlg->pPrivateHandle,
                                                &(pComponentPrivate->cam2aSettings));
    
    /*Confirm that new setting is save properly in 2A thread*/
    eError = GetCameraExposure(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError, "Error at GetCameraExposure");
    OMX_CAMERA_CHECK_COND(eExposureControl != pComponentPrivate->sAE.eExposureControl, 
                          eError,
                          OMX_ErrorHardware,
                          "Unable to verify new exposure setting")
                                                
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}
#endif

#ifdef __CAMERA_VSTAB__
/*-------------------------------------------------------------------*/
/**
  * SetCameraFrameStabilisation() 
  *
  * Turns on or off VSTAB
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraFrameStabilisation(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    CAMERA_PORT_TYPE *pPreviewPort              = NULL;
    
    pPreviewPort = &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]);
    if(pComponentPrivate->sVSTAB.bStab == OMX_TRUE)
    {
        /*Initialize VSTAB*/
        pComponentPrivate->sVSTAB_Context.iInitialized = vsFalse;
        pComponentPrivate->sVSTAB_Context.iVStabMemoryContainer = NULL;
        pComponentPrivate->sVSTAB_Context.iVStabMemory = NULL;
        pComponentPrivate->sVSTAB_Context.iVStabMemorySize = 0;
        pComponentPrivate->sVSTAB_Context.iVStabConfigParams.myInFrame.x =0;
        pComponentPrivate->sVSTAB_Context.iVStabConfigParams.myInFrame.y =0;
        pComponentPrivate->sVSTAB_Context.iVStabConfigParams.myOutFrame.x =0;
        pComponentPrivate->sVSTAB_Context.iVStabConfigParams.myOutFrame.y =0;
        
        /*init vstab lib*/
        pComponentPrivate->sVSTAB_Params.myOutFrame.x = pPreviewPort->pPortDef.format.video.nFrameWidth;
        pComponentPrivate->sVSTAB_Params.myOutFrame.y = pPreviewPort->pPortDef.format.video.nFrameHeight;
        
        switch (pPreviewPort->pPortDef.format.video.nFrameWidth)
        {
			
			case 992:
				pComponentPrivate->sVSTAB_Params.myInFrame.x = 1152;
				break;
			case 982:
				pComponentPrivate->sVSTAB_Params.myInFrame.x = 1136;
				break;
			case 976:
				pComponentPrivate->sVSTAB_Params.myInFrame.x = 1136;
				break;	
			 case 864:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 992;
                break;
			case 854:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 992;
                break;	
            case 848:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 992;
                break;
            case 720:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 832;
                break;
            case 640:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 736;
                break;
            case 352:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 416;
                break;
            case 320:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 368;
                break;
            case 176:
                pComponentPrivate->sVSTAB_Params.myInFrame.x = 208;
        }
        
        switch (pPreviewPort->pPortDef.format.video.nFrameHeight)
        {
            case 576:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 672;
                break;
			case 560:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 656;
                break;
            case 480:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 560;
                break;
            case 288:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 336;
                break;
            case 240:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 288;
                break;
            case 144:
                pComponentPrivate->sVSTAB_Params.myInFrame.y = 176;
        }
        
        OMX_CAMERA_PRINTF_L7("Input Frame %d x %d",
                pComponentPrivate->sVSTAB_Params.myInFrame.x,
                pComponentPrivate->sVSTAB_Params.myInFrame.y);
        OMX_CAMERA_PRINTF_L7("Output Frame %d x %d",
               pComponentPrivate->sVSTAB_Params.myOutFrame.x,
               pComponentPrivate->sVSTAB_Params.myOutFrame.y);
        
        pPreviewPort->pPortDef.format.video.nFrameWidth = pComponentPrivate->sVSTAB_Params.myInFrame.x;
        pPreviewPort->pPortDef.format.video.nFrameHeight = pComponentPrivate->sVSTAB_Params.myInFrame.y;

#ifdef __CAMERA_2A__ 
        OMX_CAMERA_STOP_2A(pComponentPrivate);
#endif
        OMX_CAMERA_TURN_OFF_STREAMING(pComponentPrivate);
        /* Set new widht & Height to Sensor */
        bKernelBufRequested = 0;
        eError = InitCameraPreview(pComponentPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError,"Setting new resolution to Camera for VSTAB");
        OMX_CAMERA_PRINTF_L7("New resution set to Camera for VSTAB");
        /* Set default sensor frame rate */
        eError = SetCameraSensorModeType(pComponentPrivate);
        OMX_CAMERA_IF_ERROR_BAIL(eError, "Error SetCameraSensorModeType");
        if(pComponentPrivate->eCurrentState == OMX_StateExecuting)
        {
            OMX_CAMERA_TURN_ON_STREAMING(pComponentPrivate);
#ifdef __CAMERA_2A__
            OMX_CAMERA_START_2A(pComponentPrivate);
#endif
        }
        
        pComponentPrivate->nVSTAB_Flags |= VS_CFG_DEFAULT;
        
        OMX_CAMERA_PRINTF_L7("vStabLib_Initialize");
        eError = vStabLib_Initialize(&(pComponentPrivate->sVSTAB_Context),
                                     &(pComponentPrivate->sVSTAB_Params),
                                     pComponentPrivate->nVSTAB_Flags);
        if (eError != vs_err_okay)
        {
            OMX_CAMERA_SET_ERROR_BAIL(eError,OMX_ErrorHardware,"Error initializing VStab\n");
        }
        OMX_CAMERA_PRINTF_L7("vStabLib_Initialize Done");
    }
    else
    {
        //close vstab lib
        OMX_CAMERA_PRINTF_L7("vStabLib_Close: Closing VSTAB");
        vStabLib_Close(&(pComponentPrivate->sVSTAB_Context));
    }
    
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}/*End of SetCameraFrameStabilisation*/
#endif

/*-------------------------------------------------------------------*/
/**
  * SetCameraSensorModeType() 
  *
  * Set Camera Sensor Configuration.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraSensorModeType(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_streamparm sCameraParams;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Set camera framerate */
    sCameraParams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sCameraParams.parm.capture.timeperframe.numerator = 1;
    sCameraParams.parm.capture.timeperframe.denominator = pComponentPrivate->sSensorMode.nFrameRate;

    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_S_PARM, V4L2_BUF_TYPE_VIDEO_CAPTURE) Set fps to %d",
                         sCameraParams.parm.capture.timeperframe.denominator);
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_S_PARM,
                                      &sCameraParams,
                                      pComponentPrivate), 
                          eError, 
                          OMX_ErrorHardware, 
                          "SetCameraSensorModeType failed\n");
    
                          
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * GetCameraSensorModeType() 
  *
  * Set Camera Sensor Configuration.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraSensorModeType(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_streamparm sCameraParams;
    struct v4l2_format sFrameFormat;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);

    /* Set camera framerate */
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_G_PARM, V4L2_BUF_TYPE_VIDEO_CAPTURE)");
    sCameraParams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_G_PARM,
                                      &sCameraParams,
                                      pComponentPrivate), 
                          eError, 
                          OMX_ErrorHardware, 
                          "SetCameraSensorModeType failed\n");
    OMX_CAMERA_PRINTF_L2("Time per frame %d/%d s",
                         sCameraParams.parm.capture.timeperframe.numerator,
                         sCameraParams.parm.capture.timeperframe.denominator);
    
    pComponentPrivate->sSensorMode.nFrameRate = sCameraParams.parm.capture.timeperframe.denominator;
    
    OMX_CAMERA_PRINTF_L2("ioctlCamera(VIDIOC_G_FTM, V4L2_BUF_TYPE_VIDEO_CAPTURE)");
    sFrameFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    OMX_CAMERA_CHECK_COND(ioctlCamera(pComponentPrivate->nCameraDev,
                                      VIDIOC_G_FMT,
                                      &sFrameFormat,
                                      pComponentPrivate) < 0, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_G_FMT");
    OMX_CAMERA_PRINTF_L2("Resolution width:%d height:%d",
                         sFrameFormat.fmt.pix.width,
                         sFrameFormat.fmt.pix.height);
    
    //pComponentPrivate->sSensorMode.sFrameSize.nWidth = sFrameFormat.fmt.pix.width;
    //pComponentPrivate->sSensorMode.sFrameSize.nHeight = sFrameFormat.fmt.pix.height;
                          
    /* Function executed properly */
    eError = OMX_ErrorNone;  
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * SetCameraScaleWindow() 
  *
  * Set crop window.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraScaleWindow(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                                   CAMERA_PORT_TYPE *pPortType)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef              = NULL;
    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    pPortDef = &(pPortType->pPortDef);
    
    switch (pComponentPrivate->sScaleFactor.xWidth)
    {
        case FACTOR_1X:
            pComponentPrivate->sCropWindow.nWidth = pPortDef->format.video.nFrameWidth;
            pComponentPrivate->sCropWindow.nHeight = pPortDef->format.video.nFrameHeight;
            break;
        case FACTOR_2X:
            pComponentPrivate->sCropWindow.nWidth = pPortDef->format.video.nFrameWidth/2;
            pComponentPrivate->sCropWindow.nHeight = pPortDef->format.video.nFrameHeight/2;
            break;
        case FACTOR_3X:
            pComponentPrivate->sCropWindow.nWidth = pPortDef->format.video.nFrameWidth/3;
            pComponentPrivate->sCropWindow.nHeight = pPortDef->format.video.nFrameHeight/3;
            break;
        case FACTOR_4X:
            pComponentPrivate->sCropWindow.nWidth = pPortDef->format.video.nFrameWidth/4;
            pComponentPrivate->sCropWindow.nHeight = pPortDef->format.video.nFrameHeight/4;
            break;
        default:
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorUnsupportedSetting,
                                      "Unsupported Index\n");
    }
    
    pComponentPrivate->sCropWindow.nLeft = (pPortDef->format.video.nFrameWidth - pComponentPrivate->sCropWindow.nWidth)/2;
    pComponentPrivate->sCropWindow.nTop = (pPortDef->format.video.nFrameHeight - pComponentPrivate->sCropWindow.nHeight)/2;
   
    eError = SetCameraCropWindow(pComponentPrivate);
    OMX_CAMERA_IF_ERROR_BAIL(eError,
                             "Error while setting crop window !");
   
    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * SetCameraCropWindow() 
  *
  * Set crop window.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraCropWindow(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_CAMERA_CHECK_CMD(pComponentPrivate, OMX_TRUE, OMX_TRUE);
    
    int cfd = pComponentPrivate->nCameraDev;
    int left = pComponentPrivate->sCropWindow.nLeft;
    int top = pComponentPrivate->sCropWindow.nTop;
    int width = pComponentPrivate->sCropWindow.nWidth;
    int height = pComponentPrivate->sCropWindow.nHeight;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    int ret;

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:CCRP");
    ret = ioctlCamera (cfd, VIDIOC_CROPCAP, &cropcap, pComponentPrivate);
    if (ret != 0) 
    {
        perror("VIDIOC_CROPCAP");
    return -1;
    }
  
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:GCRP");
    ret = ioctlCamera (cfd, VIDIOC_G_CROP, &crop, pComponentPrivate);
    if (ret != 0) 
    {
        perror("VIDIOC_G_CROP");
        return -1;
    }

    crop.c.left = left;
    crop.c.top = top;
    crop.c.width = width;
    crop.c.height = height;
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:SCRP");
    ret = ioctlCamera (cfd, VIDIOC_S_CROP, &crop, pComponentPrivate);
    if (ret != 0) 
    {
        perror("VIDIOC_S_CROP");
        return -1;
    }
  
    /* read back */
    __PERF_SYS_CMD(pComponentPrivate, Sending, "CAM:GCRP");
    ret = ioctlCamera (cfd, VIDIOC_G_CROP, &crop, pComponentPrivate);
    __PERF_SYS_CMD(pComponentPrivate, Sent, "CAM:GCRP");
    if (ret != 0) 
    {
        perror("VIDIOC_G_CROP");
        return -1;
    }

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * SetCameraParameters() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE SetCameraParameters(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    int fd = pComponentPrivate->nCameraDev;
    int ret = 0;

    ret = ioctlCamera(fd,
                VIDIOC_S_CROP, 
                &(pComponentPrivate->sCameraDriverParameters.sStillImageCrop),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sStillImageCrop failed");
                                   
    ret = ioctlCamera(fd,
                VIDIOC_S_FMT, 
                &(pComponentPrivate->sCameraDriverParameters.sStillImageFormat),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sStillImageFormat failed");

    ret = ioctlCamera(fd,
                VIDIOC_S_CROP,
                &(pComponentPrivate->sCameraDriverParameters.sVideoCrop),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sVideoCrop failed");
                        
    ret = ioctlCamera(fd,
                VIDIOC_S_FMT,
                &(pComponentPrivate->sCameraDriverParameters.sVideoFormat),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sVideoFormat failed");


    ret = ioctlCamera(fd,
                VIDIOC_S_CROP,
                &(pComponentPrivate->sCameraDriverParameters.sVideoOverlayCrop),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sVideoOverlayCrop failed");
                        
    ret = ioctlCamera(fd,
                VIDIOC_S_FMT,
                &(pComponentPrivate->sCameraDriverParameters.sVideoOverlayFormat),
                pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "sVideoOverlayFormat failed");

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * GetCameraParameters() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraParameters(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    int ret = 0;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    struct v4l2_crop crop;
    struct v4l2_format format;
    struct v4l2_cropcap cropcap;
    
    int fd = pComponentPrivate->nCameraDev;

    /* get the crop info of the still picture capture */
    cropcap.type = V4L2_BUF_TYPE_STILL_CAPTURE;
    crop.type    = V4L2_BUF_TYPE_STILL_CAPTURE;
    format.type  = V4L2_BUF_TYPE_STILL_CAPTURE;

    ret = ioctlCamera(fd, VIDIOC_CROPCAP, &cropcap, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_CROPCAP failed");

    ret = ioctlCamera(fd, VIDIOC_G_CROP, &crop, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_G_CROP failed");

    ret = ioctlCamera(fd, VIDIOC_G_FMT, &format, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_G_FMT failed");


    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sStillImageCropcap),
                      &cropcap,
                      sizeof(cropcap),
                      eError);
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sStillImageCrop),
                      &crop,
                      sizeof(crop),
                      eError);
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sStillImageFormat),
                      &format,
                      sizeof(format),
                      eError);
    
    /* get the crop info of the video picture capture */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctlCamera (fd, VIDIOC_CROPCAP, &cropcap, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_CROPCAP failed");
    ret = ioctlCamera (fd, VIDIOC_G_CROP, &crop, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_CROPCAP failed");
                          
    ret = ioctlCamera(fd, VIDIOC_G_FMT, &format, pComponentPrivate);
    OMX_CAMERA_CHECK_COND(ret == -1, 
                          eError, 
                          OMX_ErrorHardware, 
                          "VIDIOC_CROPCAP failed");

    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sVideoCropcap),
                      &cropcap,
                      sizeof(cropcap),
                      eError);
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sVideoCrop),
                      &crop,
                      sizeof(crop),
                      eError);
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sVideoFormat),
                      &format,
                      sizeof(format),
                      eError);

    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sVideoOverlayCrop),
                      &crop,
                      sizeof(crop),
                      eError);
    OMX_CAMERA_MEMCPY(&(pComponentPrivate->sCameraDriverParameters.sVideoOverlayFormat),
                      &format,
                      sizeof(format),
                      eError);

    /* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
  return eError;
}

#ifdef __CAMERA_2A__
/*-------------------------------------------------------------------*/
/**
  * GetWhiteBalance() 
  *
  * Get WhiteBalance.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraWhiteBalance(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    
    OMX_CAMERA_PRINTF_L6("ReadSettings");
    pComponentPrivate->cameraAlg->ReadSettings(pComponentPrivate->cameraAlg->pPrivateHandle, 
                                               &(pComponentPrivate->cam2aSettings));
    
    switch (pComponentPrivate->cam2aSettings.awb.mode)
    {
        case WHITE_BALANCE_MODE_WB_MANUAL:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlOff;
            break;
        case WHITE_BALANCE_MODE_WB_AUTO:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlAuto;
            break;
        case WHITE_BALANCE_MODE_WB_DAYLIGHT:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlSunLight;
            break;
        case WHITE_BALANCE_MODE_WB_CLOUDY:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlShade;
            break;
        case WHITE_BALANCE_MODE_WB_TUNGSTEN:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlIncandescent;
            break;
        case WHITE_BALANCE_MODE_WB_OFFICE:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlFluorescent;
            break;
        case WHITE_BALANCE_MODE_WB_HORIZON:
            pComponentPrivate->sAWB.eWhiteBalControl = OMX_WhiteBalControlHorizon;
            break;
        default :
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorHardware,
                                      "Error at Get setting of whitebalance\n");
    } 
                                                
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}



/*-------------------------------------------------------------------*/
/**
  * GetExposure() 
  *
  * Set WhiteBalance.
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE GetCameraExposure(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    
    OMX_CAMERA_PRINTF_L6("ReadSettings");
    pComponentPrivate->cameraAlg->ReadSettings(pComponentPrivate->cameraAlg->pPrivateHandle, 
                                               &(pComponentPrivate->cam2aSettings));
    
    switch (pComponentPrivate->cam2aSettings.ae.mode)
    {
        case EXPOSURE_MODE_EXP_MANUAL:
            pComponentPrivate->sAE.eExposureControl  = OMX_ExposureControlOff;
            break;
        case EXPOSURE_MODE_EXP_AUTO:
            pComponentPrivate->sAE.eExposureControl = OMX_ExposureControlAuto;
            break;
        default :
            OMX_CAMERA_SET_ERROR_BAIL(eError, 
                                      OMX_ErrorHardware,
                                      "Error at Get setting of exposure ");
    }
    
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
    return eError;
}

#endif

/*-------------------------------------------------------------------*/
/**
  * FlushDataPipes() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE FlushDataPipes(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    /* Flush StillBufPipe */
    OMX_CAMERA_FLUSH_PIPE_BUFFERS(pComponentPrivate->nStillBufPipe[0],
                                  &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT]));
    
    /* Flush PreviewBufPipe */
    OMX_CAMERA_FLUSH_PIPE_BUFFERS(pComponentPrivate->nPreviewBufPipe[0],
                                  &(pComponentPrivate->sCompPorts[OMXCAM_PREVIEW_PORT]));

    /* Function executed properly */
    eError = OMX_ErrorNone;
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * ReturnBuffers() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE ReturnBuffers(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                            CAMERA_PORT_TYPE *pPortType)
{
    OMX_U32 nCount = 0;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    
    /* If Port is tunneled */
    for (nCount = 0; nCount < pPortType->pPortDef.nBufferCountActual ; nCount++)
    {
		OMX_CAMERA_PRINTF_L3("pCameraBuffer[%d]: %p",nCount, pPortType->pCameraBuffer[nCount]);
        if(pPortType->hTunnelComponent != NULL)
        {
            if(pPortType->pCameraBuffer[nCount]->eBufferOwner != BUFFER_WITH_TUNNELEDCOMP)
            {
				OMX_CAMERA_PRINTF_L3("Send Tunneled Component Buffer [%d]",
                    pPortType->pCameraBuffer[nCount]->nIndex);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pPortType->pCameraBuffer[nCount]->pBufHeader,pBuffer),
                                  PREF(pPortType->pCameraBuffer[nCount]->pBufHeader,nFilledLen),
                                  PERF_ModuleLLMM);
#endif
                eError = OMX_EmptyThisBuffer(pPortType->hTunnelComponent, 
                                             pPortType->pCameraBuffer[nCount]->pBufHeader);
                OMX_CAMERA_IF_ERROR_BAIL(eError,
                                         "OMX_EmptyThisBuffer Failed\n");
OMX_CAMERA_PRINTF_L3("pPortType->pCameraBuffer[%d]: 0x%08x\n", nCount, pPortType->pCameraBuffer[nCount]);
                pPortType->pCameraBuffer[nCount]->eBufferOwner = BUFFER_WITH_TUNNELEDCOMP;
            }
        }
        else
        {
            if(pPortType->pCameraBuffer[nCount]->eBufferOwner != BUFFER_WITH_CLIENT)
            {
	            OMX_CAMERA_PRINTF_L3("Send to IL Client Buffer [%d]",
                    pPortType->pCameraBuffer[nCount]->nIndex);
#ifdef __PERF_INSTRUMENTATION__
                PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                                  PREF(pPortType->pCameraBuffer[nCount]->pBufHeader,pBuffer),
                                  PREF(pPortType->pCameraBuffer[nCount]->pBufHeader,nFilledLen),
                                  PERF_ModuleHLMM);
#endif
                pComponentPrivate->sCallBackInfo.FillBufferDone(pComponentPrivate->pHandle,
                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                pPortType->pCameraBuffer[nCount]->pBufHeader);
                pPortType->pCameraBuffer[nCount]->eBufferOwner = BUFFER_WITH_CLIENT;
            }
        }
        pPortType->pCameraBuffer[nCount]->bFirstTime = OMX_TRUE;
        
        if (pPortType->pPortDef.nPortIndex == OMXCAM_PREVIEW_PORT)
        {
            pPortType->pCameraBuffer[nCount]->bPreviewFillThisBuffer = OMX_FALSE;
        }
        else if(pPortType->pPortDef.nPortIndex == OMXCAM_CAPTURE_PORT)
        {
            pPortType->pCameraBuffer[nCount]->bCaptureFillThisBuffer = OMX_FALSE;
        }
    }
    
/* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
   return eError;
} /* End of ReturnBuffers */


/*-------------------------------------------------------------------*/
/**
  * WaitForBuffers() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE WaitForBuffers(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                             CAMERA_PORT_TYPE *pPortType)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer  = NULL;
    OMX_U32 nOutputBufferCount = pPortType->nBufferCount;
    OMX_U32 nCount;
    OMX_U32 nBuffersRemaining;
   
    do
    {
        nBuffersRemaining = 0;
        pthread_mutex_lock(&(pComponentPrivate->mFillThisBufferMutex));
        for(nCount = 0; nCount < nOutputBufferCount; ++nCount)
        {
            pCameraBuffer = pPortType->pCameraBuffer[nCount];
            OMX_CAMERA_PRINT_BUFFER(pComponentPrivate);
            if (pCameraBuffer->eBufferOwner == BUFFER_WITH_TUNNELEDCOMP)
            {
                OMX_CAMERA_PRINTF_L3("Camera is supplier, wait for buffer [%d]: 0x%08x\n", pCameraBuffer->nIndex, pCameraBuffer);
                nBuffersRemaining++;
            }
            pCameraBuffer->bFirstTime = OMX_TRUE;
        }
			OMX_CAMERA_PRINTF_L3("Buffers Remaining = %d\n", nBuffersRemaining);
        
        if(nBuffersRemaining)
        {
            /*MUTEX: Wait for Signal of FillThisBuffer*/
            int nRet = 0x0;
            struct timespec  ts;
            struct timeval sTime;
            struct timezone sTimeZone;  
            gettimeofday(&sTime, &sTimeZone);
            ts.tv_sec = sTime.tv_sec;
            ts.tv_sec += OMXCAM_MAX_WAITCOUNT;
            nRet = pthread_cond_timedwait(&(pComponentPrivate->sConditionFillThisBuffer),
                                          &(pComponentPrivate->mFillThisBufferMutex),
                                          &ts);
            if (nRet == ETIMEDOUT)
            {
                OMX_CAMERA_PRINTF_ERROR("OMX Camera: Warning : %d Buffers didn't return from tunneled Component\n",
                       (int) nBuffersRemaining);
                pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
                eError = OMX_ErrorNone;
                goto OMX_CAMERA_BAIL_CMD;
            }
			OMX_CAMERA_PRINTF_L3("BUFFER RECEIVED\n");
        }
        pthread_mutex_unlock(&(pComponentPrivate->mFillThisBufferMutex));
    } while (nBuffersRemaining);

/* Function executed properly */
    eError = OMX_ErrorNone;
OMX_CAMERA_BAIL_CMD:
   return eError;
} /* End of WaitForBuffers */

/*-------------------------------------------------------------------*/
/**
  * CameraQueueStatus() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_U32 CameraQueueStatus(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                          CAMERA_PORT_TYPE* pPortType)
{
    OMX_U32 nCountBuffer;
    OMX_U32 nBuffers = 0;
    
    OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
    for(nCountBuffer = 0;
        nCountBuffer < pPortType->pPortDef.nBufferCountActual;
        nCountBuffer++)
    {
        if(pPortType->pCameraBuffer[nCountBuffer] != NULL)
        {
            if(pPortType->pCameraBuffer[nCountBuffer]->bPipe)
            {
                nBuffers++;
            }
        }
    }

    pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
    return nBuffers;
}

/*-------------------------------------------------------------------*/
/**
  * CameraQueueStatus() 
  *
  * Temporal function. Workaround
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_U32 CameraQueueStatus_Still(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                          CAMERA_PORT_TYPE* pPortType)
{
    OMX_U32 nCountBuffer;
    OMX_U32 nBuffers = 0;
    
    OMX_CAMERA_PRINTF_L4("PipeMutex: Check Lock");
    pthread_mutex_lock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Locked");
    for(nCountBuffer = 0;
        nCountBuffer < pPortType->pPortDef.nBufferCountActual;
        nCountBuffer++)
    {
        if(pPortType->pCameraBuffer[nCountBuffer] != NULL)
        {
            if(pComponentPrivate->StillBuffer[nCountBuffer].bPipe)
            {
                nBuffers++;
            }
        }
    }

    pthread_mutex_unlock(&(pComponentPrivate->mPipeMutex));
    OMX_CAMERA_PRINTF_L4("PipeMutex: Unlock");
    return nBuffers;
}

#ifdef __CAMERA_IPP__
/*-------------------------------------------------------------------*/
/**
  * InitIPP() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE InitIPP(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
				int IPP_error = 0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef =
                    &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pPortDef);
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sending, 0x495050, 0x2);
    OMX_CAMERA_PRINTF_L5("IPP_Create");
    pComponentPrivate->pIPP.hIPP = IPP_Create();
				
    if(pComponentPrivate->pIPP.hIPP ==NULL)
				{
							printf("ERROR in IPP_Create()\n");
				}
    OMX_CAMERA_PRINTF_L5("IPP Handle: %p",pComponentPrivate->pIPP.hIPP);

    pComponentPrivate->pIPP.ippconfig.numberOfAlgos=5;
    pComponentPrivate->pIPP.ippconfig.orderOfAlgos[0]=IPP_START_ID;
    pComponentPrivate->pIPP.ippconfig.orderOfAlgos[1]=IPP_YUVC_422iTO422p_ID;
    pComponentPrivate->pIPP.ippconfig.orderOfAlgos[2]=IPP_CRCBS_ID;
    pComponentPrivate->pIPP.ippconfig.orderOfAlgos[3]=IPP_EENF_ID;
    pComponentPrivate->pIPP.ippconfig.orderOfAlgos[4]=IPP_YUVC_422pTO422i_ID;
    pComponentPrivate->pIPP.ippconfig.isINPLACE=OMX_TRUE;
    
    pComponentPrivate->pIPP.CRCBptr.size = sizeof(IPP_CRCBSAlgoCreateParams);
    pComponentPrivate->pIPP.CRCBptr.maxWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.CRCBptr.maxHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.CRCBptr.errorCode = 0;
    
    pComponentPrivate->pIPP.YUVCcreate.size = sizeof(IPP_YUVCAlgoCreateParams);
    pComponentPrivate->pIPP.YUVCcreate.maxWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.YUVCcreate.maxHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.YUVCcreate.errorCode = 0;
    
    OMX_CAMERA_PRINTF_L5("IPP_SetProcessingConfiguration");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_SetProcessingConfiguration(pComponentPrivate->pIPP.hIPP,
                                              pComponentPrivate->pIPP.ippconfig);

				if(IPP_error!=0)
				{	
					   printf("Error in IPP_setProcessingConfiguration\n");		
				}

    OMX_CAMERA_PRINTF_L5("IPP_SetAlgoConfig");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_SetAlgoConfig(pComponentPrivate->pIPP.hIPP,
                                  IPP_CRCBS_CREATEPRMS_CFGID,
                                  &(pComponentPrivate->pIPP.CRCBptr));
				if(IPP_error!=0)
				{	
					   printf("Error in IPP_setAlgoConfig\n");		
				}
    
    OMX_CAMERA_PRINTF_L5("IPP_SetAlgoConfig");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_SetAlgoConfig(pComponentPrivate->pIPP.hIPP,
                                 IPP_YUVC_422TO420_CREATEPRMS_CFGID,
                                 &(pComponentPrivate->pIPP.YUVCcreate));    
				if(IPP_error!=0)
				{	
					   printf("Error in IPP_setAlgoConfig\n");		
				}
	
    OMX_CAMERA_PRINTF_L5("IPP_SetAlgoConfig");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_SetAlgoConfig(pComponentPrivate->pIPP.hIPP,
                                 IPP_YUVC_420TO422_CREATEPRMS_CFGID,
                                 &(pComponentPrivate->pIPP.YUVCcreate));

    if(IPP_error!=0)
				{	
					   printf("Error in IPP_setAlgoConfig\n");		
				}
    
    OMX_CAMERA_PRINTF_L5("IPP_InitializeImagePipe");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_InitializeImagePipe(pComponentPrivate->pIPP.hIPP);
			
    if(IPP_error != 0)
				{	
					   printf("Error in IPP_InitializeImagePipe\n");		
				}
    
    OMX_CAMERA_PRINTF_L5("IPP_StartProcessing");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    IPP_error = IPP_StartProcessing(pComponentPrivate->pIPP.hIPP);
    
    if(IPP_error!=0)
				{	
					   printf("Error in IPP_StartProcessing\n");		
				}
    
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    
    pComponentPrivate->pIPP.iStarInArgs = (IPP_StarAlgoInArgs*)((char*)malloc(sizeof(IPP_StarAlgoInArgs) + BUFF_MAP_PADDING_TEST) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iStarOutArgs = (IPP_StarAlgoOutArgs*)((char*)(malloc(sizeof(IPP_StarAlgoOutArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iCrcbsInArgs = (IPP_CRCBSAlgoInArgs*)((char*)(malloc(sizeof(IPP_CRCBSAlgoInArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iCrcbsOutArgs = (IPP_CRCBSAlgoOutArgs*)((char*)(malloc(sizeof(IPP_CRCBSAlgoOutArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iEenfInArgs = (IPP_EENFAlgoInArgs*)((char*)(malloc(sizeof(IPP_EENFAlgoInArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iEenfOutArgs = (IPP_EENFAlgoOutArgs*)((char*)(malloc(sizeof(IPP_EENFAlgoOutArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iYuvcInArgs1 = (IPP_YUVCAlgoInArgs*)((char*)(malloc(sizeof(IPP_YUVCAlgoInArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iYuvcOutArgs1 = (IPP_YUVCAlgoOutArgs*)((char*)(malloc(sizeof(IPP_YUVCAlgoOutArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iYuvcInArgs2 = (IPP_YUVCAlgoInArgs*)((char*)(malloc(sizeof(IPP_YUVCAlgoInArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.iYuvcOutArgs2 = (IPP_YUVCAlgoOutArgs*)((char*)(malloc(sizeof(IPP_YUVCAlgoOutArgs) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    pComponentPrivate->pIPP.dynEENF = (IPP_EENFAlgoDynamicParams*)((char*)(malloc(sizeof(IPP_EENFAlgoDynamicParams) + BUFF_MAP_PADDING_TEST)) + PADDING_OFFSET_TEST);
    
    /*Set Dynamic Parameter*/
    memcpy(pComponentPrivate->pIPP.dynEENF,
           (void*)&IPPEENFAlgoDynamicParamsArray[0],
           sizeof(IPPEENFAlgoDynamicParamsArray[0]));

    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20002);
    OMX_CAMERA_PRINTF_L5("IPP_SetAlgoConfig");
    IPP_error = IPP_SetAlgoConfig(pComponentPrivate->pIPP.hIPP,
                                  IPP_EENF_DYNPRMS_CFGID,
                                  (void*)pComponentPrivate->pIPP.dynEENF);	
				if(IPP_error!=0)
				{	
					   printf("Error in IPP_setAlgoConfig\n");		
				}
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x2);
                      
    eError = OMX_ErrorNone;
/*OMX_CAMERA_BAIL_CMD:*/
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * DeInitIPP() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE DeInitIPP(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    int IPP_error = 0;
    OMX_CAMERA_PRINTF_L5("IPP_StopProcessing");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sending, 0x495050, 0x5);
    IPP_error = IPP_StopProcessing(pComponentPrivate->pIPP.hIPP);
    if(IPP_error != 0)
    {
       printf("Error in IPP_StopProcessing\n");
    }
    OMX_CAMERA_PRINTF_L5("IPP_DeinitializePipe");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20005);
    IPP_error = IPP_DeinitializePipe(pComponentPrivate->pIPP.hIPP);
    if(IPP_error != 0)
    {
       printf("Error in IPP_DeinitializePipe\n");
    }	
    OMX_CAMERA_PRINTF_L5("IPP_Delete");
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x20005);
    IPP_Delete(&(pComponentPrivate->pIPP.hIPP));   
    
    free(((char*)pComponentPrivate->pIPP.iStarInArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iStarOutArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iCrcbsInArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iCrcbsOutArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iEenfInArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iEenfOutArgs - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iYuvcInArgs1 - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iYuvcOutArgs1 - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iYuvcInArgs2 - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.iYuvcOutArgs2 - PADDING_OFFSET_TEST));
    free(((char*)pComponentPrivate->pIPP.dynEENF - PADDING_OFFSET_TEST));
    
    __PERF_GEN_CMD(pComponentPrivate->pPERFcomp, Sent, 0x495050, 0x5);
    OMX_CAMERA_PRINTF_L5("Terminating IPP");
    
    eError = OMX_ErrorNone;
/*OMX_CAMERA_BAIL_CMD:*/
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  * PopulateArgsIPP() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE PopulateArgsIPP(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef =
                    &(pComponentPrivate->sCompPorts[OMXCAM_CAPTURE_PORT].pPortDef);
    
    OMX_CAMERA_PRINTF_L5("IPP: PopulateArgs ENTER");
    pComponentPrivate->pIPP.iStarInArgs->size = sizeof(IPP_StarAlgoInArgs);
    pComponentPrivate->pIPP.iCrcbsInArgs->size = sizeof(IPP_CRCBSAlgoInArgs);
    pComponentPrivate->pIPP.iEenfInArgs->size = sizeof(IPP_EENFAlgoInArgs);
    pComponentPrivate->pIPP.iYuvcInArgs1->size = sizeof(IPP_YUVCAlgoInArgs);
    pComponentPrivate->pIPP.iYuvcInArgs2->size = sizeof(IPP_YUVCAlgoInArgs);
    
    pComponentPrivate->pIPP.iStarOutArgs->size = sizeof(IPP_StarAlgoOutArgs);
    pComponentPrivate->pIPP.iCrcbsOutArgs->size = sizeof(IPP_CRCBSAlgoOutArgs);
    pComponentPrivate->pIPP.iEenfOutArgs->size = sizeof(IPP_EENFAlgoOutArgs);
    pComponentPrivate->pIPP.iYuvcOutArgs1->size = sizeof(IPP_YUVCAlgoOutArgs);
    pComponentPrivate->pIPP.iYuvcOutArgs2->size = sizeof(IPP_YUVCAlgoOutArgs);
    
    pComponentPrivate->pIPP.iCrcbsInArgs->inputHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.iCrcbsInArgs->inputWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.iCrcbsInArgs->inputChromaFormat = IPP_YUV_422P;

    pComponentPrivate->pIPP.iEenfInArgs->inputChromaFormat = IPP_YUV_422P;
    pComponentPrivate->pIPP.iEenfInArgs->inFullWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.iEenfInArgs->inFullHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.iEenfInArgs->inOffsetV = 0;
    pComponentPrivate->pIPP.iEenfInArgs->inOffsetH = 0;
    pComponentPrivate->pIPP.iEenfInArgs->inputWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.iEenfInArgs->inputHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.iEenfInArgs->inPlace = 0;
    pComponentPrivate->pIPP.iEenfInArgs->NFprocessing = 0;
    
    pComponentPrivate->pIPP.iYuvcInArgs1->inputHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.iYuvcInArgs1->inputWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.iYuvcInArgs1->outputChromaFormat = IPP_YUV_422P;
    pComponentPrivate->pIPP.iYuvcInArgs1->inputChromaFormat = IPP_YUV_422ILE;
    
    pComponentPrivate->pIPP.iYuvcInArgs2->inputHeight = pPortDef->format.image.nFrameHeight;
    pComponentPrivate->pIPP.iYuvcInArgs2->inputWidth = pPortDef->format.image.nFrameWidth;
    pComponentPrivate->pIPP.iYuvcInArgs2->outputChromaFormat = IPP_YUV_422ILE;
    pComponentPrivate->pIPP.iYuvcInArgs2->inputChromaFormat = IPP_YUV_422P;
    
    pComponentPrivate->pIPP.starStatus.size = sizeof(IPP_StarAlgoStatus);
    pComponentPrivate->pIPP.CRCBSStatus.size = sizeof(IPP_CRCBSAlgoStatus);
    pComponentPrivate->pIPP.EENFStatus.size = sizeof(IPP_EENFAlgoStatus);
    
    pComponentPrivate->pIPP.statusDesc.statusPtr[0] = &(pComponentPrivate->pIPP.starStatus);
    pComponentPrivate->pIPP.statusDesc.statusPtr[1] = &(pComponentPrivate->pIPP.CRCBSStatus);
    pComponentPrivate->pIPP.statusDesc.statusPtr[2] = &(pComponentPrivate->pIPP.EENFStatus);
    pComponentPrivate->pIPP.statusDesc.numParams = 3;
    pComponentPrivate->pIPP.statusDesc.algoNum[0] = 0;
    pComponentPrivate->pIPP.statusDesc.algoNum[1] = 1;
    pComponentPrivate->pIPP.statusDesc.algoNum[2] = 2;

    OMX_CAMERA_PRINTF_L5("IPP: PopulateArgs EXIT");
    
    eError = OMX_ErrorNone;
/*OMX_CAMERA_BAIL_CMD:*/
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  * ProcessBufferIPP() 
  *
  * 
  *
  * @param pComponentPrivate component private data structure.
  *
  * @retval OMX_ErrorNone       success, ready to roll
  *         OMX_ErrorHardware   if video driver API fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE ProcessBufferIPP(OMX_CAMERA_COMPONENT_PRIVATE *pComponentPrivate,
                              OMX_CAMERA_COMPONENT_BUFFER *pCameraBuffer)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
				int IPP_error = 0;
    
    pComponentPrivate->pIPP.iInputBufferDesc.numBuffers = 1;
    pComponentPrivate->pIPP.iInputBufferDesc.bufPtr[0] = pCameraBuffer->pBufHeader->pBuffer;
    pComponentPrivate->pIPP.iInputBufferDesc.bufSize[0] = pCameraBuffer->pBufHeader->nAllocLen;
    pComponentPrivate->pIPP.iInputBufferDesc.usedSize[0] = pCameraBuffer->pBufHeader->nAllocLen;
    pComponentPrivate->pIPP.iInputBufferDesc.port[0] = 0;
    pComponentPrivate->pIPP.iInputBufferDesc.reuseAllowed[0] = 0;

    pComponentPrivate->pIPP.iOutputBufferDesc.numBuffers = 1;
    pComponentPrivate->pIPP.iOutputBufferDesc.bufPtr[0] = pCameraBuffer->pBufHeader->pBuffer;
    pComponentPrivate->pIPP.iOutputBufferDesc.bufSize[0] = pCameraBuffer->pBufHeader->nAllocLen;
    pComponentPrivate->pIPP.iOutputBufferDesc.usedSize[0] = pCameraBuffer->pBufHeader->nAllocLen;
    pComponentPrivate->pIPP.iOutputBufferDesc.port[0] = 1;
    pComponentPrivate->pIPP.iOutputBufferDesc.reuseAllowed[0] = 0;
    
    pComponentPrivate->pIPP.iInputArgs.numArgs = 5;/*JJ- be aware of the algos order*/
    pComponentPrivate->pIPP.iInputArgs.argsArray[0] = pComponentPrivate->pIPP.iStarInArgs;
    pComponentPrivate->pIPP.iInputArgs.argsArray[1] = pComponentPrivate->pIPP.iYuvcInArgs1;
    pComponentPrivate->pIPP.iInputArgs.argsArray[2] = pComponentPrivate->pIPP.iCrcbsInArgs;
    pComponentPrivate->pIPP.iInputArgs.argsArray[3] = pComponentPrivate->pIPP.iEenfInArgs;
    pComponentPrivate->pIPP.iInputArgs.argsArray[4] = pComponentPrivate->pIPP.iYuvcInArgs2;
    
    pComponentPrivate->pIPP.iOutputArgs.numArgs = 5;/*JJ- be aware of the algos order*/
    pComponentPrivate->pIPP.iOutputArgs.argsArray[0] = pComponentPrivate->pIPP.iStarOutArgs;
    pComponentPrivate->pIPP.iOutputArgs.argsArray[1] = pComponentPrivate->pIPP.iYuvcOutArgs1;
    pComponentPrivate->pIPP.iOutputArgs.argsArray[2] = pComponentPrivate->pIPP.iCrcbsOutArgs;
    pComponentPrivate->pIPP.iOutputArgs.argsArray[3] = pComponentPrivate->pIPP.iEenfOutArgs;
    pComponentPrivate->pIPP.iOutputArgs.argsArray[4] = pComponentPrivate->pIPP.iYuvcOutArgs2;
    
    OMX_CAMERA_PRINTF_L5("IPP_ProcessImage");
#ifdef __PERF_INSTRUMENTATION__
    PERF_SendingFrame(pComponentPrivate->pPERFcomp,
                      pComponentPrivate->pIPP.iInputBufferDesc.bufPtr[0],
                      pComponentPrivate->pIPP.iInputBufferDesc.usedSize[0],
                      PERF_ModuleAlgorithm);
#endif
    IPP_error = IPP_ProcessImage(pComponentPrivate->pIPP.hIPP,
                                 &(pComponentPrivate->pIPP.iInputBufferDesc),
                                 &(pComponentPrivate->pIPP.iInputArgs),
                                 NULL,
                                 &(pComponentPrivate->pIPP.iOutputArgs));
    if(IPP_error != 0)
    {
       printf("Error in IPP_ProcessImage\n");
    }
#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERFcomp,
                       pComponentPrivate->pIPP.iInputBufferDesc.bufPtr[0],
                       pComponentPrivate->pIPP.iInputBufferDesc.usedSize[0],
                       PERF_ModuleAlgorithm);
#endif
   
    OMX_CAMERA_PRINTF_L5("IPP_ProcessImage Done");
    
    eError = OMX_ErrorNone;
/*OMX_CAMERA_BAIL_CMD:*/
    return eError;
}
#endif



