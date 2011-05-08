#ifndef ECBPROCESS_INCL

/* Prozess bestimmt seine Prozessnummer */
EXT DLL PIDNR ecb_getpid(VOID);

/* Prozess sendet anderen Prozess ein signal */
EXT DLL COUNT ecb_kill(PIDNR pid,NINT signal);

/* Prozess erzeugt Sohnprozess und ueberlagert Vater mit angegebenem Programm */
EXT DLL COUNT ecb_create_ebochild(TEXT *progamm,COUNT connid,TEXT *protfile);

EXT DLL NINT ecb_fork(VOID);
EXT DLL NINT ecb_wait(NINT *);
EXT DLL NINT ecb_execl(TEXT *, TEXT *,...);
EXT DLL NINT ecb_execlp(TEXT *, TEXT *,...);
EXT DLL VOID ecb_sleep(NINT);


#define ECBPROCESS_INCL

#endif /* ECBPROCESS_INCL */
