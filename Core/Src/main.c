/*
 * main.c
 *
 *  Created on: Dec 3, 2023
 *      Author: khoad
 */

#include "stdint.h"
typedef enum
{
	LED_OFF, LED_ON
}led_state_t;
uint8_t count = 0;
//--------------------------Generate System Clock by HSI Clock-----------------------------
void Init_HSI_Clock()
{
	uint32_t* CR = (uint32_t*)(0x40023800 + 0x00);
	uint32_t* CFGR = (uint32_t*)(0x40023800 + 0x08);
	*CFGR &= ~(0b11 << 0);
	*CR |= (1 << 0);
	while(((*CR >> 1) & 1) == 0);
}
//-----------------------------------------------------------------------------------------

//--------------------------Control Led by GPIOD12-----------------------------------------
void Init_Led()
{
	uint32_t* AHB1ENR = (uint32_t*)(0x40023800 + 0x30);
	*AHB1ENR |= (1 << 3);
	uint32_t* MODER = (uint32_t*)(0x40020C00 + 0x00);
	uint32_t* OTYPER = (uint32_t*)(0x40020C00 + 0x04);
	*MODER |= (0b01 << 24);
	*OTYPER &= ~(1 << 12);
}
void Control_Led(led_state_t led_state)
{
	uint32_t* ODR = (uint32_t*)(0x40020C00 + 0x14);
	if(led_state == LED_ON)
		*ODR |= (1 << 12);
	else
		*ODR &= ~(1 << 12);
}
//-----------------------------------------------------------------------------------------

//--------------------------Set up for Timer in Time_base unit-----------------------------
uint32_t tick = 0;
void Init_Timer()
{
	uint32_t* APB2ENR = (uint32_t*)(0x40023800 + 0x44);
	*APB2ENR |= (1 << 0);
	uint16_t* CR1 = (uint16_t*)(0x40010000 + 0x00);
	uint16_t* ARR = (uint16_t*)(0x40010000 + 0x2C);
	uint16_t* PSC = (uint16_t*)(0x40010000 + 0x28);
	uint16_t* DIER = (uint16_t*)(0x40010000 + 0x0C);
	uint32_t* NVIC_ISER0 = (uint32_t*)(0xE000E100);
	*PSC = 16 - 1;
	*ARR = 1000 - 1;
	*CR1 |= (1 << 0);
	*DIER |= (1 << 0);
	*NVIC_ISER0 |= (1 << 25);
}
void Timer_Handler_Custom()
{
	tick++;
	uint16_t* SR = (uint16_t*)(0x40010000 + 0x10);
	*SR &= ~(1 << 0);
}
void delay_ms(uint32_t time)
{
	tick = 0;
	while(tick < time);
}
//-----------------------------------------------------------------------------------------

//--------------------------Independent WatchDog-------------------------------------------
void Init_IWDG()
{
	uint32_t* KR = (uint32_t*)(0x40003000 + 0x00);
	uint32_t* PR = (uint32_t*)(0x40003000 + 0x04);
	uint32_t* RLR = (uint32_t*)(0x40003000 + 0x08);
	*KR = 0x5555;
	*PR |= (0b011 << 0);
	*RLR = 5000 - 1;
	*KR = 0xCCCC;
}
//-----------------------------------------------------------------------------------------

//-------------------------------Main Function---------------------------------------------
int main()
{
	Init_HSI_Clock();
	Init_Led();
	Init_Timer();
	uint32_t* VTTB = (uint32_t*)(0xE000ED08);
	*VTTB = 0x20000000;
	uint32_t* Timer_address = (uint32_t*)(0x200000A4);
	*Timer_address = (uint32_t)(Timer_Handler_Custom) | 1;

	while(1)
	{
		Control_Led(LED_ON);
		delay_ms(300);
		Control_Led(LED_OFF);
		delay_ms(300);
		if(count++ >= 2)
		{
			//When the counter reaches to 0 => reset chip
			Init_IWDG();
			//Set up chip in Sleep mode
			uint32_t* SCR = (uint32_t*)(0xE000ED10);
			*SCR |= (1 << 2);
			__asm("WFI");
		}
	}
	return 1;
}
//-----------------------------------------------------------------------------------------
