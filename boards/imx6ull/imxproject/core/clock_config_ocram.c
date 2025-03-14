/*
 * Copyright 2017 NXP
 * All rights reserved.
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

#include "fsl_device_registers.h"
#include "clock_config.h"

void BOARD_SetRunClock(void)
{
    int32_t i;
    int32_t ddr3_switchFreqInfo[7];

    /* restore mmdc parameter */
    ddr3_switchFreqInfo[0] = MMDC->MDMISC;
    ddr3_switchFreqInfo[1] = MMDC->MDCFG0;
    ddr3_switchFreqInfo[2] = MMDC->MDCFG1;
    ddr3_switchFreqInfo[3] = MMDC->MPODTCTRL;
    ddr3_switchFreqInfo[4] = MMDC->MPDGCTRL0;
    ddr3_switchFreqInfo[5] = MMDC->MPRDDLCTL;
    ddr3_switchFreqInfo[6] = MMDC->MPWRDLCTL;

    /* disable automatic power saving. */
    MMDC->MAPSR |= MMDC_MAPSR_PSD_MASK;
    /* disable MMDC power down timer. */
    MMDC->MDPDC &= ~(MMDC_MDPDC_PRCT_0_MASK | MMDC_MDPDC_PRCT_0_MASK | MMDC_MDPDC_PWDT_0_MASK | MMDC_MDPDC_PWDT_1_MASK);
    /* delay for a while */
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
    /* enter configuration mode */
    MMDC->MDSCR = 0x8000;
    while ((MMDC->MDSCR & MMDC_MDSCR_CON_ACK_MASK) == 0)
    {
    }
    MMDC->MDSCR = 0x00008050;
    MMDC->MDSCR = 0x00008058;
    /* turn off the DLL */
    MMDC->MDSCR = 0x00018031;
    MMDC->MDSCR = 0x00018039;
    /* delay for a while */
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
    /* set DVFS - enter self refresh mode */
    MMDC->MAPSR |= MMDC_MAPSR_DVFS_MASK;
    /* exit configuration mode */
    MMDC->MDSCR = 0x0;
    /* poll DVFS ack. */
    while ((MMDC->MAPSR & MMDC_MAPSR_DVACK_MASK) == 0)
    {
    }

    /* mask handshake with mmdc_ch0 module due to no mmdc_ch0 module is this chip */
    CCM->CCDR |= CCM_CCDR_MMDC_CH0_MASK_MASK;
    CCM->CLPCR |= CCM_CLPCR_BYPASS_MMDC_CH0_LPM_HS_MASK;

    /* Set periph2_clk2 and periph_clk2 MUX to OSC */
    CCM->CBCMR = (CCM->CBCMR & ~(CCM_CBCMR_PERIPH_CLK2_SEL_MASK | CCM_CBCMR_PERIPH2_CLK2_SEL_MASK)) |
                 CCM_CBCMR_PERIPH_CLK2_SEL(1) | CCM_CBCMR_PERIPH2_CLK2_SEL(1);
    /* Let BUS and mmdc clock run on OSC */
    CCM->CBCDR =
        (CCM->CBCDR & ~(CCM_CBCDR_AXI_SEL_MASK | CCM_CBCDR_PERIPH_CLK_SEL_MASK | CCM_CBCDR_PERIPH2_CLK_SEL_MASK)) |
        CCM_CBCDR_AXI_SEL(0) | CCM_CBCDR_PERIPH_CLK_SEL(1) | CCM_CBCDR_PERIPH2_CLK_SEL(1);
    /* Wait handshake process */
    while (CCM->CDHIPR & (CCM_CDHIPR_PERIPH2_CLK_SEL_BUSY_MASK | CCM_CDHIPR_PERIPH_CLK_SEL_BUSY_MASK))
    {
    }

    /* Configure SYS PLL to 528M */
    CCM_ANALOG->PLL_SYS_SS &= ~CCM_ANALOG_PLL_SYS_SS_ENABLE_MASK;
    CCM_ANALOG->PLL_SYS_NUM = CCM_ANALOG_PLL_SYS_NUM_A(0);
    CCM_ANALOG->PLL_SYS = CCM_ANALOG_PLL_SYS_ENABLE_MASK | CCM_ANALOG_PLL_SYS_DIV_SELECT(1);
    while ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_LOCK_MASK) == 0)
    {
    }

    /* Configure USB PLL to 480M */
    CCM_ANALOG->PLL_USB1 = CCM_ANALOG_PLL_USB1_ENABLE_MASK | CCM_ANALOG_PLL_USB1_POWER_MASK |
                           CCM_ANALOG_PLL_USB1_EN_USB_CLKS_MASK | CCM_ANALOG_PLL_USB1_DIV_SELECT(0);
    while ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_USB1_LOCK_MASK) == 0)
    {
    }

    /* Configure PFD_528
     * PFD0     : Disable, set divider 27 352M
     * PFD1     : Disable, set divider 16 594M
     * PFD2     : Enable, set divider 24 396M
     * PFD3     : Disable, set divider 48 198M
     */
    /* Disable all clock output first. */
    CCM_ANALOG->PFD_528 |= CCM_ANALOG_PFD_528_PFD0_CLKGATE_MASK | CCM_ANALOG_PFD_528_PFD1_CLKGATE_MASK |
                           CCM_ANALOG_PFD_528_PFD2_CLKGATE_MASK | CCM_ANALOG_PFD_528_PFD3_CLKGATE_MASK;
    /* Set default divide value for all PFD. */
    CCM_ANALOG->PFD_528 = (CCM_ANALOG->PFD_528 &
                           ~(CCM_ANALOG_PFD_528_PFD0_FRAC_MASK | CCM_ANALOG_PFD_528_PFD1_FRAC_MASK |
                             CCM_ANALOG_PFD_528_PFD2_FRAC_MASK | CCM_ANALOG_PFD_528_PFD3_FRAC_MASK)) |
                          CCM_ANALOG_PFD_528_PFD0_FRAC(0x1B) | CCM_ANALOG_PFD_528_PFD1_FRAC(0x10) |
                          CCM_ANALOG_PFD_528_PFD2_FRAC(0x18) | CCM_ANALOG_PFD_528_PFD3_FRAC(0x30);
    /* Enable PFD_528 PFD2*/
    CCM_ANALOG->PFD_528 &= ~CCM_ANALOG_PFD_528_PFD2_CLKGATE_MASK;

    /* Configure PFD_480
     * PFD0     : Disable, set divider 12 720M
     * PFD1     : Disable, set divider 16 540M
     * PFD2     : Disable, set divider 17 508.2M
     * PFD3     : Disable, set divider 19 454.7M
     */
    /* Disable all clock output. */
    CCM_ANALOG->PFD_480 |= CCM_ANALOG_PFD_480_PFD0_CLKGATE_MASK | CCM_ANALOG_PFD_480_PFD1_CLKGATE_MASK |
                           CCM_ANALOG_PFD_480_PFD2_CLKGATE_MASK | CCM_ANALOG_PFD_480_PFD3_CLKGATE_MASK;
    /* Set default divide value for all PFD. */
    CCM_ANALOG->PFD_480 = (CCM_ANALOG->PFD_480 &
                           ~(CCM_ANALOG_PFD_480_PFD0_FRAC_MASK | CCM_ANALOG_PFD_480_PFD1_FRAC_MASK |
                             CCM_ANALOG_PFD_480_PFD2_FRAC_MASK | CCM_ANALOG_PFD_480_PFD3_FRAC_MASK)) |
                          CCM_ANALOG_PFD_480_PFD0_FRAC(0xC) | CCM_ANALOG_PFD_480_PFD1_FRAC(0x10) |
                          CCM_ANALOG_PFD_480_PFD2_FRAC(0x11) | CCM_ANALOG_PFD_480_PFD3_FRAC(0x13);

    /* Configure BUS clcok
     * AHB     : Sourced from SYS PLL PFD2 396M with divider 3 132M
     * IPG     : Sourced from AHB with divider 2 66M
     * AXI     : Sourced from SYS PLL PFD2 396M with divider 2 198M
     * MMDC    : Sourced from SYS PLL PFD2 396M with divider 1 396M
     */

    /* Set pre_periph2_clk and pre_periph_clk MUX to SYS PLL PFD2*/
    CCM->CBCMR = (CCM->CBCMR & ~(CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK | CCM_CBCMR_PRE_PERIPH2_CLK_SEL_MASK)) |
                 CCM_CBCMR_PRE_PERIPH_CLK_SEL(1) | CCM_CBCMR_PRE_PERIPH2_CLK_SEL(1);

    /* Set periph2_clk and periph_clk MUX to PLL2*/
    CCM->CBCDR = (CCM->CBCDR & ~(CCM_CBCDR_PERIPH2_CLK_SEL_MASK | CCM_CBCDR_PERIPH_CLK_SEL_MASK)) |
                 (CCM_CBCDR_PERIPH2_CLK_SEL(0) | CCM_CBCDR_PERIPH_CLK_SEL(0));
    /* Config AXI divide by 2, AHB divide by 3, IPG divide by 2, MMDC divide by 1*/
    CCM->CBCDR =
        (CCM->CBCDR &
         ~(CCM_CBCDR_AXI_PODF_MASK | CCM_CBCDR_AHB_PODF_MASK | CCM_CBCDR_IPG_PODF_MASK |
           CCM_CBCDR_FABRIC_MMDC_PODF_MASK)) |
        (CCM_CBCDR_AXI_PODF(1) | CCM_CBCDR_AHB_PODF(2) | CCM_CBCDR_IPG_PODF(1) | CCM_CBCDR_FABRIC_MMDC_PODF(0));
    /* Wait handshake process */
    while (CCM->CDHIPR &
           (CCM_CDHIPR_PERIPH2_CLK_SEL_BUSY_MASK | CCM_CDHIPR_PERIPH_CLK_SEL_BUSY_MASK | CCM_CDHIPR_AXI_PODF_BUSY_MASK |
            CCM_CDHIPR_AHB_PODF_BUSY_MASK | CCM_CDHIPR_MMDC_PODF_BUSY_MASK))
    {
    }
    /* Config other filed in CBCDR to certain value */
    CCM->CBCDR = (CCM->CBCDR &
                  ~(CCM_CBCDR_AXI_ALT_SEL_MASK | CCM_CBCDR_PERIPH_CLK2_PODF_MASK | CCM_CBCDR_PERIPH2_CLK2_PODF_MASK)) |
                 (CCM_CBCDR_AXI_ALT_SEL(0) | CCM_CBCDR_PERIPH_CLK2_PODF(0) | CCM_CBCDR_PERIPH2_CLK2_PODF(0));

    /* Select PLL3 to generate pll3_sw_clk */
    CCM->CBCDR = (CCM->CBCDR & ~CCM_CCSR_PLL3_SW_CLK_SEL_MASK) | CCM_CCSR_PLL3_SW_CLK_SEL(0);

    /* Make sure QSPI clock is enabled */
    CCM->CCGR3 |= CCM_CCGR3_CG7(3);
    /* Wait until QSPI is not busy */
    while (QuadSPI->SR & QuadSPI_SR_BUSY_MASK)
    {
    }
    /* Make QSPI enter module disable mode*/
    QuadSPI->MCR |= QuadSPI_MCR_MDIS_MASK;
    /* Changing clock source to USB1 PLL 480M with divider 6 80M*/
    CCM->CSCMR1 = (CCM->CSCMR1 & ~(CCM_CSCMR1_QSPI1_CLK_SEL_MASK | CCM_CSCMR1_QSPI1_PODF_MASK)) |
                  CCM_CSCMR1_QSPI1_CLK_SEL(0) | CCM_CSCMR1_QSPI1_PODF(5);
    /* Make QSPI exit module disable mode*/
    QuadSPI->MCR &= ~QuadSPI_MCR_MDIS_MASK;
    /* Reset AHB domain and buffer domian */
    QuadSPI->MCR |= QuadSPI_MCR_SWRSTHD_MASK | QuadSPI_MCR_SWRSTSD_MASK;
    /* Wait several time for the reset to finish*/
    for (i = 0; i < 500; i++)
    {
        QuadSPI->SR;
    }
    /* Disable module during the reset procedure */
    QuadSPI->MCR |= QuadSPI_MCR_MDIS_MASK;
    /* Clear the reset bits. */
    QuadSPI->MCR &= ~(QuadSPI_MCR_SWRSTHD_MASK | QuadSPI_MCR_SWRSTSD_MASK);
    /* Re-enable QSPI module */
    QuadSPI->MCR &= ~QuadSPI_MCR_MDIS_MASK;

    /*Continue switch ddr frequency*/
    /* enter step by step mode */
    MMDC->MADPCR0 |= MMDC_MADPCR0_SBS_EN_MASK;
    /* clear DVFS - exit self refresh mode. */
    MMDC->MAPSR &= ~MMDC_MAPSR_DVFS_MASK;
    while ((MMDC->MAPSR & MMDC_MAPSR_DVACK_MASK))
    {
    }
    /* force ZQ calibration */
    MMDC->MPZQHWCTRL = (MMDC->MPZQHWCTRL & ~(MMDC_MPZQHWCTRL_ZQ_MODE_MASK | MMDC_MPZQHWCTRL_ZQ_HW_FOR_MASK)) |
                       MMDC_MPZQHWCTRL_ZQ_MODE(3) | MMDC_MPZQHWCTRL_ZQ_HW_FOR(1);
    /* enable DQS gating */
    MMDC->MPDGCTRL0 &= ~MMDC_MPDGCTRL0_DG_DIS_MASK;
    /* enable and force measure. */
    MMDC->MPMUR0 = (MMDC->MPMUR0 & ~MMDC_MPMUR0_MU_BYP_EN_MASK) | MMDC_MPMUR0_FRC_MSR_MASK;
    while ((MMDC->MPMUR0 & MMDC_MPMUR0_FRC_MSR_MASK))
    {
    }
    /* disable dqs pull down in the IOMUX. */
    IOMUXC->SW_PAD_CTL_PAD_DDR[31] = (IOMUXC->SW_PAD_CTL_PAD_DDR[31] &
                                      ~(IOMUXC_SW_PAD_CTL_PAD_DDR_DSE_MASK | IOMUXC_SW_PAD_CTL_PAD_DDR_PKE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_DDR_PUE_MASK | IOMUXC_SW_PAD_CTL_PAD_DDR_PUS_MASK)) |
                                     IOMUXC_SW_PAD_CTL_PAD_DDR_DSE(7);
    IOMUXC->SW_PAD_CTL_PAD_DDR[32] = (IOMUXC->SW_PAD_CTL_PAD_DDR[32] &
                                      ~(IOMUXC_SW_PAD_CTL_PAD_DDR_DSE_MASK | IOMUXC_SW_PAD_CTL_PAD_DDR_PKE_MASK |
                                        IOMUXC_SW_PAD_CTL_PAD_DDR_PUE_MASK | IOMUXC_SW_PAD_CTL_PAD_DDR_PUS_MASK)) |
                                     IOMUXC_SW_PAD_CTL_PAD_DDR_DSE(7);
    /* enter configuration mode */
    MMDC->MDSCR = 0x8000;
    while ((MMDC->MDSCR & MMDC_MDSCR_CON_ACK_MASK) == 0)
    {
    }
    /* resotre misc register */
    MMDC->MDMISC = ddr3_switchFreqInfo[0];
    /* turn on the DLL */
    MMDC->MDSCR = 0x00028031;
    MMDC->MDSCR = 0x00028039;
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
    /* reset the DLL */
    MMDC->MDSCR = 0x09208030;
    MMDC->MDSCR = 0x09208038;
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
    /* config RTT_NOM = RZQ/2 */
    MMDC->MDSCR = 0x00428031;
    MMDC->MDSCR = 0x00428039;
    /* config MR2 */
    MMDC->MDSCR = 0x04008032;
    /* restore timing information */
    MMDC->MDCFG0 = ddr3_switchFreqInfo[1];
    MMDC->MDCFG1 = ddr3_switchFreqInfo[2];
    /* issue a ZQ command */
    MMDC->MDSCR = 0x04008040;
    MMDC->MDSCR = 0x04008048;
    /* restore ODT */
    MMDC->MPODTCTRL = ddr3_switchFreqInfo[3];
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
    /* restore_calibration */
    MMDC->MPDGCTRL0 = ddr3_switchFreqInfo[4];
    MMDC->MPRDDLCTL = ddr3_switchFreqInfo[5];
    MMDC->MPWRDLCTL = ddr3_switchFreqInfo[6];
    /* force measure. */
    MMDC->MPMUR0 |= MMDC_MPMUR0_FRC_MSR_MASK;
    while ((MMDC->MPMUR0 & MMDC_MPMUR0_FRC_MSR_MASK))
    {
    }
    /* clear SBS */
    MMDC->MADPCR0 &= ~MMDC_MADPCR0_SBS_EN_MASK;
    /* exit configuration mode */
    MMDC->MDSCR = 0x0;
    while ((MMDC->MDSCR & MMDC_MDSCR_CON_ACK_MASK))
    {
    }
    /* enable automatic power saving timer */
    MMDC->MAPSR &= ~MMDC_MAPSR_PSD_MASK;
    /* enable MMDC power down timer. */
    MMDC->MDPDC = (MMDC->MDPDC & ~(MMDC_MDPDC_PWDT_0_MASK | MMDC_MDPDC_PWDT_1_MASK)) | MMDC_MDPDC_PWDT_0(0x5) |
                  MMDC_MDPDC_PWDT_1(0x5);
}
