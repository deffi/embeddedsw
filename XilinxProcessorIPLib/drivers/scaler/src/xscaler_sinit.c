/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/**
 *
 * @file xscaler_sinit.c
* @addtogroup scaler_v7_0
* @{
* @details
 *
 * This file contains static initialization methods for Xilinx MVI Video Scaler
 * device driver.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver    Who  Date     Changes
 * ----- ----  -------- -------- -------------------------------------------------------
 * 1.00a  xd   05/14/09 First release
 * 2.00a  xd   12/14/09 Updated doxygen document tags
 * </pre>
 *
 *****************************************************************************/

/***************************** Include Files *********************************/

#include "xscaler.h"
#include "xparameters.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

/*****************************************************************************/
/**
 * XScaler_LookupConfig returns a reference to an XScaler_Config structure
 * based on the unique device id, <i>DeviceId</i>. The return value will refer
 * to an entry in the device configuration table defined in the xscaler_g.c
 * file.
 *
 * @param	DeviceId is the unique device ID of the device for the lookup
 *		operation.
 *
 * @return	XScaler_LookupConfig returns a reference to a config record in
 *		the configuration table (in xscaler_g.c) corresponding to
 *		<i>DeviceId</i>, or NULL if no match is found.
 *
 *****************************************************************************/
XScaler_Config *XScaler_LookupConfig(u16 DeviceId)
{
	extern XScaler_Config XScaler_ConfigTable[];
	XScaler_Config *CfgPtr = NULL;
	int i;

	for (i = 0; i < XPAR_XSCALER_NUM_INSTANCES; i++) {
		if (XScaler_ConfigTable[i].DeviceId == DeviceId) {
			CfgPtr = &XScaler_ConfigTable[i];
			break;
		}
	}

	return (CfgPtr);
}
/** @} */
