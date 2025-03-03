/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */

#include "../../h/const.h" 
#include "../../h/types.h"
#include "../../h/util.h"
#include "../../h/vpop.h"
#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/main.h"
#include "../../h/int.e"
#include "../../h/trap.e"


extern int p1();
proc_link run_queue;
state_t boot_state;


static void init() {
    STST(&boot_state); // Extract the nucleus boot state to get the end of memory from the stack pointer
    initProc();
    initSemd();
    trapinit();
    intinit();
}

void schedule() {
    proc_t *head_p = headQueue(run_queue);
    if (head_p != (proc_t *) ENULL) { // Check if the run queue is empty
        intschedule(); // Schedule the head of the run queue to run
        LDST(&(head_p->p_s)); // Load the state of the head into the CPU
    } else { // If the run queue IS empty, call intdeadlock()
        intdeadlock();
    }
}

void main() {
    init();
    proc_t *proc = allocProc(); // Get a new process
    STST(&(proc->p_s)); // Load the current nucleus's state into the process, with interrupts disabled
    proc->p_s.s_pc = (int)p1; // Set p1() to be invoked when the process is loaded
    proc->p_s.s_sp = boot_state.s_sp - (PAGESIZE * 2); // Set the SP above the nucleus stack
    proc->p_s.s_sr.ps_s = 1;
    proc->p_s.s_sr.ps_m = 0;
    proc->p_s.s_sr.ps_int = 7; // Disbale all interrupts 
    run_queue.index = ENULL; // Initialize runQueue proc_link struct
    run_queue.next = (proc_t *) ENULL;
    insertProc(&run_queue, proc); //Insert proc int the run queue
    schedule(); // Call schedule to run the process
}


