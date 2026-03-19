#include "dac.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

 

//DACͨ1ʼ
void Dac1_Init(void)
{  
  GPIO_InitTypeDef  GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitType;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹGPIOAʱ
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);//ʹDACʱ
	   
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//ģ
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//
  GPIO_Init(GPIOA, &GPIO_InitStructure);//ʼ

	DAC_InitType.DAC_Trigger=DAC_Trigger_None;	//ʹô TEN1=0
	DAC_InitType.DAC_WaveGeneration=DAC_WaveGeneration_None;//ʹòη
	DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude=DAC_LFSRUnmask_Bit0;//Ρֵ
	DAC_InitType.DAC_OutputBuffer=DAC_OutputBuffer_Disable ;	//DAC1ر BOFF1=1
  DAC_Init(DAC_Channel_1,&DAC_InitType);	 //ʼDACͨ1

	DAC_Cmd(DAC_Channel_1, ENABLE);  //ʹDACͨ1
  
  DAC_SetChannel1Data(DAC_Align_12b_R, 0);  //12λҶݸʽDACֵ
}
//ͨ1ѹ
//vol:0~3300,0~3.3V
void Dac1_Set_Vol(u16 vol)
{
	double temp=vol;
	temp/=1000;
	temp=temp*4096/3.3;
	DAC_SetChannel1Data(DAC_Align_12b_R,temp);//12λҶݸʽDACֵ
}





















































