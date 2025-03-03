/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */

/*
 * File: main.c
 * Description: This file contains the initialization and scheduling routines for the
 *              operating system's process management and interrupt handling.
 *
 * Overview:
 *   - Initializes core components such as the process table, semaphore descriptors,
 *     trap handling, and interrupt handling.
 *   - Sets up the initial process (p1) with a copy of the nucleus boot state.
 *   - Inserts the initial process into the run queue and invokes the scheduler.
 *
 * Dependencies:
 *   - const.h, types.h: Define fundamental constants and types.
 *   - util.h, vpop.h: Utility functions and virtual process operations.
 *   - procq.e, asl.e: Process queue and Abstract Semaphore List implementations.
 *   - main.h, int.e, trap.e: Main program definitions, interrupt and trap handlers.
 */

#include "../../h/const.h" 
#include "../../h/types.h"
#include "../../h/util.h"
#include "../../h/vpop.h"
#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/main.h"
#include "../../h/int.e"
#include "../../h/trap.e"

/* External declaration for the initial process function */
extern int p1();

/* Global run queue for processes ready to run */
proc_link run_queue;

/* Boot state holds the initial state of the nucleus (kernel) including the stack pointer */
state_t boot_state;

/*
 * Function: init
 * ----------------
 * Description:
 *   Initializes the core system components including:
 *     - The boot state: Captures the nucleus boot state for stack pointer and memory boundaries.
 *     - The process table: Initializes all process structures.
 *     - The semaphore descriptor table: Initializes semaphore-related data structures.
 *     - Trap and interrupt handling: Sets up trap and interrupt vectors.
 *
 * Notes:
 *   - This function is declared static since it is used only within this file.
 */
static void init() {
    STST(&boot_state); // Extract the nucleus boot state to obtain the end of memory from the stack pointer.
    initProc();        // Initialize the process table.
    initSemd();        // Initialize the semaphore descriptor table.
    trapinit();        // Initialize trap vectors.
    intinit();         // Initialize interrupt handling.
}

/*
 * Function: schedule
 * ------------------
 * Description:
 *   Schedules the next process from the run queue to run.
 *
 * Operation:
 *   - Retrieves the head process from the run queue.
 *   - If a process is available, calls the interrupt scheduling routine (intschedule)
 *     and loads the process state into the CPU.
 *   - If the run queue is empty, triggers a deadlock handling routine (intdeadlock).
 */
void schedule() {
    proc_t *head_p = headQueue(run_queue);
    if (head_p != (proc_t *) ENULL) { // If the run queue is not empty...
        intschedule();               // Schedule the head process.
        LDST(&(head_p->p_s));        // Load the state of the process into the CPU.
    } else {                         // If the run queue is empty...
        intdeadlock();               // Handle the deadlock condition.
    }
}

/*
 * Function: main
 * --------------
 * Description:
 *   The main entry point for the system.
 *
 * Operation:
 *   - Initializes system components by calling init().
 *   - Allocates a new process and initializes its state with the nucleus's boot state.
 *   - Sets up the initial process (p1) with its program counter, stack pointer, and
 *     status register values. Notably, interrupts are disabled.
 *   - Initializes the run queue and inserts the new process.
 *   - Calls schedule() to start process execution.
 */
void main() {
    init();                           // Perform system initialization.
    proc_t *proc = allocProc();       // Allocate a new process.
    STST(&(proc->p_s));               // Load the current nucleus's state into the process (with interrupts disabled).
    proc->p_s.s_pc = (int)p1;           // Set the process's program counter to point to function p1.
    proc->p_s.s_sp = boot_state.s_sp - (PAGESIZE * 2); // Set the process's stack pointer above the nucleus stack.
    proc->p_s.s_sr.ps_s = 1;           // Set supervisor mode.
    proc->p_s.s_sr.ps_m = 0;           // Set machine mode (if applicable).
    proc->p_s.s_sr.ps_int = 7;         // Disable all interrupts.
    
    /* Initialize the run queue proc_link structure */
    run_queue.index = ENULL;
    run_queue.next = (proc_t *) ENULL;
    
    insertProc(&run_queue, proc);     // Insert the newly created process into the run queue.
    schedule();                       // Invoke the scheduler to run the process.
}
