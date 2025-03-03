#ifndef ASL
#define ASL

typedef struct semd_t {
    struct semd_t *s_next, *s_prev;
    int *s_semAdd;
    proc_link s_link;
} semd_t;

#endif