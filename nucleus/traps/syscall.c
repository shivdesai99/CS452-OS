/* This code is my own work, it was written without consulting code written by other students current or previous or using any AI tools Shiv Desai */

/*
 * File: syscall.c
 * Description: Implements system call and process management operations including:
 *              - Process creation (createproc) and termination (killproc, killProcessBacktrack)
 *              - Child process management (insertChild, removeChild)
 *              - Semaphore operations (semop) using a vector of semaphore operations (vpop)
 *              - Trap state handling (trapstate, trapsysdefault)
 *              - Retrieving CPU time (getcputime)
 *
 * Overview:
 *   - Processes are arranged in a parent-child-sibling tree.
 *   - A new process is created as a copy of the parent's state.
 *   - Semaphore operations may block/unblock processes on semaphores.
 *   - System call traps are dispatched based on the value in a specific register.
 *
 * Dependencies:
 *   - const.h, types.h: Define fundamental constants and data types.
 *   - util.h, vpop.h: Utility routines and definitions for the vpop struct.
 *   - procq.e, asl.e: Process queue and Abstract Semaphore List management.
 *   - syscall.h, main.e, int.e: System call, main system definitions, and interrupt handling.
 */

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
 * Function: insertChild
 * -----------------------
 * Inserts a new child process into the parent's child list.
 *
 * Parameters:
 *   - parent_proc: Pointer to the parent process.
 *   - child_proc:  Pointer to the child process to be inserted.
 *
 * Operation:
 *   - Sets the child's parent pointer to the given parent.
 *   - If the parent has no children, the child becomes the first child.
 *   - Otherwise, traverses the sibling linked list until the end and inserts the new child.
 */
void insertChild(proc_t *parent_proc, proc_t *child_proc) {
    child_proc->parent = parent_proc;           // Set the parent pointer for the child.
    child_proc->sibling = (proc_t *) ENULL;        // Initialize the child's sibling pointer to NULL.
    if (parent_proc->child == (proc_t *) ENULL) {  // If parent has no children yet...
        parent_proc->child = child_proc;          // ...make the new child the first child.
    } else {                                      // Otherwise, insert into the existing sibling list.
        proc_t *curr_p = parent_proc->child;      // Start at the first child.
        while (curr_p->sibling != (proc_t *) ENULL) { // Traverse until reaching the last sibling.
            curr_p = curr_p->sibling;
        }
        curr_p->sibling = child_proc;             // Link the new child at the end of the sibling list.
    }
}

/*
 * Function: createproc
 * ----------------------
 * Creates a new process as a child of the currently running (parent) process.
 *
 * Parameters:
 *   - old_area: Pointer to the state_t structure (trap area) containing the parent's state.
 *
 * Operation:
 *   - Allocates a new process structure for the child.
 *   - Retrieves the parent process from the head of the run queue.
 *   - If allocation fails (e.g., process table full), sets register D2 to an error code (ENULL).
 *   - Otherwise, copies the parent's state (from the address in D4) to the child process.
 *   - Inserts the child into the parent's child list and the run queue.
 *   - Updates register D2 to 0 to indicate success.
 *   - Records CPU time for the parent and then invokes schedule() to continue execution.
 *
 * Brainstorm (in comments):
 *   - D4 holds the address of a processor state area at the time of the trap.
 *   - The parent's processor state is used as the initial state for the new process.
 *   - The parent continues executing after the system call.
 */
void createproc(state_t *old_area) {
    proc_t *new_child = allocProc();                   // Allocate a new child process.
    proc_t *parent = headQueue(run_queue);               // Retrieve the parent from the head of the run queue.

    if (new_child == (proc_t *) ENULL) {                  // Check if allocation failed.
        old_area->s_r[2] = ENULL;                        // Set D2 (return register) to error code (-1).
        return;
    }

    new_child->p_s = *(state_t *) old_area->s_r[4];      // Copy parent's state (pointed by D4) into the new child.
    insertChild(parent, new_child);                      // Insert new child into parent's child tree.
    insertProc(&run_queue, new_child);                   // Insert the new child process into the run queue.
    old_area->s_r[2] = 0;                                // Set D2 to 0 to indicate successful creation.

    store_time(parent);                                  // Record the parent's current time.
    parent->p_s = *old_area;                             // Update the parent's state from the trap area.
    schedule();                                          // Call schedule() to dispatch the next process.
}

/*
 * Function: removeChild
 * -----------------------
 * Removes a process from its parent's child list.
 *
 * Parameters:
 *   - p: Pointer to the process to remove.
 *
 * Operation:
 *   - If the process is the parent's first child, updates the parent's child pointer.
 *   - Otherwise, traverses the sibling linked list to locate the process and removes it.
 *   - Resets the process's parent, sibling, and child pointers.
 *
 * Note:
 *   - Designed to be called during process termination to detach a process from its parent's tree.
 */
void removeChild(proc_t *p) {
    if (p->parent == (proc_t *) ENULL) {  // If the process has no parent, nothing needs to be done.
        return;
    }

    if (p->parent->child == p) {          // If p is the first child of its parent...
        p->parent->child = p->sibling;      // ...set the parent's child pointer to p's sibling.
    } else {                              // Otherwise, p is in the middle or end of the sibling list.
        proc_t *curr_p = p->parent->child;  // Start from the parent's first child.
        while (curr_p->sibling != (proc_t *) ENULL && curr_p->sibling != p) { 
            curr_p = curr_p->sibling;       // Traverse until p is found.
        }
        if (curr_p->sibling != (proc_t *) ENULL) { // Ensure we found p before updating pointers.
            curr_p->sibling = curr_p->sibling->sibling; // Remove p from the sibling list.
        }
    }

    /* Reset the process's pointers to detach it completely from the parent's tree */
    p->parent = (proc_t *) ENULL;
    p->sibling = (proc_t *) ENULL;
    p->child = (proc_t *) ENULL;
}

/*
 * Function: killProcessBacktrack
 * ------------------------------
 * Recursively terminates a process and all its descendant processes.
 *
 * Parameters:
 *   - p: Pointer to the process to terminate.
 *
 * Operation:
 *   - Recursively traverses and terminates all child processes.
 *   - For each child, detaches it from the parent's child list, recursively calls killProcessBacktrack,
 *     and frees the process.
 *   - Removes the process from any semaphore queues and from the run queue.
 *   - Returns the process structure to the free list.
 *
 * Note:
 *   - This routine ensures that the entire subtree of processes is terminated.
 */
void killProcessBacktrack(proc_t *p) {
    while (p->child != (proc_t *) ENULL) {              // While there is at least one child process...
        proc_t *child_p = p->child;                       // Get the first child.
        p->child = child_p->sibling;                      // Update parent's child pointer to the next sibling.
        child_p->parent = (proc_t *) ENULL;               // Detach the child from its parent.
        child_p->sibling = (proc_t *) ENULL;              // Reset child's sibling pointer.
        killProcessBacktrack(child_p);                    // Recursively kill the child process and its subtree.
    }
    outBlocked(p);         // Remove process from any semaphore queues it might be on.
    removeChild(p);        // Remove the process from its parent's child list.
    outProc(&run_queue, p); // Remove the process from the run queue.
    freeProc(p);           // Return the process structure to the free list.
}

/*
 * Function: killproc
 * ------------------
 * Terminates the process currently at the head of the run queue.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state (old_area) from which this call was made.
 *
 * Operation:
 *   - Retrieves the process to be killed from the head of the run queue.
 *   - Calls killProcessBacktrack to recursively terminate it and its descendants.
 *   - Calls schedule() to continue process execution.
 */
void killproc(state_t *old_area) {
    proc_t *proc_to_kill = headQueue(run_queue); // Get the process at the head of the run queue.
    killProcessBacktrack(proc_to_kill);            // Recursively terminate the process and its children.
    schedule();                                    // Schedule the next process.
}

/*
 * Function: semop
 * ---------------
 * Performs a vector of semaphore operations (vpop) on the calling process.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state from which the syscall is invoked.
 *
 * Operation:
 *   - Extracts the array of semaphore operations (vpop) from register D4.
 *   - The number of operations to perform is read from register D3.
 *   - For each semaphore operation:
 *       * If the operation is LOCK (-1) and the semaphore value is less than 1,
 *         the process is blocked on the semaphore.
 *       * If the operation is UNLOCK (+1) and the semaphore value is negative,
 *         a blocked process is removed from the semaphore queue and, if unblocked completely,
 *         reinserted into the run queue.
 *       * Updates the semaphore counter by adding the operation value.
 *   - If any operation results in blocking, removes the process from the run queue.
 *   - Finally, stores the updated state, records time, and calls schedule() to dispatch.
 *
 * Implementation Note:
 *   - vpop is a structure that contains an operation (op: LOCK or UNLOCK) and a pointer to a semaphore.
 */
void semop(state_t *old_area) {
    proc_t *head_proc = headQueue(run_queue);             // Get the head process of the run queue.
    vpop *vpop_arr = (vpop *) old_area->s_r[4];            // Retrieve the vpop array from register D4.
    int op_count = old_area->s_r[3];                       // Retrieve the number of operations from register D3.
    int sem_val;                                         // Variable to hold current semaphore value.
    int i;
    int to_remove = 0;                                   // Flag to indicate if the process should be removed (blocked).

    for (i = 0; i < op_count; i++) {
        sem_val = *(vpop_arr[i].sem);                    // Read the current value of the semaphore.
        if (vpop_arr[i].op == LOCK && sem_val < 1) {      // If LOCK operation and semaphore is unavailable...
            head_proc->p_s = *old_area;                  // Save the current state of the process.
            insertBlocked(vpop_arr[i].sem, head_proc);     // Block the process on the semaphore.
            to_remove = 1;                               // Mark that the process is now blocked.
        } else if (sem_val < 0) {                         // If UNLOCK operation and semaphore has waiting processes...
            proc_t *removed_proc = removeBlocked(vpop_arr[i].sem); // Remove a process from the semaphore's queue.
            if (removed_proc != (proc_t *) ENULL && removed_proc->qcount == 0) { // If unblocked and no longer waiting on any semaphore...
                insertProc(&run_queue, removed_proc);    // Reinsert the process into the run queue.
            }
        }
        *(vpop_arr[i].sem) += vpop_arr[i].op;            // Update the semaphore value by applying the operation.
    }

    if (to_remove) {
        removeProc(&run_queue);                          // Remove the head process if it was blocked.
    }

    store_time(head_proc);                               // Record the current time for the process.
    head_proc->p_s = *old_area;                          // Update the process's state from the trap area.
    schedule();                                        // Call schedule() to select the next process to run.
}

/*
 * Function: notused
 * -----------------
 * Stub function for system calls that are not implemented.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state.
 *
 * Operation:
 *   - Halts the system by calling HALT().
 */
void notused(state_t *old_area) {
    HALT();
}

/*
 * Function: trapstate
 * -------------------
 * Sets or updates the trap state vectors for system (SYSTRAP), memory management (MMTRAP),
 * or program (PROGTRAP) traps.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state containing new trap vector values.
 *
 * Operation:
 *   - Retrieves the trap type from register D2.
 *   - Depending on the trap type, if the corresponding old trap state pointer in the head process
 *     is NULL, sets it with the value from register D3 and sets the new trap state from register D4.
 *   - If the trap state is already set, terminates the process.
 *   - Records the process time, updates the state, and calls schedule() to continue execution.
 */
void trapstate(state_t *old_area) {
    proc_t *head_proc = headQueue(run_queue);          // Get the head process.
    int trap_type = old_area->s_r[2];                    // Extract trap type from register D2.
    switch( trap_type ) {
        case SYSTRAP:
            if (head_proc->sys_old == (state_t *) ENULL) { // If system trap old state not yet set...
                head_proc->sys_old = (state_t *) old_area->s_r[3]; // Set old state from register D3.
                head_proc->sys_new = (state_t *) old_area->s_r[4]; // Set new state from register D4.
            } else {
                killproc(old_area);                      // Terminate process if already set.
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
        default: // Default to PROGTRAP behavior.
            if (head_proc->prog_old == (state_t *) ENULL) {
                head_proc->prog_old = (state_t *) old_area->s_r[3];
                head_proc->prog_new = (state_t *) old_area->s_r[4];
            } else {
                killproc(old_area);
            }
            break;
    }
    store_time(head_proc);                             // Record current CPU time.
    head_proc->p_s = *old_area;                        // Update process state.
    schedule();                                        // Dispatch the next process.
}

/*
 * Function: getcputime
 * --------------------
 * Returns the CPU time used by the head process of the run queue.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state used for this system call.
 *
 * Operation:
 *   - Retrieves the CPU time from the head process.
 *   - Stores the CPU time in register D2 of the trap state.
 *   - Records the current time and updates the process state before calling schedule().
 */
void getcputime(state_t *old_area) {
    proc_t *head_proc = headQueue(run_queue);         // Get the head process from the run queue.
    old_area->s_r[2] = head_proc->cpu_time;             // Return the CPU time in register D2.
    store_time(head_proc);                              // Record current time for the process.
    head_proc->p_s = *old_area;                         // Update process state.
    schedule();                                       // Dispatch the next process.
}

/*
 * Function: trapsysdefault
 * ------------------------
 * Default handler for system call traps (SYSTRAP), memory management traps (MMTRAP),
 * and program traps (PROGTRAP) when a new trap area has been set.
 *
 * Parameters:
 *   - old_area: Pointer to the trap state.
 *   - trap_type: An integer constant representing the type of trap.
 *
 * Operation:
 *   - Retrieves the head process from the run queue.
 *   - Depending on trap_type:
 *       * For SYSTRAP:
 *            - If a new system call trap area is available, copies old_area to the stored old area,
 *              records time, and loads the new trap state into the CPU.
 *            - Otherwise, terminates the process.
 *       * For MMTRAP:
 *            - Similar procedure is followed using memory management trap areas.
 *       * For PROGTRAP:
 *            - Similar procedure is followed using program trap areas.
 */
void trapsysdefault(state_t *old_area, int trap_type) {
    proc_t *head_proc = headQueue(run_queue);         // Get the head process.
    switch ( trap_type ) {
        case SYSTRAP:
            if (head_proc->sys_new != (state_t *) ENULL) { // If new system trap area is set...
                *head_proc->sys_old = *old_area;         // Save current trap state into the old area.
                store_time(head_proc);                   // Record CPU time.
                LDST(head_proc->sys_new);                // Load new system trap state.
            } else {                                     // Otherwise, terminate the process.
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
        default: // Default: PROGTRAP
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
