#ifndef PROCQ
#define PROCQ

/* link descriptor type */
typedef struct proc_link {
    int index;
    struct proc_t *next;
} proc_link;

typedef struct proc_t proc_t;

typedef struct proc_t {
    proc_link p_link[SEMMAX]; /* pointer to next entreis */
    state_t p_s;  /* processor state of the process */
    int qcount; /* number of queues containing this entry */
    int *semvec[SEMMAX]; /* vector of active semaphores for this entry */
    proc_t *parent;
    proc_t *child;
    proc_t *sibling;
    state_t *prog_old;
    state_t *prog_new;
    state_t *mm_old;
    state_t *mm_new;
    state_t *sys_old;
    state_t *sys_new;
    long start_time;
    int cpu_time;

} proc_t;

#endif