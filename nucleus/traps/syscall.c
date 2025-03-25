/* This code is my own work, it was written without consulting code written by other students current or previous or using any AI tools Shiv Desai */

#include "../../h/const.h" 
#include "../../h/types.h"
#include "../../h/util.h"
#include "../../h/vpop.h"
#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/syscall.h"
#include "../../h/main.e"
#include "../../h/int.e"


/*

createproc() brainstorm
    D4 will contain the address of a processor state area at the time the instruction is called
    Processor state should be used as the intial state
    The parent process should continue to execute and exists
    If the new process cannot be created due to lack of resources (process tabel full)
        - Set D2 with error code -1
    Else
        - Set D2 with 0
*/
void insertChild(proc_t *parent_proc, proc_t *child_proc) {
    child_proc->parent = parent_proc; // Set child pointer for parent
    child_proc->sibling = (proc_t *) ENULL; // Set sibling to NULL
    if (parent_proc->child == (proc_t *) ENULL) { // First child process
        parent_proc->child = child_proc; // Set parent_proc's child
    } else { // Child exists, insert into siblings LL
        proc_t *curr_p = parent_proc->child; // Get the start of the LL (child)
        while (curr_p->sibling != (proc_t *) ENULL) { // Traverse until the end of the sibling LL
            curr_p = curr_p->sibling; 
        }
        curr_p->sibling = child_proc; // Set the new child_proc to the last elements sibling
    }
}
void createproc(state_t *old_area) {
    proc_t *new_child = allocProc(); // Grab the new child process from allocProc()
    proc_t *parent = headQueue(run_queue); // Grab the parent process from the head of the run queue

    if (new_child == (proc_t *) ENULL) { // Check if allocProc() was unable to allocate a new process
        old_area->s_r[2] = ENULL; // Set D2 to error (-1)
        return;
    }

    new_child->p_s = *(state_t *) old_area->s_r[4]; // Set the new process's state the the parent's
    insertChild(parent, new_child); // Call insertChild() to place the child proc in the proper spot in the parent's process tree
    insertProc(&run_queue, new_child); // Insert the new child process into the run queue
    old_area->s_r[2] = 0; // Set D2 to 0 for return

    store_time(parent);
    parent->p_s = *old_area;
    schedule();
}


// TO-DO: Double check for potential edge cases and bugs in terms of the traversal
void removeChild(proc_t *p) {
    if (p->parent == (proc_t *) ENULL) { // Its the parent, no action needed or we have cut it off at the beginning of killBacktracking()
        return;
    }

    if (p->parent->child == p) { // The process is its parent's child process
        p->parent->child = p->sibling; // If there is only one parent, sibling should be set to (proc_t *) ENULL
    } else if (p->parent->child != p) { // Need to update the sibling LL pointers
        proc_t *curr_p = p->parent->child; // Get the child process as the starting point
        while (curr_p->sibling != (proc_t *) ENULL && curr_p->sibling != p) { // Traverse until curr_p's sibling is the process we are looking for
            curr_p = curr_p->sibling; 
        }
        // curr_p->sibling should max end at the second the last element, where the last element is p, else find the bug in createproc()
        // p Guaranteed to be in the LL structure, other wise there was a bug in createproc()
        if (curr_p->sibling != (proc_t *) ENULL) { // Ensure we do not dereference NULL
            curr_p->sibling = curr_p->sibling->sibling; // Remove process from sibling list
        }
    }

    p->parent = (proc_t *) ENULL; // Reset process pointers
    p->sibling = (proc_t *) ENULL;
    p->child = (proc_t *) ENULL;
}
void killProcessBacktrack(proc_t *p) {
    while (p->child != (proc_t *) ENULL) { // While the parent still has any child processes'
        proc_t *child_p = p->child;
        p->child = child_p->sibling; // Update the parent's child pointer to point to the curr_child processes sibling
        child_p->parent = (proc_t *) ENULL; // Clear out the child_p's pointers
        child_p->sibling = (proc_t *) ENULL;
        killProcessBacktrack(child_p);
        // Essentially we detach this child from the parent tree, and recursively traverse and process this process
        // is isolation. The parent processs of this function has now had its previous child process mapped to the
        // sibling, hence this while loop will continue until all of the parent processes children have been unlinked
        // and recursively traveresed and freed
    }
    outBlocked(p); // Remove the process of the function call from all semaphores
    removeChild(p); // This will return (proc_t *) ENULL for any children of the parent process due to mapping above, but for the main process, this will update the process's parents pointers and the LL of siblings its in safely
    outProc(&run_queue, p); // Remove it from the run_queue
    freeProc(p); // Put the proc_t struct back on the free list
}

void killproc(state_t *old_area) {
    proc_t *proc_to_kill = headQueue(run_queue);
    killProcessBacktrack(proc_to_kill); // Begin recursively processing freeing
    schedule();
}
/*

#define LOCK -1
#define UNLOCK 1

typedef struct vpop {
  int op;
  int *sem;
} vpop; 

Uses VPO of vector of semphore addreses and corresponding actions P(-1), V(+1)
The P's may or may not get the calling processing stuck on a semaphore Q, if so it comes off the RQ
The V's on active semaphores will remove the semaphore on the head, but only puts it back on the RQ if its not on additional semaphore Q'
Returns the process now at the head of the RQ
Implementation:
    - 
*/
void semop(state_t *old_area) {
    proc_t *head_proc = headQueue(run_queue); // Initialize head process
    vpop *vpop_arr = (vpop *) old_area->s_r[4]; // Initialize vpop_arr from D4
    int op_count = old_area->s_r[3]; // Initialize op_count from D3
    int sem_val; // Define sem_val to be checked weather the semaphore is in a blocking or free state
    int i;
    int to_remove = 0;

    for (i = 0; i < op_count; i++) {
        sem_val = *(vpop_arr[i].sem); // Grab the proper sem_val for the appropriate vpop struct at i
        if (vpop_arr[i].op == LOCK && sem_val < 1) { // If a LOCk operation and sem_val has no more resources, proceed with blocking logic
            head_proc->p_s = *old_area; // Save the state of the process in the head processes p_s
            insertBlocked(vpop_arr[i].sem, head_proc); // Insert the blocked process on the sema
            to_remove = 1;
            
        } else if (sem_val < 0) { // UNLOCK and the semaphore had previous blocked processes
            proc_t *removed_proc = removeBlocked(vpop_arr[i].sem); // Remove the head blocked process on the sema
            if (removed_proc != (proc_t *) ENULL && removed_proc->qcount == 0) { // Check if the removed_proc returned, and if it is not blocked on any more sema's
                insertProc(&run_queue, removed_proc); // If so, add it back to run queue
            }
        }
        *(vpop_arr[i].sem) += vpop_arr[i].op; // Update the semaphore counter variable based on OP value (+1 or -1)
    }

    if (to_remove) {
        removeProc(&run_queue); // Remove the blocked process from the run queue
    }

    store_time(head_proc);
    head_proc->p_s = *old_area;
    schedule(); // Schedule the next process to load into the CPU
}

void notused(state_t *old_area) {
    HALT();
}

void trapstate(state_t *old_area) {
    proc_t *head_proc = headQueue(run_queue); // Set parent proc
   	int trap_type = old_area->s_r[2]; // Extract trap_type from old_area D2
    switch( trap_type ) {
        case SYSTRAP:
            if (head_proc->sys_old == (state_t *) ENULL) { // Check if sys_old was previously set
                head_proc->sys_old = (state_t *) old_area->s_r[3]; // Set up old state in D3
                head_proc->sys_new = (state_t *) old_area->s_r[4]; // Set up new state in D4
            } else {
                killproc(old_area);
            }
            break;
        case MMTRAP:
            if (head_proc->mm_old == (state_t *) ENULL) {
                head_proc->mm_old = (state_t *) old_area->s_r[3];
                head_proc->mm_new = (state_t *) old_area->s_r[4]; 
            } else {
                killproc(old_area);
            }
            break;
        default: // PROGTRAP (default behavior)
            if (head_proc->prog_old == (state_t *) ENULL) {
                head_proc->prog_old = (state_t *) old_area->s_r[3];
                head_proc->prog_new = (state_t *) old_area->s_r[4];
            } else {
                killproc(old_area);
            }
            break;
    }
    store_time(head_proc);
    head_proc->p_s = *old_area;
    schedule();
}

void getcputime(state_t *old_area) {
	proc_t *head_proc = headQueue(run_queue);
	old_area->s_r[2] = head_proc->cpu_time;
    store_time(head_proc);
    head_proc->p_s = *old_area;
    schedule();
}


void trapsysdefault(state_t *old_area, int trap_type) {
    proc_t *head_proc = headQueue(run_queue);

	switch ( trap_type ) {
		case SYSTRAP:
            if (head_proc->sys_new != (state_t *) ENULL) { // Check if the new_area has been initialized
                *head_proc->sys_old = *old_area; // Store the old area in the Old Area Vec Spot
                store_time(head_proc);
                LDST(head_proc->sys_new); // Load the new area state into the CPU
            } else { // If no new area is loaded, kill the process
                killproc(old_area);
            }
			break;
		case MMTRAP:
			if (head_proc->mm_new != (state_t *) ENULL) {
                *head_proc->mm_old = *old_area;
                store_time(head_proc);
                LDST(head_proc->mm_new);
            } else {
                killproc(old_area);
            }
			break;
		default: // PROGTRAP
			if (head_proc->prog_new != (state_t *) ENULL) {
                *head_proc->prog_old = *old_area;
                store_time(head_proc);
                LDST(head_proc->prog_new);
                
            } else {
                killproc(old_area);
            }
			break;
    }
}