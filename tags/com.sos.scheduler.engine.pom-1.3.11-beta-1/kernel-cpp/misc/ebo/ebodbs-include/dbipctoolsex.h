#ifndef DBIPCTOOLSEX_INCL

EXT DLL VOID warte_ebo(VOID);
EXT DLL VOID freigabe_ebo(VOID);
EXT DLL VOID reset_ipc(VOID);
EXT DLL VOID print_sem(VOID);
EXT DLL VOID print_msgq(TEXT *);
EXT DLL VOID print_katalog(VOID);
EXT DLL VOID print_shm(VOID);

EXT DLL COUNT test_ipcenvvars(VOID);

EXT DLL ppTEXT get_ipcenvvars(COUNT*);

#define DBIPCTOOLSEX_INCL

#endif /* DBIPCTOOLSEX_INCL */

