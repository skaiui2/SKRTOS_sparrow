#include "stm32f10x.h"

/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */

uint32_t fac_us = 0;
uint32_t reload = 0;
void delay_init ( u8 SYSCLK )
{
    fac_us = SYSCLK;
}

void delay_us ( u32 nus )
{
    u32 ticks;									//存储需要节拍数
    u32 told, tnow, tcnt = 0;					//told：上一次存储的时间，tnew：当前时间；tcent：累计的时间差
    u32 reload = SysTick->LOAD;					//读取sysTick装载值
    ticks = nus * fac_us; 						//需要的节拍数
    told = SysTick->VAL;        				//读取当前计数值
    while (1)
    {
        tnow = SysTick->VAL;                                                    //时钟时递减时钟
        tcnt = tnow <= told ? tcnt + told - tnow : tcnt + reload - tnow + told; //获得上一次记录的时间和当前时间之间的差值
        if (tcnt >= ticks)
            return;  //时间超过/等于要延迟的时间,则退出.
        told = tnow; //将当前时间记录下来
    }
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
void delay_ms(uint32_t xms)
{
	while(xms--)
	{
		delay_us(1000);
	}
}
 
/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		delay_ms(1000);
	}
} 
