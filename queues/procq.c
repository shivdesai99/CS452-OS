/* This code is my own work, it was written without consulting code written by other students current or previous or using anyAItools Shiv Desai */
#include "../h/const.h" 
#include "../h/types.h"
#include "../h/procq.h"

proc_t procTable[MAXPROC];
proc_t *procFree_h;

void initProc() {
    int i, j;
    for (i = 0; i < MAXPROC; i++) {
        procTable[i].qcount = 0;
        for (j = 0; j < SEMMAX; j++) {
            procTable[i].p_link[j].next = (proc_t *) ENULL;
            procTable[i].p_link[j].index = ENULL;
            procTable[i].semvec[j] = (int *) ENULL;
        }
        
    }

    procFree_h = &procTable[0];
    for (i = 0; i < MAXPROC - 1; i++) {
        procTable[i].p_link[0].next = &procTable[i+1];
    }
    procTable[MAXPROC - 1].p_link[0].next = (proc_t *) ENULL;
}

proc_t* allocProc() {
    if (procFree_h == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    proc_t *p = procFree_h;
    procFree_h = p->p_link[0].next;
    p->p_link[0].next = (proc_t *) ENULL;
    return p;
}

void freeProc(proc_t *p) {
    if (p == (proc_t *) ENULL) {
        return;
    }
    p->qcount = 0;
    int i;
    for (i = 0; i < SEMMAX; i++) {
        p->p_link[i].next = (proc_t *) ENULL;
        p->p_link[i].index = 0;
    }
    p->p_link[0].next = procFree_h;
    procFree_h = p;
}

void insertProc(proc_link *tp, proc_t *p) {
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

proc_t* headQueue(proc_link tp) {
    if (tp.next == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    return tp.next->p_link[tp.index].next;
}

proc_t* outProc(proc_link *tp, proc_t *p) {

    if (tp->next == (proc_t *) ENULL) {
        return (proc_t *) ENULL;
    }
    
    proc_t *tail = tp->next;
    proc_t *head = tail->p_link[tp->index].next;
    proc_link *tail_link = &(tail->p_link[tp->index]);
    proc_link *head_link = &(head->p_link[tail_link->index]);

    if (tail == head) {
        if (tail == p) {
            tp->next = (proc_t *) ENULL;
            p->qcount--;
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
    prev_link->next = cur_link->next;
    prev_link->index = cur_link->index;
    p->qcount--;
    return p;
}

proc_t* removeProc(proc_link *tp) {
    if (tp->next == (proc_t *) ENULL)
        return (proc_t *) ENULL;
    
    proc_t *tail = tp->next;
    proc_t *head = tail->p_link[tp->index].next;
    proc_link *tail_link = &(tail->p_link[tp->index]);
    proc_link *head_link = &(head->p_link[tail_link->index]);

    head->qcount--;
    if (head == tail) {
        tp->next = (proc_t *) ENULL;
        return head;
    } else {
        tail_link->next = head_link->next;
        tail_link->index = head_link->index;
        return head;
    }
}