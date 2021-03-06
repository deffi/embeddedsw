/******************************************************************************
*
* Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
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

/*****************************************************************************/
/**
 *
 * @file xfsbl_handoff.c
 *
 * This is the main file which contains handoff code for the FSBL.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date        Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.00  kc   10/21/13 Initial release
 *
 * </pre>
 *
 * @note
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "xfsbl_hw.h"
#include "xil_cache.h"
#include "psu_init.h"
#include "xfsbl_main.h"
#include "xfsbl_image_header.h"

/************************** Constant Definitions *****************************/
#define XFSBL_CPU_POWER_UP		(0x1U)
#define XFSBL_CPU_SWRST			(0x2U)

/**
 * Aarch32 or Aarch64 CPU definitions
 */
#define APU_CONFIG_0_AA64N32_MASK_CPU0 (0x1U)
#define APU_CONFIG_0_AA64N32_MASK_CPU1 (0x2U)
#define APU_CONFIG_0_AA64N32_MASK_CPU2 (0x4U)
#define APU_CONFIG_0_AA64N32_MASK_CPU3 (0x8U)

#define OTHER_CPU_HANDOFF				(0x0U)
#define A53_0_64_HANDOFF_TO_A53_0_32	(0x1U)
#define A53_0_32_HANDOFF_TO_A53_0_64	(0x2U)

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
#define XFSBL_IVT_ADDRESS		(0xFFFF0000U)
#define XFSBL_HANDOFF_ADDR_PTR		(0xFFFFFF00U)
#define XFSBL_HANDOFF_FLAG_ADDR		(0xFFFFFF80U)
/************************** Function Prototypes ******************************/

static u32 XFsbl_SetCpuPwrSettings (u32 CpuSettings, u32 Flags);
static void XFsbl_UpdateResetVector (u64 HandOffAddress, u32 CpuSettings,
		u32 HandoffType);
static void XFsbl_CopyIVT(u32 CpuSettings, u32 HandoffType);
static int XFsbl_Is32BitCpu(u32 CpuSettings);

/**
 * Functions defined in xfsbl_handoff.S
 */
extern void XFsbl_Exit(PTRSIZE HandoffAddress, u32 Flags);

/************************** Variable Definitions *****************************/
/**
 * Variabled defined in xfsbl_partition_load.c
 */
extern u8 TcmVectorArray[32];
extern u32 TcmSkipLength;
extern PTRSIZE TcmSkipAddress;

int XFsbl_Is32BitCpu(u32 CpuSettings)
{
	int Status;
	u32 CpuId=0U;
        u32 ExecState=0U;

        CpuId = CpuSettings & XIH_PH_ATTRB_DEST_CPU_MASK;
        ExecState = CpuSettings & XIH_PH_ATTRB_A53_EXEC_ST_MASK;

	if ((CpuId == XIH_PH_ATTRB_DEST_CPU_R5_0) ||
	        (CpuId == XIH_PH_ATTRB_DEST_CPU_R5_1) ||
	(CpuId == XIH_PH_ATTRB_DEST_CPU_R5_L)  ||
		(ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32))
	{
		Status = TRUE;
	} else {
		Status = FALSE;
	}

	return Status;
}


/****************************************************************************/
/**
 * This function will set up the settings for the CPU's
 * This can power up the CPU or do a soft reset to the CPU's
 *
 * @param CpuId specifies for which CPU settings should be done
 *
 * @param Flags is used to specify the settings for the CPU
 * 			XFSBL_CPU_POWER_UP - This is used to power up the CPU
 * 			XFSBL_CPU_SWRST - This is used to trigger the reset to CPU
 *
 * @return
 * 		- XFSBL_SUCCESS on successful settings
 * 		- XFSBL_FAILURE
 *
 * @note
 *
 *****************************************************************************/

static u32 XFsbl_SetCpuPwrSettings (u32 CpuSettings, u32 Flags)
{
	u32 RegValue=0U;
	u32 Status=XFSBL_SUCCESS;
	u32 CpuId=0U;
	u32 ExecState=0U;
	u32 PwrStateMask = 0;

	CpuId = CpuSettings & XIH_PH_ATTRB_DEST_CPU_MASK;
	ExecState = CpuSettings & XIH_PH_ATTRB_A53_EXEC_ST_MASK;
	/**
	 * Reset the CPU
	 */
	if ((Flags & XFSBL_CPU_SWRST) != 0U)
	{

		switch(CpuId)
		{

			case XIH_PH_ATTRB_DEST_CPU_A53_0:

			PwrStateMask = PMU_GLOBAL_PWR_STATE_ACPU0_MASK |
				PMU_GLOBAL_PWR_STATE_FP_MASK |
				PMU_GLOBAL_PWR_STATE_L2_BANK0_MASK;

			Status = XFsbl_PowerUpIsland(PwrStateMask);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_A53_0_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_A53_0_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Set to Aarch32 if enabled
			 */
			if (ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)
			{
				RegValue = XFsbl_In32(APU_CONFIG_0);
				RegValue &= ~(APU_CONFIG_0_AA64N32_MASK_CPU0);
				XFsbl_Out32(APU_CONFIG_0, RegValue);
			}

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRF_APB_ACPU_CTRL);
			RegValue |= (CRF_APB_ACPU_CTRL_CLKACT_FULL_MASK |
			             CRF_APB_ACPU_CTRL_CLKACT_HALF_MASK);
			XFsbl_Out32(CRF_APB_ACPU_CTRL, RegValue);

			/**
			 * Release reset
			 */
			RegValue = XFsbl_In32(CRF_APB_RST_FPD_APU);
			RegValue &= ~(CRF_APB_RST_FPD_APU_ACPU0_RESET_MASK |
					CRF_APB_RST_FPD_APU_APU_L2_RESET_MASK |
					CRF_APB_RST_FPD_APU_ACPU0_PWRON_RESET_MASK);
			XFsbl_Out32(CRF_APB_RST_FPD_APU, RegValue);

			break;

		case XIH_PH_ATTRB_DEST_CPU_A53_1:

			PwrStateMask = PMU_GLOBAL_PWR_STATE_ACPU1_MASK |
				PMU_GLOBAL_PWR_STATE_FP_MASK |
				PMU_GLOBAL_PWR_STATE_L2_BANK0_MASK;

			Status = XFsbl_PowerUpIsland(PwrStateMask);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_A53_1_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_A53_1_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Set to Aarch32 if enabled
			 */
			if (ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)
			{
				RegValue = XFsbl_In32(APU_CONFIG_0);
				RegValue &= ~(APU_CONFIG_0_AA64N32_MASK_CPU1);
				XFsbl_Out32(APU_CONFIG_0, RegValue);
			}

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRF_APB_ACPU_CTRL);
			RegValue |= (CRF_APB_ACPU_CTRL_CLKACT_FULL_MASK |
			             CRF_APB_ACPU_CTRL_CLKACT_HALF_MASK);
			XFsbl_Out32(CRF_APB_ACPU_CTRL, RegValue);

			/**
			 * Release reset
			 */
			RegValue = XFsbl_In32(CRF_APB_RST_FPD_APU);
			RegValue &= ~(CRF_APB_RST_FPD_APU_ACPU1_RESET_MASK |
					CRF_APB_RST_FPD_APU_APU_L2_RESET_MASK |
					CRF_APB_RST_FPD_APU_ACPU1_PWRON_RESET_MASK);
			XFsbl_Out32(CRF_APB_RST_FPD_APU, RegValue);

			break;

		case XIH_PH_ATTRB_DEST_CPU_A53_2:

			PwrStateMask = PMU_GLOBAL_PWR_STATE_ACPU2_MASK |
				PMU_GLOBAL_PWR_STATE_FP_MASK |
				PMU_GLOBAL_PWR_STATE_L2_BANK0_MASK;

			Status = XFsbl_PowerUpIsland(PwrStateMask);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_A53_2_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_A53_2_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Set to Aarch32 if enabled
			 */
			if (ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)
			{
				RegValue = XFsbl_In32(APU_CONFIG_0);
				RegValue &= ~(APU_CONFIG_0_AA64N32_MASK_CPU2);
				XFsbl_Out32(APU_CONFIG_0, RegValue);
			}

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRF_APB_ACPU_CTRL);
			RegValue |= (CRF_APB_ACPU_CTRL_CLKACT_FULL_MASK |
			             CRF_APB_ACPU_CTRL_CLKACT_HALF_MASK);
			XFsbl_Out32(CRF_APB_ACPU_CTRL, RegValue);

			/**
			 * Release reset
			 */
			RegValue = XFsbl_In32(CRF_APB_RST_FPD_APU);
			RegValue &= ~(CRF_APB_RST_FPD_APU_ACPU2_RESET_MASK |
					CRF_APB_RST_FPD_APU_APU_L2_RESET_MASK |
					CRF_APB_RST_FPD_APU_ACPU2_PWRON_RESET_MASK);

			XFsbl_Out32(CRF_APB_RST_FPD_APU, RegValue);

			break;

		case XIH_PH_ATTRB_DEST_CPU_A53_3:

			PwrStateMask = PMU_GLOBAL_PWR_STATE_ACPU3_MASK |
				PMU_GLOBAL_PWR_STATE_FP_MASK |
				PMU_GLOBAL_PWR_STATE_L2_BANK0_MASK;

			Status = XFsbl_PowerUpIsland(PwrStateMask);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_A53_3_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_A53_3_POWER_UP\r\n");
				goto END;
			}


			/**
			 * Set to Aarch32 if enabled
			 */
			if (ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)
			{
				RegValue = XFsbl_In32(APU_CONFIG_0);
				RegValue &= ~(APU_CONFIG_0_AA64N32_MASK_CPU3);
				XFsbl_Out32(APU_CONFIG_0, RegValue);
			}

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRF_APB_ACPU_CTRL);
			RegValue |= (CRF_APB_ACPU_CTRL_CLKACT_FULL_MASK |
			             CRF_APB_ACPU_CTRL_CLKACT_HALF_MASK);
			XFsbl_Out32(CRF_APB_ACPU_CTRL, RegValue);

			/**
			 * Release reset
			 */
			RegValue = XFsbl_In32(CRF_APB_RST_FPD_APU);
			RegValue &= ~(CRF_APB_RST_FPD_APU_ACPU3_RESET_MASK |
					CRF_APB_RST_FPD_APU_APU_L2_RESET_MASK |
					CRF_APB_RST_FPD_APU_ACPU3_PWRON_RESET_MASK);

			XFsbl_Out32(CRF_APB_RST_FPD_APU, RegValue);

			break;

		case XIH_PH_ATTRB_DEST_CPU_R5_0:

			Status = XFsbl_PowerUpIsland(PMU_GLOBAL_PWR_STATE_R5_0_MASK);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_R5_0_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_R5_0_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Place R5, TCM's in split mode
			 */
			RegValue = XFsbl_In32(RPU_RPU_GLBL_CNTL);
			RegValue |= (RPU_RPU_GLBL_CNTL_SLSPLIT_MASK);
			RegValue &= ~(RPU_RPU_GLBL_CNTL_TCM_COMB_MASK);
			RegValue &= ~(RPU_RPU_GLBL_CNTL_SLCLAMP_MASK);
			XFsbl_Out32(RPU_RPU_GLBL_CNTL, RegValue);

			/**
			 * Place R5-0 in HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_0_CFG);
			RegValue &= ~(RPU_RPU_0_CFG_NCPUHALT_MASK);
			XFsbl_Out32(RPU_RPU_0_CFG, RegValue);

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRL_APB_CPU_R5_CTRL);
			RegValue |= (CRL_APB_CPU_R5_CTRL_CLKACT_MASK);
			XFsbl_Out32(CRL_APB_CPU_R5_CTRL, RegValue);

			/**
			 * Provide some delay,
			 * so that clock propogates properly.
			 */
			//Status = usleep(0x50U);
			(void)usleep(0x50U);


			/**
			 * Release reset to R5-0
			 */
			RegValue = XFsbl_In32(CRL_APB_RST_LPD_TOP);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_R50_RESET_MASK);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_AMBA_RESET_MASK);
			XFsbl_Out32(CRL_APB_RST_LPD_TOP, RegValue);


			/**
			 * Take R5-0 out of HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_0_CFG);
			RegValue |= RPU_RPU_0_CFG_NCPUHALT_MASK;
			XFsbl_Out32(RPU_RPU_0_CFG, RegValue);
			break;

		case XIH_PH_ATTRB_DEST_CPU_R5_1:

			Status = XFsbl_PowerUpIsland(PMU_GLOBAL_PWR_STATE_R5_1_MASK);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_R5_1_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_R5_1_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Place R5, TCM's in split mode
			 */
			RegValue = XFsbl_In32(RPU_RPU_GLBL_CNTL);
			RegValue |= RPU_RPU_GLBL_CNTL_SLSPLIT_MASK;
			RegValue &= ~(RPU_RPU_GLBL_CNTL_TCM_COMB_MASK);
			RegValue &= ~(RPU_RPU_GLBL_CNTL_SLCLAMP_MASK);
			XFsbl_Out32(RPU_RPU_GLBL_CNTL, RegValue);

			/**
			 * Place R5-1 in HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_1_CFG);
			RegValue &= ~(RPU_RPU_1_CFG_NCPUHALT_MASK);
			XFsbl_Out32(RPU_RPU_1_CFG, RegValue);

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRL_APB_CPU_R5_CTRL);
			RegValue |= CRL_APB_CPU_R5_CTRL_CLKACT_MASK;
			XFsbl_Out32(CRL_APB_CPU_R5_CTRL, RegValue);

			/**
			 * Provide some delay,
			 * so that clock propogates properly.
			 */
			//Status = usleep(0x50U);
			(void)usleep(0x50U);

			/**
			 * Release reset to R5-1
			 */
			RegValue = XFsbl_In32(CRL_APB_RST_LPD_TOP);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_R51_RESET_MASK);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_AMBA_RESET_MASK);
			XFsbl_Out32(CRL_APB_RST_LPD_TOP, RegValue);


			/**
			 * Take R5-1 out of HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_1_CFG);
			RegValue |= RPU_RPU_1_CFG_NCPUHALT_MASK;
			XFsbl_Out32(RPU_RPU_1_CFG, RegValue);
			break;
		case XIH_PH_ATTRB_DEST_CPU_R5_L:

			Status = XFsbl_PowerUpIsland(PMU_GLOBAL_PWR_STATE_R5_0_MASK);
			if (Status != XFSBL_SUCCESS) {
				Status = XFSBL_ERROR_R5_L_POWER_UP;
				XFsbl_Printf(DEBUG_GENERAL, "XFSBL_ERROR_R5_L_POWER_UP\r\n");
				goto END;
			}

			/**
			 * Place R5, TCM's in safe mode
			 */
			RegValue = XFsbl_In32(RPU_RPU_GLBL_CNTL);
			RegValue &= ~(RPU_RPU_GLBL_CNTL_SLSPLIT_MASK);
			RegValue |= RPU_RPU_GLBL_CNTL_TCM_COMB_MASK;
			RegValue |= RPU_RPU_GLBL_CNTL_SLCLAMP_MASK;
			XFsbl_Out32(RPU_RPU_GLBL_CNTL, RegValue);

			/**
			 * Place R5-0 in HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_0_CFG);
			RegValue &= ~(RPU_RPU_0_CFG_NCPUHALT_MASK);
			XFsbl_Out32(RPU_RPU_0_CFG, RegValue);

			/**
			 * Place R5-1 in HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_1_CFG);
			RegValue &= ~(RPU_RPU_1_CFG_NCPUHALT_MASK);
			XFsbl_Out32(RPU_RPU_1_CFG, RegValue);

			/**
			 *  Enable the clock
			 */
			RegValue = XFsbl_In32(CRL_APB_CPU_R5_CTRL);
			RegValue |= CRL_APB_CPU_R5_CTRL_CLKACT_MASK;
			XFsbl_Out32(CRL_APB_CPU_R5_CTRL, RegValue);

			/**
			 * Provide some delay,
			 * so that clock propogates properly.
			 */
			//Status = usleep(0x50U);
			(void )usleep(0x50U);

			/**
			 * Release reset to R5-0, R5-1
			 */
			RegValue = XFsbl_In32(CRL_APB_RST_LPD_TOP);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_R50_RESET_MASK);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_R51_RESET_MASK);
			RegValue &= ~(CRL_APB_RST_LPD_TOP_RPU_AMBA_RESET_MASK);
			XFsbl_Out32(CRL_APB_RST_LPD_TOP, RegValue);


			/**
			 * Take R5-0 out of HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_0_CFG);
			RegValue |= RPU_RPU_0_CFG_NCPUHALT_MASK;
			XFsbl_Out32(RPU_RPU_0_CFG, RegValue);

			/**
			 * Take R5-1 out of HALT state
			 */
			RegValue = XFsbl_In32(RPU_RPU_1_CFG);
			RegValue |= RPU_RPU_1_CFG_NCPUHALT_MASK;
			XFsbl_Out32(RPU_RPU_1_CFG, RegValue);
			break;

		default:
			XFsbl_Printf(DEBUG_GENERAL,
			    "XFSBL_ERROR_HANDOFF_CPUID\n\r");
			Status = XFSBL_ERROR_HANDOFF_CPUID;
			break;
		}

	}
END:
	return Status;
}

/****************************************************************************/
/**
 * FSBL exit function before the assembly code
 *
 * @param HandoffAddress is handoff address for the FSBL running cpu
 *
 * @param Flags is to determine whether to handoff to applicatio or
 * 			to be in wfe state
 *
 * @return None
 *
 *
 *****************************************************************************/
void XFsbl_HandoffExit(u64 HandoffAddress, u32 Flags)
{


	/**
	 * Flush the L1 data cache and L2 cache, Disable Data Cache
	 */
	Xil_DCacheDisable();

	XFsbl_Printf(DEBUG_GENERAL,"Exit from FSBL \n\r");

	/**
	 * Exit to handoff address
	 * PTRSIZE is used since handoff is in same running cpu
	 * and address is of PTRSIZE
	 */
	XFsbl_Exit((PTRSIZE) HandoffAddress, Flags);

	/**
	 * should not reach here
	 */
	return ;

}


/****************************************************************************/
/**
*
* @param
*
* @return
*
* @note
*
*
*****************************************************************************/
static void XFsbl_UpdateResetVector (u64 HandOffAddress, u32 CpuSettings,
		u32 HandoffType)
{
	u32 HandOffAddressLow=0U;
	u32 HandOffAddressHigh=0U;
	u32 LowAddressReg=0U;
	u32 HighAddressReg=0U;
	u32 CpuId;

	CpuId = CpuSettings & XIH_PH_ATTRB_DEST_CPU_MASK;

	/**
	 * copy the IVT to 0xffff0000
	 */
	XFsbl_CopyIVT(CpuSettings, HandoffType);

	/**
	 * Writing u64/u32 will be decided on the handoff Cpu
	 */
	if (XFsbl_Is32BitCpu(CpuSettings) == TRUE)
	{
		/**
		 * for R5 and A53(32-bit) cpus, write 32bit handoff address
		 * Hence is also applicable for A53_0_64_TO_A53_0_32
		 */
		XFsbl_Out32(XFSBL_HANDOFF_ADDR_PTR,
					(u32 )HandOffAddress);
	} else if (HandoffType == A53_0_32_HANDOFF_TO_A53_0_64) {
		/**
		 * for A53_0_32_TO_A53_0_64 case, write 64 bit handoff address
		 * to the predfined location
		 */
		XFsbl_Out32(XFSBL_HANDOFF_ADDR_PTR,
				(u32 )(HandOffAddress & 0xFFFFFFFFU));
		XFsbl_Out32(XFSBL_HANDOFF_ADDR_PTR+4U,
				(u32 )((HandOffAddress>>32) & 0xFFFFFFFFU));
	}
	else {
		/**
		 * for A53 cpu, write 64bit handoff address
		 * to the RVBARADDR in APU
		 */

		HandOffAddressLow = (u32 )(HandOffAddress & 0xFFFFFFFFU);
		HandOffAddressHigh = (u32 )((HandOffAddress>>32)
							& 0xFFFFFFFFU);
		switch (CpuId)
		{
			case XIH_PH_ATTRB_DEST_CPU_A53_0:
				LowAddressReg = APU_RVBARADDR0L;
				HighAddressReg = APU_RVBARADDR0H;
				break;
			case XIH_PH_ATTRB_DEST_CPU_A53_1:
				LowAddressReg = APU_RVBARADDR1L;
				HighAddressReg = APU_RVBARADDR1H;
				break;
			case XIH_PH_ATTRB_DEST_CPU_A53_2:
				LowAddressReg = APU_RVBARADDR2L;
				HighAddressReg = APU_RVBARADDR2H;
				break;
			case XIH_PH_ATTRB_DEST_CPU_A53_3:
				LowAddressReg = APU_RVBARADDR3L;
				HighAddressReg = APU_RVBARADDR3H;
				break;
			default:
				/**
				 * error can be triggered here
				 */
				break;
		}
		XFsbl_Out32(LowAddressReg, HandOffAddressLow);
		XFsbl_Out32(HighAddressReg, HandOffAddressHigh);
	}

	return;
}

/****************************************************************************/
/**
 * This function will copy the Arm predefined code to the Reset IVT address
 *
 * @param CpuId is used to determine which arm predefined code to be loaded
 *	  by checking if the handoff cpu is of 32 bit
 *
 * @param HandoffType used to determine which arm A53 predefined code to be
 *	  loaded. Supports handoff to A53(32-bit) and A53(64bit) processors
 *
 * @return
 *
 * @note
 *
 *****************************************************************************/
static void XFsbl_CopyIVT(u32 CpuSettings, u32 HandoffType)
{
	u32 Index=0U;
	const u32 XFsbl_ArmR5PredefinedCode[] =
	{
		/**
		 * 1. Move 0xffffff00 to r0
		 * 2. load value stored in r0 to lr
		 * 3. dsb
		 * 4. isb
		 * 5. Move 0xAA to r1
		 * 6. Move 0xffffff80 to r0
		 * 7. str r1 to [r0]
		 * 8. dsb
		 * 9. isb
		 * 10. branch to lr
		 */

		/* mvn r0, #255 */
		0xE3E000FFU,
		/* ldr lr, [r0] */
		0xE590E000U,
		/* dsb */
		0xF57FF04FU,
		/* isb */
		0xF57FF06FU,
		/* mov r1, #170 */
		0xE3A010AAU,
		/* mvn r0, #127  */
		0xE3E0007FU,
		/* str r1, [r0]  */
		0xE5801000U,
		/* dsb */
		0xF57FF04FU,
		/* isb */
		0xF57FF06FU,
		/* bx	lr */
		0xE12FFF1EU
	};

	const u32 XFsbl_ArmA53Cpu32PredefinedCode[] =
	{
		/**
		 * 1. Move 0xffffff00 to r0
		 * 2. load value stored in r0 to lr
		 * 3. dsb
		 * 4. isb
		 * 5. branch to lr
		 */

		/* mvn r0, #255 */
		0xE3E000FFU,
		/* ldr lr, [r0] */
		0xE590E000U,
		/* dsb */
		0xF57FF04FU,
		/* isb */
		0xF57FF06FU,
		/* bx	lr */
		0xE12FFF1EU
	};

	const u32 XFsbl_ArmA53Cpu64PredefinedCode[] =
	{
		/**
		 * 1. Move 0xffffff00 to x0
		 * 2. load value stored in x0 to x30
		 * 3. dsb
		 * 4. isb
		 * 5. branch to x30
		 */

		/* mov	x0, #0xffffff00 */
		0xB2785FE0U,
		/* ldr	x30, [x0] */
		0xF940001EU,
		/* dsb sy */
		0xD5033F9FU,
		/* isb */
		0xD5033FDFU,
		/* br	x30 */
		0xD61F03C0U
	};



	/**
	 * Load the predefined ARM code to the reset vector address
	 * For R5 load the R5 code otherwise A53 code
	 */
	if ((XFsbl_Is32BitCpu(CpuSettings) == TRUE) &&
			(HandoffType == OTHER_CPU_HANDOFF)) {
		/**
		 * Load R5 code
		 */
		for (Index=0U;
				Index < sizeof(XFsbl_ArmR5PredefinedCode)/sizeof(u32); Index++)
		{
			XFsbl_Out32(XFSBL_IVT_ADDRESS + (Index*4U),
					XFsbl_ArmR5PredefinedCode[Index]);
		}
	}
	else if (HandoffType == A53_0_64_HANDOFF_TO_A53_0_32) {
		for (Index=0U;
				Index < sizeof(XFsbl_ArmA53Cpu32PredefinedCode)/sizeof(u32);
				Index++) {
			XFsbl_Out32(XFSBL_IVT_ADDRESS + (Index*4U),
					XFsbl_ArmA53Cpu32PredefinedCode[Index]);
		}
	}
	else if (HandoffType == A53_0_32_HANDOFF_TO_A53_0_64) {
		for (Index=0U;
				Index < sizeof(XFsbl_ArmA53Cpu64PredefinedCode)/sizeof(u32);
				Index++) {
			XFsbl_Out32(XFSBL_IVT_ADDRESS + (Index*4U),
					XFsbl_ArmA53Cpu64PredefinedCode[Index]);
		}
	}
	else {
		/* for MISRA C compliance  */
	}


	return;
}

/*****************************************************************************/
/**
 * This function handoff the images to the respective cpu's
 *
 * @param	FsblInstancePtr is pointer to the XFsbl Instance
 *
 * @param	PartitionNum is the partition number of the image
 *
 * @param	EarlyHandoff is flag to indicate if called for early handoff or not
 *
 * @return	returns the error codes described in xfsbl_error.h on any error
 *
 * @note	This function should not return incase of success
 *
 *****************************************************************************/

u32 XFsbl_Handoff (XFsblPs * FsblInstancePtr, u32 PartitionNum, u32 EarlyHandoff)
{
	u32 Status=XFSBL_SUCCESS;
	u32 CpuIndex=0U;
	u32 CpuId=0U;
	u32 ExecState=0U;
	u32 CpuSettings=0U;
	u64 HandoffAddress=0U;
	u64 RunningCpuHandoffAddress=0U;
	u32 RunningCpuExecState=0U;
	s32 RunningCpuHandoffAddressPresent=FALSE;
	u32 CpuNeedsEarlyHandoff = FALSE;

	static u32 CpuIndexEarlyHandoff = 0;

	/**
	 * if JTAG bootmode, be in while loop as of now
	 * Check if Process can be parked in HALT state
	 */
	if (FsblInstancePtr->PrimaryBootDevice ==
			XFSBL_JTAG_BOOT_MODE)
	{
		/**
		 * Mark Error status with Fsbl completed
		 */
		XFsbl_Out32(XFSBL_ERROR_STATUS_REGISTER_OFFSET,
		    XFSBL_COMPLETED);

		/**
		 * Exit from FSBL
		 */
		XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);
	}

	/**
	 * if XIP image present
	 * Put QSPI in linear mode
	 */

	/**
	 * FSBL hook before Handoff
	 */
	Status = XFsbl_HookBeforeHandoff(EarlyHandoff);
	if (Status != XFSBL_SUCCESS)
	{
		Status = XFSBL_ERROR_HOOK_BEFORE_HANDOFF;
		XFsbl_Printf(DEBUG_GENERAL,
				"XFSBL_ERROR_HOOK_BEFORE_HANDOFF\r\n");
		goto END;
	}

	/**
	 * Disable Data Cache to have smooth data
	 * transfer between the processors.
	 * Data transfer is required to update flag for CPU out of reset
	 */
	Xil_DCacheDisable();

	/**
	 * get cpu out of reset
	 *
	 */

	/**
	 * If we are doing early handoff, remember the CPU index to avoid
	 * traversing through for the next early handoff
	 */
	if (EarlyHandoff == TRUE) {
		CpuIndex = CpuIndexEarlyHandoff;
	}

	while (CpuIndex < FsblInstancePtr->HandoffCpuNo)
	{
		CpuSettings =
				FsblInstancePtr->HandoffValues[CpuIndex].CpuSettings;

		CpuId = CpuSettings & XIH_PH_ATTRB_DEST_CPU_MASK;
		ExecState = CpuSettings & XIH_PH_ATTRB_A53_EXEC_ST_MASK;

		/**
		 * Run the code in this loop in the below conditions:
		 * - This function called for early handoff and CPU needs early handoff
		 * - This function called for regular handoff and CPU doesn't need early
		 *   handoff
		 * - This function called for regular handoff and CPU needs early
		 *   handoff AND if handoff is to running CPU
		 *
		 */
		CpuNeedsEarlyHandoff = XFsbl_CheckEarlyHandoffCpu(CpuId);
		if (((EarlyHandoff == TRUE) && (CpuNeedsEarlyHandoff == TRUE)) ||
				((EarlyHandoff != TRUE) && (CpuNeedsEarlyHandoff != TRUE)) ||
				(((EarlyHandoff != TRUE) && (CpuNeedsEarlyHandoff == TRUE)) &&
						(CpuId == FsblInstancePtr->ProcessorID))) {

			/**
			 * Check if handoff address is present
			 */
			if (CpuId != FsblInstancePtr->ProcessorID)
			{
				/**
				 * Check for power status of the cpu
				 * Update the IVT
				 * Take cpu out of reset
				 */
				Status = XFsbl_SetCpuPwrSettings(
						CpuSettings, XFSBL_CPU_POWER_UP);
				if (XFSBL_SUCCESS != Status)
				{
					XFsbl_Printf(DEBUG_GENERAL,"Power Up "
							"Cpu 0x%0lx failed \n\r", CpuId);

					XFsbl_Printf(DEBUG_GENERAL,
							"XFSBL_ERROR_PWR_UP_CPU\n\r");
					Status = XFSBL_ERROR_PWR_UP_CPU;
					goto END;
				}

				/**
				 * Read the handoff address from structure
				 */
				HandoffAddress = (u64 )
					FsblInstancePtr->HandoffValues[CpuIndex].HandoffAddress;

				/**
				 * Update the handoff address at reset vector address
				 */
				XFsbl_UpdateResetVector(HandoffAddress, CpuSettings,
						OTHER_CPU_HANDOFF);

				XFsbl_Printf(DEBUG_INFO,"CPU 0x%0lx reset release, "
						"Exec State 0x%0lx, HandoffAddress: %0lx\n\r",
						CpuId, ExecState, (PTRSIZE )HandoffAddress);

				/**
				 * Reset the flag at 0xffffff80
				 * Write Zero at 0xffffff80
				 */
				XFsbl_Out32(XFSBL_HANDOFF_FLAG_ADDR, 0U);
				/**
				 * Take CPU out of reset
				 */
				Status = XFsbl_SetCpuPwrSettings(
						CpuSettings, XFSBL_CPU_SWRST);
				if (XFSBL_SUCCESS != Status)
				{
					goto END;
				}

				/**
				 * Wait till the CPU executed the
				 * predefined code for R5
				 */
				if ((CpuId == XIH_PH_ATTRB_DEST_CPU_R5_0) ||
						(CpuId == XIH_PH_ATTRB_DEST_CPU_R5_1) ||
						(CpuId == XIH_PH_ATTRB_DEST_CPU_R5_L) )
				{
					while (XFsbl_In32(XFSBL_HANDOFF_FLAG_ADDR)
							!= 0xAAU)
					{
						/**
						 * wait for flag
						 * block for MISRA C compliance
						 */
					};
				}

			} else {
				/**
				 * Update the running cpu handoff address
				 */
				RunningCpuHandoffAddressPresent = TRUE;
				RunningCpuHandoffAddress = (u64 )
				FsblInstancePtr->HandoffValues[CpuIndex].HandoffAddress;
				RunningCpuExecState = ExecState;

				/**
				 * Update reset vector address for
				 * - FSBL running on A53-0 (64bit), handoff to A53-0 (32 bit)
				 * - FSBL running on A53-0 (32bit), handoff to A53-0 (64 bit)
				 */
				if ((FsblInstancePtr->A53ExecState ==
						XIH_PH_ATTRB_A53_EXEC_ST_AA64) &&
						(ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)) {
					XFsbl_UpdateResetVector(RunningCpuHandoffAddress,
							CpuSettings, A53_0_64_HANDOFF_TO_A53_0_32);
				}
				else if ((FsblInstancePtr->A53ExecState ==
						XIH_PH_ATTRB_A53_EXEC_ST_AA32) &&
						(ExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA64)) {
					XFsbl_UpdateResetVector(RunningCpuHandoffAddress,
							CpuSettings, A53_0_32_HANDOFF_TO_A53_0_64);
				}
				else
				{
					/* for MISRA C compliance */
				}
			}
		}

		if ((EarlyHandoff == TRUE) && (CpuNeedsEarlyHandoff == TRUE)){

			/* Enable cache again as we will continue loading partitions */
			Xil_DCacheEnable();

			if (PartitionNum <
					(FsblInstancePtr->
							ImageHeader.ImageHeaderTable.NoOfPartitions-1U)) {
				/**
				 * If this is not the last handoff CPU, return back and continue
				 * loading remaining partitions in stage 3
				 */
				CpuIndexEarlyHandoff++;
				Status = XFSBL_STATUS_CONTINUE_PARTITION_LOAD;
			}
			else {
				/**
				 * Early handoff to all required CPUs is done, continue with
				 * regular handoff for remaining applications, as applicable
				 */
				Status = XFSBL_STATUS_CONTINUE_OTHER_HANDOFF;
			}
			goto END;
		}

		/**
		 * Go to the next cpu
		 */
		CpuIndex++;
		CpuIndexEarlyHandoff++;
	}


	/**
	 * Remove the R5 vectors from TCM and load APP data
	 * if present
	 */
	if (TcmSkipLength != 0U)
	{
		XFsbl_MemCpy((u8 *)TcmSkipAddress, TcmVectorArray,
				TcmSkipLength);
	}


	/**
	 * Mark Error status with Fsbl completed
	 */
	XFsbl_Out32(XFSBL_ERROR_STATUS_REGISTER_OFFSET, XFSBL_COMPLETED);

#ifdef XFSBL_WDT_PRESENT
	/* Stop WDT as we are exiting FSBL */
	XFsbl_StopWdt();
#endif

	/**
	 * call to the handoff routine
	 * which will never return
	 */
	if (RunningCpuHandoffAddressPresent ==  TRUE)
	{
		XFsbl_Printf(DEBUG_INFO,
		    "Running Cpu Handoff address: 0x%0lx, Exec State: %0lx\n\r",
		     (PTRSIZE )RunningCpuHandoffAddress, RunningCpuExecState);
		if (RunningCpuExecState == XIH_PH_ATTRB_A53_EXEC_ST_AA32)
		{
			XFsbl_HandoffExit(RunningCpuHandoffAddress,
			    XFSBL_HANDOFFEXIT_32);
		} else {
			XFsbl_HandoffExit(RunningCpuHandoffAddress,
			    XFSBL_HANDOFFEXIT);
		}
	} else {
		XFsbl_HandoffExit(0U, XFSBL_NO_HANDOFFEXIT);
	}

END:
	return Status;
}

/*****************************************************************************/
/**
 * This function determines if the given CPU needs early handoff or not.
 * Currently early handoff is provided for R5
 *
 * @param	CpuId is Mask of CPU Id in partition attributes
 *
 * @return	TRUE if this CPU needs early handoff, and FALSE if not
 *
 *****************************************************************************/
u32 XFsbl_CheckEarlyHandoffCpu(u32 CpuId)
{
	u32 CpuNeedEarlyHandoff = FALSE;
#if defined(XFSBL_EARLY_HANDOFF)
	if ((CpuId == XIH_PH_ATTRB_DEST_CPU_R5_0) ||
			(CpuId == XIH_PH_ATTRB_DEST_CPU_R5_1) ||
			(CpuId == XIH_PH_ATTRB_DEST_CPU_R5_L))
	{
		CpuNeedEarlyHandoff = TRUE;
	}
#endif
	return CpuNeedEarlyHandoff;

}

/*****************************************************************************/
/**
 * This function determines if the given partition needs early handoff
 *
 * @param	FsblInstancePtr is pointer to the XFsbl Instance
 *
 * @param	PartitionNum is the partition number of the image
 *
 * @return	TRUE if this partitions needs early handoff, and FALSE if not
 *
 *****************************************************************************/
u32 XFsbl_CheckEarlyHandoff(XFsblPs * FsblInstancePtr, u32 PartitionNum)
{

	u32 Status = FALSE;
#if defined(XFSBL_EARLY_HANDOFF)
	u32 CpuNeedsEarlyHandoff = FALSE;
	u32 DestinationCpu = 0;
	u32 DestinationDev = 0;
	u32 DestinationCpuNxt = 0;
	u32 DestinationDevNxt = 0;

	DestinationCpu = XFsbl_GetDestinationCpu(
			&FsblInstancePtr->ImageHeader.PartitionHeader[PartitionNum]);
	DestinationDev = XFsbl_GetDestinationDevice(
			&FsblInstancePtr->ImageHeader.PartitionHeader[PartitionNum]);
	if ((DestinationCpu == XIH_PH_ATTRB_DEST_CPU_NONE) &&
			((DestinationDev == XIH_PH_ATTRB_DEST_DEVICE_PS) ||
					(DestinationDev == XIH_PH_ATTRB_DEST_DEVICE_NONE)))
	{
		/* If dest device is not PS, retain the dest CPU as NONE/0 */
		DestinationCpu = FsblInstancePtr->ProcessorID;
	}

	if ((PartitionNum + 1) <=
		(FsblInstancePtr->ImageHeader.ImageHeaderTable.NoOfPartitions-1U)) {

		DestinationCpuNxt = XFsbl_GetDestinationCpu(
			&FsblInstancePtr->ImageHeader.PartitionHeader[PartitionNum + 1]);
		DestinationDevNxt = XFsbl_GetDestinationDevice(
			&FsblInstancePtr->ImageHeader.PartitionHeader[PartitionNum + 1]);

		if ((DestinationCpuNxt == XIH_PH_ATTRB_DEST_CPU_NONE) &&
				((DestinationDevNxt == XIH_PH_ATTRB_DEST_DEVICE_PS) ||
						(DestinationDevNxt == XIH_PH_ATTRB_DEST_DEVICE_NONE))) {
			DestinationCpuNxt = FsblInstancePtr->ProcessorID;
		}
	}

	/**
	 *  Early handoff needed if destination CPU needs early handoff AND
	 *  if handoff CPU is not same as running CPU AND
	 *  if this is the last partition of this application
	 */
	CpuNeedsEarlyHandoff = XFsbl_CheckEarlyHandoffCpu(DestinationCpu);
	if ((CpuNeedsEarlyHandoff == TRUE) &&
			(DestinationCpu != FsblInstancePtr->ProcessorID) &&
			(DestinationCpuNxt != DestinationCpu)) {

		Status = TRUE;
	}
#endif
	return Status;
}
