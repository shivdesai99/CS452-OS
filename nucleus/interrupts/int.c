/* This code is my own work, it was written without consulting code written by other students current or previous or using any AI tools Shiv Desai */

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/util.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/main.e"
#include "../../h/trap.e"
#include "../../h/int.e"

void static intterminalhandler();
void static intprinterhandler();
void static intdiskhandler();
void static intfloppyhandler();

void static init_device_table();
void static intsemop(int* sem, int OP);

void static inthandler(int abs_device_no);
void static passUpCompletionStatus(dev_entry_t *device);

void static intclockhandler();
void static sleep();

#define TOTAL_DEVICES 15
#define QUANTUM 5000

state_t *term_old_state, *print_old_state, *disk_old_state, *flop_old_state, *clock_old_state;
state_t *term_new_state, *print_new_state, *disk_new_state, *flop_new_state, *clock_new_state;

int pseudo_clock;
int timeslice = QUANTUM;
long timePassed = 0;


// char msg_normal_termination[] = "Last Process Terminated, Shutting Down";
// char msg_deadlock[] = "Deadlock Detected, Shutting Down";

/*
typedef struct {
    devreg_t *dev_reg;
    int sema;
    int length;
    int status;
} dev_entry_t;
*/

dev_entry_t device_table[TOTAL_DEVICES];

void static sleep() {
    asm("stop #0x2000");
}

void static init_device_table() {
    int device_indx = 0;

    int i;
    // Initialize Terminal Devices (5 total)
    for (i = 0; i < 5; i++, device_indx++) {
        device_table[device_indx].dev_reg = (devreg_t *)(0x1400 + 0x10 * i);
        device_table[device_indx].sema = 0;
        device_table[device_indx].length = 0;
        device_table[device_indx].status = ENULL;
    }

    // Initialize Printer Devices (2 total)
    for (i = 0; i < 2; i++, device_indx++) {
        device_table[device_indx].dev_reg = (devreg_t *)(0x1450 + 0x10 * i);
        device_table[device_indx].sema = 0;
        device_table[device_indx].length = 0;
        device_table[device_indx].status = ENULL;
    }

    // Initialize Disk Devices (4 total)
    for (i = 0; i < 4; i++, device_indx++) {
        device_table[device_indx].dev_reg = (devreg_t *)(0x1470 + 0x10 * i);
        device_table[device_indx].sema = 0;
        device_table[device_indx].length = 0;
        device_table[device_indx].status = ENULL;
    }

    // Initialize Floppy Devices (4 total)
    for (i = 0; i < 4; i++, device_indx++) {
        device_table[device_indx].dev_reg = (devreg_t *)(0x14b0 + 0x10 * i);
        device_table[device_indx].sema = 0;
        device_table[device_indx].length = 0;
        device_table[device_indx].status = ENULL;
    }
}
/*
void newAreaInit(state_t *new_area) {
    new_area->s_sr.ps_t = 0; // Clear trace bit
    new_area->s_sr.ps_s = 1; // Set supervisor mode
    new_area->s_sr.ps_m = 0; // Set memory management off
    new_area->s_sr.ps_int = 7;
    new_area->s_sp = boot_state.s_sp;
}
*/
void intinit() {

    term_old_state = (state_t *) 0x9c8;
    term_new_state = term_old_state + 1;
    newAreaInit(term_new_state);
    term_new_state->s_pc = (int) intterminalhandler;

    print_old_state = (state_t *) 0xa60;
    print_new_state = print_old_state + 1;
    newAreaInit(print_new_state);
    print_new_state->s_pc = (int) intprinterhandler;

    disk_old_state = (state_t *) 0xaf8;
    disk_new_state = disk_old_state + 1;
    newAreaInit(disk_new_state);
    disk_new_state->s_pc = (int) intdiskhandler;

    flop_old_state = (state_t *) 0xb90;
    flop_new_state = flop_old_state + 1;
    newAreaInit(flop_new_state);
    flop_new_state->s_pc = (int) intfloppyhandler;

    clock_old_state = (state_t *) 0xc28;
    clock_new_state = clock_old_state + 1;
    newAreaInit(clock_new_state);
    clock_new_state->s_pc = (int) intclockhandler;

    pseudo_clock = 0;
    init_device_table();
}

void waitforpclock(state_t* old_area) {
    proc_t* head_proc = headQueue(run_queue);
    intsemop(&pseudo_clock, LOCK);

    store_time(head_proc);
    head_proc->p_s = *old_area;
    schedule();
}

void waitforio(state_t* old_area) {
    proc_t *head_proc = headQueue(run_queue);
    int abs_device_no = old_area->s_r[4];
    dev_entry_t *device = &device_table[abs_device_no];

    if (device->sema == UNLOCK) {
        device->sema -= 1;
        old_area->s_r[2] = device->length;
        old_area->s_r[3] = device->status;
    } else {
        intsemop(&(device->sema), LOCK);
    }

    head_proc->p_s = *old_area;
    schedule();
}
/*
This function is similar to the semop call in the first part. 
It has tw oarguments, the address of a semaphore (instead of a state_t), and the operation. 
This function should use the ASL and should call insertBlocked and removeBlocked.
*/

void print0(char *msg, int length) {
    dev_entry_t *device = &device_table[5];
    device->dev_reg->d_amnt = length;
    device->dev_reg->d_stat = ENULL;
    device->dev_reg->d_badd = msg;
    device->dev_reg->d_op = IOWRITE;

    while (device->dev_reg->d_stat != 0);

}   

void static intsemop(int* sem, int OP) {
    proc_t *head_proc = headQueue(run_queue);

    if (OP == LOCK && *sem < 1) {
        insertBlocked(sem, head_proc);
        removeProc(&run_queue);
        calc_time(head_proc);
    } else if (OP == UNLOCK && *sem < 0) {
        proc_t *removed_proc = removeBlocked(sem);
        if (removed_proc != (proc_t *) ENULL && removed_proc->qcount == 0) { // Check if the removed_proc returned, and if it is not blocked on any more sema's
            insertProc(&run_queue, removed_proc); // If so, add it back to run queue
        }
    }
    *sem += OP;
}

void intschedule() {
    LDIT(&timeslice);
}

void intdeadlock() {
    // Check for processes on device semaphores
    int i;
    int proc_on_device_sema = 0;
    for (i = 0; i < 15; i++) {
        if (headBlocked(&(device_table[i].sema)) != (proc_t *) ENULL) {
            proc_on_device_sema = 1;
            break;
        }
    }

    state_t sleepState;
    STST(&sleepState);
    sleepState.s_sr.ps_int = 0;
    sleepState.s_pc = (int) sleep;

    if (headBlocked(&pseudo_clock) != (proc_t *) ENULL) {
        intschedule();
        LDST(&sleepState);
    } else if (proc_on_device_sema) {
        LDST(&sleepState);
    } else if (headASL() == FALSE) {
        print0("nucleus: normal termination", 27);
        HALT();
    } else {
        print0("nucleus: deadlocked termination", 31);
        // print("nucleus: deadlocked termination");
        HALT();
    }
}

/*
print("halt: end of program");
/*
    asm("trap #4");
/*
  HALT();
*/



void static inthandler(int abs_device_no) {
    dev_entry_t *device = &device_table[abs_device_no];
    if (device->sema == LOCK) {
        passUpCompletionStatus(device);
    } else if (device->sema == 0) {
        device->sema += 1;
        device->length = device->dev_reg->d_amnt;
        device->status = device->dev_reg->d_stat;
    }
}

void static passUpCompletionStatus(dev_entry_t* device) {
    proc_t *waiting_proc = headBlocked(&(device->sema));
    waiting_proc->p_s.s_r[2] = device->dev_reg->d_amnt;
    waiting_proc->p_s.s_r[3] = device->dev_reg->d_stat;
    intsemop(&(device->sema), UNLOCK);
}

void static intterminalhandler() {
    proc_t *head_proc = headQueue(run_queue);
    inthandler(term_old_state->s_tmp.tmp_int.in_dno);
    head_proc->p_s = *term_old_state;
    if (head_proc != (proc_t *) ENULL) {
        LDST(term_old_state);
    } else {
        schedule();
    }
}

void static intprinterhandler() {
    proc_t *head_proc = headQueue(run_queue);
    inthandler(print_old_state->s_tmp.tmp_int.in_dno + 5);
    
    if (head_proc != (proc_t *) ENULL) {
        head_proc->p_s = *print_old_state;
        LDST(print_old_state);
    } else {
        schedule();
    }
}

void static intdiskhandler() {
    proc_t *head_proc = headQueue(run_queue);
    inthandler(disk_old_state->s_tmp.tmp_int.in_dno + 7);
    
    if (head_proc != (proc_t *) ENULL) {
        head_proc->p_s = *disk_old_state;
        LDST(disk_old_state);
    } else {
        schedule();
    }
}

void static intfloppyhandler() {
    proc_t *head_proc = headQueue(run_queue);
    inthandler(flop_old_state->s_tmp.tmp_int.in_dno + 11);

    if (head_proc != (proc_t *) ENULL) {
        head_proc->p_s = *flop_old_state;
        LDST(flop_old_state);
    } else {
        schedule();
    }
}

void static intclockhandler() {
    proc_t *head_proc = headQueue(run_queue);
    timePassed += QUANTUM;

    if (head_proc != (proc_t *) ENULL) {
        head_proc = removeProc(&run_queue);
        calc_time(head_proc);
        head_proc->p_s = *clock_old_state;
        insertProc(&run_queue, head_proc);
    }

    if (timePassed >= 100000) { // 100 milliseconds = 100000 microseconds 
        if (headBlocked(&pseudo_clock) != (proc_t *) ENULL) {
            intsemop(&pseudo_clock, UNLOCK);
        }
        timePassed = 0;
    }
    schedule();
}