typedef struct semd_t {
    struct semd_t *s_next, *s_prev;
    int *s_semAdd;
    proc_link s_link;
} semd_t;


void initSemd();
semd_t* allocSemd();
semd_t* getSema(int *semAdd);
void addOrResetSemvec(proc_t *p, int *semAdd, int flag);
void removeSema(semd_t *sema);
int insertBlocked(int *semAdd, proc_t *p);
proc_t* removeBlocked(int *semAdd);
proc_t* headBlocked(int *semAdd);
proc_t* outBlocked(proc_t *p);
int headASL();