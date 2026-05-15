/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this
 * list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice,
 * this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_mmdc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Pointers to MMDC bases for each instance. */
static const MMDC_Type *s_mmdcBases[] = MMDC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of MMDC clock name. */
static const clock_ip_name_t s_mmdcClocks[][FSL_CLOCK_MMDC_IPG_GATE_COUNT] = MMDC_CLOCKS;
/* Array of MMDC  ACLK name. */
static const clock_ip_name_t s_mmdcAClock[] = MMDC_ACLK_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief LPDDR2 device define */
#define MR(x) (x)                                  /*!< convert the mode register addr */
#define LPDDR2_AUTO_INITIALIZE_IN_PROGRESS (0x01U) /*!< device auto intialize progress flag */
#define LPDDR2_CALIBRATION_CODE_AFTER_INIT (0xFFU) /*!< calibration code after init */
#define LPDDR2_DEVICE_TEMP_UPDATE (0x80U)          /*!< device temp update flag */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for MMDC module.
 *
 * @param base MMDC peripheral base address.
 */
uint32_t MMDC_GetInstance(MMDC_Type *base);

/*!
* @brief Get power saving structure.
* @param base MMDC peripheral base address
* @param mmdc device configuration collection, pointer for the read back configuration.
*/
static void MMDC_GetPowerSavingStatus(MMDC_Type *base, mmdc_power_config_t *config);

/*!
* @brief MMDC set the bit and wait the bit self clean
* @param base MMDC peripheral base address
* @param bit mask
*/
static status_t MMDC_SetAndWaitClear(uint32_t volatile *addr, uint32_t mask);

/*!
 * @brief calculate the result of log2
 * @param number of 2**n
 */
static uint8_t MMDC_Log(uint32_t n);
/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t MMDC_GetInstance(MMDC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_mmdcBases); instance++)
    {
        if (s_mmdcBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_mmdcBases));

    return instance;
}

static uint8_t MMDC_Log(uint32_t n)
{
    uint8_t cnt = 0;
    while (n % 2 == 0)
    {
        n = n / 2;
        cnt++;
    }
    return cnt;
}

static status_t MMDC_SetAndWaitClear(uint32_t volatile *addr, uint32_t mask)
{
    assert(addr != NULL);

    uint32_t timeout = MMDC_TIMEOUT;

    /* Mask bit first. */
    *addr |= mask;

    /* Wait the bit self clear or will return kStatus_MMDC_WaitFlagTimeout by timeout. */
    while (timeout)
    {
        if ((*addr & mask) == 0U)
        {
            break;
        }

        timeout--;
    }

    return timeout ? kStatus_Success : kStatus_MMDC_WaitFlagTimeout;
}

void MMDC_GetDefaultConfig(mmdc_config_t *config)
{
    assert(NULL != config);

    config->bankInterleave = true;
    config->secondDDRClock = true;
    config->enableOnlyCS0 = true;
    config->devType = kMMDC_DDR3;
    config->devSize = 0x40000000U;
    config->devBank = kMMDC_Bank8;
    config->rowWidth = kMMDC_Row16Bits;
    config->colWidth = kMMDC_Col10Bits;
    config->burstLen = kMMDC_BurstLen8;
    config->ODTConfig = NULL;
    config->timing = NULL;
    config->zqCalibration = NULL;
    config->deviceConfig[0] = NULL;
    config->deviceConfig[1] = NULL;
    config->readDQSCalibration[0] = NULL;
    config->readDQSCalibration[1] = NULL;
    config->wLevelingCalibration[0] = NULL;
    config->wLevelingCalibration[1] = NULL;
    config->readCalibration[0] = NULL;
    config->readCalibration[1] = NULL;
    config->writeCalibration[0] = NULL;
    config->writeCalibration[1] = NULL;
    config->tuning = NULL;
    config->autoRefresh = NULL;
    config->powerConfig = NULL;
}

status_t MMDC_Init(MMDC_Type *base, mmdc_config_t *config)
{
    assert(NULL != config);

    uint32_t rmdctl, rmisc;
    status_t result = kStatus_Success;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
#if (FSL_CLOCK_MMDC_IPG_GATE_COUNT == 2)
    if (config->enableOnlyCS0)
    {
        /* Calucate the divider to bus freq/device freq. */
        CLOCK_EnableClock(s_mmdcClocks[0][0]);
#if FSL_FEATURE_MMDC_HAS_CLK32_GATE
        CLOCK_EnableClock(s_mmdcClocks[0][1]);
#endif
    }
    else
    {
        CLOCK_EnableClock(s_mmdcClocks[0][0]);
        /* Calucate the divider to bus freq/device freq. */
        CLOCK_EnableClock(s_mmdcClocks[0][1]);
    }
#else
    /* Calucate the divider to bus freq/device freq. */
    CLOCK_EnableClock(s_mmdcClocks[0][0]);
#endif
    /* Enable MMDC aclock.*/
    CLOCK_EnableClock(s_mmdcAClock[0U]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Enter configuration mode.  */
    base->MDSCR |= MMDC_MDSCR_CON_REQ_MASK;

    /* Configure timing params. */
    if (config->timing != NULL)
    {
        MMDC_SetTiming(base, config->devType, config->timing);
    }

    /* Config on die termination. */
    if (config->ODTConfig != NULL)
    {
        /* ODT configuration. */
        base->MPODTCTRL = MMDC_MPODTCTRL_ODT1_INT_RES(config->ODTConfig->odtByte1Config) |
                          MMDC_MPODTCTRL_ODT0_INT_RES(config->ODTConfig->odtByte0Config) |
                          MMDC_MPODTCTRL_ODT_RD_ACT_EN(config->ODTConfig->enableActiveReadOdt) |
                          MMDC_MPODTCTRL_ODT_RD_PAS_EN(config->ODTConfig->enableInactiveReadOdt) |
                          MMDC_MPODTCTRL_ODT_WR_ACT_EN(config->ODTConfig->enableActiveWriteOdt) |
                          MMDC_MPODTCTRL_ODT_WR_PAS_EN(config->ODTConfig->enableInactiveWriteOdt);
    }
    else
    {
        base->MPODTCTRL = 0;
    }

    /* Config the CS0_END */
    if (config->enableOnlyCS0)
    {
        base->MDASP =
            ((MMDC_DEVICE_START_ADDRESS + config->devSize) >> 25U) - 1U; /* Cs0_end_addr increments in size of 256Mb. */
    }
    else
    {
        base->MDASP = (((MMDC_DEVICE_START_ADDRESS + (config->devSize / 2)) >> 25U) -
                       1U); /* Cs0_end_addr increments in size of 256Mb. */
    }

    /* Mdmisc register configuration. */
    rmisc = base->MDMISC;
    rmisc &= ~(MMDC_MDMISC_DDR_TYPE_MASK | MMDC_MDMISC_DDR_4_BANK_MASK | MMDC_MDMISC_LPDDR2_S2_MASK |
               MMDC_MDMISC_BI_ON_MASK | MMDC_MDMISC_CK1_GATING_MASK);

    rmisc |= MMDC_MDMISC_DDR_4_BANK(config->devBank) | MMDC_MDMISC_BI_ON(config->bankInterleave) |
             MMDC_MDMISC_CK1_GATING(config->secondDDRClock);

    if ((config->devType == kMMDC_LPDDR2_S2) || (config->devType == kMMDC_LPDDR2_S4))
    {
        rmisc |= MMDC_MDMISC_DDR_TYPE(1) | MMDC_MDMISC_LPDDR2_S2(config->devType);
    }

    base->MDMISC = rmisc;

    /* Mdctl register configuration. */
    rmdctl = base->MDCTL;
    /* Only support 16bit data bus width. */
    rmdctl &= ~(MMDC_MDCTL_ROW_MASK | MMDC_MDCTL_COL_MASK | MMDC_MDCTL_BL_MASK | MMDC_MDCTL_DSIZ_MASK);
    rmdctl |= MMDC_MDCTL_ROW(config->rowWidth) | MMDC_MDCTL_COL(config->colWidth) | MMDC_MDCTL_BL(config->burstLen);
    base->MDCTL = rmdctl;

    /* Perform ZQ calibration. */
    if (config->zqCalibration != NULL)
    {
        MMDC_DoZQCalibration(base, config->devType, config->zqCalibration);
    }

    /* Device initialization. */
    if (config->deviceConfig[0] != NULL)
    {
        MMDC_DeviceInit(base, config->devType, 0, config->deviceConfig[0]);
    }

    if (config->deviceConfig[1] != NULL)
    {
        MMDC_DeviceInit(base, config->devType, 1, config->deviceConfig[1]);
    }

    /* Select calibration target - Do CS0 first. */
    if ((config->readDQSCalibration[0] != NULL) || (config->readCalibration[0] != NULL) ||
        (config->writeCalibration[0] != NULL) || (config->wLevelingCalibration[0] != NULL))
    {
        base->MDMISC &= ~MMDC_MDMISC_CALIB_PER_CS_MASK;
        base->MDMISC |= MMDC_MDMISC_CALIB_PER_CS(0);
    }

    /* Perform read DQS calibration. */
    if (config->readDQSCalibration[0] != NULL)
    {
        result = MMDC_ReadDQSGatingCalibration(base, config->readDQSCalibration[0]);
        if (result)
        {
            return result;
        }
    }
    else
    {
        /* Disable DQS gate. */
        base->MPDGCTRL0 |= MMDC_MPDGCTRL0_DG_DIS_MASK;
    }

    /* Perform read calibration. */
    if (config->readCalibration[0] != NULL)
    {
        result = MMDC_ReadCalibration(base, config->readCalibration[0]);
        if (result)
        {
            return result;
        }
    }

    /* Perform write calibration. */
    if (config->writeCalibration[0] != NULL)
    {
        result = MMDC_WriteCalibration(base, config->writeCalibration[0]);
        if (result)
        {
            return result;
        }
    }

    /* Perform write leveling calibration. */
    if (config->wLevelingCalibration[0] != NULL)
    {
        result = MMDC_WriteLevelingCalibration(base, config->wLevelingCalibration[0]);
        if (result)
        {
            return result;
        }
    }

    /* Select calibration target - Do CS1 . */
    if ((config->readDQSCalibration[1] != NULL) || (config->readCalibration[1] != NULL) ||
        (config->writeCalibration[1] != NULL) || (config->wLevelingCalibration[1] != NULL))
    {
        base->MDMISC &= ~MMDC_MDMISC_CALIB_PER_CS_MASK;
        base->MDMISC |= MMDC_MDMISC_CALIB_PER_CS(1);
    }

    /* Perform read DQS calibration. */
    if (config->readDQSCalibration[1] != NULL)
    {
        result = MMDC_ReadDQSGatingCalibration(base, config->readDQSCalibration[1]);
        if (result)
        {
            return result;
        }
    }

    /* Perform read calibration. */
    if (config->readCalibration[1] != NULL)
    {
        result = MMDC_ReadCalibration(base, config->readCalibration[1]);
        if (result)
        {
            return result;
        }
    }

    /* Perform write calibration. */
    if (config->writeCalibration[1] != NULL)
    {
        result = MMDC_WriteCalibration(base, config->writeCalibration[1]);
        if (result)
        {
            return result;
        }
    }

    /* Perform write leveling calibration. */
    if (config->wLevelingCalibration[1] != NULL)
    {
        result = MMDC_WriteLevelingCalibration(base, config->wLevelingCalibration[1]);
        if (result)
        {
            return result;
        }
    }

    /* Perform fine tuning. */
    if (config->tuning != NULL)
    {
        MMDC_DoFineTuning(base, config->devType, config->tuning);
    }

    /* Enable power saving. */
    if (config->powerConfig != NULL)
    {
        MMDC_EnablePowerSaving(base, config->powerConfig);
    }

    /* Enable auto-refresh. */
    if (config->autoRefresh != NULL)
    {
        MMDC_EnableAutoRefresh(base, config->autoRefresh);
    }

    /* Exit configuration mode. */
    MMDC_EnterConfigurationMode(base, false);

    return kStatus_Success;
}

void MMDC_Deinit(MMDC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Calucate the divider to bus freq/device freq. */
#if (FSL_CLOCK_MMDC_IPG_GATE_COUNT == 2)
    /* disable all IPG clock */
    CLOCK_DisableClock(s_mmdcClocks[0][0]);
    CLOCK_DisableClock(s_mmdcClocks[0][1]);
#else
    /* disable the IPG clock. */
    CLOCK_DisableClock(s_mmdcClocks[0][0]);
#endif
    /* Disable MMDC Aclock. */
    CLOCK_DisableClock(s_mmdcAClock[0U]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void MMDC_Reset(MMDC_Type *base)
{
    /* Device enter self-refresh. */
    MMDC_EnableLowPowerMode(base, true);

    MMDC_EnterConfigurationMode(base, true);

    /* This bit is clear automatically. */
    base->MDMISC |= MMDC_MDMISC_RST_MASK;

    MMDC_EnterConfigurationMode(base, false);

    /* Device exit self-refresh. */
    MMDC_EnableLowPowerMode(base, false);
}

void MMDC_SetTiming(MMDC_Type *base, mmdc_device_type_t devType, mmdc_device_timing_t *timing)
{
    assert(NULL != timing);

    uint32_t rMdcfg0, rMdcfg1, rMdcfg2, rMdor, rMpdc, rMisc;
    uint32_t rMdrwd = 0;

    rMdcfg0 = base->MDCFG0;
    rMdcfg1 = base->MDCFG1;
    rMdcfg2 = base->MDCFG2;
    rMdor = base->MDOR;
    rMpdc = base->MDPDC;
    rMisc = base->MDMISC;

    /* Config the refresh timing. */
    rMpdc &= ~(MMDC_MDPDC_TCKSRE_MASK | MMDC_MDPDC_TCKSRX_MASK | MMDC_MDPDC_TCKE_MASK);
    rMpdc |= MMDC_MDPDC_TCKSRE(timing->tCKSRE_Clocks) | MMDC_MDPDC_TCKSRX(timing->tCKSRX_Clocks) |
             MMDC_MDPDC_TCKE(timing->tCKE_Clocks - 1U);

    /* Config the read latency, command and address timing. */
    rMdcfg0 &= ~(MMDC_MDCFG0_TRFC_MASK | MMDC_MDCFG0_TXS_MASK | MMDC_MDCFG0_TCL_MASK | MMDC_MDCFG0_TFAW_MASK |
                 MMDC_MDCFG0_TXP_MASK);
    rMdcfg0 |= (MMDC_MDCFG0_TRFC(timing->tRFC_Clocks - 1U) | MMDC_MDCFG0_TXS(timing->tXSR_Clocks - 1U) |
                MMDC_MDCFG0_TCL(timing->tCL_Clocks - 3U) | MMDC_MDCFG0_TFAW(timing->tFAW_Clocks - 1U) |
                MMDC_MDCFG0_TXP(timing->tXP_Clocks - 1U));

    /* Config the reset timing
    * 1.idle time after first CKE assert
    */
    rMdor &= ~(MMDC_MDOR_RST_TO_CKE_MASK | MMDC_MDOR_TXPR_MASK | MMDC_MDOR_SDE_TO_RST_MASK);
    rMdor |= (MMDC_MDOR_RST_TO_CKE(timing->tRSTtoCKE_Clocks + 2U) | MMDC_MDOR_TXPR(timing->tXPR_Clocks - 1U));

    rMisc &= ~(MMDC_MDMISC_RALAT_MASK | MMDC_MDMISC_WALAT_MASK);
    rMisc |= MMDC_MDMISC_RALAT(timing->ralat_Clocks) | MMDC_MDMISC_WALAT(timing->walat_Clocks);

    rMdcfg1 &= ~(MMDC_MDCFG1_TMRD_MASK | MMDC_MDCFG1_TRAS_MASK | MMDC_MDCFG1_TWR_MASK | MMDC_MDCFG1_TRCD_MASK |
                 MMDC_MDCFG1_TRP_MASK | MMDC_MDCFG1_TRC_MASK | MMDC_MDCFG1_TRPA_MASK | MMDC_MDCFG1_TCWL_MASK);
    rMdcfg1 |= MMDC_MDCFG1_TMRD(timing->tMRD_Clocks - 1U) | MMDC_MDCFG1_TRAS(timing->tRAS_Clocks - 1U) |
               MMDC_MDCFG1_TWR(timing->tWR_Clocks - 1U);
    rMdcfg2 &= ~(MMDC_MDCFG2_TRRD_MASK | MMDC_MDCFG2_TRTP_MASK | MMDC_MDCFG2_TWTR_MASK);
    rMdcfg2 |= MMDC_MDCFG2_TRRD(timing->tRRD_Clocks - 1U) | MMDC_MDCFG2_TRTP(timing->tRTP_Clocks - 1U) |
               MMDC_MDCFG2_TWTR(timing->tWTR_Clocks - 1U);

    if ((devType == kMMDC_LPDDR2_S2) || (devType == kMMDC_LPDDR2_S4))
    {
        uint32_t rMdcfg3 = base->MDCFG3LP;
        rMdcfg3 &= ~(MMDC_MDCFG3LP_RC_LP_MASK | MMDC_MDCFG3LP_TRCD_LP_MASK | MMDC_MDCFG3LP_TRPAB_LP_MASK |
                     MMDC_MDCFG3LP_TRPPB_LP_MASK);
        rMdcfg3 |= MMDC_MDCFG3LP_RC_LP(timing->tRC_Clocks - 1U) | MMDC_MDCFG3LP_TRCD_LP(timing->tRCD_Clocks - 1U) |
                   MMDC_MDCFG3LP_TRPAB_LP(timing->tRPA_Clocks - 1U) | MMDC_MDCFG3LP_TRPPB_LP(timing->tRP_Clocks - 1U);

        base->MDCFG3LP = rMdcfg3;
        rMdcfg1 |= MMDC_MDCFG1_TCWL(timing->tCWL_Clocks - 1U);
        rMdrwd = MMDC_MDRWD_TDAI(timing->tDAI_Clocks - 1U);

        /* MDOTC should be set to zero for LPDDR2 mode. */
        base->MDOTC = 0;
    }
    else if (devType == kMMDC_DDR3)
    {
        uint32_t rMDOTC = 0;

        rMdcfg1 |= MMDC_MDCFG1_TRCD(timing->tRCD_Clocks - 1U) | MMDC_MDCFG1_TRP(timing->tRP_Clocks - 1U) |
                   MMDC_MDCFG1_TRC(timing->tRC_Clocks - 1U) |
                   MMDC_MDCFG1_TRPA(timing->tRPA_Clocks - timing->tRP_Clocks) |
                   MMDC_MDCFG1_TCWL(timing->tCWL_Clocks - 2U);

        rMDOTC = MMDC_MDOTC_TAOFPD(timing->tAOFPD_Clocks - 1U) | MMDC_MDOTC_TAONPD(timing->tAONPD_Clocks - 1U) |
                 MMDC_MDOTC_TANPD(timing->tCWL_Clocks - 2U) | MMDC_MDOTC_TAXPD(timing->tCWL_Clocks - 2U) |
                 MMDC_MDOTC_TODTLON(timing->tCWL_Clocks - 1U) | MMDC_MDOTC_TODT_IDLE_OFF(timing->tODTIdleOff_Clocks);
        base->MDOTC = rMDOTC;

        rMdcfg0 &= ~(MMDC_MDCFG0_TXPDLL_MASK);
        rMdcfg0 |= MMDC_MDCFG0_TXPDLL(timing->tXPDLL_Clocks - 1U);

        rMdcfg2 &= ~(MMDC_MDCFG2_TDLLK_MASK);
        rMdcfg2 |= MMDC_MDCFG2_TDLLK(timing->tDLLK_Clocks - 1U);

        rMdor |= MMDC_MDOR_SDE_TO_RST(timing->tSDEtoRST_Clocks + 2U);
    }
    else
    {
    }

    /* Configure the delay between back to back read and write access.*/
    rMdrwd |= MMDC_MDRWD_RTW_SAME(timing->tRTWSAME_Clocks) | MMDC_MDRWD_WTR_DIFF(timing->tWTRDIFF_Clocks) |
              MMDC_MDRWD_WTW_DIFF(timing->tWTWDIFF_Clocks) | MMDC_MDRWD_RTW_DIFF(timing->tRTWDIFF_Clocks) |
              MMDC_MDRWD_RTR_DIFF(timing->tRTRDIFF_Clocks);

    /* Load to register. */
    base->MDCFG0 = rMdcfg0;
    base->MDCFG1 = rMdcfg1;
    base->MDCFG2 = rMdcfg2;
    base->MDOR = rMdor;
    base->MDPDC = rMpdc;
    base->MDMISC = rMisc;
    base->MDRWD = rMdrwd;
}

status_t MMDC_ReadDQSGatingCalibration(MMDC_Type *base, mmdc_readDQS_calibration_config_t *config)
{
    assert(NULL != config);

    uint32_t rMPDGCTRL0, rMPPDCMPR1, rMPRDDLCTL;
    mmdc_cmd_config_t cmdConfig;
    status_t result = kStatus_Success;

    switch (config->mode)
    {
        case kMMDC_CalWithPreSetValue:

            /* Load the preset value. */
            rMPDGCTRL0 = base->MPDGCTRL0;
            rMPDGCTRL0 &= ~(MMDC_MPDGCTRL0_DG_CMP_CYC_MASK | MMDC_MPDGCTRL0_DG_DIS_MASK |
                            MMDC_MPDGCTRL0_DG_HC_DEL1_MASK | MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET1_MASK |
                            MMDC_MPDGCTRL0_DG_HC_DEL0_MASK | MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET0_MASK);

            rMPDGCTRL0 |= (MMDC_MPDGCTRL0_DG_CMP_CYC(config->waitCycles) |
                           MMDC_MPDGCTRL0_DG_HC_DEL0(config->dqsGatingHalfDelay0) |
                           MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET0(config->dqsGatingAbsDelay0) |
                           MMDC_MPDGCTRL0_DG_HC_DEL1(config->dqsGatingHalfDelay1) |
                           MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET1(config->dqsGatingAbsDelay1));

            base->MPDGCTRL0 = rMPDGCTRL0;
            base->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;

            break;

        case kMMDC_CalWithMPR:
        case kMMDC_CalWithPreDefine:
            /* Reset read fifo. */
            MMDC_SetAndWaitClear(&base->MPDGCTRL0, MMDC_MPDGCTRL0_RST_RD_FIFO_MASK);

            /* Send precharge all cmd first. */
            cmdConfig.cmd = kMMDC_PreChargeAll;
            MMDC_HandleCommand(base, &cmdConfig);

            if (config->mode == kMMDC_CalWithMPR) /* Calibration with DQ */
            {
                /* Enter the DDR device into MPR mode. */
                cmdConfig.cmd = kMMDC_WriteModeRegister;
                cmdConfig.argMsb = 0x0U;
                cmdConfig.argLsb = 0x04U;
                cmdConfig.bankAddr = MR(3);

                /* Config the MMDC to work with MPR/DQ mode. */
                base->MPPDCMPR2 |= MMDC_MPPDCMPR2_MPR_CMP_MASK;
            }
            else if (config->mode == kMMDC_CalWithPreDefine) /* Calibration with pre-defined value. */
            {
                rMPPDCMPR1 = base->MPPDCMPR1;
                rMPPDCMPR1 &= ~(MMDC_MPPDCMPR1_PDV1_MASK | MMDC_MPPDCMPR1_PDV2_MASK);
                rMPPDCMPR1 |= MMDC_MPPDCMPR1_PDV1(MMDC_PRE_DEFINE_VALUE_DEFAULT) |
                              MMDC_MPPDCMPR1_PDV2(MMDC_PRE_DEFINE_VALUE_DEFAULT);
                base->MPPDCMPR1 = rMPPDCMPR1;

                /* Issue write access to device by set this bit. */
                result = MMDC_SetAndWaitClear(&base->MPSWDAR0, MMDC_MPSWDAR0_SW_DUMMY_WR_MASK);
                if (result)
                {
                    return result;
                }
            }
            else
            {
            }

            rMPRDDLCTL = base->MPRDDLCTL;

            /* Set the default value. */
            rMPRDDLCTL |= MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET0(config->readDelay0) |
                          MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET1(config->readDelay1);
            base->MPRDDLCTL = rMPRDDLCTL;

            /* Assert HW read calibration and wait the calibration complete. */
            result = MMDC_SetAndWaitClear(&base->MPDGCTRL0, MMDC_MPDGCTRL0_HW_DG_EN_MASK);
            if (result)
            {
                return result;
            }

            /* Check if error occur during calibration. */
            rMPDGCTRL0 = base->MPDGCTRL0;
            if (rMPDGCTRL0 & MMDC_MPDGCTRL0_HW_DG_ERR_MASK)
            {
                result = kStatus_MMDC_ErrorDGCalibration;
            }

            /* Save the calibration result. */
            rMPDGCTRL0 = base->MPDGCTRL0;
            config->dqsGatingHalfDelay0 =
                (rMPDGCTRL0 & MMDC_MPDGCTRL0_DG_HC_DEL0_MASK) >> MMDC_MPDGCTRL0_DG_HC_DEL0_SHIFT;
            config->dqsGatingHalfDelay1 =
                (rMPDGCTRL0 & MMDC_MPDGCTRL0_DG_HC_DEL1_MASK) >> MMDC_MPDGCTRL0_DG_HC_DEL1_SHIFT;
            config->dqsGatingAbsDelay0 =
                (rMPDGCTRL0 & MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET0_MASK) >> MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET0_SHIFT;
            config->dqsGatingAbsDelay1 =
                (rMPDGCTRL0 & MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET1_MASK) >> MMDC_MPDGCTRL0_DG_DL_ABS_OFFSET1_SHIFT;
            break;

        default:

            break;
    }

    return result;
}

status_t MMDC_ReadCalibration(MMDC_Type *base, mmdc_read_calibration_config_t *config)
{
    assert(NULL != config);

    uint32_t rMPRDDLCTL, rMPPDCMPR1, rMPRDDLHWCTL;
    mmdc_cmd_config_t cmdConfig;
    status_t result = kStatus_Success;

    switch (config->mode)
    {
        case kMMDC_CalWithPreSetValue:

            /* Load the preset value. */
            rMPRDDLCTL = base->MPRDDLCTL;
            rMPRDDLCTL &= ~(MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET0_MASK | MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET1_MASK);

            rMPRDDLCTL |= MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET0(config->readDelay0) |
                          MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET1(config->readDelay1);

            base->MPRDDLCTL = rMPRDDLCTL;
            base->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;

            break;

        /* This case is only for LPDDR2 device. */
        case kMMDC_CalWithMPR:

        case kMMDC_CalWithPreDefine:
            result = MMDC_SetAndWaitClear(&base->MPDGCTRL0, MMDC_MPDGCTRL0_RST_RD_FIFO_MASK);
            if (result)
            {
                return result;
            }

            /* Send precharge all cmd first. */
            memset(&cmdConfig, 0, sizeof(cmdConfig));
            cmdConfig.cmd = kMMDC_PreChargeAll;
            MMDC_HandleCommand(base, &cmdConfig);

            if (config->mode == kMMDC_CalWithMPR) /* Calibration with DQ */
            {
                /* Enter the DDR device into MPR mode. */
                cmdConfig.cmd = kMMDC_WriteModeRegister;
                cmdConfig.argMsb = 0x04U;
                cmdConfig.argLsb = 0x0U;
                cmdConfig.bankAddr = MR(3);

                /* Config the MMDC to work with MPR/DQ mode. */
                base->MPPDCMPR2 |= MMDC_MPPDCMPR2_MPR_CMP_MASK;
            }
            else if (config->mode == kMMDC_CalWithPreDefine) /* Calibration with pre-defined value. */
            {
                rMPPDCMPR1 = base->MPPDCMPR1;
                rMPPDCMPR1 &= ~(MMDC_MPPDCMPR1_PDV1_MASK | MMDC_MPPDCMPR1_PDV2_MASK);
                rMPPDCMPR1 |= MMDC_MPPDCMPR1_PDV1(MMDC_PRE_DEFINE_VALUE_DEFAULT) |
                              MMDC_MPPDCMPR1_PDV2(MMDC_PRE_DEFINE_VALUE_DEFAULT);
                base->MPPDCMPR1 = rMPPDCMPR1;

                /* Issue write access to device by set this bit. */
                result = MMDC_SetAndWaitClear(&base->MPSWDAR0, MMDC_MPSWDAR0_SW_DUMMY_WR_MASK);
                if (result)
                {
                    return result;
                }
            }
            else
            {
            }

            rMPRDDLCTL = base->MPRDDLCTL;
            rMPRDDLCTL &= ~(MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET0_MASK | MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET1_MASK);
            rMPRDDLCTL |= MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET0(config->readDelay0) |
                          MMDC_MPRDDLCTL_RD_DL_ABS_OFFSET1(config->readDelay1);
            base->MPRDDLCTL = rMPRDDLCTL;

            /* Assert HW read calibration and wait the calibration complete. */
            result = MMDC_SetAndWaitClear(&base->MPRDDLHWCTL, MMDC_MPRDDLHWCTL_HW_RD_DL_EN_MASK);
            if (result)
            {
                return result;
            }

            /* Check if error occur during calibration. */
            rMPRDDLHWCTL = base->MPRDDLHWCTL;
            if (rMPRDDLHWCTL & (MMDC_MPRDDLHWCTL_HW_RD_DL_ERR0_MASK | MMDC_MPRDDLHWCTL_HW_RD_DL_ERR1_MASK))
            {
                result = kStatus_MMDC_ErrorReadCalibration;
            }

            /* Save the calibration result. */
            rMPRDDLCTL = base->MPRDDLCTL;
            config->readDelay0 = rMPRDDLCTL & 0xFFU;
            config->readDelay1 = (rMPRDDLCTL >> 8U) & 0xFFU;

            break;

        default:

            break;
    }

    return result;
}

status_t MMDC_WriteCalibration(MMDC_Type *base, mmdc_write_calibration_config_t *config)
{
    assert(NULL != config);

    uint32_t rMPWRDLCTL, rMPWRDLHWCTL, rMPPDCMPR1;
    mmdc_cmd_config_t cmdConfig;
    status_t result = kStatus_Success;

    switch (config->mode)
    {
        case kMMDC_CalWithPreSetValue:

            /* Load the preset value. */
            rMPWRDLCTL = base->MPWRDLCTL;
            rMPWRDLCTL &= ~(MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET0_MASK | MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET1_MASK);

            rMPWRDLCTL |= MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET0(config->writeDelay0) |
                          MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET1(config->writeDelay1);

            base->MPWRDLCTL = rMPWRDLCTL;
            base->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;

            break;

        case kMMDC_CalWithPreDefine:

            result = MMDC_SetAndWaitClear(&base->MPDGCTRL0, MMDC_MPDGCTRL0_RST_RD_FIFO_MASK);
            if (result)
            {
                return result;
            }

            /* Send precharge all cmd first. */
            memset(&cmdConfig, 0, sizeof(cmdConfig));
            cmdConfig.cmd = kMMDC_PreChargeAll;
            MMDC_HandleCommand(base, &cmdConfig);

            rMPPDCMPR1 = base->MPPDCMPR1;
            rMPPDCMPR1 &= ~(MMDC_MPPDCMPR1_PDV1_MASK | MMDC_MPPDCMPR1_PDV2_MASK);
            rMPPDCMPR1 |=
                MMDC_MPPDCMPR1_PDV1(MMDC_PRE_DEFINE_VALUE_DEFAULT) | MMDC_MPPDCMPR1_PDV2(MMDC_PRE_DEFINE_VALUE_DEFAULT);
            base->MPPDCMPR1 = rMPPDCMPR1;

            /* Issue a SW_DUMMY_WRITE ,polling the write complete. */
            result = MMDC_SetAndWaitClear(&base->MPSWDAR0, MMDC_MPSWDAR0_SW_DUMMY_WR_MASK);
            if (result)
            {
                return result;
            }

            /* Make sure the init value is config in the read delay line. */
            rMPWRDLCTL = base->MPRDDLCTL;
            rMPWRDLCTL &= ~(MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET0_MASK | MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET1_MASK);
            /* Set the default value. */
            rMPWRDLCTL |= MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET0(config->writeDelay0) |
                          MMDC_MPWRDLCTL_WR_DL_ABS_OFFSET1(config->writeDelay1);
            base->MPWRDLCTL = rMPWRDLCTL;

            /* Assert HW write calibartion and wait the calibration complete. */
            result = MMDC_SetAndWaitClear(&base->MPWRDLHWCTL, MMDC_MPWRDLHWCTL_HW_WR_DL_EN_MASK);
            if (result)
            {
                return result;
            }

            /* Check if error occur during calibration. */
            rMPWRDLHWCTL = base->MPWRDLHWCTL;
            if (rMPWRDLHWCTL & (MMDC_MPWRDLHWCTL_HW_WR_DL_ERR0_MASK | MMDC_MPWRDLHWCTL_HW_WR_DL_ERR1_MASK))
            {
                result = kStatus_MMDC_ErrorWriteCalibration;
            }

            rMPWRDLCTL = base->MPWRDLCTL;
            config->writeDelay0 = rMPWRDLCTL & 0xFFU;
            config->writeDelay1 = (rMPWRDLCTL >> 8U) & 0xFFU;

            break;

        default:

            break;
    }

    return result;
}

status_t MMDC_WriteLevelingCalibration(MMDC_Type *base, mmdc_writeLeveling_calibration_config_t *config)
{
    assert(NULL != config);

    uint32_t rMPWLDECTRL0;
    mmdc_cmd_config_t cmdConfig;
    status_t result = kStatus_Success;

    switch (config->mode)
    {
        case kMMDC_CalWithPreSetValue:
            rMPWLDECTRL0 = base->MPWLDECTRL0;
            rMPWLDECTRL0 &= ~(MMDC_MPWLDECTRL0_WL_CYC_DEL1_MASK | MMDC_MPWLDECTRL0_WL_HC_DEL1_MASK |
                              MMDC_MPWLDECTRL0_WL_DL_ABS_OFFSET1_MASK | MMDC_MPWLDECTRL0_WL_CYC_DEL0_MASK |
                              MMDC_MPWLDECTRL0_WL_HC_DEL0_MASK | MMDC_MPWLDECTRL0_WL_DL_ABS_OFFSET0_MASK);
            rMPWLDECTRL0 |= (MMDC_MPWLDECTRL0_WL_CYC_DEL1(config->wLevelingOneDelay1) |
                             MMDC_MPWLDECTRL0_WL_HC_DEL1(config->wLevelingHalfDelay1) |
                             MMDC_MPWLDECTRL0_WL_DL_ABS_OFFSET1(config->wLevelingAbsDelay1) |
                             MMDC_MPWLDECTRL0_WL_CYC_DEL0(config->wLevelingOneDelay0) |
                             MMDC_MPWLDECTRL0_WL_HC_DEL0(config->wLevelingHalfDelay0) |
                             MMDC_MPWLDECTRL0_WL_DL_ABS_OFFSET0(config->wLevelingAbsDelay0));

            base->MPWLDECTRL0 = rMPWLDECTRL0;
            base->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;

            break;

        case kMMDC_CalWithMPR:

            /* Write leveling mode. */
            cmdConfig.argMsb = 0x0U;
            cmdConfig.argLsb = 0x08U; /* A7=1, enable write leveling mode. */
            cmdConfig.bankAddr = MR(1);
            MMDC_HandleCommand(base, &cmdConfig);

            /* Activate the DQS output enable. */
            base->MDSCR |= MMDC_MDSCR_WL_EN_MASK;

            /* Active automatic calibration. */
            result = MMDC_SetAndWaitClear(&base->MPWLGCR, MMDC_MPWLGCR_HW_WL_EN_MASK);
            if (result)
            {
                return result;
            }

            /* Exit leveling mode. */
            cmdConfig.argMsb = 0x0U;
            cmdConfig.argLsb = 0x0U; /* A7=0, disable write leveling mode. */
            cmdConfig.bankAddr = MR(1);
            MMDC_HandleCommand(base, &cmdConfig);

            /* Disalbe the DQS output enable. */
            base->MDSCR &= ~MMDC_MDSCR_WL_EN_MASK;

            if (base->MPWLGCR & (MMDC_MPWLGCR_WL_HW_ERR1_MASK | MMDC_MPWLGCR_WL_HW_ERR0_MASK))
            {
                result = kStatus_MMDC_ErrorWriteLevelingCalibration;
            }

            break;
        default:
            break;
    }

    return result;
}

void MMDC_DoFineTuning(MMDC_Type *base, mmdc_device_type_t devType, mmdc_fine_tuning_config_t *config)
{
    /* Read delay-line fine tuning. */
    base->MPRDDQBY0DL = config->rDQOffset0 & MMDC_READ_DQS_FINE_TUNING_MASK; /* Max value is 7 every bit
                                                                            in the DQ byte0 relative to the read DQS */
    base->MPRDDQBY1DL = config->rDQOffset1 & MMDC_READ_DQS_FINE_TUNING_MASK; /* Max value is 7 every bit
                                                                            in the DQ byte1 relative to the read DQS */

    /* Write delay-line fine tuning. */
    base->MPWRDQBY0DL |= config->wDQOffset0 & MMDC_WRITE_DQS_FINE_TUNING_MASK; /* Max value is 3 every bit in the
                                                                               DQ byte0 relative to the write
                                                                               DQS */
    base->MPWRDQBY1DL |= config->wDQOffset1 & MMDC_WRITE_DQS_FINE_TUNING_MASK; /* Max value is 3 every bit in the
                                                                               DQ byte1 relative to the write
                                                                               DQS */

    /* Command & Address delay line fine tuning for LPDDR2. */
    if ((devType == kMMDC_LPDDR2_S2) || (devType == kMMDC_LPDDR2_S4))
    {
        base->MPPDCMPR2 &= MMDC_MPPDCMPR2_CA_DL_ABS_OFFSET_MASK;
        base->MPPDCMPR2 |= MMDC_MPPDCMPR2_CA_DL_ABS_OFFSET(config->caDelay);
    }

    /* Configure duty cycle timing. */
    base->MPDCCR = MMDC_MPDCCR_RD_DQS1_FT_DCC(config->rDQDuty1) | MMDC_MPDCCR_RD_DQS0_FT_DCC(config->rDQDuty0) |
                   MMDC_MPDCCR_CK_FT1_DCC(config->ddrCKDutyCtl1) | MMDC_MPDCCR_CK_FT0_DCC(config->ddrCKDutyCtl0) |
                   MMDC_MPDCCR_WR_DQS1_FT_DCC(config->wDQDuty1) | MMDC_MPDCCR_WR_DQS0_FT_DCC(config->wDQDuty0);

    /* Force measurement. */
    base->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;
}

void MMDC_DeviceInit(MMDC_Type *base, mmdc_device_type_t devType, uint8_t targetCS, mmdc_device_config_t *devConfig)
{
    assert(NULL != devConfig);

    mmdc_cmd_config_t config;

    if (targetCS == 0U)
    {
        base->MDCTL |= MMDC_MDCTL_SDE_0_MASK; /* Select device 0 */
    }
    else
    {
        base->MDCTL |= MMDC_MDCTL_SDE_1_MASK; /* Select device 1 */
    }

    config.targetCS = targetCS;

    switch (devType)
    {
        case kMMDC_LPDDR2_S2:
        case kMMDC_LPDDR2_S4:

            /* Precharge all. */
            config.bankAddr = 0U;
            config.argLsb = 0x00U;
            config.argMsb = 0x00U;
            config.bankAddr = 0;
            config.cmd = kMMDC_PreChargeAll;
            MMDC_HandleCommand(base, &config);

            /* Reset commmand,bring the device to the device
            * auto-initialization state in Power-On initialization sequence
            * mode reigster: MR63
            */
            config.argLsb = MR(63U); /* Register addr. */
            config.argMsb = 0U;
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Do IO calibration
            * mode register: MR10
            * OP Code:0xFF
            */
            config.argLsb = MR(10U);                            /* Register A */
            config.argMsb = LPDDR2_CALIBRATION_CODE_AFTER_INIT; /* Calibration cmd after initialization. */
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Set MR1 parameter
            *  82->01
            */
            config.argLsb = MR(1U);
            config.argMsb = devConfig->MR1;
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Set MR2 parameter
            * Read latency/Write latency 04->02
            */
            config.argLsb = MR(2U);
            config.argMsb = devConfig->MR2;
            MMDC_HandleCommand(base, &config);

            /* Set MR3 parameter
            * Read latency/Write latency 04->02
            */
            config.argLsb = MR(3U);
            config.argMsb = devConfig->MR3;
            MMDC_HandleCommand(base, &config);

            break;
        case kMMDC_DDR3:
            /* Issue MRS Command to load MR2 with all application settings. */
            config.argLsb = devConfig->MR2;
            config.argMsb = devConfig->MR2 >> 8U;
            config.bankAddr = MR(2);
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Issue MRS Command to load MR3 with all application settings. */
            config.argLsb = devConfig->MR3;
            config.argMsb = devConfig->MR3 >> 8U;
            config.bankAddr = MR(3);
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Issue MRS Command to load MR1 with all application settings. */
            config.argLsb = devConfig->MR1;
            config.argMsb = devConfig->MR1 >> 8U;
            config.bankAddr = MR(1);
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* Issue MRS Command to load MR0 with all application settings. */
            config.argLsb = devConfig->MR0;
            config.argMsb = devConfig->MR0 >> 8U;
            config.bankAddr = MR(0);
            config.cmd = kMMDC_WriteModeRegister;
            MMDC_HandleCommand(base, &config);

            /* DDR device ZQ calibration. */
            config.argLsb = 0x04U;
            config.argMsb = 0x00U;
            config.bankAddr = 0;
            config.cmd = kMMDC_ZQCalibration;
            MMDC_HandleCommand(base, &config);

            break;
        default:
            break;
    }
}

status_t MMDC_EnterConfigurationMode(MMDC_Type *base, bool enable)
{
    uint32_t timeout = MMDC_TIMEOUT;

    if (enable)
    {
        /* If already in configuration mode ,do not need assert again. */
        if ((base->MDSCR & MMDC_MDSCR_CON_REQ_MASK) != MMDC_MDSCR_CON_REQ_MASK)
        {
            /* Assert the configuration request. */
            base->MDSCR |= MMDC_MDSCR_CON_REQ_MASK;

            /* Wait the configuration request ack. */
            while (timeout)
            {
                if (base->MDSCR & MMDC_MDSCR_CON_ACK_MASK)
                {
                    break;
                }

                timeout--;
            }
        }
    }
    else
    {
        base->MDSCR &= ~MMDC_MDSCR_CON_REQ_MASK;
    }

    return timeout ? kStatus_Success : kStatus_MMDC_WaitFlagTimeout;
}

status_t MMDC_EnableLowPowerMode(MMDC_Type *base, bool enable)
{
    uint32_t timeout = MMDC_TIMEOUT;

    if (enable)
    {
        /* Assert the low power mode request. */
        base->MAPSR |= MMDC_MAPSR_LPMD_MASK;

        /* Wait the Low power mode ack. */
        while (timeout)
        {
            if (base->MAPSR & MMDC_MAPSR_LPACK_MASK)
            {
                /* Already in self-refresh mode. */
                break;
            }

            timeout--;
        }
    }
    else
    {
        base->MAPSR &= ~MMDC_MAPSR_LPMD_MASK;
    }

    return timeout ? kStatus_Success : kStatus_MMDC_WaitFlagTimeout;
}

status_t MMDC_EnableDVFSMode(MMDC_Type *base, bool enable)
{
    uint32_t timeout = MMDC_TIMEOUT;

    if (enable)
    {
        /* Assert the DVFS mode request. */
        base->MAPSR |= MMDC_MAPSR_DVFS_MASK;

        /* Wait the DVFS mode ack. */
        while (timeout)
        {
            if (base->MAPSR & MMDC_MAPSR_DVACK_MASK)
            {
                /* Already in self-refresh mode. */
                break;
            }

            timeout--;
        }
    }
    else
    {
        base->MAPSR &= ~MMDC_MAPSR_DVFS_MASK;
    }

    return timeout ? kStatus_Success : kStatus_MMDC_WaitFlagTimeout;
}

status_t MMDC_GetReadData(MMDC_Type *base, uint32_t *data)
{
    uint32_t timeout = MMDC_TIMEOUT;

    while (timeout)
    {
        /* Wait the data ready flag set. */
        if ((base->MDSCR) & MMDC_MDSCR_MRR_READ_DATA_VALID_MASK)
        {
            break;
        }

        timeout--;
    }

    /* If not timeout get the read value. */
    if (timeout)
    {
        *data = base->MDMRR;
        return kStatus_Success;
    }

    return kStatus_MMDC_WaitFlagTimeout;
}

void MMDC_HandleCommand(MMDC_Type *base, mmdc_cmd_config_t *config)
{
    assert(NULL != config);

    uint32_t mdScr = 0;

    /* Config the cmd parameter
    * 1.cmd
    * 2.cmd target
    * 3.cmd bank address
    * 4.cmd op-code/address
    */

    /* Load to register. */
    mdScr = MMDC_MDSCR_CMD_ADDR_LSB_MR_ADDR(config->argLsb) | MMDC_MDSCR_CMD_ADDR_MSB_MR_OP(config->argMsb) |
            MMDC_MDSCR_CMD(config->cmd) | MMDC_MDSCR_CMD_BA(config->bankAddr) | MMDC_MDSCR_CMD_CS(config->targetCS) |
            MMDC_MDSCR_CON_REQ_MASK;

    base->MDSCR = mdScr;
}

void MMDC_DoZQCalibration(MMDC_Type *base, mmdc_device_type_t devType, mmdc_zq_config_t *zqCal)
{
    assert(NULL != zqCal);

    uint32_t zqHwCtrl, zqSwCtrl;
    uint32_t rMPPDCMPR2;

    zqHwCtrl = 0;
    zqSwCtrl = base->MPZQSWCTRL;

    /* Enter configuration mode first. */
    MMDC_EnterConfigurationMode(base, true);

    switch (zqCal->mode)
    {
        /*ZQ calibration to IO pads through HW*/
        case kMMDC_ZQCaltoIOHW:

            zqHwCtrl = base->MPZQHWCTRL;
            zqHwCtrl &= ~(MMDC_MPZQHWCTRL_ZQ_EARLY_COMPARATOR_EN_TIMER_MASK | MMDC_MPZQHWCTRL_ZQ_MODE_MASK);
            zqHwCtrl |= MMDC_MPZQHWCTRL_ZQ_EARLY_COMPARATOR_EN_TIMER(zqCal->earlyCompTimer) |
                        MMDC_MPZQHWCTRL_ZQ_MODE(kMMDC_ZQCaltoIODeviceLong);

            zqSwCtrl &= ~(MMDC_MPZQSWCTRL_ZQ_CMP_OUT_SMP_MASK | MMDC_MPZQSWCTRL_USE_ZQ_SW_VAL_MASK);
            zqSwCtrl |= MMDC_MPZQSWCTRL_ZQ_CMP_OUT_SMP(zqCal->cmpOutSample);

            base->MPZQHWCTRL |= zqHwCtrl;
            base->MPZQSWCTRL |= zqSwCtrl;

            /* Trigger/wait mmdc force Hw calibration IO pads. */
            MMDC_SetAndWaitClear(&base->MPZQHWCTRL, MMDC_MPZQHWCTRL_ZQ_HW_FOR_MASK);
            break;

        /*ZQ calibration to IO pads together with long cmd to device. */
        case kMMDC_ZQCaltoIODeviceLong:

        /*ZQ calibration to device only. */
        case kMMDC_ZQCaltoDeviceOnly:

        /*ZQ calibration to IO pads together with long/short cmd to device. */
        case kMMDC_ZQCaltoIODeviceLongShort:

            zqHwCtrl = MMDC_MPZQHWCTRL_ZQ_HW_PER(zqCal->hwZQFreq) |
                       MMDC_MPZQHWCTRL_ZQ_EARLY_COMPARATOR_EN_TIMER(zqCal->earlyCompTimer - 1U) |
                       MMDC_MPZQHWCTRL_ZQ_HW_FOR_MASK |
                       MMDC_MPZQHWCTRL_ZQ_MODE(zqCal->mode); /* The devCalmode must set to 1or3*/
            zqSwCtrl &= ~MMDC_MPZQSWCTRL_ZQ_CMP_OUT_SMP_MASK;
            zqSwCtrl |= MMDC_MPZQSWCTRL_ZQ_CMP_OUT_SMP(zqCal->cmpOutSample / 8);

            if ((devType == kMMDC_LPDDR2_S2) || (devType == kMMDC_LPDDR2_S4))
            {
                uint32_t zqLP2Ctrl = base->MPZQLP2CTL;
                zqLP2Ctrl &= ~(MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQCL_MASK | MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQCS_MASK |
                               MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQINIT_MASK);
                zqLP2Ctrl |= MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQCL(zqCal->tZQCl_Clocks / 2 - 1U) |
                             MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQCS(zqCal->tZQCs_Clocks / 4 - 1U) |
                             MMDC_MPZQLP2CTL_ZQ_LP2_HW_ZQINIT(zqCal->tZQInit_Clocks / 2 - 1U);
                base->MPZQLP2CTL = zqLP2Ctrl;
            }
            else if (devType == kMMDC_DDR3)
            {
                zqHwCtrl |= MMDC_MPZQHWCTRL_TZQ_CS(MMDC_Log(zqCal->tZQCs_Clocks) - 5U) |
                            MMDC_MPZQHWCTRL_TZQ_OPER(MMDC_Log(zqCal->tZQCl_Clocks) - 5U) |
                            MMDC_MPZQHWCTRL_TZQ_INIT(MMDC_Log(zqCal->tZQInit_Clocks) - 5U);
            }
            else
            {
            }

            base->MPZQSWCTRL = zqSwCtrl;
            base->MPZQHWCTRL = zqHwCtrl;

            break;

        case kMMDC_ZQFinetuning:

            rMPPDCMPR2 = base->MPPDCMPR2;
            rMPPDCMPR2 &= ~(MMDC_MPPDCMPR2_ZQ_PD_OFFSET_MASK | MMDC_MPPDCMPR2_ZQ_PU_OFFSET_MASK);
            rMPPDCMPR2 |= MMDC_MPPDCMPR2_ZQ_PD_OFFSET(zqCal->hwPullDownOffset) |
                          MMDC_MPPDCMPR2_ZQ_PU_OFFSET(zqCal->hwPullUpOffset);
            base->MPPDCMPR2 = rMPPDCMPR2;

            /* Enable ZQ fine tuning. */
            base->MPPDCMPR2 |= MMDC_MPPDCMPR2_ZQ_OFFSET_EN_MASK;

            break;

        case kMMDC_DisZQFinetuning:

            /* Disable ZQ fine tuning. */
            base->MPPDCMPR2 &= ~MMDC_MPPDCMPR2_ZQ_OFFSET_EN_MASK;

            break;

        default:
            break;
    }

    /* Exit configuration mode. */
    MMDC_EnterConfigurationMode(base, false);
}

void MMDC_EnablePowerSaving(MMDC_Type *base, mmdc_power_config_t *config)
{
    assert(NULL != config);

    uint32_t rMAPSR, rMDPDC;

    /* Enter configuration mode. */
    MMDC_EnterConfigurationMode(base, true);

    rMAPSR = base->MAPSR;
    rMAPSR &= ~(MMDC_MAPSR_PST_MASK | MMDC_MAPSR_PSD_MASK);
    if (config->idleClockToPS == 0U)
    {
        config->idleClockToPS = 0x10U; /* Default value, will multiplied by 64 */
    }
    rMAPSR |= MMDC_MAPSR_PST(config->idleClockToPS);
    base->MAPSR = rMAPSR;

    rMDPDC = base->MDPDC;

    rMDPDC &= ~(MMDC_MDPDC_PRCT_0_MASK | MMDC_MDPDC_PWDT_0_MASK | MMDC_MDPDC_PRCT_1_MASK | MMDC_MDPDC_PWDT_1_MASK);
    rMDPDC |= MMDC_MDPDC_PRCT_0(config->idleClockToPrecharge0) | MMDC_MDPDC_PWDT_0(config->idleClockToPD0) |
              MMDC_MDPDC_PRCT_1(config->idleClockToPrecharge1) | MMDC_MDPDC_PWDT_1(config->idleClockToPD1);

    base->MDPDC = rMDPDC;

    /* Exit configuration mode. */
    MMDC_EnterConfigurationMode(base, false);
}

static void MMDC_GetPowerSavingStatus(MMDC_Type *base, mmdc_power_config_t *config)
{
    assert(NULL != config);

    uint32_t rMAPSR, rMDPDC;

    rMAPSR = base->MAPSR;
    rMDPDC = base->MDPDC;

    if (rMAPSR & MMDC_MAPSR_PSS_MASK)
    {
        config->isInAutoPS = true; /* Mmdc is in automatic power saving mode. */
    }

    if (rMAPSR & MMDC_MAPSR_RIS_MASK)
    {
        config->rIdle = true; /* Read request buffer is idle. */
    }

    if (rMAPSR & MMDC_MAPSR_WIS_MASK)
    {
        config->wIdle = true; /* Write request buffer is idle. */
    }

    /* Get the timer value for restore. */
    config->idleClockToPS = (rMAPSR & MMDC_MAPSR_PST_MASK) >> MMDC_MAPSR_PST_SHIFT;
    config->idleClockToPD0 = (rMDPDC & MMDC_MDPDC_PWDT_0_MASK) >> MMDC_MDPDC_PWDT_0_SHIFT;
    config->idleClockToPrecharge0 = (rMDPDC & MMDC_MDPDC_PRCT_0_MASK) >> MMDC_MDPDC_PRCT_0_SHIFT;
    config->idleClockToPD1 = (rMDPDC & MMDC_MDPDC_PWDT_1_MASK) >> MMDC_MDPDC_PWDT_1_SHIFT;
    config->idleClockToPrecharge1 = (rMDPDC & MMDC_MDPDC_PRCT_1_MASK) >> MMDC_MDPDC_PRCT_1_SHIFT;
}

void MMDC_EnableAutoRefresh(MMDC_Type *base, mmdc_auto_refresh_t *config)
{
    assert(NULL != config);

    uint32_t mdRfresh;

    /* Enter configuration mode. */
    MMDC_EnterConfigurationMode(base, true);

    mdRfresh = base->MDREF;
    mdRfresh &= ~(MMDC_MDREF_REF_CNT_MASK | MMDC_MDREF_REFR_MASK | MMDC_MDREF_REF_SEL_MASK);
    mdRfresh |= MMDC_MDREF_REF_CNT(config->refreshCnt) | MMDC_MDREF_REFR(config->refreshRate - 1U) |
                MMDC_MDREF_REF_SEL(config->refreshTrigSrc);

    base->MDREF = mdRfresh;
    /* Exit configuration mode. */
    MMDC_EnterConfigurationMode(base, false);
}

void MMDC_EnhancePerformance(MMDC_Type *base, const mmdc_performance_config_t *config)
{
    assert(NULL != config);

    uint32_t rMisc, rMarcr;

    /* Enter configuration mode. */
    MMDC_EnterConfigurationMode(base, true);

    /* Config the cmd prediction mode. */
    rMisc = base->MDMISC;
    rMisc &= ~MMDC_MDMISC_MIF3_MODE_MASK;
    rMisc |= MMDC_MDMISC_MIF3_MODE(config->cmdPredict);
    base->MDMISC = rMisc;
    /* Config the dynamic scoring mode. */
    rMarcr = base->MAARCR;
    rMarcr &= ~(MMDC_MAARCR_ARCR_ACC_HIT_MASK | MMDC_MAARCR_ARCR_DYN_JMP_MASK | MMDC_MAARCR_ARCR_RCH_EN_MASK |
#if (defined(FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL) && FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL)
                MMDC_MAARCR_ARCR_ARB_REO_DIS_MASK | MMDC_MAARCR_ARCR_REO_DIS_MASK |
#endif
                MMDC_MAARCR_ARCR_DYN_MAX_MASK | MMDC_MAARCR_ARCR_GUARD_MASK | MMDC_MAARCR_ARCR_PAG_HIT_MASK);

    rMarcr |= MMDC_MAARCR_ARCR_ACC_HIT(config->rateAccessHit) | MMDC_MAARCR_ARCR_DYN_JMP(config->dynJump) |
              MMDC_MAARCR_ARCR_DYN_MAX(config->dynMax) | MMDC_MAARCR_ARCR_GUARD(config->guard) |
              MMDC_MAARCR_ARCR_PAG_HIT(config->ratePageHit) | MMDC_MAARCR_ARCR_RCH_EN(config->enRCH);

#if (defined(FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL) && FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL)
    if (!config->enReordering)
    {
        if (!config->enArbitration)
        {
            rMarcr |= MMDC_MAARCR_ARCR_ARB_REO_DIS_MASK;
        }
        else
        {
            rMarcr |= MMDC_MAARCR_ARCR_REO_DIS_MASK;
        }
    }
#endif

    base->MAARCR = rMarcr;

    /* Exit configuration mode. */
    MMDC_EnterConfigurationMode(base, false);
}

void MMDC_Profiling(MMDC_Type *base, mmdc_profiling_config_t *config)
{
    assert(NULL != config);

    uint32_t mDpcr1;

    switch (config->type)
    {
        case kMMDC_EnProfilingWithID:
            mDpcr1 = base->MADPCR1;
            mDpcr1 &= ~(MMDC_MADPCR1_PRF_AXI_ID_MASK | MMDC_MADPCR1_PRF_AXI_IDMASK_MASK);
            mDpcr1 |= MMDC_MADPCR1_PRF_AXI_ID(config->axiID) | MMDC_MADPCR1_PRF_AXI_IDMASK(config->axiIDMask);
            base->MADPCR1 = mDpcr1;
            /* Enable profiling. */
            base->MADPCR0 |= MMDC_MADPCR0_DBG_EN_MASK;

            break;

        case kMMDC_FreezeProfiling:
            /* Froze profiling. */
            base->MADPCR0 |= MMDC_MADPCR0_PRF_FRZ_MASK;

            /* Return the counter. */
            config->totalCount = base->MADPSR0;
            config->busyCount = base->MADPSR1;
            config->readCount = base->MADPSR2;
            config->writeCount = base->MADPSR3;
            config->readByteCount = base->MADPSR4;
            config->writeByteCount = base->MADPSR5;

            break;

        case kMMDC_CheckOverFlow:
            if (base->MADPCR0 & MMDC_MADPCR0_CYC_OVF_MASK)
            {
                config->overFlowCount = true;

                /* Clear profiling overflow. */
                base->MADPCR0 |= MMDC_MADPCR0_CYC_OVF_MASK;
            }

            break;

        default:
            break;
    }
}

status_t MMDC_LPDDR2UpdateDerate(MMDC_Type *base, mmdc_auto_refresh_t *config, uint32_t type)
{
    uint32_t rMDMR4, timeout = MMDC_TIMEOUT;

    MMDC_EnterConfigurationMode(base, true);

    if (type != kMMDC_NoUpdateDerate)
    {
        /* Update the refresh rate. */
        if (type & kMMDC_UpdateRefreshRate)
        {
            MMDC_EnableAutoRefresh(base, config);
        }
        /* Derate relate timing parameter. */
        if (type & kMMDC_DerateTiming)
        {
            rMDMR4 = base->MDMR4;

            rMDMR4 |= MMDC_MDMR4_TRCD_DE_MASK | MMDC_MDMR4_TRP_DE_MASK | MMDC_MDMR4_TRC_DE_MASK |
                      MMDC_MDMR4_TRAS_DE_MASK | MMDC_MDMR4_TRRD_DE_MASK;

            /* Reload to register. */
            base->MDMR4 = rMDMR4;
        }

        /* Assert request to load the derating timing. */
        base->MDMR4 |= MMDC_MDMR4_UPDATE_DE_REQ_MASK;

        /* Wait the ack which indicate the new value are token. */
        while (timeout)
        {
            if (base->MDMR4 & MMDC_MDMR4_UPDATE_DE_ACK_MASK)
            {
                break;
            }
            timeout--;
        }
    }

    MMDC_EnterConfigurationMode(base, false);

    return timeout ? kStatus_Success : kStatus_MMDC_WaitFlagTimeout;
}

status_t MMDC_MonitorLPDDR2OperationTemp(MMDC_Type *base, uint32_t *mr4)
{
    uint32_t MR4;
    status_t errorCheck = kStatus_Success;

    mmdc_cmd_config_t cmdConfig;
    mmdc_power_config_t pwr;

    /* Store pre-setting value. */
    MMDC_GetPowerSavingStatus(base, &pwr);

    /* Disable power saving. */
    MMDC_DisablePowerSaving(base);

    /* Read mode register 4 */
    memset(&cmdConfig, 0, sizeof(cmdConfig));
    cmdConfig.argLsb = MR(4U);
    cmdConfig.cmd = kMMDC_ReadModeRegister;
    MMDC_HandleCommand(base, &cmdConfig);

    errorCheck = MMDC_GetReadData(base, &MR4);

    if (errorCheck)
    {
        return errorCheck;
    }

    *mr4 = MR4; /* Restore mode register4 value. */

    /* Restore pre-setting value. */
    MMDC_EnablePowerSaving(base, &pwr);

    return errorCheck;
}

void MMDC_ExclusiveAccess(MMDC_Type *base, mmdc_exaccess_config_t *config, uint32_t type)
{
    assert(NULL != config);

    /* Enter configuration mode. */
    MMDC_EnterConfigurationMode(base, true);

    /* Config monitor ID 0 */
    if (type & kMMDC_ExMonitorID0)
    {
        base->MAEXIDR0 &= ~MMDC_MAEXIDR0_EXC_ID_MONITOR0_MASK;
        base->MAEXIDR0 |= MMDC_MAEXIDR0_EXC_ID_MONITOR0(config->excMonitorID0);
    }

    /* Config monitor ID 1 */
    if (type & kMMDC_ExMonitorID1)
    {
        base->MAEXIDR0 &= ~MMDC_MAEXIDR0_EXC_ID_MONITOR1_MASK;
        base->MAEXIDR0 |= MMDC_MAEXIDR0_EXC_ID_MONITOR1(config->excMonitorID1);
    }

    /* Config monitor ID 2 */
    if (type & kMMDC_ExMonitorID2)
    {
        base->MAEXIDR1 &= ~MMDC_MAEXIDR1_EXC_ID_MONITOR2_MASK;
        base->MAEXIDR1 |= MMDC_MAEXIDR1_EXC_ID_MONITOR2(config->excMonitorID2);
    }

    /* Config monitor ID 3 */
    if (type & kMMDC_ExMonitorID3)
    {
        base->MAEXIDR1 &= ~MMDC_MAEXIDR1_EXC_ID_MONITOR3_MASK;
        base->MAEXIDR1 |= MMDC_MAEXIDR1_EXC_ID_MONITOR3(config->excMonitorID3);
    }

    /* Config exclusive access reponse. */
    if (type & kMMDC_ExAccessResponse)
    {
        base->MAARCR &=
            ~(MMDC_MAARCR_ARCR_EXC_ERR_EN_MASK | MMDC_MAARCR_ARCR_SEC_ERR_EN_MASK | MMDC_MAARCR_ARCR_SEC_ERR_LOCK_MASK);
        base->MAARCR |= (MMDC_MAARCR_ARCR_EXC_ERR_EN(config->excErrEn) | MMDC_MAARCR_ARCR_SEC_ERR_EN(config->secErrEn) |
                         MMDC_MAARCR_ARCR_SEC_ERR_LOCK(config->secErrLock));
    }
    /* Exit configution mode. */
    MMDC_EnterConfigurationMode(base, false);
}
