/* This code is my own work, it was written without consulting code written by other students current or previous or using any AI tools Shiv Desai */

/*
 * File: trap_sys.c
 * Description: This file contains the initialization routines for trap areas and 
 *              associated handlers for program, memory management, and system calls.
 *              It also provides utility functions to record and compute CPU time for
 *              processes when traps occur.
 *
 * Overview:
 *   - newAreaInit: Initializes a new trap area with default state values.
 *   - store_time / calc_time: Record and compute CPU usage for a process.
 *   - trapinit: Sets up the trap vectors in memory and initializes old/new trap areas.
 *   - trapsyshandler: Handles system call traps, dispatching based on the system call number.
 *   - trapmmhandler & trapproghandler: Handle memory management and program traps respectively.
 *
 * Dependencies:
 *   - const.h, types.h: Fundamental constants and type definitions.
 *   - util.h, vpop.h: Utility routines and virtual process operations.
 *   - procq.e, asl.e: Process queue and Abstract Semaphore List implementations.
 *   - trap.h, main.e, syscall.e, int.e: Trap, main, system call, and interrupt handling.
 */

#include "../../h/const.h" 
#include "../../h/types.h"
#include "../../h/util.h"
#include "../../h/vpop.h"
#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/trap.h"
#include "../../h/main.e"
#include "../../h/syscall.e"
#include "../../h/int.e"

/* Global pointers to trap areas for program, memory management, and system calls.
 * Each "old_area" holds the state at the moment of the trap, while the corresponding
 * "new_area" is prepared for the trap handler.
 */
state_t *prog_old_area;
state_t *prog_new_area;
state_t *mm_old_area;
state_t *mm_new_area;
state_t *sys_old_area;
state_t *sys_new_area;

/*
 * Function: newAreaInit
 * ---------------------
 * Initializes a new trap area state structure with default settings.
 *
 * Parameters:
 *   - new_area: Pointer to a state_t structure representing a new trap area.
 *
 * Operation:
 *   - Clears the trace bit (ps_t), sets supervisor mode (ps_s), disables memory management (ps_m),
 *     and disables interrupts (ps_int set to 7) in the status register.
 *   - Initializes the stack pointer (s_sp) to the same value as the nucleus boot state.
 */
static void newAreaInit(state_t *new_area) {
    new_area->s_sr.ps_t = 0; // Clear trace bit
    new_area->s_sr.ps_s = 1; // Set supervisor mode
    new_area->s_sr.ps_m = 0; // Disable memory management
    new_area->s_sr.ps_int = 7; // Disable interrupts
    new_area->s_sp = boot_state.s_sp; // Initialize stack pointer from boot state
}

/*
 * Function: store_time
 * --------------------
 * Records the current time into a process's start_time field.
 *
 * Parameters:
 *   - p: Pointer to the process (proc_t) for which the time is recorded.
 *
 * Operation:
 *   - Calls STCK to read the current clock value and stores it in p->start_time.
 */
void store_time(proc_t *p) {
	long time;
	STCK(&time);            // Retrieve current time from clock register
	p->start_time = time;   // Store the starting time for process p
}

/*
 * Function: calc_time
 * -------------------
 * Calculates the CPU time used by a process since its last recorded start_time.
 *
 * Parameters:
 *   - p: Pointer to the process (proc_t) for which CPU time is being calculated.
 *
 * Operation:
 *   - Retrieves the current time and computes the elapsed time since the process started.
 *   - Accumulates this elapsed time into the process's cpu_time field.
 */
void calc_time(proc_t *p) {
    long curr_t;
	STCK(&curr_t); // Get current time
	p->cpu_time += (int)(curr_t - (p->start_time)); // Update CPU time with elapsed time
}

/*
 * Function: trapinit
 * ------------------
 * Initializes trap vectors and sets up trap areas for program, memory management, and system calls.
 *
 * Operation:
 *   - Sets various memory-mapped trap vector entries with the addresses of their respective handlers.
 *   - Initializes the program trap area:
 *       * Sets prog_old_area to the trap area defined by BEGINTRAP.
 *       * Creates a new trap area (prog_new_area) immediately after prog_old_area.
 *       * Initializes prog_new_area using newAreaInit and sets its program counter to trapproghandler.
 *   - Similarly, initializes trap areas for memory management (mm_old_area, mm_new_area) and system calls (sys_old_area, sys_new_area)
 *     with corresponding handlers trapmmhandler and trapsyshandler.
 */
void trapinit() {
    // EVT (Exception Vector Table) Initialization: set trap vector addresses to their handler routines.
    *(int *) 0x008 = (int) STLDMM;
	*(int *) 0x00c = (int) STLDADDRESS;
	*(int *) 0x010 = (int) STLDILLEGAL;
	*(int *) 0x014 = (int) STLDZERO;
	*(int *) 0x020 = (int) STLDPRIVILEGE;
	*(int *) 0x08c = (int) STLDSYS;
	*(int *) 0x94 = (int) STLDSYS9;
	*(int *) 0x98 = (int) STLDSYS10;
	*(int *) 0x9c = (int) STLDSYS11;
	*(int *) 0xa0 = (int) STLDSYS12;
	*(int *) 0xa4 = (int) STLDSYS13;
	*(int *) 0xa8 = (int) STLDSYS14;
	*(int *) 0xac = (int) STLDSYS15;
	*(int *) 0xb0 = (int) STLDSYS16;
	*(int *) 0xb4 = (int) STLDSYS17;
	*(int *) 0x100 = (int) STLDTERM0;
	*(int *) 0x104 = (int) STLDTERM1;
	*(int *) 0x108 = (int) STLDTERM2;
	*(int *) 0x10c = (int) STLDTERM3;
	*(int *) 0x110 = (int) STLDTERM4;
	*(int *) 0x114 = (int) STLDPRINT0;
	*(int *) 0x11c = (int) STLDDISK0;
	*(int *) 0x12c = (int) STLDFLOPPY0;
	*(int *) 0x140 = (int) STLDCLOCK;
    
    // Program trap area initialization:
    prog_old_area = (state_t *) BEGINTRAP;      // Set old trap area address for program traps
    prog_new_area = prog_old_area + 1;          // New trap area is located immediately after old area
    newAreaInit(prog_new_area);                 // Initialize new program trap area
    prog_new_area->s_pc = (int)trapproghandler; // Set program counter to the program trap handler

    // Memory management trap area initialization:
    mm_old_area = (state_t *) 0x898; // Set old trap area address for memory management traps
    mm_new_area = mm_old_area + 1;
    newAreaInit(mm_new_area);
    mm_new_area->s_pc = (int)trapmmhandler;

    // System call trap area initialization:
    sys_old_area = (state_t *) 0x930; // Set old trap area address for system calls
    sys_new_area = sys_old_area + 1; 
    newAreaInit(sys_new_area);
    sys_new_area->s_pc = (int)trapsyshandler;
}

/*
 * Function: trapsyshandler
 * ------------------------
 * Handles system call traps by dispatching the appropriate system call routine.
 *
 * Operation:
 *   - Retrieves the current head process from the run queue and computes its CPU time.
 *   - Extracts the system call number from the old trap area (sys_old_area).
 *   - If a user-mode process (not in supervisor mode) calls a SYS call numbered 1-8,
 *     it is treated as a privileged violation, and a default program trap is raised.
 *   - Otherwise, based on the system call number, dispatches the corresponding routine:
 *       1. createproc: Create a new process.
 *       2. killproc: Terminate a process.
 *       3. semop: Perform semaphore operations.
 *       4. notused: (SYS4) Not used.
 *       5. trapstate: Specify trap state vector.
 *       6. getcputime: Retrieve CPU time for the process.
 *       Default: For system calls SYS9 to SYS17, handle with trapsysdefault.
 *
 * Note:
 *   - The system call handler passes a pointer to the old trap area to functions in syscall.c and int.c.
 */
static void trapsyshandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);                                       // Update CPU time for the head process
    int sys_trap_no = (int) sys_old_area->s_tmp.tmp_sys.sys_no; // Get system call number
    
    // Check for unauthorized user-mode SYS calls for system calls 1-8
    if (sys_old_area->s_sr.ps_s != 1 && sys_trap_no < 9) { 
		sys_old_area->s_tmp.tmp_pr.pr_typ = PRIVILEGE;  // Mark as a privilege violation
        trapsysdefault(sys_old_area, PROGTRAP);         // Invoke default trap handling for program traps
	} else {
        // Dispatch system call based on its number
        switch ( sys_trap_no ) {
                case 1:
                    createproc(sys_old_area); // Create Process (SYS1)
                    break;
                case 2:
                    killproc(sys_old_area); // Terminate Process (SYS2)
                    break;
                case 3:
                    semop(sys_old_area); // Semaphore Operation (SYS3)
                    break;
                case 4:
                    notused(sys_old_area); // Not Used (SYS4)
                    break;
                case 5:
                    trapstate(sys_old_area); // Specify Trap State Vector (SYS5)
                    break;
                case 6:
                    getcputime(sys_old_area); // Get CPU Time (SYS6)
                    break;
                default:
                    trapsysdefault(sys_old_area, SYSTRAP); // Handle other SYS traps (SYS9 - SYS17)
                    break;
        }
    }
}

/*
 * Function: trapmmhandler
 * -----------------------
 * Handles memory management traps.
 *
 * Operation:
 *   - Retrieves the head process from the run queue and updates its CPU time.
 *   - Invokes the default trap handling routine with the MMTRAP code.
 */
static void trapmmhandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);                   // Update CPU time for the head process
    trapsysdefault(mm_old_area, MMTRAP);    // Invoke default memory management trap handling
}

/*
 * Function: trapproghandler
 * -------------------------
 * Handles program traps.
 *
 * Operation:
 *   - Retrieves the head process from the run queue and updates its CPU time.
 *   - Invokes the default trap handling routine with the PROGTRAP code.
 */
static void trapproghandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);                       // Update CPU time for the head process
    trapsysdefault(prog_old_area, PROGTRAP);    // Invoke default program trap handling
}