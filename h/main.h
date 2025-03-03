/*

This module coordinates the initialization of the nucleus 
and it starts the execution of the first process, p1(). 

It also provides a scheduling function. 
The module contains twofunctions: main() and init(). init() is static. 
It also contains a function that it exports: schedule().

main()
This function calls init(), sets up the processor state for p1(), 
adds p1() to the RQ and calls schedule(). 

init()
This function determines howmuch physical memory there is in the system. 
It then calls initProc(), initSemd(), trapinit() and intinit()


schedule()
This function determines howmuch physical memory there is in the system. 
It then calls initProc(), initSemd(), trapinit() and intinit

Foreach module X, you should create three files: X.h, X.e and X.c. 
X.e and X.c should include X.h.
*/

extern proc_link run_queue;
extern state_t boot_state;

