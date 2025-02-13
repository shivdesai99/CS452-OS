
/* link descriptor type */
typedef struct proc_link {
    int index;
    struct proc_t *next;
} proc_link;

typedef struct proc_t {
    proc_link p_link[SEMMAX]; /* pointer to next entreis */
    state_t p_s;  /* processor state of the process */
    int qcount; /* number of queues containing this entry */
    int *semvec[SEMMAX]; /* vector of active semaphores for this entry */
} proc_t;



void initProc();
proc_t* allocProc();
void freeProc(proc_t *p);
void insertProc(proc_link *tp, proc_t *p);
proc_t* headQueue(proc_link tp);
proc_t* outProc(proc_link *tp, proc_t *p);
proc_t* removeProc(proc_link *tp);