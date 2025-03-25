/* This code is my own work, it was written without consulting code written by other students current or previous or using any AI tools Shiv Desai */

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


state_t *prog_old_area;
state_t *prog_new_area;
state_t *mm_old_area;
state_t *mm_new_area;
state_t *sys_old_area;
state_t *sys_new_area;

void newAreaInit(state_t *new_area) {
    new_area->s_sr.ps_t = 0; // Clear trace bit
    new_area->s_sr.ps_s = 1; // Set supervisor mode
    new_area->s_sr.ps_m = 0; // Set memory management off
    new_area->s_sr.ps_int = 7;
    new_area->s_sp = boot_state.s_sp;
}

void store_time(proc_t *p) {
	long time;
	STCK(&time);
	p->start_time = time;
}

void calc_time(proc_t *p) {
    long curr_t;
	STCK(&curr_t);
	p->cpu_time += (int)(curr_t - (p->start_time));
}


void trapinit() {
    // EVT Initialization
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
    // Program trap old and new area initialization
    prog_old_area = (state_t *) BEGINTRAP; // Trap Area Address PROG (BEGINTRAP)
    prog_new_area = prog_old_area + 1; // set right above old_area (76 bits)
    newAreaInit(prog_new_area);
    prog_new_area->s_pc = (int)trapproghandler; // Load trapproghandler() at new area for Trap STLDXXXX()

    // Memory management trap old and new area initialization
    mm_old_area = (state_t *) 0x898; // Trap Area Address MM
    mm_new_area = mm_old_area + 1;
    newAreaInit(mm_new_area);
    mm_new_area->s_pc = (int)trapmmhandler;

    // SYS trap old and new area initialization
    sys_old_area = (state_t *) 0x930; // Trap Area Address SYS
    sys_new_area = sys_old_area + 1;
    newAreaInit(sys_new_area);
    sys_new_area->s_pc = (int)trapsyshandler;
}

// Note: trapsyshandler passes a pointer to the old trap area to the functions in syscall.c and int.c

static void trapsyshandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);
    int sys_trap_no = (int) sys_old_area->s_tmp.tmp_sys.sys_no;
    
    if (sys_old_area->s_sr.ps_s != 1 && sys_trap_no < 9) { // Catch if a user mode process is calling a SYS 1-8 function
		sys_old_area->s_tmp.tmp_pr.pr_typ = PRIVILEGE;
        trapsysdefault(sys_old_area, PROGTRAP); // Set the user process executing a SYS call as a privileged program trap to be passed up
	} else {
        switch ( sys_trap_no ) {
                case 1:
                    createproc(sys_old_area); // Create_Process (SYS1)
                    break;
                case 2:
                    killproc(sys_old_area); // Terminate_Process (SYS2)
                    break;
                case 3:
                    semop(sys_old_area); // Sem_OP (SYS3)
                    break;
                case 4:
                    notused(sys_old_area); // NOTUSED (SYS4)
                    break;
                case 5:
                    trapstate(sys_old_area); // Specify_Trap_State_Vector (SYS5)
                    break;
                case 6:
                    getcputime(sys_old_area); // Get_CPU_Time (SYS6)
                    break;
                case 7:
                    waitforpclock(sys_old_area);
                case 8:
                    waitforio(sys_old_area);
                default:
                    trapsysdefault(sys_old_area, SYSTRAP); // SYS Traps (SYS9 - SYS17)
                    break;
        }
    }
    
}

static void trapmmhandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);
    trapsysdefault(mm_old_area, MMTRAP);
}

static void trapproghandler() {
    proc_t *head_proc = headQueue(run_queue);
    calc_time(head_proc);
    trapsysdefault(prog_old_area, PROGTRAP);
}
