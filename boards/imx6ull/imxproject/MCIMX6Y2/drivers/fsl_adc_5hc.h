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

#ifndef _FSL_ADC_5HC_H_
#define _FSL_ADC_5HC_H_

#include "fsl_common.h"

/*!
 *  @addtogroup adc_5hc_12b1msps_sar
 *  @{
 */

/*******************************************************************************
* Definitions
******************************************************************************/
/*! @brief ADC driver version */
#define FSL_ADC_5HC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */

/*!
 * @brief Converter's status flags.
 */
typedef enum _adc_5hc_status_flags
{
    kADC_5HC_ConversionActiveFlag = ADC_5HC_GS_ADACT_MASK, /*!< Conversion is active,not support w1c. */
    kADC_5HC_CalibrationFailedFlag = ADC_5HC_GS_CALF_MASK, /*!< Calibration is failed,support w1c. */
    kADC_5HC_AsynchronousWakeupInterruptFlag =
        ADC_5HC_GS_AWKST_MASK, /*!< Asynchronous wakeup interrupt occured, support w1c. */
} adc_5hc_status_flags_t;

/*!
 * @brief Reference voltage source.
 */
typedef enum _adc_5hc_reference_voltage_source
{
    kADC_5HC_ReferenceVoltageSourceAlt0 = 0U, /*!< For external pins pair of VrefH and VrefL. */
} adc_5hc_reference_voltage_source_t;

/*!
 * @brief Sample time duration.
 */
typedef enum _adc_5hc_sample_period_mode
{
    /* This group of enumeration is for internal use which is related to register setting. */
    kADC_5HC_SamplePeriod2or12Clocks = 0U, /*!< Long sample 12 clocks or short sample 2 clocks. */
    kADC_5HC_SamplePeriod4or16Clocks = 1U, /*!< Long sample 16 clocks or short sample 4 clocks. */
    kADC_5HC_SamplePeriod6or20Clocks = 2U, /*!< Long sample 20 clocks or short sample 6 clocks. */
    kADC_5HC_SamplePeriod8or24Clocks = 3U, /*!< Long sample 24 clocks or short sample 8 clocks. */
    /* This group of enumeration is for a public user. */
    /* For long sample mode. */
    kADC_5HC_SamplePeriodLong12Clcoks = kADC_5HC_SamplePeriod2or12Clocks, /*!< Long sample 12 clocks. */
    kADC_5HC_SamplePeriodLong16Clcoks = kADC_5HC_SamplePeriod4or16Clocks, /*!< Long sample 16 clocks. */
    kADC_5HC_SamplePeriodLong20Clcoks = kADC_5HC_SamplePeriod6or20Clocks, /*!< Long sample 20 clocks. */
    kADC_5HC_SamplePeriodLong24Clcoks = kADC_5HC_SamplePeriod8or24Clocks, /*!< Long sample 24 clocks. */
    /* For short sample mode. */
    kADC_5HC_SamplePeriodShort2Clocks = kADC_5HC_SamplePeriod2or12Clocks, /*!< Short sample 2 clocks. */
    kADC_5HC_SamplePeriodShort4Clocks = kADC_5HC_SamplePeriod4or16Clocks, /*!< Short sample 4 clocks. */
    kADC_5HC_SamplePeriodShort6Clocks = kADC_5HC_SamplePeriod6or20Clocks, /*!< Short sample 6 clocks. */
    kADC_5HC_SamplePeriodShort8Clocks = kADC_5HC_SamplePeriod8or24Clocks, /*!< Short sample 8 clocks. */
} adc_5hc_sample_period_mode_t;

/*!
 * @brief Clock source.
 */
typedef enum _adc_5hc_clock_source
{
    kADC_5HC_ClockSourceIPG = 0U,     /*!< Select IPG clock to generate ADCK. */
    kADC_5HC_ClockSourceIPGDiv2 = 1U, /*!< Select IPG clock divided by 2 to generate ADCK. */
    kADC_5HC_ClockSourceAD = 3U,      /*!< Select Asynchronous clock to generate ADCK. */
} adc_5hc_clock_source_t;

/*!
 * @brief Clock divider for the converter.
 */
typedef enum _adc_5hc_clock_drvier
{
    kADC_5HC_ClockDriver1 = 0U, /*!< For divider 1 from the input clock to the module. */
    kADC_5HC_ClockDriver2 = 1U, /*!< For divider 2 from the input clock to the module. */
    kADC_5HC_ClockDriver4 = 2U, /*!< For divider 4 from the input clock to the module. */
    kADC_5HC_ClockDriver8 = 3U, /*!< For divider 8 from the input clock to the module. */
} adc_5hc_clock_driver_t;

/*!
 * @brief Converter's resolution.
 */
typedef enum _adc_5hc_resolution
{
    kADC_5HC_Resolution8Bit = 0U,  /*!< Single End 8-bit resolution. */
    kADC_5HC_Resolution10Bit = 1U, /*!< Single End 10-bit resolution. */
    kADC_5HC_Resolution12Bit = 2U, /*!< Single End 12-bit resolution. */
} adc_5hc_resolution_t;

/*!
 * @brief Converter hardware compare mode.
 */
typedef enum _adc_5hc_hardware_compare_mode
{
    kADC_5HC_HardwareCompareMode0 = 0U, /*!< Compare true if the result is less than the value1. */
    kADC_5HC_HardwareCompareMode1 = 1U, /*!< Compare true if the result is greater than or equal to value1. */
    kADC_5HC_HardwareCompareMode2 = 2U, /*!< Value1 <= Value2, compare true if the result is less than value1 Or
                                                          the result is Greater than value2.
                                         Value1 >  Value2, compare true if the result is less than value1 And the
                                                          result is greater than value2*/
    kADC_5HC_HardwareCompareMode3 = 3U, /*!< Value1 <= Value2, compare true if the result is greater than or equal
                                                          to value1 And the result is less than or equal to value2.
                                         Value1 >  Value2, compare true if the result is greater than or equal to
                                                          value1 Or the result is less than or equal to value2. */
} adc_5hc_hardware_compare_mode_t;

/*!
 * @brief Converter hardware average mode.
 */
typedef enum _adc_5hc_hardware_average_mode
{
    kADC_5HC_HardwareAverageCount4 = 0U,   /*!< For hardware average with 4 samples. */
    kADC_5HC_HardwareAverageCount8 = 1U,   /*!< For hardware average with 8 samples. */
    kADC_5HC_HardwareAverageCount16 = 2U,  /*!< For hardware average with 16 samples. */
    kADC_5HC_HardwareAverageCount32 = 3U,  /*!< For hardware average with 32 samples. */
    kADC_5HC_HardwareAverageDiasable = 4U, /*!< Disable the hardware average function. */
} adc_5hc_hardware_average_mode_t;

/*!
 * @brief Converter configuration.
 */
typedef struct _adc_5hc_config
{
    bool enableOverWrite;                                      /*!< Enable the overwriting. */
    bool enableContinuousConversion;                           /*!< Enable the continuous conversion mode. */
    bool enableHighSpeed;                                      /*!< Enable the high-speed mode. */
    bool enableLowPower;                                       /*!< Enable the low power mode. */
    bool enableLongSample;                                     /*!< Enable the long sample mode. */
    bool enableAsynchronousClockOutput;                        /*!< Enable the asynchronous clock output. */
    adc_5hc_reference_voltage_source_t referenceVoltageSource; /*!< Select the reference voltage source. */
    adc_5hc_sample_period_mode_t samplePeriodMode; /*!< Select the sample period in long sample mode or short mode. */
    adc_5hc_clock_source_t clockSource; /*!< Select the input clock source to generate the internal clock ADCK. */
    adc_5hc_clock_driver_t
        clockDriver; /*!< Select the divide ratio used by the ADC to generate the internal clock ADCK. */
    adc_5hc_resolution_t resolution; /*!< Select the ADC resolution mode. */
} adc_5hc_config_t;

/*!
 * @brief Converter Offset configuration.
 */
typedef struct _adc_5hc_offest_config
{
    bool enableSigned;    /*!< if false,The offset value is added with the raw result.
                               if true,The offset value is subtracted from the raw converted value. */
    uint32_t offsetValue; /*!< User configurable offset value(0-4095). */
} adc_5hc_offest_config_t;

/*!
 * @brief ADC hardware compare configuration.
 *
 * In kADC_5HC_HardwareCompareMode0, compare true if the result is less than the value1.
 * In kADC_5HC_HardwareCompareMode1, compare true if the result is greater than or equal to value1.
 * In kADC_5HC_HardwareCompareMode2, Value1 <= Value2, compare true if the result is less than value1 Or the result is
 * Greater than value2.
 *                               Value1 >  Value2, compare true if the result is less than value1 And the result is
 * Greater than value2.
 * In kADC_5HC_HardwareCompareMode3, Value1 <= Value2, compare true if the result is greater than or equal to value1 And
 * the
 * result is less than or equal to value2.
 *                               Value1 >  Value2, compare true if the result is greater than or equal to value1 Or the
 * result is less than or equal to value2.
 */
typedef struct _adc_5hc_hardware_compare_config
{
    adc_5hc_hardware_compare_mode_t hardwareCompareMode; /*!< Select the hardware compare mode.
                                                            See "adc_5hc_hardware_compare_mode_t". */
    uint16_t value1;                                     /*!< Setting value1(0-4095) for hardware compare mode. */
    uint16_t value2;                                     /*!< Setting value2(0-4095) for hardware compare mode. */
} adc_5hc_hardware_compare_config_t;

/*!
 * @brief ADC channel conversion configuration.
 */
typedef struct _adc_5hc_channel_config
{
    uint32_t channelNumber;                    /*!< Setting the conversion channel number. The available range is 0-31.
                                                    See channel connection information for each chip in Reference
                                                    Manual document. */
    bool enableInterruptOnConversionCompleted; /*!< Generate an interrupt request once the conversion is completed. */
} adc_5hc_channel_config_t;
/*******************************************************************************
* API
******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initialize the ADC module.
 *
 * @param base ADC peripheral base address.
 * @param config Pointer to "adc_5hc_config_t" structure.
 */
void ADC_5HC_Init(ADC_5HC_Type *base, const adc_5hc_config_t *config);

/*!
 * @brief De-initializes the ADC module.
 *
 * @param base ADC peripheral base address.
 */
void ADC_5HC_Deinit(ADC_5HC_Type *base);

/*!
 * @brief Gets an available pre-defined settings for the converter's configuration.
 *
 * This function initializes the converter configuration structure with available settings. The default values are:
 * @code
 *  config->enableAsynchronousClockOutput = true;
 *  config->enableOverWrite =               false;
 *  config->enableContinuousConversion =    false;
 *  config->enableHighSpeed =               false;
 *  config->enableLowPower =                false;
 *  config->enableLongSample =              false;
 *  config->referenceVoltageSource =        kADC_5HC_ReferenceVoltageSourceAlt0;
 *  config->samplePeriodMode =              kADC_5HC_SamplePeriod2or12Clocks;
 *  config->clockSource =                   kADC_5HC_ClockSourceAD;
 *  config->clockDriver =                   kADC_5HC_ClockDriver1;
 *  config->resolution =                    kADC_5HC_Resolution12Bit;
 * @endcode
 * @param base   ADC peripheral base address.
 * @param config Pointer to the configuration structure.
 */
void ADC_5HC_GetDefaultConfig(adc_5hc_config_t *config);

/*!
 * @brief Configures the conversion channel.
 *
 * This operation triggers the conversion when in software trigger mode. When in hardware trigger mode, this API
 * configures the channel while the external trigger source helps to trigger the conversion.
 *
 * Note that the "Channel Group" has a detailed description.
 * To allow sequential conversions of the ADC to be triggered by internal peripherals, the ADC has more than one
 * group of status and control registers, one for each conversion. The channel group parameter indicates which group of
 * registers are used, for example channel group 0 is for Group A registers and channel group 1 is for Group B
 * registers. The
 * channel groups are used in a "ping-pong" approach to control the ADC operation.  At any point, only one of
 * the channel groups is actively controlling ADC conversions. The channel group 0 is used for both software and
 * hardware
 * trigger modes. Channel groups 1 and greater indicate potentially multiple channel group registers for
 * use only in hardware trigger mode. See the chip configuration information in the appropriate MCU reference manual
 * about the
 * number of HCn registers (channel groups) specific to this device.  None of the channel groups 1 or greater are used
 * for software trigger operation. Therefore, writing to these channel groups does not initiate a new conversion.
 * Updating the channel group 0 while a different channel group is actively controlling a conversion is allowed and
 * vice versa. Writing any of the channel group registers while that specific channel group is actively controlling a
 * conversion aborts the current conversion.
 *
 * @param base          ADC peripheral base address.
 * @param channelGroup  Channel group index.
 * @param config        Pointer to the "adc_5hc_channel_config_t" structure for the conversion channel.
 */
void ADC_5HC_SetChannelConfig(ADC_5HC_Type *base, uint32_t channelGroup, const adc_5hc_channel_config_t *config);

/*!
 * @brief  Gets the conversion value.
 *
 * @param  base         ADC peripheral base address.
 * @param  channelGroup Channel group index.
 *
 * @return              Conversion value.
 */
static inline uint32_t ADC_5HC_GetChannelConversionValue(ADC_5HC_Type *base, uint32_t channelGroup)
{
    assert(channelGroup < ADC_5HC_R_COUNT);

    return base->R[channelGroup];
}

/*!
 * @brief Gets the status flags of channel.
 *
 * A conversion is completed when the result of the conversion is transferred into the data
 * result registers. (provided the compare function & hardware averaging is disabled), this is
 * indicated by the setting of COCOn. If hardware averaging is enabled, COCOn sets only,
 * if the last of the selected number of conversions is complete. If the compare function is
 * enabled, COCOn sets and conversion result data is transferred only if the compare
 * condition is true. If both hardware averaging and compare functions are enabled, then
 * COCOn sets only if the last of the selected number of conversions is complete and the
 * compare condition is true.
 *
 * @param base         ADC peripheral base address.
 * @param channelGroup Channel group index.
 *
 * @return             Status flags of channel.return 0 means COCO flag is 0,return 1 means COCOflag is 1.
 */
static inline uint32_t ADC_5HC_GetChannelStatusFlags(ADC_5HC_Type *base, uint32_t channelGroup)
{
    assert(channelGroup < ADC_5HC_HC_COUNT);

    /* If flag is set,return 1,otherwise, return 0. */
    return (((base->HS) & (1U << channelGroup)) >> channelGroup);
}

/*!
 * @brief  Automates the hardware calibration.
 *
 * This auto calibration helps to adjust the plus/minus side gain automatically.
 * Execute the calibration before using the converter. Note that the software trigger should be used
 * during calibration.
 *
 * @param  base ADC peripheral base address.
 *
 * @return                 Execution status.
 * @retval kStatus_Success Calibration is done successfully.
 * @retval kStatus_Fail    Calibration has failed.
 */
status_t ADC_5HC_DoAutoCalibration(ADC_5HC_Type *base);

/*!
 * @brief Set user defined offset.
 *
 * @param base   ADC peripheral base address.
 * @param config Pointer to "adc_5hc_offest_config_t" structure.
 */
void ADC_5HC_SetOffsetConfig(ADC_5HC_Type *base, const adc_5hc_offest_config_t *config);

/*!
 * @brief Enables generating the DMA trigger when the conversion is complete.
 *
 * @param base   ADC peripheral base address.
 * @param enable Switcher of the DMA feature. "true" means enabled, "false" means not enabled.
 */
static inline void ADC_5HC_EnableDMA(ADC_5HC_Type *base, bool enable)
{
    if (enable)
    {
        base->GC |= ADC_5HC_GC_DMAEN_MASK;
    }
    else
    {
        base->GC &= ~ADC_5HC_GC_DMAEN_MASK;
    }
}

/*!
 * @brief Enables the hardware trigger mode.
 *
 * @param base ADC peripheral base address.
 * @param enable Switcher of the trigger mode. "true" means hardware tirgger mode,"false" means software mode.
 */
static inline void ADC_5HC_EnableHardwareTrigger(ADC_5HC_Type *base, bool enable)
{
    if (enable)
    {
        base->CFG |= ADC_5HC_CFG_ADTRG_MASK;
    }
    else
    {
        base->CFG &= ~ADC_5HC_CFG_ADTRG_MASK;
    }
}

/*!
 * @brief Configures the hardware compare mode.
 *
 * The hardware compare mode provides a way to process the conversion result automatically by using hardware. Only the
 * result
 * in the compare range is available. To compare the range, see "adc_5hc_hardware_compare_mode_t" or the appopriate
 * reference
 * manual for more information.
 *
 * @param base ADC peripheral base address.
 * @param Pointer to "adc_5hc_hardware_compare_config_t" structure.
 *
 */
void ADC_5HC_SetHardwareCompareConfig(ADC_5HC_Type *base, const adc_5hc_hardware_compare_config_t *config);

/*!
 * @brief Configures the hardware average mode.
 *
 * The hardware average mode provides a way to process the conversion result automatically by using hardware. The
 * multiple
 * conversion results are accumulated and averaged internally making them easier to read.
 *
 * @param base ADC peripheral base address.
 * @param mode Setting the hardware average mode. See "adc_5hc_hardware_average_mode_t".
 */
void ADC_5HC_SetHardwareAverageConfig(ADC_5HC_Type *base, adc_5hc_hardware_average_mode_t mode);

/*!
 * @brief Gets the converter's status flags.
 *
 * @param base ADC peripheral base address.
 *
 * @return Flags' mask if indicated flags are asserted. See "adc_5hc_status_flags_t".
 */
static inline uint32_t ADC_5HC_GetStatusFlags(ADC_5HC_Type *base)
{
    return base->GS;
}

/*!
 * @brief Clears the converter's status falgs.
 *
 * @param base ADC peripheral base address.
 * @param mask Mask value for the cleared flags. See "adc_5hc_status_flags_t".
 */
void ADC_5HC_ClearStatusFlags(ADC_5HC_Type *base, uint32_t mask);

#endif /* _FSL_ADC_5HC_H_ */
