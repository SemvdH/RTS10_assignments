#include <stdint.h>
#include <stdbool.h>
#include <stm32f4xx.h>
#include "VersdOS.h"

/*-------------------------------pin numbers for leds ---------------------------*/

#define GREEN 12
#define ORANGE 13
#define RED 14
#define BLUE 15

/*------------------------------------DEMO---------------------------------------*/
// blocking delay function
// keep the CPU busy for at least ticks SysTicks
void blocking_delay(unsigned int ticks)
{
	while (ticks != 0) {
		extern bool SysTick_flag;
		while (SysTick_flag == false); // busy wait
		SysTick_flag = false;
		ticks--;
	}
}

void delay_nonblocking (unsigned int ticks) {

	// ask os to not be scheduled for the amount of ticks (use counter)
	requestDelay(ticks);
}

void toggleGreen(void)
{
	while(1) {
		blocking_delay(20);
    	GPIOD->ODR ^= 1 << GREEN;
    	delay_nonblocking(1000);
	}
}

void toggleOrange(void)
{
	while(1) {
    	GPIOD->ODR ^= 1 << ORANGE;
    	delay_nonblocking(700);
	}
}

void toggleRed(void)
{
	while (1) {
    	GPIOD->ODR ^= 1 << RED;
    	delay_nonblocking(400);
	}
}

void toggleBlue(void)
{
	while(1) {
		blocking_delay(100);
    	GPIOD->ODR ^= 1 << BLUE;
    	delay_nonblocking(80);
	}
}

int main(void)
{
	// Use HSE (8 MHz)
	// Enable HSE
	RCC->CR |= RCC_CR_HSEON;
	// Wait until HSE is stable
	while ((RCC->CR & RCC_CR_HSERDY) == 0);

	// Power interface clock enable
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	// Regulator voltage scaling output selection Scale 1 mode <= 100 MHz
	PWR->CR |= PWR_CR_VOS;
	// Use 3 wait states when reading Flash at 100 MHz.
	FLASH->ACR = FLASH_ACR_LATENCY_3WS;
	// Wait until 3 wait states are used
	while ((FLASH->ACR & FLASH_ACR_LATENCY_3WS) == 0);
	// Enable flash prefetch buffer, instruction and data cache
	FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

	// Use PLL to generate 100 MHz clock
	// PLL output = 8 / M * N / P: M = 4, N = 100, P = 2 to generate 100 MHz
	RCC->PLLCFGR = 0x20000000
		| RCC_PLLCFGR_PLLSRC_HSE
		| (0 << RCC_PLLCFGR_PLLP_Pos)
		| (100 << RCC_PLLCFGR_PLLN_Pos)
		| (4 << RCC_PLLCFGR_PLLM_Pos);
	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;
	// Wait until PLL is locked
	while ((RCC->CR & RCC_CR_PLLRDY) == 0);
	// Select PLL as the system clock. AHB clock divided by 2.
	RCC->CFGR |= RCC_CFGR_SW_PLL | RCC_CFGR_PPRE1_DIV2;
	// Wait until PLL used as the system clock
	while ((RCC->CFGR & RCC_CFGR_SWS_PLL) == 0);
	// Disable HSI
	RCC->CR &= ~RCC_CR_HSION;

	// GPIO Port D Clock Enable
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	// GPIO Port D led pins Push/Pull Output
	GPIOD->MODER |= 1 << 2 * GREEN | 1 << 2 * ORANGE | 1 << 2 * RED | 1 << 2 * BLUE;


    // Create tasks. Provide fpointer, stacksize, and priority:
	addTaskToList(toggleGreen, 128, 3,500);
    addTaskToList(toggleOrange, 128, 2,100);
    addTaskToList(toggleRed, 128, 2,200);
    addTaskToList(toggleBlue, 128, 3,600);

	// Start VersdOS with 1 ms sysTick
	startVersdOS(1);

	while(1);
}
