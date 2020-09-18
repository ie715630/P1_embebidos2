/**
 * @file rtos.c
 * @author ITESO
 * @date Feb 2018
 * @brief Implementation of rtos API
 *
 * This is the implementation of the rtos module for the
 * embedded systems II course at ITESO
 */

#include "rtos.h"
#include "rtos_config.h"
#include "clock_config.h"
#include "MK66F18.h"
#include "fsl_debug_console.h"

#ifdef RTOS_ENABLE_IS_ALIVE
#include "fsl_gpio.h"
#include "fsl_port.h"
#endif

/******************************************************************************/
// Module defines
/******************************************************************************/

#define FORCE_INLINE 	__attribute__((always_inline)) inline

#define STACK_FRAME_SIZE			8
#define STACK_LR_OFFSET				2
#define STACK_PSR_OFFSET			1
#define STACK_PSR_DEFAULT			0x01000000

#define IDLE_TASK_NUM 				4

/******************************************************************************/
// IS ALIVE definitions
/******************************************************************************/

#ifdef RTOS_ENABLE_IS_ALIVE
#define CAT_STRING(x,y)  		x##y
#define alive_GPIO(x)			CAT_STRING(GPIO,x)
#define alive_PORT(x)			CAT_STRING(PORT,x)
#define alive_CLOCK(x)			CAT_STRING(kCLOCK_Port,x)
static void init_is_alive(void);
static void refresh_is_alive(void);
#endif

/******************************************************************************/
// Task definitions
/******************************************************************************/

#define newTask_ptr ((rtos_tcb_t*) &(task_list.tasks[task_list.nTasks]))
#define currentTask_ptr ((rtos_tcb_t*) &(task_list.tasks[task_list.current_task]))

/******************************************************************************/
// Type definitions
/******************************************************************************/

typedef enum
{
	S_READY = 0, S_RUNNING = 1, S_WAITING = 2, S_SUSPENDED = 3
} task_state_e;
typedef enum
{
	kFromISR = 0, kFromNormalExec	// Coming from systick/delay
} task_switch_type_e;

typedef struct
{
	uint8_t priority;
	task_state_e state;
	uint32_t *sp;
	void (*task_body)();
	rtos_tick_t local_tick;
	uint32_t reserved[10];
	uint32_t stack[RTOS_STACK_SIZE];
} rtos_tcb_t;

/******************************************************************************/
// Global (static) task list
/******************************************************************************/

struct
{
	uint8_t nTasks;
	rtos_task_handle_t current_task;
	rtos_task_handle_t next_task;
	rtos_tcb_t tasks[RTOS_MAX_NUMBER_OF_TASKS + 1];
	rtos_tick_t global_tick;
} task_list =
{ 0 };

/******************************************************************************/
// Local methods prototypes
/******************************************************************************/

static void reload_systick(void);
static void dispatcher(task_switch_type_e type);
static void activate_waiting_tasks();
FORCE_INLINE static void context_switch(task_switch_type_e type);
static void idle_task(void);

/******************************************************************************/
// API implementation
/******************************************************************************/

void rtos_start_scheduler(void)
{
#ifdef RTOS_ENABLE_IS_ALIVE
	init_is_alive();
#endif
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
			| SysTick_CTRL_ENABLE_Msk;
	reload_systick();

//	task_list.global_tick = 0;
//	task_list.current_task = -1;

	task_list.current_task = rtos_create_task(idle_task, 0, kAutoStart);

	while(true)
	{
		// Do nothing
	}
}

rtos_task_handle_t rtos_create_task(void (*task_body)(), uint8_t priority,
		rtos_autostart_e autostart)
{
	if(RTOS_MAX_NUMBER_OF_TASKS <= task_list.nTasks)
	{
		return -1;
	}
	
	newTask_ptr->priority = priority;
	newTask_ptr->state = (kAutoStart == autostart) ? (S_READY):(S_SUSPENDED);
	newTask_ptr->task_body = task_body;
	newTask_ptr->local_tick = 0;
	newTask_ptr->sp = &(newTask_ptr->stack[RTOS_STACK_SIZE - 1 - STACK_FRAME_SIZE]);
	
	newTask_ptr->stack[RTOS_STACK_SIZE - STACK_LR_OFFSET] = ((uint32_t)(task_body));
	newTask_ptr->stack[RTOS_STACK_SIZE - STACK_PSR_OFFSET] = STACK_PSR_DEFAULT;
	
	/* Incrementamos el numero de tareas registradas */
	task_list.nTasks++;
	
	return((rtos_task_handle_t) (task_list.nTasks - 1));
}

rtos_tick_t rtos_get_clock(void)
{
	return task_list.global_tick;
}

void rtos_delay(rtos_tick_t ticks)
{
	currentTask_ptr->state = S_WAITING;
	currentTask_ptr->local_tick = ticks;
	// todo(Sergio): Check this
	dispatcher(kFromNormalExec);
}

void rtos_suspend_task(void)
{
	rtos_task_handle_t current_task_num = task_list.current_task;
	rtos_tcb_t task_to_suspend = task_list.tasks[current_task_num];
	task_to_suspend.state = S_SUSPENDED;
	dispatcher(kFromNormalExec);
}

void rtos_activate_task(rtos_task_handle_t task)
{
	task_list.tasks[task].state = S_READY;
	// todo(Sergio): Check this
	dispatcher(kFromNormalExec);
}

/******************************************************************************/
// Local methods implementation
/******************************************************************************/

static void reload_systick(void)
{
	SysTick->LOAD = USEC_TO_COUNT(RTOS_TIC_PERIOD_IN_US,
			CLOCK_GetCoreSysClkFreq());
	SysTick->VAL = 0;
}

static void dispatcher(task_switch_type_e type)
{
	uint8_t highest_priority_found = 0;
	task_list.next_task = 0;
	
	for(uint8_t task_num = 0; task_num < task_list.nTasks; task_num++)
	{
		if((S_READY == task_list.tasks[task_num].state) ||
				(S_RUNNING == task_list.tasks[task_num].state))
		{
			if(task_list.tasks[task_num].priority >= highest_priority_found)
			{
				highest_priority_found = task_list.tasks[task_num].priority;
				task_list.next_task = task_num;
			}
		}
	}

	if(task_list.next_task != task_list.current_task)
	{
		context_switch(type);	
	}
}


FORCE_INLINE static void context_switch(task_switch_type_e type)
{	
	/* Flag to indicate if this function has already been
	 * executed or not
	 * */
	static uint8_t first_execution = 1;
	
	/* Save the stack pointer of processor on a variable */
	register uint32_t sp asm("sp");
	
	register uint32_t r0 asm("r0");
	(void) r0;

	if(0 != first_execution)
	{
		first_execution = 0;
	}
	else
	{
		asm("mov r0, r7");
		task_list.tasks[task_list.current_task].sp = (uint32_t*) r0;

//		currentTask_ptr->sp = ((uint32_t*) (sp));
//		currentTask_ptr->sp += (kFromNormalExec == type) ? (-(STACK_FRAME_SIZE+1)) : (STACK_FRAME_SIZE + 1);
		if(kFromNormalExec == type) {
			currentTask_ptr->sp -= 	9; //(0x24);
		}
		else {
			currentTask_ptr->sp -= 	-(0x1F4);
		}
	}
	
	task_list.current_task = task_list.next_task;
	currentTask_ptr->state = S_RUNNING;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static void activate_waiting_tasks()
{
	// Todo(Check why its 0 )
//	uint8_t num_tasks = task_list.nTasks;
//	uint8_t num_tasks = 5;
	for(uint8_t task_num = 0; task_num < task_list.nTasks; task_num++)
	{
		rtos_tcb_t current_task = task_list.tasks[task_num];	

		if(S_WAITING == current_task.state)
		{
			current_task.local_tick--;
			if(0 == current_task.local_tick)
			{
				current_task.state = S_RUNNING;
			}
		}
	}
}

static void idle_task(void)
{
	for(;;);
}

/****************************************************/
// ISR implementation
/****************************************************/

void SysTick_Handler(void)
{
#ifdef RTOS_ENABLE_IS_ALIVE
	refresh_is_alive();
#endif
	task_list.global_tick++;
	activate_waiting_tasks();
	reload_systick();
	dispatcher(kFromISR);
}

void PendSV_Handler(void)
{
	register int32_t r0 asm("r0");
	(void) r0;
	SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
	r0 = (int32_t) task_list.tasks[task_list.current_task].sp;
	asm("mov r7,r0");
}

/******************************************************************************/
// IS ALIVE SIGNAL IMPLEMENTATION
/******************************************************************************/

#ifdef RTOS_ENABLE_IS_ALIVE
static void init_is_alive(void)
{
	gpio_pin_config_t gpio_config =
	{ kGPIO_DigitalOutput, 1, };

	port_pin_config_t port_config =
	{ kPORT_PullDisable, kPORT_FastSlewRate, kPORT_PassiveFilterDisable,
			kPORT_OpenDrainDisable, kPORT_LowDriveStrength, kPORT_MuxAsGpio,
			kPORT_UnlockRegister, };
	CLOCK_EnableClock(alive_CLOCK(RTOS_IS_ALIVE_PORT));
	PORT_SetPinConfig(alive_PORT(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
			&port_config);
	GPIO_PinInit(alive_GPIO(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
			&gpio_config);
}

static void refresh_is_alive(void)
{
	static uint8_t state = 0;
	static uint32_t count = 0;
	SysTick->LOAD = USEC_TO_COUNT(RTOS_TIC_PERIOD_IN_US,
			CLOCK_GetCoreSysClkFreq());
	SysTick->VAL = 0;
	if (RTOS_IS_ALIVE_PERIOD_IN_US / RTOS_TIC_PERIOD_IN_US - 1 == count)
	{
		GPIO_PinWrite(alive_GPIO(RTOS_IS_ALIVE_PORT), RTOS_IS_ALIVE_PIN,
				state);
		state = state == 0 ? 1 : 0;
		count = 0;
	} else //
	{
		count++;
	}
}
#endif
///

