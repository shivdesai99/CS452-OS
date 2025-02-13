/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */
#include "../h/types.h"
#include "../h/const.h"
#include "../h/procq.e"
#include "../h/asl.h"

semd_t semdTable[MAXPROC];
semd_t* semd_h;
semd_t* semdFree_h;


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
 
void addOrResetSemvec(proc_t *p, int *semAdd, int flag) {
    int i;
    if (flag) {
        for (i = 0; i < SEMMAX; i++) {
            if (p->semvec[i] == (int *) ENULL) {
                p->semvec[i] = semAdd;
                break;
            }
        }
    } else {
        for (i = 0; i < SEMMAX; i++) {
            if (p->semvec[i] == semAdd) {
                p->semvec[i] = (int *) ENULL;
                break;
            }
        }
    }
}

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
    sema->s_next = semdFree_h;
    sema->s_prev = (semd_t *) ENULL;
    semdFree_h = sema;
}

int insertBlocked(int *semAdd, proc_t *p) {
    semd_t *curr = getSema(semAdd);
    if (curr != (semd_t *) ENULL) {
        insertProc(&(curr->s_link), p);
        addOrResetSemvec(p, semAdd, 1);
        return FALSE;
    }

    semd_t *new_sema = allocSemd();
    if (new_sema == (semd_t *) ENULL) {
        return TRUE;
    }
    new_sema->s_semAdd = semAdd;
    new_sema->s_link.next = (proc_t *) ENULL;
    new_sema->s_link.index = ENULL;

    if (semd_h == (semd_t *) ENULL) {
        semd_h = new_sema;
        new_sema->s_next = (semd_t *) ENULL;
        new_sema->s_prev = (semd_t *) ENULL;
    } else if ( new_sema->s_semAdd < semd_h->s_semAdd ) {
        new_sema->s_next = semd_h;
        new_sema->s_prev = (semd_t *) ENULL;
        semd_h->s_prev = new_sema;
        semd_h = new_sema;
    } else {
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

    insertProc(&(new_sema->s_link), p);
    addOrResetSemvec(p, semAdd, 1);
    
    return FALSE;
}


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
        removeSema(sema);
    }
    return p;
}

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
                addOrResetSemvec(p, p->semvec[i], 0);
                if (sema->s_link.next == (proc_t *) ENULL) {
                    removeSema(sema);
                }
            }
        }
    }
    return (found) ? p : (proc_t *) ENULL;
}

proc_t* headBlocked(int *semAdd) {
    semd_t *sema = getSema(semAdd);
    if (sema != (semd_t *) ENULL) {
        return headQueue(sema->s_link);
    }
    return (proc_t *) ENULL;
}

int headASL() {
    return (semd_h != (semd_t *) ENULL) ? TRUE : FALSE;
}
