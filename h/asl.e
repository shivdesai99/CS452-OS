#include "asl.h"

extern void initSemd();
extern semd_t* allocSemd();
extern semd_t* getSema(int *semAdd);
extern void addOrResetSemvec(proc_t *p, int *semAdd, int flag);
extern void removeSema(semd_t *sema);
extern int insertBlocked(int *semAdd, proc_t *p);
extern proc_t* removeBlocked(int *semAdd);
extern proc_t* headBlocked(int *semAdd);
extern proc_t* outBlocked(proc_t *p);
extern int headASL();