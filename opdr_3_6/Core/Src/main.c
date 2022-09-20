#include <stdint.h>
#include <stdio.h>
#include <stm32f4xx.h>

#define TASKS_AMOUNT 8

typedef void (*fnPtr)();

// enum representing the states of a traffic light
enum traffic_light {
	RED, ORANGE, GREEN
};
enum traffic_light light = RED;

enum state {
	WAITING,DELAYING, READY
};

enum LED_state {
	ON,OFF
};

struct task {
	int period;
	int counter;
	int delay;
	int delay_counter;
	enum state state;
	enum LED_state led_state;
	fnPtr functionPtr;
};

struct task tasks[TASKS_AMOUNT];

// amount of half seconds have passed
int ticks = 0;

void initialize_task(int pos, int delay, int period, fnPtr functionPtr) {
	tasks[pos].delay = delay;
	tasks[pos].delay_counter = tasks[pos].delay;
	tasks[pos].period = period;
	tasks[pos].counter = tasks[pos].period;
	tasks[pos].functionPtr = functionPtr;
	tasks[pos].state = WAITING;
	tasks[pos].led_state = OFF;
}

// run all ready tasks
void runReadyTasks() {
	for (int i = 0; i < TASKS_AMOUNT; i++) {
		//check if task is ready
		if (tasks[i].state == READY) {
			// check if task has a valid pointer
			if (tasks[i].functionPtr != NULL) {
				tasks[i].functionPtr();
				tasks[i].led_state = !tasks[i].led_state;
				tasks[i].state = WAITING;
			}
		}
	}
}

void toggleRed() {
	GPIOD->ODR ^= GPIO_ODR_ODR_14;
}

void toggleGreen() {
	GPIOD->ODR ^= GPIO_ODR_ODR_12;
}

void toggleOrange() {
	GPIOD->ODR ^= GPIO_ODR_ODR_13;
}

void toggleBlue() {
	GPIOD->ODR ^= GPIO_ODR_ODR_15;
}

// interrupt handler
void SysTick_Handler() {
	// increase amount of half seconds bassed
	ticks++;

	for (int i = 0; i < TASKS_AMOUNT; i++) {
		if (tasks[i].state == WAITING) {
			tasks[i].counter--;
			if (tasks[i].counter == 0) {
				if (tasks[i].led_state == OFF) {
					tasks[i].state = DELAYING;
				} else {
					tasks[i].state = READY;
				}
				tasks[i].counter = tasks[i].period;
			}
		} else if (tasks[i].state == DELAYING) {
			tasks[i].delay_counter--;
			if (tasks[i].delay_counter == 0) {
				tasks[i].state = READY;
				tasks[i].delay_counter = tasks[i].delay;
			}
		}
	}
}

int main(void) {
	// enable systick and interrupts
	SysTick_Config(16000); // interrupt every 1 ms

	// GPIO Port D Clock Enable
	RCC->AHB1ENR = RCC_AHB1ENR_GPIODEN;
	// GPIO Port D Pin 15 down to 12 Push/Pull Output
	GPIOD->MODER = GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0
			| GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
	// Set green and red LEDs
	GPIOD->ODR = GPIO_ODR_ODR_14 | GPIO_ODR_ODR_12;

	// initialize tasks with periods and functions
	initialize_task(0, 100, 200, &toggleGreen);
	initialize_task(1, 200, 500, &toggleOrange);
	initialize_task(2, 300, 750, &toggleRed);
	initialize_task(3, 400, 300, &toggleBlue);

	// Do forever:
	while (1) {
		// Wait a moment
		__WFI();

		// run the tasks that are ready
		runReadyTasks();
	}
}
