/*
 * Copyright (c) 2010, Kelvin Lawson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. No personal names or organizations' names associated with the
 *    Atomthreads project may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ATOMTHREADS PROJECT AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include "main.h"
#include "atom.h"
#include "atomport-private.h"
#include "bsp.h"

#include "w1.h"

/* Constants */

/*
 * Idle thread stack size
 *
 * This needs to be large enough to handle any interrupt handlers
 * and callbacks called by interrupt handlers (e.g. user-created
 * timer callbacks) as well as the saving of all context when
 * switching away from this thread.
 *
 * In this case, the idle stack is allocated on the BSS via the
 * idle_thread_stack[] byte array.
 */
#define IDLE_STACK_SIZE_BYTES       128


/*
 * Main thread stack size
 *
 * Note that this is not a required OS kernel thread - you will replace
 * this with your own application thread.
 *
 * In this case the Main thread is responsible for calling out to the
 * test routines. Once a test routine has finished, the test status is
 * printed out on the UART and the thread remains running in a loop
 * flashing a LED.
 *
 * The Main thread stack generally needs to be larger than the idle
 * thread stack, as not only does it need to store interrupt handler
 * stack saves and context switch saves, but the application main thread
 * will generally be carrying out more nested function calls and require
 * stack for application code local variables etc.
 *
 * With all OS tests implemented to date on the STM8, the Main thread
 * stack has not exceeded 384 bytes. To allow all tests to run we set
 * a minimum main thread stack size of 204 bytes. This may increase in
 * future as the codebase changes but for the time being is enough to
 * cope with all of the automated tests.
 */
#define MAIN_STACK_SIZE_BYTES       256


/*
 * Startup code stack
 *
 * Some stack space is required at initial startup for running the main()
 * routine. This stack space is only temporarily required at first bootup
 * and is no longer required as soon as the OS is started. By default
 * Cosmic sets this to the top of RAM and it grows down from there.
 *
 * Because we only need this temporarily you may reuse the area once the
 * OS is started, and are free to use some area other than the top of RAM.
 * For convenience we just use the default region here.
 */


/* Local data */

/* Application threads' TCBs */
static ATOM_TCB main_tcb;

/* Main thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t main_thread_stack[MAIN_STACK_SIZE_BYTES];

/* Idle thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t idle_thread_stack[IDLE_STACK_SIZE_BYTES];


/* Forward declarations */
static void main_thread_func (uint32_t param);

extern void ds18b20_init();
extern uint16_t ds18b20_read(struct w1_master *dev,w1_err_t *err);
extern uint16_t ds18b20_read2(struct w1_master *dev,w1_err_t *err);
extern struct w1_master w1_therm_master;
extern uint16_t ds18b20_match_read(struct w1_master *dev,uint8_t rom[],w1_err_t *err);
extern uint8_t *ds18b20_get_rom(uint8_t index);
/**
 * \b main
 *
 * Program entry point.
 *
 * Sets up the STM8 hardware resources (system tick timer interrupt) necessary
 * for the OS to be started. Creates an application thread and starts the OS.
 *
 * If the compiler supports it, stack space can be saved by preventing
 * the function from saving registers on entry. This is because we
 * are called directly by the C startup assembler, and know that we will
 * never return from here. The NO_REG_SAVE macro is used to denote such 
 * functions in a compiler-agnostic way, though not all compilers support it.
 *
 */
NO_REG_SAVE void main ( void )
{
    int8_t status;

    /**
     * Note: to protect OS structures and data during initialisation,
     * interrupts must remain disabled until the first thread
     * has been restored. They are reenabled at the very end of
     * the first thread restore, at which point it is safe for a
     * reschedule to take place.
     */

    bsp_init();    
    /* Initialise the OS before creating our threads */
    status = atomOSInit(&idle_thread_stack[0], IDLE_STACK_SIZE_BYTES, TRUE);
    if (status == ATOM_OK)
    {
        /* Enable the system tick timer */
        archInitSystemTickTimer();

        /* Create an application thread */
        status = atomThreadCreate(&main_tcb,
                     CFG_ATOM_THREAD_DEFAULT_PRIO, main_thread_func, 0,
                     &main_thread_stack[0],
                     MAIN_STACK_SIZE_BYTES,
                     TRUE);
        if (status == ATOM_OK)
        {
            /**
             * First application thread successfully created. It is
             * now possible to start the OS. Execution will not return
             * from atomOSStart(), which will restore the context of
             * our application thread and start executing it.
             *
             * Note that interrupts are still disabled at this point.
             * They will be enabled as we restore and execute our first
             * thread in archFirstThreadRestore().
             */
            atomOSStart();
        }
    }

    /* There was an error starting the OS if we reach here */
    while (1)
    {
    }

}


/**
 * \b main_thread_func
 *
 * Entry point for main application thread.
 *
 * This is the first thread that will be executed when the OS is started.
 *
 * @param[in] param Unused (optional thread entry parameter)
 *
 * @return None
 */
static void main_thread_func (uint32_t param){
  uint8_t state=0;
  uint16_t temp;
  w1_err_t err;
  ds18b20_init();
  while (1)  {
    
    temp=ds18b20_read2(&w1_therm_master,&err);
    temp=ds18b20_read(&w1_therm_master,&err);
    temp=ds18b20_match_read(&w1_therm_master,ds18b20_get_rom(0),&err);
    temp=ds18b20_match_read(&w1_therm_master,ds18b20_get_rom(1),&err);
    temp=ds18b20_match_read(&w1_therm_master,ds18b20_get_rom(2),&err);
    if(err!=W1_ERR_NONE){
      state^=0x01;    
      bsp_led_set(BSP_ID_LED0,state&0x01);
    }
    if(temp!=0){
      bsp_led_set(BSP_ID_LED0,state&0x01);
    }
    atomTimerDelay (SYSTEM_TICKS_PER_SEC);
  }
}


void assert_failed(uint8_t* file, uint32_t line){
  while(1){
    
  }
}
