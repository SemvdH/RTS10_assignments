#include <stdint.h>

#define RCC_AHB1ENR_BIT_GPIODEN *(volatile uint32_t*)(0x42000000 + 0x00023830 * 32 + 3 * 4)
#define GPIOD_BASE 0x40020C00
#define GPIOD_MODER *(volatile uint32_t*)(GPIOD_BASE + 0x00)
#define GPIOD_ODR *(volatile uint32_t*)(GPIOD_BASE + 0x14) // RM0383 8.4.6

#define STK_BASE 0xE000E010
#define STK_CTRL *(volatile uint32_t*)(STK_BASE + 0x00) // control register
#define STK_LOAD *(volatile uint32_t*)(STK_BASE + 0x04) // load reload register

int main(void)
{

	// put count value in reload register
	if (STK_CTRL & (1 << 2)) {
		STK_LOAD = 8000000; // AHB is 16 MHz, so 0.5s is 8000000
	} else {
		STK_LOAD = 1000000; // AHB/8 is 2 MHz, so 0.5s is 1000000
	}

	STK_CTRL |= 1; // enable systick timer


	// GPIO Port D Clock Enable
	RCC_AHB1ENR_BIT_GPIODEN = 1;
	// GPIO Port D Pin 15 down to 12 Push/Pull Output
	GPIOD_MODER = 0x55000000;
	// Set green and red LEDs
	GPIOD_ODR = 0x5000;
	// Do forever:
	while (1)
	{
		// Wait a moment
		while (( STK_CTRL & (1 << 16) ) == 0) ;
		// Flip all LEDs
		GPIOD_ODR ^= 0xF000;
	}
}
