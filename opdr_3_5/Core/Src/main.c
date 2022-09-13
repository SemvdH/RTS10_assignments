#include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>

enum traffic_light {
	RED, ORANGE, GREEN
};
enum traffic_light light = RED;
int ticks = 0;
void SysTick_Handler() {
	ticks++;
}


/**
 * convert seconds to ticks
 */
int get_ticks(float s) {
	return s * 16000000; // 16 MHz clock
}

int main(void) {
	// enable systick and interrupts
	SysTick_Config(8000000);

	// GPIO Port D Clock Enable
	RCC->AHB1ENR = RCC_AHB1ENR_GPIODEN;
	// GPIO Port D Pin 15 down to 12 Push/Pull Output
	GPIOD->MODER = GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0
			| GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
	// Set green and red LEDs
	GPIOD->ODR = GPIO_ODR_ODR_14 | GPIO_ODR_ODR_12;

	// Do forever:
	while (1) {
		// Wait a moment
		__WFI();

		// Flip all LEDs
		switch (light) {
		case RED:
			GPIOD->ODR = GPIO_ODR_ODR_14;
			if (ticks > 10) {
				ticks = 0;
				light = ORANGE;
			}
//			light = ORANGE;
			break;
		case ORANGE:
			GPIOD->ODR = GPIO_ODR_ODR_13;
			if (ticks > 2) {
				ticks = 0;
				light = GREEN;
			}
//			light = GREEN;
			break;
		case GREEN:
			GPIOD->ODR = GPIO_ODR_ODR_12;
			if (ticks > 8) {
				ticks = 0;
				light = RED;
			}
//			light = RED;
			break;
		}
	}
}
