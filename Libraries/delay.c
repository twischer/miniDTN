#include "delay.h"

void delay_init()
{
	// Clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	// Timer init with 1us tick
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period =  0xFFFF;
	// TODO Check if it is realy one us with the -1
	TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 1000 / 1000 / 2) - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// Timer enable
	TIM_Cmd(TIM2, ENABLE);
}
