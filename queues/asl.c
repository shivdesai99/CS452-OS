/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */

/*
 * File: asl.c
 * Description: This file implements the Abstract Semaphore List (ASL) along with
 *              operations to manage semaphore descriptors and the blocked process queues.
 *
 * Overview:
 *   - The semaphore descriptor table (semdTable) is statically allocated with MAXPROC entries.
 *   - The free list (semdFree_h) holds semaphore descriptors that are not in use.
 *   - The active ASL (semd_h) is a sorted linked list of semaphore descriptors that currently
 *     have processes blocked on them.
 *   - Each semaphore descriptor (semd_t) contains a pointer to a semaphore address (s_semAdd)
 *     and a process queue (s_link) that holds the blocked processes.
 *
 * Dependencies:
 *   - types.h: Provides type definitions for proc_t and semd_t.
 *   - const.h: Contains constants such as MAXPROC, SEMMAX, and ENULL.
 *   - procq.e: Implements process queue operations.
 *   - asl.h: Contains declarations related to the ASL.
 */

#include "../h/types.h"
#include "../h/const.h"
#include "../h/procq.e"
#include "../h/asl.h"

/* Global semaphore descriptor table and pointers for the active list and free list */
semd_t semdTable[MAXPROC];
semd_t* semd_h;        /* Head of the active semaphore descriptor list (ASL) */
semd_t* semdFree_h;    /* Head of the free list for semaphore descriptors */

/*
 * Function: initSemd
 * ------------------
 * Initializes the semaphore descriptor table and builds the free list.
 *
 * Details:
 *   - Sets the free list pointer (semdFree_h) to the beginning of the semdTable.
 *   - Clears the active semaphore descriptor list by setting semd_h to ENULL.
 *   - Iterates over semdTable to initialize each descriptor:
 *       * Links each descriptor to the next one to form the free list.
 *       * Sets the previous pointer (s_prev), semaphore address (s_semAdd), and
 *         process queue (s_link) pointers to ENULL.
 */
void initSemd() {
    int i;
    semdFree_h = &semdTable[0];
    semd_h = (semd_t *) ENULL;

    for (i = 0; i < MAXPROC - 1; i++) {
        semdTable[i].s_next = &semdTable[i+1];
        semdTable[i].s_prev = (semd_t *) ENULL;
        semdTable[i].s_semAdd = (int *) ENULL;
        semdTable[i].s_link.next = (proc_t *) ENULL;
        semdTable[i].s_link.index = ENULL;
    }
    semdTable[MAXPROC - 1].s_next = (semd_t *) ENULL;
    semdTable[MAXPROC - 1].s_prev = (semd_t *) ENULL;
    semdTable[MAXPROC - 1].s_semAdd = (int *) ENULL;
    semdTable[MAXPROC - 1].s_link.next = (proc_t *) ENULL;
    semdTable[MAXPROC - 1].s_link.index = ENULL;
}

/*
 * Function: allocSemd
 * -------------------
 * Allocates a semaphore descriptor from the free list.
 *
 * Returns:
 *   - Pointer to a free semaphore descriptor if available.
 *   - ENULL if the free list is empty.
 *
 * Details:
 *   - Removes the first semaphore descriptor from semdFree_h.
 *   - Clears its next and previous pointers.
 */
semd_t* allocSemd() {
    if (semdFree_h == (semd_t *) ENULL) {
        return (semd_t *) ENULL;
    }
    semd_t *semaphore = semdFree_h;
    semdFree_h = semaphore->s_next;
    semaphore->s_next = (semd_t *) ENULL;
    semaphore->s_prev = (semd_t *) ENULL;
    return semaphore;
}

/*
 * Function: getSema
 * -----------------
 * Searches the active ASL for a semaphore descriptor associated with the given semaphore address.
 *
 * Parameters:
 *   - semAdd: Pointer to the semaphore address.
 *
 * Returns:
 *   - Pointer to the semaphore descriptor if found.
 *   - ENULL if no matching descriptor exists.
 */
semd_t* getSema(int *semAdd) {
    semd_t *sema = semd_h;
    while (sema != (semd_t *) ENULL) {
        if (sema->s_semAdd == semAdd) {
            return sema;
        }
        sema = sema->s_next;
    }
    return (semd_t *) ENULL;
}

/*
 * Function: addOrResetSemvec
 * --------------------------
 * Updates a process's semaphore vector by either adding or resetting a semaphore address.
 *
 * Parameters:
 *   - p: Pointer to the process.
 *   - semAdd: Pointer to the semaphore address.
 *   - flag: If non-zero, adds semAdd to the process's semaphore vector;
 *           if zero, resets (removes) semAdd from the vector.
 *
 * Details:
 *   - The process's semvec is an array of pointers (of size SEMMAX) tracking the
 *     semaphores the process is blocked on.
 */
void addOrResetSemvec(proc_t *p, int *semAdd, int flag) {
    int i;
    if (flag) {
        /* Add the semaphore address to the first available slot */
        for (i = 0; i < SEMMAX; i++) {
            if (p->semvec[i] == (int *) ENULL) {
                p->semvec[i] = semAdd;
                break;
            }
        }
    } else {
        /* Reset (remove) the semaphore address by setting it to ENULL */
        for (i = 0; i < SEMMAX; i++) {
            if (p->semvec[i] == semAdd) {
                p->semvec[i] = (int *) ENULL;
                break;
            }
        }
    }
}

/*
 * Function: removeSema
 * --------------------
 * Removes a semaphore descriptor from the active ASL and returns it to the free list.
 *
 * Parameters:
 *   - sema: Pointer to the semaphore descriptor to be removed.
 *
 * Details:
 *   - If the descriptor is at the head of the ASL, update semd_h.
 *   - Otherwise, update the surrounding links (s_prev and s_next) to remove the descriptor.
 *   - Clears the semaphore address and process queue pointers.
 *   - Adds the descriptor back to the free list (semdFree_h).
 */
void removeSema(semd_t *sema) {
    if (sema == semd_h) {
        semd_h = sema->s_next;
        if (semd_h != (semd_t *) ENULL) {
            semd_h->s_prev = (semd_t *) ENULL;
        }
    } else {
        if (sema->s_prev != (semd_t *) ENULL) {
            sema->s_prev->s_next = sema->s_next;
        }
        if (sema->s_next != (semd_t *) ENULL) {
            sema->s_next->s_prev = sema->s_prev;
        }
    }
    sema->s_semAdd = (int *) ENULL;
    sema->s_link.next = (proc_t *) ENULL;
    sema->s_link.index = ENULL;
    sema->s_next = semdFree_h;
    sema->s_prev = (semd_t *) ENULL;
    semdFree_h = sema;
}

/*
 * Function: insertBlocked
 * -----------------------
 * Inserts a process into the blocked queue for a given semaphore.
 *
 * Parameters:
 *   - semAdd: Pointer to the semaphore address on which the process is blocked.
 *   - p: Pointer to the process to be inserted.
 *
 * Returns:
 *   - FALSE if the process is successfully inserted.
 *   - TRUE if a new semaphore descriptor could not be allocated.
 *
 * Details:
 *   - First, the function searches the active ASL for a semaphore descriptor
 *     associated with semAdd.
 *   - If found, the process is inserted into that descriptor's process queue.
 *   - Otherwise, a new semaphore descriptor is allocated, its semaphore address is set,
 *     and it is inserted into the active ASL in sorted order.
 *   - After inserting the process into the semaphore's process queue, the process's
 *     semaphore vector is updated to reflect the block.
 */
int insertBlocked(int *semAdd, proc_t *p) {
    semd_t *curr = getSema(semAdd);
    if (curr != (semd_t *) ENULL) {
        /* Semaphore descriptor already exists; insert the process into its queue */
        insertProc(&(curr->s_link), p);
        addOrResetSemvec(p, semAdd, 1);
        return FALSE;
    }

    /* Allocate a new semaphore descriptor */
    semd_t *new_sema = allocSemd();
    if (new_sema == (semd_t *) ENULL) {
        return TRUE;  /* Allocation failed */
    }
    new_sema->s_semAdd = semAdd;
    new_sema->s_link.next = (proc_t *) ENULL;
    new_sema->s_link.index = ENULL;

    /* Insert the new semaphore descriptor into the active ASL in sorted order */
    if (semd_h == (semd_t *) ENULL) {
        /* Active list is empty; new descriptor becomes the head */
        semd_h = new_sema;
        new_sema->s_next = (semd_t *) ENULL;
        new_sema->s_prev = (semd_t *) ENULL;
    } else if ( new_sema->s_semAdd < semd_h->s_semAdd ) {
        /* New descriptor should be inserted at the beginning of the list */
        new_sema->s_next = semd_h;
        new_sema->s_prev = (semd_t *) ENULL;
        semd_h->s_prev = new_sema;
        semd_h = new_sema;
    } else {
        /* Find the appropriate insertion point to keep the list sorted */
        curr = semd_h;
        while (curr->s_next != (semd_t *) ENULL && new_sema->s_semAdd > curr->s_next->s_semAdd) {
            curr = curr->s_next;
        }
        new_sema->s_next = curr->s_next;
        new_sema->s_prev = curr;
        if (curr->s_next != (semd_t *) ENULL) {
            curr->s_next->s_prev = new_sema;
        }
        curr->s_next = new_sema;
    }

    /* Insert the process into the new semaphore descriptor's blocked queue */
    insertProc(&(new_sema->s_link), p);
    addOrResetSemvec(p, semAdd, 1);
    
    return FALSE;
}

/*
 * Function: removeBlocked
 * -----------------------
 * Removes the first process from the blocked queue for a given semaphore.
 *
 * Parameters:
 *   - semAdd: Pointer to the semaphore address.
 *
 * Returns:
 *   - Pointer to the process removed from the blocked queue.
 *   - ENULL if no process is blocked on the semaphore.
 *
 * Details:
 *   - Retrieves the semaphore descriptor for the given semAdd.
 *   - Removes a process from the descriptor's blocked queue using removeProc.
 *   - Updates the process's semaphore vector to remove the semaphore address.
 *   - If the blocked queue becomes empty after removal, the semaphore descriptor is
 *     removed from the active ASL and returned to the free list.
 */
proc_t* removeBlocked(int *semAdd) {
    semd_t *sema = getSema(semAdd);
    if (sema == (semd_t *) ENULL) {
        return (proc_t *) ENULL; 
    }

    proc_t *p = removeProc(&(sema->s_link));
    if (p != (proc_t *) ENULL) {
        addOrResetSemvec(p, semAdd, 0);
    }

    if (sema->s_link.next == (proc_t *) ENULL) {
        /* No more processes are blocked on this semaphore; remove the descriptor */
        removeSema(sema);
    }
    return p;
}

/*
 * Function: outBlocked
 * --------------------
 * Removes a specific process from any blocked queues it might be on.
 *
 * Parameters:
 *   - p: Pointer to the process to be removed from blocked queues.
 *
 * Returns:
 *   - p if the process was found and removed from any semaphore queue.
 *   - ENULL if the process was not found on any blocked queue.
 *
 * Details:
 *   - Iterates over the process's semaphore vector (semvec).
 *   - For each non-ENULL semaphore address, attempts to remove the process from
 *     the corresponding semaphore descriptor's blocked queue using outProc.
 *   - If the process is removed, the semaphore's count is incremented (via *s_semAdd)
 *     and, if its blocked queue becomes empty, the semaphore descriptor is removed.
 *   - Finally, the process's semaphore vector is cleared.
 */
proc_t* outBlocked(proc_t *p) {
    int found = FALSE;
    int i;
    for (i = 0; i < SEMMAX; i++) {
        if (p->semvec[i] != (int *) ENULL) {
            semd_t *sema = getSema(p->semvec[i]);
            if (sema == (semd_t *) ENULL) {
                continue;
            }
            if (outProc(&(sema->s_link), p) == p) {
                found = TRUE;
                /* Increment the semaphore count as the process is removed from the block */
                (*sema->s_semAdd)++;
                // The following line is commented out; its purpose is to reset the semvec entry.
                // addOrResetSemvec(p, p->semvec[i], 0);
                if (sema->s_link.next == (proc_t *) ENULL) {
                    removeSema(sema);
                }
            }
        }
    }
    /* Clear all entries in the process's semaphore vector */
    for (i = 0; i < SEMMAX; i++) {
        p->semvec[i] = (int *) ENULL;
    }
    return (found) ? p : (proc_t *) ENULL;
}

/*
 * Function: headBlocked
 * ---------------------
 * Retrieves the process at the head of the blocked queue for a given semaphore.
 *
 * Parameters:
 *   - semAdd: Pointer to the semaphore address.
 *
 * Returns:
 *   - Pointer to the process at the head of the blocked queue.
 *   - ENULL if no such semaphore descriptor exists or if the queue is empty.
 */
proc_t* headBlocked(int *semAdd) {
    semd_t *sema = getSema(semAdd);
    if (sema != (semd_t *) ENULL) {
        return headQueue(sema->s_link);
    }
    return (proc_t *) ENULL;
}

/*
 * Function: headASL
 * -----------------
 * Checks whether the active semaphore descriptor list (ASL) is non-empty.
 *
 * Returns:
 *   - TRUE if there is at least one semaphore descriptor in the active list.
 *   - FALSE if the active list is empty.
 */
int headASL() {
    return (semd_h != (semd_t *) ENULL) ? TRUE : FALSE;
}
