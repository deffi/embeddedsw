/*******************************************************************************
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
*******************************************************************************/
/******************************************************************************/
/**
 *
 * @file xvphy_hdmi.h
 *
 * The Xilinx Video PHY (VPHY) driver. This driver supports the Xilinx Video PHY
 * IP core.
 *
 * @note	None.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -----------------------------------------------
 * 1.0   gm   10/19/15 Initial release.
 * </pre>
 *
*******************************************************************************/

#ifndef XVPHY_HDMI_H_
/* Prevent circular inclusions by using protection macros. */
#define XVPHY_HDMI_H_

/************************** Constant Definitions ******************************/

#define XVPHY_HDMI_GTHE3_DRU_LRATE		2500000000U
#define XVPHY_HDMI_GTHE3_PLL_SCALE		1000
#define XVPHY_HDMI_GTHE3_QPLL0_REFCLK_MIN	61250000LL
#define XVPHY_HDMI_GTHE3_QPLL1_REFCLK_MIN	50000000LL
#define XVPHY_HDMI_GTHE3_CPLL_REFCLK_MIN	100000000LL
#define XVPHY_HDMI_GTHE3_TX_MMCM_SCALE		1
#define XVPHY_HDMI_GTHE3_TX_MMCM_FVCO_MIN	600000000U
#define XVPHY_HDMI_GTHE3_TX_MMCM_FVCO_MAX	1200000000U
#define XVPHY_HDMI_GTHE3_RX_MMCM_SCALE		1
#define XVPHY_HDMI_GTHE3_RX_MMCM_FVCO_MIN	600000000U
#define XVPHY_HDMI_GTHE3_RX_MMCM_FVCO_MAX	1200000000U

#define XVPHY_HDMI_GTHE2_DRU_LRATE		2500000000U
#define XVPHY_HDMI_GTHE2_PLL_SCALE		1000
#define XVPHY_HDMI_GTHE2_QPLL_REFCLK_MIN	61250000LL
#define XVPHY_HDMI_GTHE2_CPLL_REFCLK_MIN	80000000LL
#define XVPHY_HDMI_GTHE2_TX_MMCM_SCALE		1
#define XVPHY_HDMI_GTHE2_TX_MMCM_FVCO_MIN	600000000U
#define XVPHY_HDMI_GTHE2_TX_MMCM_FVCO_MAX	1200000000U
#define XVPHY_HDMI_GTHE2_RX_MMCM_SCALE		1
#define XVPHY_HDMI_GTHE2_RX_MMCM_FVCO_MIN	600000000U
#define XVPHY_HDMI_GTHE2_RX_MMCM_FVCO_MAX	1200000000U

#define XVPHY_HDMI_GTXE2_DRU_LRATE		2000000000U
#define XVPHY_HDMI_GTXE2_PLL_SCALE		1000
#define XVPHY_HDMI_GTXE2_QPLL_REFCLK_MIN	74125000LL
#define XVPHY_HDMI_GTXE2_CPLL_REFCLK_MIN	80000000LL
#define XVPHY_HDMI_GTXE2_TX_MMCM_SCALE		1
#define XVPHY_HDMI_GTXE2_TX_MMCM_FVCO_MIN	800000000U
#define XVPHY_HDMI_GTXE2_TX_MMCM_FVCO_MAX	1866000000U
#define XVPHY_HDMI_GTXE2_RX_MMCM_SCALE		1
#define XVPHY_HDMI_GTXE2_RX_MMCM_FVCO_MIN	600000000U
#define XVPHY_HDMI_GTXE2_RX_MMCM_FVCO_MAX	1200000000U

/**************************** Function Prototypes *****************************/

u32 XVphy_HdmiQpllParam(XVphy *InstancePtr, u8 QuadId, XVphy_ChannelId ChId,
		XVphy_DirectionType Dir);
u32 XVphy_HdmiCpllParam(XVphy *InstancePtr, u8 QuadId, XVphy_ChannelId ChId,
		XVphy_DirectionType Dir);
void XVphy_TxAlignReset(XVphy *InstancePtr, XVphy_ChannelId ChId, u8 Reset);
void XVphy_TxAlignStart(XVphy *InstancePtr, XVphy_ChannelId ChId, u8 Start);
void XVphy_ClkDetEnable(XVphy *InstancePtr, u8 Enable);
void XVphy_ClkDetTimerClear(XVphy *InstancePtr, u8 QuadId,
		XVphy_DirectionType Dir);
void XVphy_ClkDetSetFreqLockThreshold(XVphy *InstancePtr, u16 ThresholdVal);
u8 XVphy_ClkDetCheckFreqZero(XVphy *InstancePtr, XVphy_DirectionType Dir);
void XVphy_ClkDetSetFreqTimeout(XVphy *InstancePtr, u32 TimeoutVal);
void XVphy_ClkDetTimerLoad(XVphy *InstancePtr, u8 QuadId,
		XVphy_DirectionType Dir, u32 TimeoutVal);
void XVphy_DruReset(XVphy *InstancePtr, XVphy_ChannelId ChId, u8 Reset);
void XVphy_DruEnable(XVphy *InstancePtr, XVphy_ChannelId ChId, u8 Enable);
u16 XVphy_DruGetVersion(XVphy *InstancePtr);
void XVphy_DruSetCenterFreqHz(XVphy *InstancePtr, XVphy_ChannelId ChId,
		u64 CenterFreqHz);
void XVphy_DruSetGain(XVphy *InstancePtr, XVphy_ChannelId ChId, u8 G1, u8 G1_P,
		u8 G2);
u64 XVphy_DruCalcCenterFreqHz(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId);
void XVphy_HdmiGtDruModeEnable(XVphy *InstancePtr, u8 Enable);
void XVphy_HdmiIntrHandlerCallbackInit(XVphy *InstancePtr);

#endif /* XVPHY_HDMI_H_ */
