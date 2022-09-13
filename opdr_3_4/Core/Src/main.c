#include <stdint.h>
#include <stm32f4xx.h>

void SysTick_Handler() {
	// do nothing
}

int main(void)
{
	// enable systick and interrupts
	SysTick_Config(8000000);

	// GPIO Port D Clock Enable
	RCC->AHB1ENR = RCC_AHB1ENR_GPIODEN;
	// GPIO Port D Pin 15 down to 12 Push/Pull Output
	GPIOD->MODER = GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
	// Set green and red LEDs
	GPIOD->ODR = GPIO_ODR_ODR_14 | GPIO_ODR_ODR_12;
	// Do forever:


	while (1)
	{
		// Wait a moment
		__WFI();
		// Flip all LEDs
		GPIOD->ODR ^= GPIO_ODR_ODR_15 | GPIO_ODR_ODR_14 | GPIO_ODR_ODR_13 | GPIO_ODR_ODR_12;
	}
}
