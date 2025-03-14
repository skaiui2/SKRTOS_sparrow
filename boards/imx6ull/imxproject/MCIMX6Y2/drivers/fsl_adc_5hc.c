/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_adc_5hc.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for ADC module.
 *
 * @param base ADC peripheral base address
 */
static uint32_t ADC_5HC_GetInstance(ADC_5HC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to ADC bases for each instance. */
static ADC_5HC_Type *const s_adcBases[] = ADC_5HC_BASE_PTRS;

/*! @brief Pointers to ADC clocks for each instance. */
static const clock_ip_name_t s_adcClocks[] = ADC_5HC_CLOCKS;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t ADC_5HC_GetInstance(ADC_5HC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_adcBases); instance++)
    {
        if (s_adcBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_adcBases));

    return instance;
}

void ADC_5HC_Init(ADC_5HC_Type *base, const adc_5hc_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    /* Enable the clock. */
    CLOCK_EnableClock(s_adcClocks[ADC_5HC_GetInstance(base)]);
    /* ADCx_CFG */
    tmp32 = base->CFG & (ADC_5HC_CFG_AVGS_MASK | ADC_5HC_CFG_ADTRG_MASK); /* Reserve AVGS and ADTRG bits. */
    tmp32 |= ADC_5HC_CFG_REFSEL(config->referenceVoltageSource) | ADC_5HC_CFG_ADSTS(config->samplePeriodMode) |
             ADC_5HC_CFG_ADICLK(config->clockSource) | ADC_5HC_CFG_ADIV(config->clockDriver) |
             ADC_5HC_CFG_MODE(config->resolution);
    if (config->enableOverWrite)
    {
        tmp32 |= ADC_5HC_CFG_OVWREN_MASK;
    }
    if (config->enableLongSample)
    {
        tmp32 |= ADC_5HC_CFG_ADLSMP_MASK;
    }
    if (config->enableLowPower)
    {
        tmp32 |= ADC_5HC_CFG_ADLPC_MASK;
    }
    if (config->enableHighSpeed)
    {
        tmp32 |= ADC_5HC_CFG_ADHSC_MASK;
    }
    base->CFG = tmp32;

    /* ADCx_GC  */
    tmp32 = base->GC & ~(ADC_5HC_GC_ADCO_MASK | ADC_5HC_GC_ADACKEN_MASK);
    if (config->enableContinuousConversion)
    {
        tmp32 |= ADC_5HC_GC_ADCO_MASK;
    }
    if (config->enableAsynchronousClockOutput)
    {
        tmp32 |= ADC_5HC_GC_ADACKEN_MASK;
    }
    base->GC = tmp32;
}

void ADC_5HC_Deinit(ADC_5HC_Type *base)
{
    CLOCK_DisableClock(s_adcClocks[ADC_5HC_GetInstance(base)]);
}

void ADC_5HC_GetDefaultConfig(adc_5hc_config_t *config)
{
    assert(NULL != config);

    config->enableAsynchronousClockOutput = true;
    config->enableOverWrite = false;
    config->enableContinuousConversion = false;
    config->enableHighSpeed = false;
    config->enableLowPower = false;
    config->enableLongSample = false;
    config->referenceVoltageSource = kADC_5HC_ReferenceVoltageSourceAlt0;
    config->samplePeriodMode = kADC_5HC_SamplePeriod2or12Clocks;
    config->clockSource = kADC_5HC_ClockSourceAD;
    config->clockDriver = kADC_5HC_ClockDriver1;
    config->resolution = kADC_5HC_Resolution12Bit;
}

void ADC_5HC_SetChannelConfig(ADC_5HC_Type *base, uint32_t channelGroup, const adc_5hc_channel_config_t *config)
{
    assert(NULL != config);
    assert(channelGroup < ADC_5HC_HC_COUNT);

    uint32_t tmp32;

    tmp32 = ADC_5HC_HC_ADCH(config->channelNumber);
    if (config->enableInterruptOnConversionCompleted)
    {
        tmp32 |= ADC_5HC_HC_AIEN_MASK;
    }
    base->HC[channelGroup] = tmp32;
}

/*
 *To complete calibration, the user must follow the below procedure:
 *  1. Configure ADC_5HC_CFG with actual operating values for maximum accuracy.
 *  2. Configure the ADC_5HC_GC values along with CAL bit.
 *  3. Check the status of CALF bit in ADC_5HC_GS and the CAL bit in ADC_5HC_GC.
 *  4. When CAL bit becomes '0' then check the CALF status and COCO[0] bit status.
 */
status_t ADC_5HC_DoAutoCalibration(ADC_5HC_Type *base)
{
    status_t status = kStatus_Success;
    bool bHWTrigger = false;

    /* The calibration would be failed when in hardwar mode.
     * Remember the hardware trigger state here and restore it later if the hardware trigger is enabled.*/
    if (0U != (ADC_5HC_CFG_ADTRG_MASK & base->CFG))
    {
        bHWTrigger = true;
        ADC_5HC_EnableHardwareTrigger(base, false);
    }

    /* Clear the CALF and launch the calibration. */
    base->GS = ADC_5HC_GS_CALF_MASK; /* Clear the CALF. */
    base->GC |= ADC_5HC_GC_CAL_MASK; /* Launch the calibration. */

    /* Check the status of CALF bit in ADC_5HC_GS and the CAL bit in ADC_5HC_GC. */
    while (0U != (base->GC & ADC_5HC_GC_CAL_MASK))
    {
        /* Check the CALF when the calibration is active. */
        if (0U != (ADC_5HC_GetStatusFlags(base) & kADC_5HC_CalibrationFailedFlag))
        {
            status = kStatus_Fail;
            break;
        }
    }

    /* When CAL bit becomes '0' then check the CALF status and COCO[0] bit status. */
    if (0U == ADC_5HC_GetChannelStatusFlags(base, 0U)) /* Check the COCO[0] bit status. */
    {
        status = kStatus_Fail;
    }
    if (0U != (ADC_5HC_GetStatusFlags(base) & kADC_5HC_CalibrationFailedFlag)) /* Check the CALF status. */
    {
        status = kStatus_Fail;
    }

    /* Clear conversion done flag. */
    ADC_5HC_GetChannelConversionValue(base, 0U);

    /* Restore original trigger mode. */
    if (true == bHWTrigger)
    {
        ADC_5HC_EnableHardwareTrigger(base, true);
    }

    return status;
}

void ADC_5HC_SetOffsetConfig(ADC_5HC_Type *base, const adc_5hc_offest_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    tmp32 = ADC_5HC_OFS_OFS(config->offsetValue);
    if (config->enableSigned)
    {
        tmp32 |= ADC_5HC_OFS_SIGN_MASK;
    }
    base->OFS = tmp32;
}

void ADC_5HC_SetHardwareCompareConfig(ADC_5HC_Type *base, const adc_5hc_hardware_compare_config_t *config)
{
    uint32_t tmp32;

    tmp32 = base->GC & ~(ADC_5HC_GC_ACFE_MASK | ADC_5HC_GC_ACFGT_MASK | ADC_5HC_GC_ACREN_MASK);
    if (NULL == config) /* Pass "NULL" to disable the feature. */
    {
        base->GC = tmp32;
        return;
    }
    /* Enable the feature. */
    tmp32 |= ADC_5HC_GC_ACFE_MASK;

    /* Select the hardware compare working mode. */
    switch (config->hardwareCompareMode)
    {
        case kADC_5HC_HardwareCompareMode0:
            break;
        case kADC_5HC_HardwareCompareMode1:
            tmp32 |= ADC_5HC_GC_ACFGT_MASK;
            break;
        case kADC_5HC_HardwareCompareMode2:
            tmp32 |= ADC_5HC_GC_ACREN_MASK;
            break;
        case kADC_5HC_HardwareCompareMode3:
            tmp32 |= ADC_5HC_GC_ACFGT_MASK | ADC_5HC_GC_ACREN_MASK;
            break;
        default:
            break;
    }
    base->GC = tmp32;

    /* Load the compare values. */
    tmp32 = ADC_5HC_CV_CV1(config->value1) | ADC_5HC_CV_CV2(config->value2);
    base->CV = tmp32;
}

void ADC_5HC_SetHardwareAverageConfig(ADC_5HC_Type *base, adc_5hc_hardware_average_mode_t mode)
{
    uint32_t tmp32;

    if (mode == kADC_5HC_HardwareAverageDiasable)
    {
        base->GC &= ~ADC_5HC_GC_AVGE_MASK;
    }
    else
    {
        tmp32 = base->CFG & ~ADC_5HC_CFG_AVGS_MASK;
        tmp32 |= ADC_5HC_CFG_AVGS(mode);
        base->CFG = tmp32;
        base->GC |= ADC_5HC_GC_AVGE_MASK; /* Enable the hardware compare. */
    }
}

void ADC_5HC_ClearStatusFlags(ADC_5HC_Type *base, uint32_t mask)
{
    uint32_t tmp32 = 0;

    if (0U != (mask & kADC_5HC_CalibrationFailedFlag))
    {
        tmp32 |= ADC_5HC_GS_CALF_MASK;
    }
    if (0U != (mask & kADC_5HC_ConversionActiveFlag))
    {
        tmp32 |= ADC_5HC_GS_ADACT_MASK;
    }
    base->GS = tmp32;
}
