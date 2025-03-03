/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */

/* 
 * File: procq.c
 * Description: This file implements the management of process structures and the 
 *              process queue mechanism. It includes functions to initialize the 
 *              process table, allocate and free process entries, and manipulate 
 *              process queues by inserting, removing, and clearing process links.
 *
 * Dependencies: 
 *    - const.h: Contains constant definitions (e.g., MAXPROC, SEMMAX, ENULL)
 *    - types.h: Contains type definitions including proc_t and state_t
 *    - procq.h: Contains definitions for process queue links (proc_link)
 */

#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.h"

/* Global process table and free list pointer */
proc_t procTable[MAXPROC];
proc_t *procFree_h;

/*
 * Function: initProc
 * ------------------
 * Initializes the process table.
 *
 * Description:
 *   - Iterates over the procTable array to set default values for each process,
 *     including zeroing the queue count, clearing parent/child/sibling pointers,
 *     and initializing state pointers and CPU time values.
 *   - For each process, initializes the array of proc_link structures (p_link) 
 *     and the semaphore vector (semvec) to indicate that no links or semaphores are in use.
 *   - Builds the free list of processes by linking all processes using the first
 *     element (p_link[0]) of each proc_t.
 */
void initProc() {
    int i, j;
    for (i = 0; i < MAXPROC; i++) {
        procTable[i].qcount = 0;
        procTable[i].parent = (proc_t *) ENULL;
        procTable[i].child = (proc_t *) ENULL;
        procTable[i].sibling = (proc_t *) ENULL;
        procTable[i].mm_old = (state_t *) ENULL;
		procTable[i].mm_new = (state_t *) ENULL;
		procTable[i].prog_old = (state_t *) ENULL;
		procTable[i].prog_new = (state_t *) ENULL;
		procTable[i].sys_old = (state_t *) ENULL;
		procTable[i].sys_new = (state_t *) ENULL;
        procTable[i].start_time = 0;
        procTable[i].cpu_time = 0;
        
        for (j = 0; j < SEMMAX; j++) {
            procTable[i].p_link[j].next = (proc_t *) ENULL;
            procTable[i].p_link[j].index = ENULL;
            procTable[i].semvec[j] = (int *) ENULL;
        }
        
    }

    /* Build the free list by linking all procTable entries via p_link[0] */
    procFree_h = &procTable[0];
    for (i = 0; i < MAXPROC - 1; i++) {
        procTable[i].p_link[0].next = &procTable[i+1];
        procTable[i].p_link[0].index = 0;
    }
    procTable[MAXPROC - 1].p_link[0].next = (proc_t *) ENULL;
    procTable[19].p_link[0].index = 0;
}

/*
 * Function: allocProc
 * -------------------
 * Allocates a process from the free list.
 *
 * Returns:
 *    - A pointer to an available proc_t from the free list.
 *    - ENULL if no free process is available.
 *
 * Description:
 *   - Retrieves the first process from the free list (procFree_h),
 *     updates the free list pointer, and clears the process's first link.
 */
proc_t* allocProc() {
    if (procFree_h == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    proc_t *p = procFree_h;
    procFree_h = p->p_link[0].next;
    p->p_link[0].next = (proc_t *) ENULL;
    return p;
}

/*
 * Function: freeProc
 * ------------------
 * Returns a process to the free list.
 *
 * Parameters:
 *    - p: Pointer to the proc_t structure to be freed.
 *
 * Description:
 *   - Resets all fields of the given process to default values,
 *     including process links, state pointers, and CPU time counters.
 *   - Adds the process back to the head of the free list.
 */
void freeProc(proc_t *p) {
    if (p == (proc_t *) ENULL) {
        return;
    }
    p->qcount = 0;
    p->parent = (proc_t *) ENULL;
    p->child = (proc_t *) ENULL;
    p->sibling = (proc_t *) ENULL;
    p->mm_old = (state_t *) ENULL;
    p->mm_new = (state_t *) ENULL;
    p->prog_old = (state_t *) ENULL;
    p->prog_new = (state_t *) ENULL;
    p->sys_old = (state_t *) ENULL;
    p->sys_new = (state_t *) ENULL;
    p->start_time = 0;
    p->cpu_time = 0;
    int i;
    for (i = 0; i < SEMMAX; i++) {
        p->p_link[i].next = (proc_t *) ENULL;
        p->p_link[i].index = ENULL;
    }
    p->p_link[0].next = procFree_h;
    procFree_h = p;
}

/*
 * Function: insertProc
 * ----------------------
 * Inserts a process into a process queue.
 *
 * Parameters:
 *    - tp: Pointer to a proc_link structure representing the tail pointer of a queue.
 *    - p:  Pointer to the process to be inserted.
 *
 * Description:
 *   - Checks for valid (non-NULL) parameters.
 *   - Verifies that the process has not exceeded the maximum allowed queue links (SEMMAX).
 *   - Finds an available slot in the process's p_link array.
 *   - If the queue is empty (tp->next is ENULL), initializes it with process p.
 *   - Otherwise, appends process p to the end of the queue by updating the appropriate
 *     linking pointers and indices.
 *   - Panics if any parameter is NULL, if the process has reached max queues, or if no
 *     available link slot is found.
 */
void insertProc(proc_link *tp, proc_t *p) {
    if (tp == (proc_link *) ENULL || p == (proc_t *) ENULL) {
        panic("insertProc: Null pointer parameter");
    }
    if (p->qcount >= SEMMAX) {
        panic("insertProc Failed: process has reached max queues");
    }
    int slot;
    for (slot = 0; slot < SEMMAX; slot++) {
        if (p->p_link[slot].next == (proc_t *) ENULL)
            break;
    }
    if (slot == SEMMAX) {
        panic("insertProc Failed: no available link slot");
    }
    p->qcount++;

    if (tp->next == (proc_t *) ENULL) {
        p->p_link[slot].next = p;  
        p->p_link[slot].index = slot;
        tp->next = p;
        tp->index = slot;
    } else {
        proc_t *tail = tp->next;
        proc_t *head = tail->p_link[tp->index].next;

        p->p_link[slot].next = head;
        p->p_link[slot].index = tail->p_link[tp->index].index;

        tail->p_link[tp->index].index = slot;
        tail->p_link[tp->index].next = p;

        tp->next = p;
        tp->index = slot;
    }
}

/*
 * Function: headQueue
 * -------------------
 * Retrieves the head process of a queue.
 *
 * Parameters:
 *    - tp: A proc_link structure representing the queue.
 *
 * Returns:
 *    - Pointer to the process at the head of the queue.
 *    - ENULL if the queue is empty.
 *
 * Description:
 *   - Uses the provided proc_link structure to locate the head of the queue.
 *   - Note that this function does not remove the process from the queue.
 */
proc_t* headQueue(proc_link tp) {
    if (tp.next == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    return tp.next->p_link[tp.index].next;
}

/*
 * Function: clearProcLink
 * -----------------------
 * Clears a specific process link from a process's link array.
 *
 * Parameters:
 *    - p:  Pointer to the process whose link is to be cleared.
 *    - pl: Pointer to the specific proc_link within the process that should be cleared.
 *
 * Description:
 *   - Iterates through the p_link array of the process.
 *   - When the matching link (by address) is found, resets its next pointer and index to ENULL.
 */
void clearProcLink(proc_t *p, proc_link *pl) {
    int i;
    for (i = 0; i < SEMMAX; i++) {
        if (&(p->p_link[i]) == pl) {
            p->p_link[i].next = (proc_t *) ENULL;
            p->p_link[i].index = ENULL;
            break;
        }
    }
}

/*
 * Function: outProc
 * -----------------
 * Removes a specified process from a process queue.
 *
 * Parameters:
 *    - tp: Pointer to the proc_link structure representing the queue.
 *    - p:  Pointer to the process to be removed.
 *
 * Returns:
 *    - Pointer to the removed process if successful.
 *    - ENULL if the process is not found in the queue.
 *
 * Description:
 *   - Traverses the queue starting from the tail to locate process p.
 *   - Updates the linking pointers to remove process p from the queue.
 *   - Handles the special case where the queue contains only one process.
 *   - Decrements the process's queue count (qcount) and clears its corresponding link.
 */
proc_t* outProc(proc_link *tp, proc_t *p) {
    if (tp == (proc_link *) ENULL || tp->next == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }

    if (tp->next == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    
    proc_t *tail = tp->next;
    proc_t *head = tail->p_link[tp->index].next;
    proc_link *tail_link = &(tail->p_link[tp->index]);
    proc_link *head_link = &(head->p_link[tail_link->index]);

    if (tail == head) {
        if (tail == p) {
            p->qcount--;
            p->p_link[tp->index].next = (proc_t *) ENULL;
            p->p_link[tp->index].index = ENULL;
            tp->next = (proc_t *) ENULL;
            return p;
        } else {
            return (proc_t *) ENULL;
        }
    }

    int prev_index;
    proc_link *prev_link = tail_link;
    proc_link *cur_link = head_link;
    proc_t *prev_process = tail;
    
    while (prev_link->next != p) {
        prev_index = prev_link->index;
        prev_process = prev_link->next;
        prev_link = cur_link;
        cur_link = &(cur_link->next->p_link[cur_link->index]);
        if (prev_link->next == head) {
            return (proc_t *) ENULL;
        }
    }
    if (prev_link->next == tp->next) {
        tp->index = prev_index;
        tp->next = prev_process;
    }
    int plink_index = prev_link->index;
    prev_link->next = cur_link->next;
    prev_link->index = cur_link->index;
    p->qcount--;

    if (p->p_link[plink_index].next == (proc_t *) ENULL) {
        panic("outProc: Removed p's proc_link is NULL");
    }
    p->p_link[plink_index].next = (proc_t *) ENULL;
    p->p_link[plink_index].index = ENULL;

    return p;
}

/*
 * Function: removeProc
 * --------------------
 * Removes the process at the head of a process queue.
 *
 * Parameters:
 *    - tp: Pointer to the proc_link structure representing the queue.
 *
 * Returns:
 *    - Pointer to the process removed from the head of the queue.
 *    - ENULL if the queue is empty.
 *
 * Description:
 *   - Retrieves both the tail and head processes from the queue.
 *   - Decrements the head process's queue count.
 *   - If the queue becomes empty after removal, resets the tail pointer.
 *   - Otherwise, adjusts the linking pointers to maintain the integrity of the queue.
 */
proc_t* removeProc(proc_link *tp) {
    if (tp == (proc_link *) ENULL || tp->next == (proc_t *) ENULL)
        return (proc_t *) ENULL;
    
    proc_t *tail = tp->next;
    proc_t *head = tail->p_link[tp->index].next;
    proc_link *tail_link = &(tail->p_link[tp->index]);
    proc_link *head_link = &(head->p_link[tail_link->index]);

    head->qcount--;
    if (head == tail) {
        tp->next = (proc_t *) ENULL;
        head->p_link[tp->index].next = (proc_t *) ENULL;
        head->p_link[tp->index].index = ENULL;
        return head;
    } else {
        int slot = tail_link->index;
        tail_link->next = head_link->next;
        tail_link->index = head_link->index;

        head->p_link[slot].next = (proc_t *) ENULL;
        head->p_link[slot].index = ENULL;
        return head;
    }
}

/*
 * Global error buffer for panic messages.
 */
char myerrbuf[128];

/*
 * Function: panic
 * ---------------
 * Handles unrecoverable errors by displaying an error message and halting execution.
 *
 * Parameters:
 *    - s: A string containing the error message.
 *
 * Description:
 *   - Copies the provided error message into the global error buffer (myerrbuf).
 *   - Invokes a trap instruction to stop the simulator, effectively halting further execution.
 */
panic(s)
register char *s;
{
	register char *i=myerrbuf;

	while ((*i++ = *s++) != '\0')
		;

	asm("  trap    #0");  /* stop the simulator */
}
