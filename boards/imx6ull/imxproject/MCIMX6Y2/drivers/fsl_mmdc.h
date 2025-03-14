/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_MMDC_H
#define _FSL_MMDC_H

#include "fsl_common.h"

/*!
 * @addtogroup mmdc
 * @{
 */

/******************************************************************************
 * Definitions.
 *****************************************************************************/
/*! @name Driver version */
/*@{*/
/*! @brief MMDC driver Version 2.1.1 */
#define FSL_MMDC_DRIVER_VERSION (MAKE_VERSION(2U, 1U, 1U))
/*@}*/

/*! @brief MMDC retry times */
/*@{*/
#define MMDC_TIMEOUT (500U)
/*@}*/

/*!< @brief define the read DQS fine tuning mask value */
/*@{*/
#define MMDC_READ_DQS_FINE_TUNING_MASK (0x77777777U)
/*@}*/

/*!< @brief define the write DQS fine tuning mask value */
/*@{*/
#define MMDC_WRITE_DQS_FINE_TUNING_MASK (0xF3333333U)
/*@}*/

/*!< @brief define the calibration predefine value */
/*@{*/
#define MMDC_PRE_DEFINE_VALUE_DEFAULT (0xCCU)
/*@}*/

/*!< @brief according to ERR005778 description */
/*@{*/
#define MMDC_MEASUREUNIT_ERR_FREQ (100000000U)
/*@}*/

/*! @brief  MMDC status return codes. */
enum _mmdc_status
{
    kStatus_MMDC_ErrorDGCalibration = MAKE_STATUS(kStatusGroup_MMDC, 1),    /*!< MMDC error DG calibration. */
    kStatus_MMDC_ErrorReadCalibration = MAKE_STATUS(kStatusGroup_MMDC, 2),  /*!< MMDC error read calibration. */
    kStatus_MMDC_ErrorWriteCalibration = MAKE_STATUS(kStatusGroup_MMDC, 3), /*!< MMDC error write calibration. */
    kStatus_MMDC_ErrorWriteLevelingCalibration = MAKE_STATUS(kStatusGroup_MMDC, 4),
    /*!< MMDC error write leveling calibration. */
    kStatus_MMDC_WaitFlagTimeout = MAKE_STATUS(kStatusGroup_MMDC, 5), /*!< MMDC wait flag timeout. */
};

/*! @brief LPDDR2 device list*/
typedef enum _mmdc_device_type
{
    kMMDC_LPDDR2_S4 = 0x0U, /*!< LPDDR2-S4 */
    kMMDC_LPDDR2_S2 = 0x1U, /*!< LPDDR2-S2 */
    kMMDC_DDR3 = 0x2U,      /*!< DDR3 device */
} mmdc_device_type_t;

/*! @brief LPDDR2 device bank number*/
typedef enum _mmdc_device_bank_num
{
    kMMDC_Bank8 = 0U, /*!< bank number 8 */
    kMMDC_Bank4 = 1U, /*!< bank number 4 */
} mmdc_device_bank_num_t;

/*! @brief define  for row addr width*/
typedef enum _mmdc_row_addr_width
{
    kMMDC_Row11Bits = 0U, /*!< row addr 11 bits */
    kMMDC_Row12Bits = 1U, /*!< row addr 12 bits */
    kMMDC_Row13Bits = 2U, /*!< row addr 13 bits */
    kMMDC_Row14Bits = 3U, /*!< row addr 14 bits */
    kMMDC_Row15Bits = 4U, /*!< row addr 15 bits */
    kMMDC_Row16Bits = 5U, /*!< row addr 16 bits */
} mmdc_row_addr_width_t;

/*! @brief define for col addr width */
typedef enum _mmdc_col_addr_width
{
    kMMDC_Col9Bits = 0U,  /*!< col addr 9 bits */
    kMMDC_Col10Bits = 1U, /*!< col addr 10 bits */
    kMMDC_Col11Bits = 2U, /*!< col addr 11 bits */
    kMMDC_Col8Bits = 3U,  /*!< col addr 8 bits */
    kMMDC_Col12Bits = 4U, /*!< col addr 12 bits */
} mmdc_col_addr_width_t;

/*! @brief define for burst length*/
typedef enum _mmdc_burst_len
{
    kMMDC_BurstLen4 = 0U, /*!< brust len 4 */
    kMMDC_BurstLen8,      /*!< burst len 8 */
    kMMDC_BurstLen16,     /*!< reserved */
} mmdc_burst_len_t;

/*!
* @brief define for command type
* 	auto refresh cmd: select correct CMD_CS before issue this cmd
* 	write mode register cmd:
        DDR2/DDR3:  CMD_CS,CMD_BA,CMD_ADDR_LSB,CMD_ADDR_MSB
        LPDDR2:		CMD_CS,MR_OP,MR_ADDR
* 	ZQ calibration cmd:
        DDR2/DDR3:	CMD_CS,{CMD_ADDR_MSB,CMD_ADDR_LSB}=0x400,or 0x0
        LPDDR2:		through MRW cmd
* 	PreChargeAll cmd: selecet correct CMD_CS
* 	MRR cmd: only for LPDDR2/LPDDR3 device,must set correct CMD_CS,MR_ADDR
*/
typedef enum _mmdc_cmd_type
{
    kMMDC_NormalOperation = 0x0U,   /*!< normal operation cmd*/
    kMMDC_AutoRefresh = 0x2U,       /*!< auto refresh cmd*/
    kMMDC_WriteModeRegister = 0x3U, /*!< load mode register for DDR2/DDR3,MRW for LPDDR2*/
    kMMDC_ZQCalibration = 0x04U,    /*!< ZQ calibration cmd*/
    kMMDC_PreChargeAll = 0x05U,     /*!< Precharge all cmd*/
    kMMDC_ReadModeRegister = 0x6U,  /*!< mode register read cmd*/
} mmdc_cmd_type_t;

/*! @brief MMDC ZQ calibration type.*/
typedef enum _mmdc_zq_calmode
{
    kMMDC_ZQCaltoIOHW = 0x4U,          /*!<ZQ calibration to IO pads only through HW*/
    kMMDC_ZQCaltoIODeviceLong = 0x01U, /*!<ZQ calibration to IO pads together with ZQ long cmd to device*/
    kMMDC_ZQCaltoDeviceOnly = 0x02U,   /*!<ZQ calibration to device with long/short cmd*/
    kMMDC_ZQCaltoIODeviceLongShort =
        0x03U,                  /*!< ZQ calibration to IO pads together with ZQ calibration cmd long/short to device*/
    kMMDC_ZQFinetuning = 0x0AU, /*!< HW ZQ res offset fine tuning */
    kMMDC_DisZQFinetuning = 0x0BU, /*!< disable HW ZQ res offset */
} mmdc_zq_calmode_t;

/*! @brief MMDC ZQ calibration frequency.*/
typedef enum _mmdc_zq_calfreq
{
    kMMDC_ZQCalFreq1ms = 0x0U, /*!<ZQ calibration is peformed every 1ms*/
    kMMDC_ZQCalFreq2ms = 0x1U, /*!<ZQ calibration is peformed every 2ms*/
    kMMDC_ZQCalFreq4ms = 0x2U, /*!<ZQ calibration is peformed every 4ms*/
    kMMDC_ZQCalFreq1s = 0x6U,  /*!<ZQ calibration is peformed every 1s*/
    kMMDC_ZQCalFreq16s = 0xEU, /*!<ZQ calibration is peformed every 16s*/
    kMMDC_ZQCalFreq32s = 0xFU, /*!<ZQ calibration is peformed every 32s*/
} mmdc_zq_calfreq_t;

/*! @brief define MMDC refresh selector-select source of the clock that will trigger each refresh cycle.*/
typedef enum _mmdc_refresh_sel
{
    kMMDC_RefreshTrigBy64K = 0U, /*!< refresh trigger frequency 64K */
    kMMDC_RefreshTrigBy32K,      /*!< refresh trigger frequency 32K */
    kMMDC_RefreshTrigDDRCycles,  /*!< refresh trigger every amount of cycles that are configured in REF_CNT field */
    kMMDC_RefreshTrigNone,       /*!< auto refresh disable */
} mmdc_refresh_sel_t;

/*! @brief define MMDC profiling action define .*/
typedef enum _mmdc_profiling_action
{
    kMMDC_EnProfilingWithID, /*!< enable profiling with special ID */
    kMMDC_FreezeProfiling,   /*!< freeze profiling */
    kMMDC_CheckOverFlow,     /*!< check the counter overflow */
} mmdc_profiling_action_t;

/*! @brief MMDC calibration type define .*/
typedef enum _mmdc_calibration_type
{
    kMMDC_CalWithPreSetValue, /*!< calibration with preset value */
    kMMDC_CalWithMPR,         /*!< HW calibration with the MPR */
    kMMDC_CalWithPreDefine,   /*!< calibration with Pre-defined value */

} mmdc_calibration_type_t;

/*! @brief define MMDC wait cycles before comparing data during calibration.*/
typedef enum _mmdc_calibaration_waitcycles
{
    kMMDC_Wait16DDRCycles = 0U,    /*!< wait 16 DDR cycles before comparing sample data */
    kMMDC_Wait32DDRCycles = 0x01U, /*!< wait 32 DDR cycles before comparing sample data */
} mmdc_calibaration_waitcycles_t;

/*! @brief define MMDC parameter fine tuning duty cyle.*/
typedef enum _mmdc_fine_tuning_dutycycle
{
    kMMDC_DutyHighPercent48_5 = 0x1U, /*!< 51.5% low 48.5% high */
    kMMDC_DutyHighPercent50 = 0x2U,   /*!< 50% duty cycle */
    kMMDC_DutyHighPercent51_5 = 0x4U, /*!< 48.5% low 51.5% high */
} mmdc_fine_tuning_dutycycle_t;

/*! @brief define MMDC on chip termination configurations.*/
typedef enum _mmdc_termination_config
{
    kMMDC_RttNomDisabled = 0x0U, /*!< Rtt_Nom Disabled */
    kMMDC_RttNom120ohm = 0x1U,   /*!< Rtt_Nom 120 Ohm */
    kMMDC_RttNom60ohm = 0x2U,    /*!< Rtt_Nom 60 Ohm */
    kMMDC_RttNom40ohm = 0x3U,    /*!< Rtt_Nom 40 Ohm */
    kMMDC_RttNom30ohm = 0x4U,    /*!< Rtt_Nom 30 Ohm */
    kMMDC_RttNom24ohm = 0x5U,    /*!< Rtt_Nom 24 Ohm */
    kMMDC_RttNom20ohm = 0x6U,    /*!< Rtt_Nom 20 Ohm */
    kMMDC_RttNom17ohm = 0x7U,    /*!< Rtt_Nom 17 Ohm */
} mmdc_termination_config_t;

/*! @brief define LPDDR2 device derating type.*/
enum _mmdc_lpddr2_derate
{
    kMMDC_NoUpdateDerate = 0U,       /*!< no derate */
    kMMDC_UpdateRefreshRate = 0x01U, /*!< refresh rate derate */
    kMMDC_DerateTiming = 0x02U,      /*!< derating relate timing */
};

/*! @brief MMDC exclusive acess config type.*/
enum _mmdc_exaccess_type
{
    kMMDC_ExMonitorID0 = 0x01U,     /*!< config the exclusive access ID0 */
    kMMDC_ExMonitorID1 = 0x02U,     /*!< config the exclusive access ID1 */
    kMMDC_ExMonitorID2 = 0x04U,     /*!< config the exclusive access ID2 */
    kMMDC_ExMonitorID3 = 0x08U,     /*!< config the exclusive access ID3 */
    kMMDC_ExAccessResponse = 0x10U, /*!< config the exclusive access reponse */
};

/*! @brief MMDC read DQS gating calibration configuration collection.*/
typedef struct _mmdc_readDQS_calibration_config
{
    mmdc_calibration_type_t mode;              /*!< select calibration mode. */
    mmdc_calibaration_waitcycles_t waitCycles; /*!< MMDC wait cycles before comparing sample data. */
    uint8_t dqsGatingHalfDelay0;               /*!< Read DQS gating half cycles delay count for Byte0. */
    uint8_t dqsGatingAbsDelay0;  /*!< Absolute read DQS gating delay offset for Byte0, So the total read DQS gating
                                      delay is (dqsGatingHalfDelay0)*0.5*cycle + (dqsGatingAbsDelay0)*1/256*cycle. */
    uint8_t dqsGatingHalfDelay1; /*!< Read DQS gating half cycles delay count for Byte1. */
    uint8_t dqsGatingAbsDelay1;  /*!< Absolute read DQS gating delay offset for Byte1, So the total read DQS gating
                                      delay is (dqsGatingHalfDelay1)*0.5*cycle + (dqsGatingAbsDelay1)*1/256*cycle. */
    uint8_t readDelay0;          /*!< When using hardware calibration(MPR/Predefined mode), user should input
                                      RD_DL_ABS_OFFSET to place read DQS inside the read DQ window. */
    uint8_t readDelay1;          /*!< When using hardware calibration(MPR/Predefined mode), user should input
                                      RD_DL_ABS_OFFSET to place read DQS inside the read DQ window. */
} mmdc_readDQS_calibration_config_t;

/*! @brief MMDC write leveling calibration configuration collection.*/
typedef struct _mmdc_writeLeveling_calibration_config
{
    mmdc_calibration_type_t mode; /*!< select calibration mode. */
    uint8_t wLevelingOneDelay0;   /*!< Write leveling one cycles delay count for Byte0*/
    uint8_t wLevelingHalfDelay0;  /*!< Write leveling half cycles delay count for Byte0*/
    uint8_t wLevelingAbsDelay0;   /*!< Absolute Write leveling delay offset for Byte0, So the total delay is the sum of
                      (wLevelingAbsDelay0/256*cycle) + (wLevelingHalfDelay0*halfcycle) + (wLevelingOneDelay0*cycle). */
    uint8_t wLevelingOneDelay1;   /*!< Write leveling one cycles delay count for Byte1*/
    uint8_t wLevelingHalfDelay1;  /*!< Write leveling half cycles delay count for Byte1*/
    uint8_t wLevelingAbsDelay1;   /*!< Absolute Write leveling delay offset for Byte1, So the total delay is the sum of
                      (wLevelingAbsDelay1/256*cycle) + (wLevelingHalfDelay1*halfcycle) + (wLevelingOneDelay1*cycle). */
} mmdc_writeLeveling_calibration_config_t;

/*! @brief MMDC read calibration configuration collection.*/
typedef struct _mmdc_read_calibration_config
{
    mmdc_calibration_type_t mode; /*!< select calibration mode. */
    uint8_t readDelay0;           /*!< delay between read DQS strobe and read data of Byte0, RD_DL_ABS_OFFSET0.
                                  The delay of the delay-line would be (RD_DL_ABS_OFFSET0 / 256) * MMDC AXIclock (fast clock).
                                  when using hardware calibration(MPR/Predefined mode), user should input RD_DL_ABS_OFFSET0
                                  to place read DQS inside the read DQ window. */
    uint8_t readDelay1;           /*!< delay between read DQS strobe and read data of Byte1, RD_DL_ABS_OFFSET1.
                                  The delay of the delay-line would be (RD_DL_ABS_OFFSET1 / 256) * MMDC AXIclock (fast clock).
                                  when using hardware calibration(MPR/Predefined mode), user should input RD_DL_ABS_OFFSET1
                                  to place read DQS inside the read DQ window. */
} mmdc_read_calibration_config_t;

typedef struct _mmdc_write_calibration_config
{
    mmdc_calibration_type_t mode; /*!< select calibration mode. */
    uint8_t writeDelay0;          /*!< delay between write DQS strobe and write data of Byte0, WR_DL_ABS_OFFSET0
                                 The delay of the delay-line would be (WR_DL_ABS_OFFSET0 / 256) * MMDC AXIclock (fast clock).
                                 when using hardware calibration(MPR/Predefined mode), user should input WR_DL_ABS_OFFSET0
                                 to place write DQS inside the write DQ window. */
    uint8_t writeDelay1;          /*!< delay between write DQS strobe and write data of Byte1, WR_DL_ABS_OFFSET1
                                 The delay of the delay-line would be (WR_DL_ABS_OFFSET1 / 256) * MMDC AXIclock (fast clock).
                                 when using hardware calibration(MPR/Predefined mode), user should input WR_DL_ABS_OFFSET1
                                 to place write DQS inside the write DQ window. */
} mmdc_write_calibration_config_t;

/*! @brief MMDC write calibration configuration collection.*/
typedef struct _mmdc_fine_tuning_config
{
    uint32_t rDQOffset0; /*!< fine-tuning adjustment to every bit in the read DQ byte0 relative
                              to the read DQS, max dealy units can be add is 7*/
    uint32_t rDQOffset1; /*!< fine-tuning adjustment to every bit in the read DQ byte1 relative
                              to the read DQS, max dealy units can be add is 7*/
    uint32_t wDQOffset0; /*!< fine-tuning adjustment to every bit in the write DQ byte0 relative
                              to the write DQS, max dealy units can be add is 7*/
    uint32_t wDQOffset1; /*!< fine-tuning adjustment to every bit in the write DQ byte1 relative
                              to the write DQS, max dealy units can be add is 7*/
    uint32_t caDelay;    /*!< CA delay line fine tuning parameter. */

    mmdc_fine_tuning_dutycycle_t rDQDuty0;      /*!< Read DQS duty cycle fine tuning control of Byte1 */
    mmdc_fine_tuning_dutycycle_t rDQDuty1;      /*!< Read DQS duty cycle fine tuning control of Byte0 */
    mmdc_fine_tuning_dutycycle_t ddrCKDutyCtl0; /*!< Primary duty cycle fine tuning control of DDR clock */
    mmdc_fine_tuning_dutycycle_t ddrCKDutyCtl1; /*!< Secondary duty cycle fine tuning control of DDR clock */
    mmdc_fine_tuning_dutycycle_t wDQDuty0;      /*!< Write DQS duty cycle fine tuning control of Byte0 */
    mmdc_fine_tuning_dutycycle_t wDQDuty1;      /*!< Write DQS duty cycle fine tuning control of Byte1 */
} mmdc_fine_tuning_config_t;

/*! @brief MMDC odt configuration collection.*/
typedef struct __mmdc_odt_config
{
    mmdc_termination_config_t odtByte1Config; /*! On chip ODT byte1 resistor.*/
    mmdc_termination_config_t odtByte0Config; /*! On chip ODT byte0 resistor.*/
    bool enableActiveReadOdt;                 /*! Active read CS ODT enable.*/
    bool enableInactiveReadOdt;               /*! Inactive read CS ODT enable.*/
    bool enableActiveWriteOdt;                /*! Active write CS ODT enable.*/
    bool enableInactiveWriteOdt;              /*! Inactive write CS ODT enable.*/
} mmdc_odt_config_t;

/*! @brief MMDC power configutation collection.*/
typedef struct _mmdc_power_config
{
    bool wIdle;      /*!< get write request buffer Idle status */
    bool rIdle;      /*!< get read request buffer Idle status */
    bool isInAutoPS; /*!< indicate mmdc if in a automatic power saving mode */

    uint8_t idleClockToPS;         /*!< define the idle clock which device will automatically enter
                                       auto self-refresh mode ,default is 1024 clock cycles,
                                       max is 16320 cycles,calucate formula is idleClockToPS * 64 = idle clock
                                       note: idleClockToPS = 0 is forbidden*/
    uint8_t idleClockToPrecharge0; /*!< define the idle clock which device will automatically precharged.
                                       default is disable ,max clock is 128 clocks, calucate formula
                                       2^idleClockToPrecharge  = idle clock */
    uint8_t idleClockToPD0;        /*!< define the idle clock which device will enter power down, default is
                                       disable ,max clock is 32768 clocks, calucate formula
                                       idleClockToPD *16 = idle clock */
    uint8_t idleClockToPrecharge1; /*!< define the idle clock which device will automatically precharged.
                                       default is disable ,max clock is 128 clocks, calucate formula
                                       2^idleClockToPrecharge  = idle clock */
    uint8_t idleClockToPD1;        /*!< define the idle clock which device will enter power down, default is
                                       disable ,max clock is 32768 clocks, calucate formula
                                       idleClockToPD *16 = idle clock */

} mmdc_power_config_t;

/*! @brief MMDC ZQ configuration collection.*/
typedef struct _mmdc_zq_config
{
    mmdc_zq_calmode_t mode;  /*!< zq calibration mode. */
    uint8_t earlyCompTimer;  /*!< this field define the interval between the warming up of
                                  the comp of the ultra cal pad and the begining of the
                                  ZQ cal process with pads*/
    uint16_t tZQCl_Clocks;   /*!< This is the period of time that
                                  the MMDC has to wait after sending a short ZQ calibration
                                  and before sending other commands,max value 112 cycles,see
                                  RM for more detail ,lpddr2 device default is 360ns*/
    uint16_t tZQCs_Clocks;   /*!< This is the period of time that the MMDC has
                                  to wait after sending a long ZQ calibration and before
                                  sending other commands*/
    uint16_t tZQInit_Clocks; /*!< This is the period of time that the MMDC has to wait
                                  after sending a init ZQ calibration and before sending
                                  other commands.lpddr2 device default is 1us*/

    mmdc_zq_calfreq_t hwZQFreq; /*!< ZQ periodic calibration freq */

    uint8_t cmpOutSample; /*!< define the amount of cycle between driving the ZQ signal
                               to pad and till sampling the cmp enable output*/

    uint8_t hwPullDownOffset; /*!< define ZQ hardware pull down offset, used for fine tuning */
    uint8_t hwPullUpOffset;   /*!< define ZQ hardware pull up offset, used for fine tuning*/
} mmdc_zq_config_t;

/*! @brief MMDC cmd configuration collection.*/
typedef struct _mmdc_cmd_config
{
    uint8_t argMsb;      /*!< define the CMD_ADDR_MSB_MR_OP, for
                              lpddr2 device this field is mode
                              register oprand */
    uint8_t argLsb;      /*!< define the CMD_ADDR_LSB_MR_ADDR,for
                              lpddr2 device this field is mode
                              register addr*/
    uint8_t bankAddr;    /*!< define the bank address,this field
                              not relate with lpddr2 device */
    uint8_t targetCS;    /*!< select which CS to drive low.*/
    mmdc_cmd_type_t cmd; /*!< define the cmd to be send */

} mmdc_cmd_config_t;

/*! @brief MMDC device device timing configuration collection.
* clocks is ddr clock,(a+b), a is the value write to reigster,b is offset
*/
typedef struct _mmdc_device_timing
{
    uint8_t tRFC_Clocks;   /*!< Refresh cmd to active or refresh cmd time
                            default is (0x32+1) clocks,max is (255+1) clocks*/
    uint8_t tCKSRX_Clocks; /*!< Valid clock before self-refresh exit,self-refresh timing
                            default is 2 clocks,max is 7 clocks */
    uint8_t tCKSRE_Clocks; /*!< Valid clock after self-refresh entry,self-refresh timing
                            default is 2 clocks.max is 7 clocks*/
    uint8_t tXSR_Clocks;   /*!< exit self refresh to a valid cmd,self-refresh timing
                            min value should set to 0x16,represent 23 clocks,max is 256 clocks*/

    /*  CKE timing*/
    uint8_t tCKE_Clocks; /*!< CKE minimum pulse width,default is (3+1) clocks,max is (7+1) clocks*/

    /*  read/write latency*/
    uint8_t tCL_Clocks;   /*!< CAS read latency,default is(3+3) clocks, max is (8+3) clocks*/
    uint8_t tCWL_Clocks;  /*!< CAS write latency, default is (3+1)clocks, max is (6+1) clocks*/
    uint8_t ralat_Clocks; /*!< define write additional latency in misc, default is disable,
                               max is (7+2)clocks*/
    uint8_t walat_Clocks; /*!< define read additional latency in misc, default is disable,
                               max is 3 clocks*/

    /*  cmd and address timing*/
    uint8_t tFAW_Clocks; /*!< Four bank active window,all bank,default is (6+1) clocks,max is
                            (31+1)clocks*/
    uint8_t tRAS_Clocks; /*!< row active time,Active to Precharge cmd period,same bank,default is (9+1)
                            clocks ,max is (30+1)clocks*/
    uint8_t tRC_Clocks;  /*!< Active to active or refresh cmd period,default is (0+1)clocks,max is
                            (62+1)clocks*/
    uint8_t tRCD_Clocks; /*!< Active cmd to internal read/write delay time,default is 0+1 clocks,max
                           is 14+1 clocks*/
    uint8_t tRP_Clocks;  /*!< Precharge cmd period-per bank,default is 0+1 clock,max is
                           14+1 clocks*/
    uint8_t tRPA_Clocks; /*!< Precharge cmd period-all bank,default is 0+1 clock,max is
                           14+1 clocks*/
    uint8_t tWR_Clocks;  /*!< Write recovery time,default is 0+1 clock,max is 7+1 clocks*/
    uint8_t tMRD_Clocks; /*!< Mode register set cmd cycle,should set to max(tMRR,tMRW),
                            default is 1+1 clock,max is 15+1 clocks*/
    uint8_t tRTP_Clocks; /*!< Internal read cmd to pre-charge cmd delay,default is 2+1 clock,max is
                           7+1 clocks*/
    uint8_t tWTR_Clocks; /*!< Internal write cmd to read cmd delay,default is 2+1 clock,max is
                           7+1 clocks*/
    uint8_t tRRD_Clocks; /*!< active bankA to active bankB ,Internal read cmd to pre-charge cmd,
                           default is 0+1 clock,max is 6+1 clocks*/
    uint8_t tXP_Clocks;  /*!< exit power down to any cmd, default (1+1) clocks,
                           max is (7 +1) clocks*/
    /*  reset timing*/
    uint8_t tRSTtoCKE_Clocks; /*!< idle time until first reset is assert,default is 14-2 clock,max is
                                0X3f-2 clocks,for LPDDR2 device  default is 200us*/
    uint32_t tDAI_Clocks;     /*!< Maximum device auto initialization period for LPDDR2, not relavant to DDR3. */

    uint8_t tRTWSAME_Clocks; /*!< Read to write commands delay for same chip select, total delay is calculated
                              according to: BL/2 + RTW_SAME + (tCL-tCWL) + RALAT */
    uint8_t tWTRDIFF_Clocks; /*!< Write to read commands delay for different chip select, total delay is calculated
                              according to: BL/2 + WTR_DIFF + (tCL-tCWL) + RALAT*/
    uint8_t tWTWDIFF_Clocks; /*!< Write to write commands delay for different chip select, total delay is
                              calculated according to: BL/2 + WTW_DIFF */
    uint8_t tRTWDIFF_Clocks; /*!< Read to write commands delay for different chip select, total delay is calculated
                              according to: BL/2 + RTW_DIFF + (tCL - tCWL) + RALAT */
    uint8_t tRTRDIFF_Clocks; /*!< Read to read commands delay for different chip select. total delay is calculated
                              according to: BL/2 + RTR_DIFF */

    uint8_t tXPDLL_Clocks;      /*!< Exit precharge power down with DLL frozen to commands requiring DLL,
                                  not relavant to LPDDR2. */
    uint16_t tDLLK_Clocks;      /*!< DLL locking time, not relavant to LPDDR2. */
    uint8_t tXPR_Clocks;        /*!< CLKE High to a valid command, not relevant to LPDDR2. */
    uint8_t tSDEtoRST_Clocks;   /*!< Time from SDE enable until DDR #reset is high, not relavant to LPDDR2. */
    uint8_t tAOFPD_Clocks;      /*!< Asynchronous RTT turn-off delay, not relavant to LPDDR2. */
    uint8_t tAONPD_Clocks;      /*!< Asynchronous RTT turn-on delay, not relavant to LPDDR2. */
    uint8_t tODTIdleOff_Clocks; /*!< ODT turn off latency, not relavant to LPDDR2. */
} mmdc_device_timing_t;

/*! @brief MMDC auto refresh configuration collection.*/
typedef struct _mmdc_auto_refresh
{
    uint16_t refreshCnt;               /*!< define refresh counter which is how many DDR clock cycles arrive
                                        will trigger auto refresh, only applied when choose refreshTrigSrc as
                                        kMMDC_RefreshTrigDDRCycles*/
    uint16_t refreshRate;              /*!< refresh rate-means how much cmd will
                                            send once auto refresh being trigger*/
    mmdc_refresh_sel_t refreshTrigSrc; /*!< select refresh trigger clock source */
} mmdc_auto_refresh_t;

/*! @brief MMDC exclusive access configuration collection.*/
typedef struct _mmdc_exaccess_config
{
    uint16_t excMonitorID0; /*!< exclusive monitor ID 0*/
    uint16_t excMonitorID1; /*!< exclusive monitor ID 1*/
    uint16_t excMonitorID2; /*!< exclusive monitor ID 2*/
    uint16_t excMonitorID3; /*!< exclusive monitor ID 3*/

    bool secErrLock; /*!< define if lock ARCR_SEC_ERR_EN this bit can't
                        update if locked */
    bool secErrEn;   /*!< This bit defines whether security read/write
                        access violation result in SLV Error response
                        or in OKAY response*/
    bool excErrEn;   /*!< This bit defines whether exclusive read/write
                        access violation of AXI 6.2.4 rule result in SLV
                        Error response or in OKAY response .
                        Default value is 0x1 response is SLV Error*/

} mmdc_exaccess_config_t;

/*! @brief MMDC module profiling configuration collection.*/
typedef struct _mmdc_profiling_config
{
    mmdc_profiling_action_t type; /*!< profiling action */

    bool overFlowCount; /*!< profiling cycle counter over flag*/

    uint16_t axiIDMask; /*!< profiling AXI ID mask */
    uint16_t axiID;     /*!< profiling AXI ID */

    uint32_t totalCount;     /*!< total cycle count-readonly */
    uint32_t busyCount;      /*!< busy count-readonly */
    uint32_t readCount;      /*!< total read count-readonly */
    uint32_t writeCount;     /*!< total write count-readonly */
    uint32_t readByteCount;  /*!< read byte count-readonly */
    uint32_t writeByteCount; /*!< total write byte count-readonly */

} mmdc_profiling_config_t;

/*! @brief MMDC performance configuration collection.*/
typedef struct _mmdc_performance_config
{
#if (defined(FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL) && FSL_FEATURE_MMDC_HAS_ARB_REO_CONTROL)
    bool enArbitration; /*!< define if enable arbitration in MAARCR*/
    bool enReordering;  /*!< define if enable reordering in MAARCR*/
#endif
    bool enRCH; /*!< define if enable real time channel in MAARCR */

    uint32_t ratePageHit;   /*!< static score taken into account in case the pending access has a page hit in MAARCR*/
    uint32_t rateAccessHit; /*!< static score taken into account in case the pending access
                             is same as before in MAARCR*/
    uint32_t dynJump; /*!< dynamic score give to any pending access in case it was not chosen in arbitration in MAARCR*/
    uint32_t dynMax;  /*!< dynamic score max value in MAARCR*/

    uint32_t guard; /*!< use to prevent a starvation of access*/

    uint32_t cmdPredict; /*!< define cmd prediction work mode in misc*/

} mmdc_performance_config_t;

/*! @brief MMDC module relate configuration collection.*/
typedef struct _mmdc_device_config
{
    uint16_t MR0; /* Mode register 0, Only relavant to DDR3 */
    uint16_t MR1; /* Mode register 1. */
    uint16_t MR2; /* Mode register 2. */
    uint16_t MR3; /* Mode register 3. */
} mmdc_device_config_t;

/*! @brief MMDC module relate configuration collection.*/
typedef struct _mmdc_config
{
    mmdc_device_type_t devType;     /*!< define device type */
    uint32_t devSize;               /*!< define the size of the device  */
    mmdc_device_bank_num_t devBank; /*!< define device total bank number */
    mmdc_row_addr_width_t rowWidth; /*!< define row width in MDCTL */
    mmdc_col_addr_width_t colWidth; /*!< define col width in MDCTL */
    mmdc_burst_len_t burstLen;      /*!< define burst length MDCTL */
    bool bankInterleave;            /*!< define indicate bank interleave on/off in misc */
    bool secondDDRClock;            /*!< define gating the secondary DDR clock in misc */
    bool enableOnlyCS0;             /*!< Only enable CS0 */
    mmdc_odt_config_t *ODTConfig;   /*!< Pointer to on die termination config,
                                         NULL means disable, for LPDDR2, pass NULL. */
    mmdc_device_timing_t *timing;   /*! Pointer to device timing structure. */

    mmdc_zq_config_t *zqCalibration;                          /*! Pointer to ZQ calibration config,
                                                               NULL means do not need. */
    mmdc_device_config_t *deviceConfig[2];                    /*! Pointer to device configuration CS0/CS1,
                                                                NULL means do not need. */
    mmdc_readDQS_calibration_config_t *readDQSCalibration[2]; /*! Pointer to read DQS calibration config CS0/CS1,
                                                                NULL meansdo not need, for LPDDR2, pass NULL. */
    mmdc_writeLeveling_calibration_config_t *wLevelingCalibration[2]; /*! Pointer to write leveling calibration config,
                                                                       NULL means do not need, for LPDDR2, pass NULL. */
    mmdc_read_calibration_config_t *readCalibration[2];               /*! Pointer to read calibration config CS0/CS1,
                                                                       NULL means do not need. */
    mmdc_write_calibration_config_t *writeCalibration[2];             /*! Pointer to write calibration config CS0/CS1,
                                                                        NULL means do not need. */
    mmdc_fine_tuning_config_t *tuning;                                /*! Pointer to fine tuning config,
                                                                       NULL means do not need. */
    mmdc_auto_refresh_t *autoRefresh;                                 /*! Pointer to auto refresh config structure,
                                                                       NULL means do not need. */
    mmdc_power_config_t *powerConfig;                                 /*! Pointer to auto power config structure,
                                                                       NULL means do not need. */
} mmdc_config_t;

/*! @brief MMDC switch frequency api prototype.*/
typedef void (*MMDC_SwitchFrequency)(MMDC_Type *, void *, void *, uint32_t);

/*************************************************************************************************
 * API
 ************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
* @brief MMDC module get the default configutation
* get timing/power/zq configuration
* @param base MMDC peripheral base address
* @param mmdc config collection pointer
*    config->bankInterleave = true;
*    config->secondDDRClock = true;
*    config->enableOnlyCS0 = true;
*    config->devType = kMMDC_DDR3;
*    config->devSize = 0x40000000U;
*    config->devBank = kMMDC_Bank8;
*    config->rowWidth = kMMDC_Row16Bits;
*    config->colWidth = kMMDC_Col10Bits;
*    config->burstLen = kMMDC_BurstLen8;
*    config->ODTConfig = NULL;
*    config->timing = NULL;
*    config->zqCalibration = NULL;
*    config->deviceConfig[0] = NULL;
*    config->deviceConfig[1] = NULL;
*    config->readDQSCalibration[0] = NULL;
*    config->readDQSCalibration[1] = NULL;
*    config->wLevelingCalibration[0] = NULL;
*    config->wLevelingCalibration[1] = NULL;
*    config->readCalibration[0] = NULL;
*    config->readCalibration[1] = NULL;
*    config->writeCalibration[0] = NULL;
*    config->writeCalibration[1] = NULL;
*    config->tuning = NULL;
*    config->autoRefresh = NULL;
*    config->powerConfig = NULL;
*/
void MMDC_GetDefaultConfig(mmdc_config_t *config);

/*!
* @brief MMDC module initialization function
* @param base MMDC peripheral base address
* @param mmdc config collection pointer
* @retval kStatus_Success Initialization succeed
* @retval kStatus_MMDC_ErrorDGCalibration Error happened during hardware DQS gate calibration
* @retval kStatus_MMDC_ErrorReadCalibration Error happened during hardware read calibration
* @retval kStatus_MMDC_ErrorWriteCalibration Error happened during hardware write calibration
* @retval kStatus_MMDC_ErrorWriteLevelingCalibration Error happened during hardware write leveling calibration
*/
status_t MMDC_Init(MMDC_Type *base, mmdc_config_t *config);

/*!
* @brief MMDC module deinit function
* @param base MMDC peripheral base address
*/
void MMDC_Deinit(MMDC_Type *base);

/*!
 * @name device operation
 * @{
 */

/*!
 * @name device operation
 * @{
 */

/*!
* @brief MMDC module process the command,support transfer multi cmd in once function call
* @param base MMDC peripheral base address
* @param cmd configuration collection
*/
void MMDC_HandleCommand(MMDC_Type *base, mmdc_cmd_config_t *config);

/*!
* @brief MMDC get the read data
* @param base MMDC peripheral base address
* @param the pointer which used to store read data
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout Read data flag wait timeout
*/
status_t MMDC_GetReadData(MMDC_Type *base, uint32_t *data);

/*!
* @brief MMDC module enhance performance function
* @param base MMDC peripheral base address
* @param performance configuration collection
*/
void MMDC_EnhancePerformance(MMDC_Type *base, const mmdc_performance_config_t *config);

/*!
* @brief Enable MMDC module periodic refresh scheme config
* @param base MMDC peripheral base address
* @param mmdc auto refresh configuration collection
*/
void MMDC_EnableAutoRefresh(MMDC_Type *base, mmdc_auto_refresh_t *config);

/*!
* @brief Disable MMDC module periodic refresh scheme
* @param base MMDC peripheral base address
*/
static inline void MMDC_DisableAutoRefresh(MMDC_Type *base)
{
    /* disable the auto refresh ,then manual issue a refresh cmd */
    base->MDREF |= MMDC_MDREF_REF_SEL_MASK;
    /* MMDC will start a refresh cycle immediately according to number
    of refresh commands that are configured in refreshRate field */
    base->MDREF |= MMDC_MDREF_START_REF_MASK;
}

/*!
* @brief MMDC enable automatic power saving.
* @param base MMDC peripheral base address
* @param mmdc device configuration collection, pointer for the configuration.
*/
void MMDC_EnablePowerSaving(MMDC_Type *base, mmdc_power_config_t *config);

/*!uint8_t targetCS,
* @brief MMDC disable automatic power saving.
* @param base MMDC peripheral base address
*/
static inline void MMDC_DisablePowerSaving(MMDC_Type *base)
{
    /* disable auto power down/precharge */
    base->MDPDC &= ~(MMDC_MDPDC_PWDT_0_MASK | MMDC_MDPDC_PRCT_0_MASK);
    base->MAPSR |= MMDC_MAPSR_PSD_MASK;
}

/*!
* @brief MMDC profiling mechanism
* @param base MMDC peripheral base address
* @param mmdc profiling status and control
*/
void MMDC_Profiling(MMDC_Type *base, mmdc_profiling_config_t *config);

/*!
* @brief MMDC update device refresh  rate and derate timing
* for LPDDR2 device only
* @param base MMDC peripheral base address
* @param auto refresh configuration collection,can set
* to NULL,when do not change refresh rate
* @param derating type
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout LPDDR2 AC timing/refresh derate wait flag timeout
*/
status_t MMDC_LPDDR2UpdateDerate(MMDC_Type *base, mmdc_auto_refresh_t *config, uint32_t type);

/*!
* @brief MMDC device operation temp monitor function
* @param base MMDC peripheral base address
* @param MR4 pointer,use to store the mode register4 value
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout Get MR4 data flag timeout
*/
status_t MMDC_MonitorLPDDR2OperationTemp(MMDC_Type *base, uint32_t *mr4);

/*!
* @brief MMDC module read DQS gating calibration function
* @param base MMDC peripheral base address
* @param config calibration configuration collection
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_ErrorDGCalibration Read DQS data gate hardware calibration error
*/
status_t MMDC_ReadDQSGatingCalibration(MMDC_Type *base, mmdc_readDQS_calibration_config_t *config);

/*!
* @brief MMDC module write leveling calibration function
* @param base MMDC peripheral base address
* @param config calibration configuration collection
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_ErrorWriteLevelingCalibration write leveling hardware calibration error
*/
status_t MMDC_WriteLevelingCalibration(MMDC_Type *base, mmdc_writeLeveling_calibration_config_t *config);

/*!
* @brief MMDC module write calibration function
* @param base MMDC peripheral base address
* @param config calibration configuration collection
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_ErrorWriteCalibration write hardware calibration error
*/
status_t MMDC_WriteCalibration(MMDC_Type *base, mmdc_write_calibration_config_t *config);

/*!
* @brief MMDC module read calibration function
* @param base MMDC peripheral base address
* @param config calibration configuration collection
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_ErrorReadCalibration read hardware calibration error
*/
status_t MMDC_ReadCalibration(MMDC_Type *base, mmdc_read_calibration_config_t *config);

/*!
* @brief MMDC module read calibration function
* @param base MMDC peripheral base address
* @param devType MMDC device type
* @param config fine tuning configuration collection
*/
void MMDC_DoFineTuning(MMDC_Type *base, mmdc_device_type_t devType, mmdc_fine_tuning_config_t *config);

/*!
* @brief set timing parameter
* @param MMDC peripheral base address
* @param timing pointer to timing structure
*/
void MMDC_SetTiming(MMDC_Type *base, mmdc_device_type_t devType, mmdc_device_timing_t *timing);

/*!
 * @brief Initialize MMDC controlled device.
 * @param MMDC base address
 * @param device basic config info pointer
 */
void MMDC_DeviceInit(MMDC_Type *base, mmdc_device_type_t devType, uint8_t targetCS, mmdc_device_config_t *devConfig);

/*!
* @brief MMDC module enter/exit configuration mode function
* @param base MMDC peripheral base address
* @param enable enter/exit flag
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout Enter configuration mode time out
*/
status_t MMDC_EnterConfigurationMode(MMDC_Type *base, bool enable);

/*!
* @brief MMDC do ZQ calibration function
* @param base MMDC peripheral base address
* @param devType device type
* @param zqCal info pointer
*/
void MMDC_DoZQCalibration(MMDC_Type *base, mmdc_device_type_t devType, mmdc_zq_config_t *zqCal);

/*!
* @brief MMDC enable/disable low power mode
* Once enable device will enter self-refresh mode
* @param base MMDC peripheral base address
* @param enable enable/disable flag
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout Enter low power mode timeout
*/
status_t MMDC_EnableLowPowerMode(MMDC_Type *base, bool enable);

/*!
* @brief MMDC enable/disable dynamic frequency change mode
* Once enable device will enter self-refresh mode
* @param base MMDC peripheral base address
* @param enable enable/disable flag
* @retval kStatus_Success Read data succeed
* @retval kStatus_MMDC_WaitFlagTimeout Enter DVFS mode timeout
*/
status_t MMDC_EnableDVFSMode(MMDC_Type *base, bool enable);

/*!
* @brief MMDC module reset function
* when you call this function will reset all internal register
* user need bo module init function bue do not need to do device init
* @param base MMDC peripheral base address
*/
void MMDC_Reset(MMDC_Type *base);

/*! @brief define the mmdc switch frequency.
* @param MMDC base address
* @param CCM base address
* @param iomux base address
* @param target frequency value for LPDDR2 and parameter address for DDR3
* @param assembly switch frequency code address
*/
static inline void MMDC_SwitchDeviceFrequency(
    MMDC_Type *base, void *ccm, void *iomux, uint32_t param, uint32_t codeAddr)
{
    (*(MMDC_SwitchFrequency)(codeAddr))(base, ccm, iomux, param);
}
/*!
 * @name debug
 * @{
 */

/*!
* @brief MMDC enable/disable the SBS-step by step debug feature
* @param base MMDC peripheral base address
*/
static inline void MMDC_EnableSBS(MMDC_Type *base, bool enable)
{
    if (enable)
    {
        /* enable the SBS feature */
        base->MADPCR0 |= MMDC_MADPCR0_SBS_EN_MASK;
    }
    else
    {
        /* disable the SBS feature */
        base->MADPCR0 &= ~MMDC_MADPCR0_SBS_EN_MASK;
    }
}

/*!
* @brief MMDC trigger the MMDC dispatch the one pending request to device
* @param base MMDC peripheral base address
*/
static inline void MMDC_TriggerSBS(MMDC_Type *base)
{
    /* trigger the MMDC dispatch request to device */
    base->MADPCR0 |= MMDC_MADPCR0_SBS_MASK;
}

/*!
* @brief MMDC get AXI ddr which was dispatched by MMDC in SBS mode
* @param base MMDC peripheral base address
*/
static inline uint32_t MMDC_GetAXIAddrBySBS(MMDC_Type *base)
{
    /* return MASBS0 which contain the AXI addr */
    return base->MASBS0;
}

/*!
* @brief MMDC get AXI attribute which was dispatched by MMDC in SBS mode
* @param base MMDC peripheral base address
*/
static inline uint32_t MMDC_GetAXIAttributeBySBS(MMDC_Type *base)
{
    /* return MASBS1 which contain the AXI attribute */
    return base->MASBS1;
}

/*!
* @brief MMDC enable/disable profiling feature
* @param base MMDC peripheral base address
* @param enable or disable flag
*/
static inline void MMDC_EnableProfiling(MMDC_Type *base, bool enable)
{
    if (enable)
    {
        /* enable profiling feature */
        base->MADPCR0 |= MMDC_MADPCR0_DBG_EN_MASK;
    }
    else
    {
        /* disable profiling feature */
        base->MADPCR0 &= ~MMDC_MADPCR0_DBG_EN_MASK;
    }
}

/*!
* @brief MMDC resume profiling
* @param base MMDC peripheral base address
*/
static inline void MMDC_ResumeProfiling(MMDC_Type *base)
{
    /* resume profiling */
    base->MADPCR0 &= ~MMDC_MADPCR0_PRF_FRZ_MASK;
}

/*!
* @brief MMDC reset profiling
* @param base MMDC peripheral base address
*/
static inline void MMDC_ResetProfiling(MMDC_Type *base)
{
    /* reset profiling */
    base->MADPCR0 |= MMDC_MADPCR0_DBG_RST_MASK;
}

/*!
* @brief MMDC exclusive access config function,config the monitor ID and response
* @param base MMDC peripheral base address
* @param exclusive access config collection
* @param exclusive access config type
*/
void MMDC_ExclusiveAccess(MMDC_Type *base, mmdc_exaccess_config_t *config, uint32_t type);

/*! @} */
#if defined(__cplusplus)
}
#endif
/*! @} */

#endif /* _FSL_MMDC_H */
