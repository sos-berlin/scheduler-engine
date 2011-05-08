#ifndef DBCLNTEX_INCL

#ifndef FILE
#include <stdio.h>
#endif

EXT DLL FILE *log_fd;			/* Protokolldatei Laufzeitumgebung */

EXT DLL VOID  ridmid(LONG,COUNT);
EXT DLL VOID  ridmid_srvadm(VOID);
EXT DLL COUNT ebo_logon(COUNT,TEXT **);
EXT DLL COUNT ebo_logoff(VOID);
EXT DLL COUNT ebo_optr(COUNT);
EXT DLL COUNT ebo_cltr(COUNT);
EXT DLL COUNT ebo_read(COUNT,COUNT,COUNT,COUNT*);
EXT DLL COUNT ebo_lock(COUNT);

EXT DLL COUNT ebo_uinf(COUNT);
EXT DLL COUNT ebo_unlk(COUNT);
EXT DLL COUNT ebo_unlk_all(VOID);
EXT DLL COUNT ebo_update(COUNT, COUNT, COUNT);

EXT DLL COUNT status_server(SSTATUS *);
EXT DLL COUNT ebo_shutdown(COUNT *);
EXT DLL COUNT kill_user(USER_NO,LONG,COUNT);
EXT DLL VOID  logoff_user(USER_NO, LONG);

EXT DLL COUNT  time_trace_on(COUNT, LONG);
EXT DLL COUNT  fkt_trace_on(COUNT, LONG);
EXT DLL COUNT  trace_off(COUNT);


EXT DLL VOID list_transtab(VOID);
EXT DLL VOID list_transtab_user(USER_NO);

EXT DLL VOID list_lock_datei(COUNT);
EXT DLL VOID list_lock_user(USER_NO);
EXT DLL VOID list_locktab(VOID);

EXT DLL VOID list_usertab(VOID);

EXT DLL COUNT nxt_usertab(USER_NO*,UINF_S*,COUNT);
EXT DLL COUNT delebofile(TEXT *, COUNT);

EXT DLL COUNT srv_is_running(VOID);

EXT DLL COUNT same_machine(VOID);

#define DBCLNTEX_INCL

#endif /* DBCLNTEX_INCL */
