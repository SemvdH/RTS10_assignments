/*
 * Copyright 2015 Daniel Versluis
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 *
 * schedule.h
 *
 *  Created on: 12 nov. 2013
 *      Author: Daniel Versluis
 *       Email:	VersD @ hr.nl
 *
 * Description:
 * Contains the scheduler and the systick ISR.
 * Simple and incomplete implementation.
 *
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stm32f4xx.h>
#include "VersdOS.h"

//The (external) assembly functions
void pushRegistersToCurrentPSP(void);
void popRegistersFromCurrentPSP(void);
void * readPSP(void);
void writePSP(void * ptr);
void returnToPSP(void);
void returnToMSP(void);

#define TASK_MASK MAX_TASKS-1
#define IDLE_TASK MAX_TASKS

//What states can our task have?
enum taskState {UNUSED = 0, RUNNING, READY, WAITING};

//The task itself
typedef struct _task{
	int             *stack;		// pointer to the stack on the heap
	uint32_t		counter;	// a counter for delays
	void (*function)(void);		// function to execute
	enum taskState	state;		// state
	int8_t			priority;	// priority
} task;

// List of tasks
//add one space for idle task
task taskList[MAX_TASKS+1];
task * currentTask;
task * taskToExecute;

// Idle task
void idleTask(void)
{
	while (1)
	{
		__asm(" wfi"); // Sleep until next SysTick
	}
}



void addTaskToListAtIndex(void (*function)(void), uint32_t stackSize, int8_t priority, size_t pos)
{
	task *taskToAdd = &taskList[pos];

	taskToAdd->function = function;
	// Allocate memory... do we wanna use malloc or our own implementation ;-) ?
	taskToAdd->stack = (int *)malloc(stackSize)+stackSize;

	/*
	 * For debugging purposes we initialize the stack with
	 * values that we can recognize.
	 */
	*(--(taskToAdd->stack)) 	= 0x01000000;					//XSPR Thumb bit set
	*(--(taskToAdd->stack)) 	= (int)taskToAdd->function; 	//set PC to function pointer, cast as int to silence the compiler
	*(--(taskToAdd->stack)) 	= 0xFFFFFFFD; 					//LR, return with process stack (PSP)
	*(--(taskToAdd->stack)) 	= 0x0000000C;					//R12	Initial values used for debugging purposes
	*(--(taskToAdd->stack)) 	= 0x00000003;					//R3
	*(--(taskToAdd->stack)) 	= 0x00000002;					//R2
	*(--(taskToAdd->stack)) 	= 0x00000001;					//R1
	*(--(taskToAdd->stack)) 	= 0x00000000;					//R0

	if(pos!=IDLE_TASK)
	{
		*(--(taskToAdd->stack)) 	= 0x0000000B;					//R11
		*(--(taskToAdd->stack)) 	= 0x0000000A;					//R10
		*(--(taskToAdd->stack)) 	= 0x00000009;					//R9
		*(--(taskToAdd->stack)) 	= 0x00000008;					//R8
		*(--(taskToAdd->stack)) 	= 0x00000007;					//R7
		*(--(taskToAdd->stack)) 	= 0x00000006;					//R6
		*(--(taskToAdd->stack)) 	= 0x00000005;					//R5
		*(--(taskToAdd->stack)) 	= 0x00000004;					//R4
		// Initialize the task properties
		taskToAdd->state 		= READY;
	}else{
		taskToAdd->state 		= RUNNING;
		currentTask = taskToAdd;
		// Update the CPU PSP with our new stack pointer
		writePSP(taskToAdd->stack);
	}

	taskToAdd->priority 	= priority;
}

/*
 * Function to add a new task to the taskList
 * Not sorted, prioritized or any of that kind.
 *
 * It simply allocates memory for the new task stack,
 * fills the stack up so that the context switch will
 * successfully pop these registers and start running
 * at the correct address when returning from the SysTick ISR
 */
void addTaskToList(void (*function)(void), uint32_t stackSize, int8_t priority)
{
	size_t i = 0;
	// Simply find the next empty slot
	// Loops when no more slots are available
	while(taskList[i].state != UNUSED)
	{
		//increment i and roll back at the limit
		i++;
		i &= TASK_MASK;
	}
	addTaskToListAtIndex(function, stackSize, priority, i);
}

void startVersdOS(uint16_t sysTickPeriodIn_ms) {
	// Configure SysTick of 1 ms
	SysTick->LOAD = sysTickPeriodIn_ms * CLOCK_FREQ_IN_KHz - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk //Clock source selection = Processor clock (AHB)
					| SysTick_CTRL_TICKINT_Msk //Counting down to zero to asserts the SysTick exception request
					| SysTick_CTRL_ENABLE_Msk; //Counter enable

	//set systick and pendsv interrupt priority to lowest.
	//svc will be highest.
	SCB->SHP[2] |= 0xFF<<24;
	SCB->SHP[2]  |= 0xFF<<16;

	// Create Idle task
	addTaskToListAtIndex(idleTask, 128, -1, IDLE_TASK);

	__asm(" wfi"); // Sleep until next SysTick
}

// currentTask is running now, return next task to run
task * schedule()
{
	task* tempTaskPtr = currentTask;
	task *idleTaskPtr = &taskList[IDLE_TASK];

	if (tempTaskPtr->state == RUNNING) {
		tempTaskPtr->state = READY;
	}

	int teller=0;

	//Find next ready, non idle task.
	do
	{
		tempTaskPtr++;

		if( (tempTaskPtr-1) == idleTaskPtr || tempTaskPtr == idleTaskPtr)
		{
			//since idle task is the last in the list, we've reached the end
			//and need to continue at the beginning
			tempTaskPtr = &taskList[0];
		}
	}while(tempTaskPtr->state != READY && teller++ <= MAX_TASKS);

	//if no task was found
	if(tempTaskPtr->state != READY)
	{
		//idle task
		tempTaskPtr = idleTaskPtr;
	}

	return tempTaskPtr;
}

// flag used in blocking_delay function
bool SysTick_flag;

/*
 * Interrupt routine for the Systick timer
 * simply calls the scheduler
 * */
void SysTick_Handler(void)
{
	SysTick_flag = true;

	for (int i = 0; i < MAX_TASKS; i++)
	{
		if (taskList[i].state == WAITING)
		{
			taskList[i].counter--;
			if (taskList[i].counter == 0) {
				taskList[i].state = READY;
			}
		}
	}
	//select the next task
	taskToExecute = schedule();
	//request context switch
	SCB->ICSR |= (1<<28);
}

__attribute__((naked)) // No function entry and exit code
void PendSV_Handler(void)
{
	//Push {R4-R11} context to PSP
	pushRegistersToCurrentPSP();
	//Save the new stack pointer after the push
	currentTask->stack = readPSP();

	currentTask = taskToExecute;

	//Load the new stack pointer from (new) currentTask
	writePSP(currentTask->stack);

	//Pop {R4-R11} context from PSP
	popRegistersFromCurrentPSP();

	returnToPSP();
}

/* The most simple SVC implementation
 */

void SVC_Handler(void)
{
	taskToExecute = schedule();
	SCB->ICSR |= (1<<28);
}

//Call Super Visor
void taskYield(void)
{
	asm("	svc #1");

}

void requestDelay(uint32_t ticks)
{
	currentTask->counter = ticks;
	currentTask->state = WAITING;
	taskYield();
}


